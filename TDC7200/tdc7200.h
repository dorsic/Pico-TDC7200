
#ifndef __TDC7200_H__
#define __TDC7200_H__

#ifdef __cplusplus
extern "C" {
#endif

// TDC7201 register addresses - 8-bit
// These can be read or written.
#define CONFIG1 0x00
#define CONFIG2 0x01
#define INT_STATUS 0x02
#define INT_MASK 0x03
#define COARSE_CNTR_OVF_H 0x04
#define COARSE_CNTR_OVF_L 0x05
#define CLOCK_CNTR_OVF_H 0x06
#define CLOCK_CNTR_OVF_L 0x07
#define CLOCK_CNTR_STOP_MASK_H 0x08
#define CLOCK_CNTR_STOP_MASK_L 0x09
#define MINREG8 0x00
#define MAXREG8 0x09

// Not actual chip registers, but 16-bit pairs of 8
#define COARSE_CNTR_OVF 0x0A
#define CLOCK_CNTR_OVF 0x0B
#define CLOCK_CNTR_STOP_MASK 0x0C

// CONFIG1 register bit masks
// Calibrate after every measurement, even if interrupted?
#define _CF1_FORCE_CAL 0b10000000
// Add a parity bit (even parity) for 24-bit registers?
#define _CF1_PARITY_EN 0b01000000
// Invert TRIGG signals (falling edge instead of rising edge)?
#define _CF1_TRIGG_EDGE 0b00100000
// Inverted STOP signals (falling edge instead of rising edge)?
#define _CF1_STOP_EDGE 0b00010000
// Inverted START signals (falling edge instead of rising edge)?
#define _CF1_START_EDGE 0b00001000
// Neasurememnt mode 1 or 2? (Other values reserved.)
#define _CF1_MEAS_MODE 0b00000110	// bit mask
#define _CF1_MM1 0b00000000
#define _CF1_MM2 0b00000010
// Start new measurement
// Automagically starts a measurement.
// Is automagically cleared when a measurement is complete.
// DO NOT poll this to see when a measurement is done!
// Use the INT1 (or INT2) signal instead.
#define _CF1_START_MEAS 0b00000001


// CONFIG2 register bit masks
// Number of calibration periods
#define _CF2_CALIBRATION_PERIODS 0b11000000	// bit mask
#define _CF2_CAL_PERS_2 0b00000000
#define _CF2_CAL_PERS_10 0b01000000	// on reset
#define _CF2_CAL_PERS_20 0b10000000
#define _CF2_CAL_PERS_40 0b11000000
// Number of cycles to average over
#define _CF2_AVG_CYCLES 0b00111000	// bit mask
#define _CF2_AVG_CYC_1 0b00000000	// no averaging, default
#define _CF2_AVG_CYC_2 0b00001000
#define _CF2_AVG_CYC_4 0b00010000
#define _CF2_AVG_CYC_8 0b00011000
#define _CF2_AVG_CYC_16 0b00100000
#define _CF2_AVG_CYC_32 0b00101000
#define _CF2_AVG_CYC_64 0b00110000
#define _CF2_AVG_CYC_128 0b00111000
// Number of stop pulses to wait for.
#define _CF2_NUM_STOP 0b00000111	// bit mask
#define _CF2_NSTOP_1 0b00000000	// default on reset
#define _CF2_NSTOP_2 0b00000001
#define _CF2_NSTOP_3 0b00000010
#define _CF2_NSTOP_4 0b00000011
#define _CF2_NSTOP_5 0b00000100

// INT_STATUS register bit masks
// Upper 3 bits are reserved.
// Writing a 1 to any of the other bits should clear their status.
// Did the measurement complete?
#define _IS_COMPLETE 0b00010000
// Has the measurement started?
#define _IS_STARTED 0b00001000
// Did the clock overflow?
#define _IS_CLOCK_OVF 0b00000100
// Did the coarse counter overflow?
#define _IS_COARSE_OVF 0b00000010
// Was an interrupt generated?
// May be identical information to _IS_COMPLETE.
#define _IS_INTERRUPT 0b00000001

// INT_MASK register bit masks
// Upper 5 bits are reserved.
// Is the clock counter overflow enabled?
#define _IM_CLOCK_OVF 0b00000100
// Is the coarse counter overflow enabled?
#define _IM_COARSE_OVF 0b00000010
// Is the measurement complete interrupt enabled?
#define _IM_MEASUREMENT 0b00000001

// TDC register address bit masks
#define _AI 0x80	// 0b10000000	# bit mask
#define _WRITE 0x40	// 0b01000000	# bit mask
#define _ADDRESS 0x3F // 0b00111111	# bit mask

// TDC7200 register addresses - 24-bit
// These can be read but usually should not be written,
// as they contain results of measurement or calibration.
#define TIME1 0x10
#define CLOCK_COUNT1 0x11
#define TIME2 0x12
#define CLOCK_COUNT2 0x13
#define TIME3 0x14
#define CLOCK_COUNT3 0x15
#define TIME4 0x16
#define CLOCK_COUNT4 0x17
#define TIME5 0x18
#define CLOCK_COUNT5 0x19
#define TIME6 0x1A
#define CALIBRATION1 0x1B
#define CALIBRATION2 0x1C
#define MINREG24 0x10
#define MAXREG24 0x1C

// Note that side #1 and #2 of the chip EACH have a full set of registers!
// Which one you are talking to depends on the chip select!
// Within spidev, you need to close one side and then open the other to switch.

// TDC7201 clock, for calculating actual TOFs
#define CLK_FREQ_MIN 4000000
#define CLK_FREQ_MAX 16000000
#define SPI_SPEED_MIN 50000
#define SPI_SPEED_MAX 25000000
#define MHZ_10 10000000
#define MHZ_12 12000000

#ifdef  __cplusplus
}
#endif /*  __cplusplus */

