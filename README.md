# Pico-TDC7200
RPi Pico C library for interfacing TDC7200 chip via SPI.

## Configuration & Usage
See `tdc7200_create` function from [tdc7200.h](TDC7200\tdc7200.h) to create an instance object and configure SPI pins.
Default configuration uses SPI1 with pins GPIO10-GPIO15 and 10 MHz clock.

See [tdc7200_test.c](tdc7200_test.c) for further details.
