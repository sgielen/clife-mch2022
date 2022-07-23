# Colored Game of Life

## License

This repository is based on https://github.com/badgeteam/mch2022-template-app, see that repo for original licenses.

The changes for clife and clife itself are MIT licensed.

## How to make
```sh
git clone --recursive https://github.com/sgielen/clife-mch2022.git
cd clife-mch2022
make
```

The default target of the Makefile (the one executed if you just run `make`) installs the proper ESP-IDF version and all other dependencies, then builds the project and tries to install it on an attached Badge. Because this process checks all the dependencies for updates, this can become tedious during development, so you'll probably want to execute specific targets:

- prepare : this is (one of) the targets executed in the default task and the one that, technically, only needs to run once to install dependencies
- clean : clean the build environment. This does NOT clean up installed dependencies.
- build : well ... build. Compiles you sources and assembles a binary to install.
- install : This install the binary that was build, you can only call `install`, it depends on `build`. *Note* installation is not and SHOULD NOT be performed with the typical `idf.py flash` call, see the note below for details.
- monitor : start the serial monitor to examine log output
- menuconfig : The IDF build system has a fairly elaborate configuration system that can be accessed via `menuconfig`. You'll know if you need it. Or try it out to explore.


### Note: Why not to use `idf.py flash` to install my native app.

If you have previously used the IDF, you may have noticed that we don’t use
`idf.py flash` to install the app on the Badge. (And if you haven’t, you can
safely skip this section. :)

The `idf.py flash` command assumes that the binary to flash is the main (and
only) application for the device. This is not the case for the Badge, though.
The main application is the launcher app, i.e. the app with the menu that
starts by default. The `make install` target of the Makefile copies our newly
created app into the
[appfs](https://github.com/badgeteam/esp32-component-appfThe Makefile cs)
instead of overwriting the launcher. Once copied to the appfs, the launcher can
find your app and it should appear in the apps menu.

Obviously you _can_ use idf.py flash but you’ll delete the launcher app and would
need to reinstall it later.
