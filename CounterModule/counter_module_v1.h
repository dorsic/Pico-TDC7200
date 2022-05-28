#ifndef COUNTER_MODULE_V1_H_
#define COUNTER_MODULE_V1_H_

#include "../TDC7200/tdc7200.h"
/*
extern "C" {
    #include "../TDC7200/tdc7200.h"
}
*/

// communication with integrated PicoDIV in Counter Module
#define PDIVRX_PIN  16  // DEBUG / povodne bolo naopak s TX
#define PDIVCS_PIN  17  // GATE PIN
#define PDIVCLK_PIN 18
#define PDIVTX_PIN  19

#define PDIVRST_PIN 2       // Reset PIN

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
//#define SOURCE_TRIGCLK 8
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
#define MODE2_TIMEOUT_NS 2000000000   // 2s

#define FNC_TI     1
#define FNC_FREQ    2
#define FNC_PERIOD  3



void cm_forward(const char* message);

uint16_t cm_forward_and_read(const char* message, char* buf);

void cm_initialize();


// PicoDIV functions

// TDC7200 functions

// valid source XO | EXTCLK | INTCLK
uint8_t cm_set_refclock(uint8_t source, uint32_t freq_hz);

tdc7200_meas_t cm_tic_measure();

uint8_t cm_tic_set_mode(uint8_t measurement_mode);

uint8_t cm_tic_get_mode();

uint8_t cm_tic_set_calperiods(uint8_t periods);

uint8_t cm_tic_set_force_calibration(uint8_t force);

uint8_t cm_tic_set_nstops(uint8_t nstops);

uint8_t cm_tic_set_edge(uint8_t channel, uint8_t falling);

uint8_t cm_tic_pet_enable(uint8_t on);

uint8_t cm_tic_pet_enabled();

uint32_t cm_tic_get_pet();

void cm_freq_set_gatefreq(double freq);

double cm_freq_get_gatefreq();

void cm_freq_set_nominalfreq(double freq);

double cm_freq_get_nominalfreq();

double cm_freq_measure();

uint8_t cm_set_fnc(uint8_t fnc_to_measure);

uint8_t cm_get_fnc();

void cm_tic_read_channel(uint8_t source);

void cm_tic_get_channel(char* buf, uint8_t source);


// valid source CHA | CHB
void cm_tic_set_start(uint8_t source, uint8_t store_value);

// valid source CHB | DIVCLK | REFCLK
void cm_tic_set_stop(uint8_t source, uint8_t store_value);

void cm_div_reset();

#endif