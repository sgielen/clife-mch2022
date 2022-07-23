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
	uint8_t red;
	uint8_t green;
	uint8_t blue;
	uint8_t age;

	MulticolorValue() : value(false), red(0), green(0), blue(0), age(0){}
	MulticolorValue(std::vector<MulticolorValue> vec) : value(true), age(0) {
		std::vector<int> hues;
		for(auto const &cell : vec) {
			if(cell.value) {
				float r = float(cell.red) / UINT8_MAX;
				float g = float(cell.green) / UINT8_MAX;
				float b = float(cell.blue) / UINT8_MAX;
				float min = std::min(r, std::min(g, b));
				float max = std::max(r, std::max(g, b));
				// hue from 0 to 360
				int hue = max == 0 ? 0 :
				            r == max ? (60 * (0 + (g-b)/(max-min))) :
				            g == max ? (60 * (2 + (b-r)/(max-min))) :
				                       (60 * (4 + (r-g)/(max-min)));
				if(hue < 0) {
					hue += 360;
				}
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
		float sat = 1;
		float value = 1;
		int h_i = std::floor(avghue / 60);
		float f = (avghue / 60.) - h_i;
		float p = value * (1 - sat);
		float q = value * (1 - f * sat);
		float t = value * (1 - (1 - f) * sat);

		value *= 255;
		p *= 255;
		q *= 255;
		t *= 255;

		switch(h_i) {
		case 0:
			red = value;
			green = t;
			blue = p;
			break;
		case 1:
			red = q;
			green = value;
			blue = p;
			break;
		case 2:
			red = p;
			green = value;
			blue = t;
			break;
		case 3:
			red = p;
			green = q;
			blue = value;
			break;
		case 4:
			red = t;
			green = p;
			blue = value;
			break;
		case 5:
			red = value;
			green = p;
			blue = q;
			break;
		default:
			abort();
		};
	}
	MulticolorValue(bool value) : value(value), red(0), green(0), blue(0), age(0) {
		if(value) {
			char color = (rand() % 6) + 1;
			red   = (color & 1) ? 255 : 0;
			green = (color & 2) ? 255 : 0;
			blue  = (color & 4) ? 255 : 0;
		}
	}

	void age_once() {
		age = (age + 1) % 50;
	}

	operator bool() const { return value; }
	std::string hash() const {
		if(value) {
			return std::string("1") + char(red) + char(green) + char(blue);
		} else {
			return "0\x00\x00\x00";
		}
	}

	char getChar() const {
		return (value ? 'o' : ' ');
	}

	void begin_screen(std::ostream &os) const {
/*
		if(!concise) {
			std::cout << "+";
			for(int i = 0; i < get_width(); ++i) {
				std::cout << "-";
			}
			std::cout << "+\n";
		}
*/
	}
	void end_screen(std::ostream &os) const {
/*
		if(!to_ledscreen && !concise) {
			begin_screen(os);
			std::cout << "\x1b[1;1H" << std::flush;
		}
*/
	}
	void begin_line(std::ostream &os) const {
/*
		if(!to_ledscreen && !concise) {
			std::cout << "|";
		}
*/
	}
	void end_line(std::ostream &os) const {
/*
		if(!to_ledscreen && !concise) {
			std::cout << "|\n";
		}
*/
	}

	void print(std::ostream &os) const {
/*
		if(value && to_ledscreen) {
			os << red << green << blue;
		} else if(value) {
			double brightness = std::max(0.3, 1 - age / 20.);

			uint8_t redval = (red * brightness) / 43;
			uint8_t greenval = (green * brightness) / 43;
			uint8_t blueval = (blue * brightness) / 43;
			uint8_t code = 16 + 36 * redval + 6 * greenval + blueval;
			os << "\x1b[38;5;" << int(code) << "mo\x1b[m";
		} else if(to_ledscreen) {
			os << '\x00' << '\x00' << '\x00';
		} else {
			os << ' ';
		}
*/
	}
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

		for (int y = 0; y < field.get_height(); ++y) {
			taskYIELD();
			auto const &row = field[y];
			for (int x = 0; x < field.get_width(); ++x) {
				auto const &cell = row[x];
				pax_col_t col = pax_col_rgb(cell.red, cell.green, cell.blue);

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
