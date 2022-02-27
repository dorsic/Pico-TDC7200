#ifndef COUNTER_MODULE_V1_H_
#define COUNTER_MODULE_V1_H_

#include "../TDC7200/tdc7200.h"
/*
extern "C" {
    #include "../TDC7200/tdc7200.h"
}
*/

// communication with integrated PicoDIV in Counter Module
#define PDIVTX_PIN  16
#define PDIVCS_PIN  17
#define PDIVCLK_PIN 18
#define PDIVRX_PIN  19

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

// counter module settings
#define SOURCE_CHA   1
#define SOURCE_CHB   2
#define SOURCE_REFCLK   3
#define SOURCE_DIVCLK   4
#define SOURCE_INTCLK   5
#define SOURCE_EXTCLK   6
#define SOURCE_TCXO     7

#define REG_REF    0x01
#define REG_REFF   0x02
#define REG_DIVF   0x03
#define REG_SLW    0x04

#define UART_TIMEOUT_US     1000000
#define MHZ_1       1000000
#define MHZ_8       8000000
#define MHZ_10     10000000
#define MHZ_12     12000000
#define MHZ_16     16000000

void cm1_initialize();

// valid source CHA | CHB
void cm1_set_start_source(uint8_t source);

// valid source CHB | DIVCLK | REFCLK
void cm1_set_stop_source(uint8_t source);

// valid source INTCLK | EXTCLK
void cm1_set_refclock(uint8_t source, uint32_t freq_hz);

// PicoDIV functions

uint8_t cm1_pdiv_get_refclock();

// valid source TCXO | REFCLK
void cm1_pdiv_set_refclock(uint8_t source, uint32_t freq_hz);

uint32_t cm1_pdiv_get_refclock_freq();

void cm1_pdiv_set_freq_hz(uint32_t freq_hz);

uint32_t cm1_pdiv_get_freq_hz();

void cm1_pdiv_set_plength_ns(uint32_t plength_ns);

uint32_t cm1_pdiv_get_plength_ns();

void cm1_pdiv_slide(uint32_t n_refclock_cycles);

void cm1_pdiv_on();

void cm1_pdiv_off();

// TDC7200 functions
tdc7200_meas_t cm1_tic_measure();

bool cm1_tic_set_meas_mode(uint8_t measurement_mode);

bool cm1_tic_set_calperiods(uint8_t periods);

bool cm1_tic_set_force_calibration(bool force);

bool cm1_tic_set_nstops(uint8_t nstops);

#endif