#endif  /*  __TDC7200_H__ */

#include "hardware/spi.h"

typedef struct _tdc7200_obj_t {
    spi_inst_t *spi_obj;
    uint8_t SCLK;
    uint8_t CS;
    uint8_t DIN;
    uint8_t DOUT;
    uint8_t INTB;
    uint8_t TRIGG;

    uint32_t clk_freq;
    double clk_period;
    bool force_cal;
    uint8_t meas_mode;
    bool trigg_falling;
    bool start_falling;
    bool stop_falling;
    uint8_t calibration_periods;
    uint8_t avg_cycles;
    uint8_t num_stops;
    uint16_t clock_cntr_stop;
    uint16_t clock_cntr_ovf;
    double timeout;
    bool retain_state;

    uint32_t reg1[MAXREG24+1];
} tdc7200_obj_t;

typedef struct _tdc7200_meas_t {
    uint32_t time[6];
    uint32_t clock_count[5];
    uint32_t calibration[2];
    uint8_t num_stops;
    double normLSB;
    double tof[5];
    uint8_t int_status;
    uint8_t error;
} tdc7200_meas_t;

tdc7200_obj_t tdc7200_create(spi_inst_t *spi_obj, uint8_t sclk, uint8_t cs, uint8_t din, uint8_t dout, 
                                                uint8_t intb, uint8_t trigg, uint32_t clk_freq);
tdc7200_obj_t tdc7200_create_defaults();

void tdc7200_configure(tdc7200_obj_t *self, uint32_t clk_freq, bool force_cal, uint8_t meas_mode, bool trigg_edge_falling,
        bool start_edge_falling, bool stop_edge_falling, uint8_t calibration_periods, uint8_t avg_cycles, uint8_t num_stops, 
        uint16_t clock_cntr_stop, uint16_t clock_cntr_ovf, uint32_t timeout_ns, bool retain_state);

void tdc7200_configure_defaults(tdc7200_obj_t *self);
void tdc7200_reconfigure(tdc7200_obj_t *self);

tdc7200_meas_t tdc7200_measure(tdc7200_obj_t *self);
