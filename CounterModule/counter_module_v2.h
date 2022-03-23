#ifndef COUNTER_MODULE_V2_H_
#define COUNTER_MODULE_V2_H_

#include "../TDC7200/tdc7200.h"
/*
extern "C" {
    #include "../TDC7200/tdc7200.h"
}
*/

// communication with integrated PicoDIV in Counter Module
#define PDIVRX_PIN  16
#define PDIVCS_PIN  17      // GATE_GPIO
#define PDIVCLK_PIN 18
#define PDIVTX_PIN  19

// communication with TDC7200 of the Counter Module
#define TRIGG_PIN   15
#define INTB_PIN    14
#define CSB_PIN     13
#define DOUT_PIN    12
#define DIN_PIN     11
#define SCLK_PIN    10

#define SWCLKSEL2_PIN   9
#define SWCLKSEL1_PIN   8
#define SWCHIN2_PIN     7
#define SWCHIN1_PIN     6

#define GATE_PIN 17
#define GATE_PIO pio0
#define GATE_SM 0

// counter module settings
#define SOURCE_CH1   1
#define SOURCE_CH2   2
#define SOURCE_REFCLK   3
#define SOURCE_DIVCLK   4
#define SOURCE_INTCLK   5
#define SOURCE_EXTCLK   6
#define SOURCE_XO     7
#define SOURCE_TRIGCLK 8
#define SOURCE_START 9
#define SOURCE_STOP 10

#define PDIVCOM_UART
//#define PDIVCOM_SPI
#ifdef PDIVCOM_UART
#define UART_TIMEOUT_US     100000
#define UART_PIO pio1
#define UART_TX_SM  3
#define UART_RX_SM  2
#endif
#ifdef PDIVCOM_SPI
#define SPI spi0
#define SPI_MHZ 1
#endif

#define MHZ_1       1000000
#define MHZ_8       8000000
#define MHZ_10     10000000
#define MHZ_12     12000000
#define MHZ_16     16000000

#define MODE1_TIMEOUT_NS 1100
#define MODE2_TIMEOUT_NS 5000000

void cm_forward(const char* message);

uint16_t cm_forward_and_read(const char* message, char* buf);

void cm_initialize();


// PicoDIV functions

// TDC7200 functions

// valid source XO | EXTCLK | INTCLK
bool cm_set_refclock(uint8_t source, uint32_t freq_hz);

tdc7200_meas_t cm_tic_measure();

bool cm_tic_set_meas_mode(uint8_t measurement_mode);

bool cm_tic_set_calperiods(uint8_t periods);

bool cm_tic_set_force_calibration(bool force);

bool cm_tic_set_nstops(uint8_t nstops);

bool cm_tic_set_edge(uint8_t channel, bool falling);

bool cm_tic_pet_enable(bool on);

bool cm_tic_pet_enabled();

uint8_t cm_get_meas_mode();

uint32_t cm_get_pet();

#endif