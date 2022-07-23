#include "main.h"
#include <string>
#include <algorithm>
#include <ctime>
#include "clife/clife.hpp"
#include "clife/util.hpp"

static pax_buf_t buf;
xQueueHandle buttonQueue;

#include <esp_log.h>
static const char *TAG = "mch2022-demo-app";

// Updates the screen with the latest buffer.
void disp_flush() {
    ili9341_write(get_ili9341(), (const uint8_t*)buf.buf);
}

// Exits the app, returning to the launcher.
void exit_to_launcher() {
    REG_WRITE(RTC_CNTL_STORE0_REG, 0);
    esp_restart();
}

struct MulticolorValue {
	bool value;
	uint8_t hue;
	uint8_t age;

	MulticolorValue() : value(false), hue(0), age(0){}
	MulticolorValue(std::vector<MulticolorValue> vec) : value(true), age(0) {
		std::vector<int> hues;
		for(auto const &cell : vec) {
			if(cell.value) {
				int hue = int(cell.hue) * 360 / UINT8_MAX;
				hues.push_back(hue);
			}
		}
		assert(hues.size() == 3);
		std::sort(hues.begin(), hues.end());
		int range1 = hues[2] - hues[0];
		int range2 = (hues[0] + 360) - hues[1];
		int range3 = (hues[1] + 360) - hues[2];
		int avghue;
		if(std::min(range1, std::min(range2, range3)) == range1) {
			avghue = (hues[0] + hues[1] + hues[2]) / 3;
		} else if(std::min(range1, std::min(range2, range3)) == range2) {
			avghue = (hues[0] + hues[1] + hues[2] + 360) / 3;
		} else {
			avghue = (hues[0] + hues[1] + hues[2] + 720) / 3;
		}
		avghue %= 360;
		hue = avghue * UINT8_MAX / 360;
	}
	MulticolorValue(bool value) : value(value), hue(0), age(0) {
		if(value) {
			char color = rand() & 6;
			hue = (UINT8_MAX / 6) * color;
		}
	}

	void age_once() {
		age = (age + 1) % 50;
	}

	operator bool() const { return value; }
	std::string hash() const {
		if(value) {
			return std::string("1") + char(hue);
		} else {
			return "0\x00";
		}
	}

	char getChar() const {
		return (value ? 'o' : ' ');
	}

	void begin_screen(std::ostream &os) const {}
	void end_screen(std::ostream &os) const {}
	void begin_line(std::ostream &os) const {}
	void end_line(std::ostream &os) const {}

	void print(std::ostream &os) const {}
};

/*
template <typename FieldType>
void check_stop_condition(FieldType field, std::vector<std::string> &earlier_hashes, bool &done, int &repeats_to_do) {
	std::string hash = field.field_hash();
	for(size_t i = 0; i < earlier_hashes.size(); ++i) {
		if(earlier_hashes[i] == hash) {
			done = true;
			repeats_to_do = 50;
			break;
		}
	}
	earlier_hashes.push_back(hash);
}
*/

extern "C"
void app_main() {
	ESP_LOGI(TAG, "Starting clife");

	// Initialize the screen, the I2C and the SPI busses.
	bsp_init();

	// Initialize the RP2040 (responsible for buttons, etc).
	bsp_rp2040_init();

	// This queue is used to receive button presses.
	buttonQueue = get_rp2040()->queue;

	// Initialize graphics for the screen.
	pax_buf_init(&buf, NULL, 320, 240, PAX_BUF_16_565RGB);

	init_random();

	std::vector<std::string> earlier_hashes;
	auto const resolutionFactor = 4;
	GameOfLifeField<MulticolorValue> field(320 / resolutionFactor /*col*/, 240 / resolutionFactor /*row*/);

	// Initialize NVS.
	nvs_flash_init();

	taskYIELD();
	field.generateRandom(35);

	bool field_done = false;
	int repeats_to_do = 0;

	// make sure the queue is empty
	while (1) {
		rp2040_input_message_t message;
		xQueueReceive(buttonQueue, &message, 0);
		if (!message.input || !message.state) {
			break;
		}
	}

	while(!field_done || repeats_to_do > 0) {
		taskYIELD();

		if(repeats_to_do > 0) {
			--repeats_to_do;
		}
		field.nextState();
		if(!field_done) {
			//check_stop_condition(field, earlier_hashes, field_done, repeats_to_do);
		}

		pax_col_t black = pax_col_rgb(0, 0, 0);

		for (int y = 0; y < field.get_height(); ++y) {
			taskYIELD();
			auto const &row = field[y];
			for (int x = 0; x < field.get_width(); ++x) {
				auto const &cell = row[x];
				uint8_t brightness = 255 - (2 * cell.age);
				pax_col_t col = black;

				if (cell.value) {
					col = pax_col_hsv(cell.hue, 255, brightness);
				}

				pax_draw_rect(&buf, col, x * resolutionFactor, y * resolutionFactor, resolutionFactor, resolutionFactor);
			}
		}

		disp_flush();
		taskYIELD();

		rp2040_input_message_t message;
		xQueueReceive(buttonQueue, &message, 0);

		if (message.input == RP2040_INPUT_BUTTON_START && message.state) {
			// Regenerate
			field.generateRandom(35);
		}
		if (message.input == RP2040_INPUT_BUTTON_HOME && message.state) {
			// Exit to launcher.
			exit_to_launcher();
		}
	}
}
