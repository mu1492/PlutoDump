# PlutoDump

**Dump internal data from Analog Devices [ADALM-PLUTO](https://www.analog.com/en/resources/evaluation-hardware-and-software/evaluation-boards-kits/adalm-pluto.html "ADALM-PLUTO") SDR**

- It depends on [libiio](https://github.com/analogdevicesinc/libiio/tree/libiio-v0 "libiio").  Please refer to the official documentation for on-host building.
- The Linaro toolchain (arm-linux-gnueabihf) can be used for cross-building on the Linux host.

Build steps:

	cd PlutoDump
	mkdir build
	cd build
	cmake ..
	make
	