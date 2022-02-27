
#include <stdlib.h>
//#include <math.h>
#include <stdio.h>
//#include <stdint.h>
//#include <string.h>
#include "tdc7200.h"
#include "pico/stdlib.h"
#include "hardware/spi.h"

#define LOG_VERBOSE 2
#define LOG_INFO 1
#define LOG_OFF 0
#define LOG LOG_INFO

#define SPI_TROTTLING_US 0
#define SPI_SPEED_HZ   25000000

double tdc7200_period(uint32_t freq) { 1.0/(double)freq; }

void tdc7200_write8(tdc7200_obj_t *self, const uint8_t reg, const uint8_t val) {
    uint8_t buf[2] = { reg | _WRITE, val };
    gpio_put(self->CS, 0); // Indicate beginning of communication
    spi_write_blocking(self->spi_obj, buf, 2); // Send data[]
    gpio_put(self->CS, 1); // Signal end of communication
    sleep_us(SPI_TROTTLING_US);   
}

void tdc7200_write16(tdc7200_obj_t *self, const uint8_t reg, const uint16_t val) {
    uint8_t buf[3] = { reg | _WRITE | _AI, (val>>8)&0xFF, val&0xFF };
    gpio_put(self->CS, 0); // Indicate beginning of communication
    spi_write_blocking(self->spi_obj, buf, 3); // Send data[]
    gpio_put(self->CS, 1); // Signal end of communication
    sleep_us(SPI_TROTTLING_US);
}

int tdc7200_read(tdc7200_obj_t *self, const uint8_t reg, uint8_t *buf, const uint8_t nbytes) {
    int num_bytes_read = 0;
    uint8_t mb = 0;

    // Determine if multiple byte (MB) bit should be set
    if (nbytes < 1) {
        return -1;
    } else if (nbytes == 1) {
        mb = 0;
    } else {
        mb = 1;
    }

    // Construct message (set ~W bit high)
    uint8_t msg[1] = { (mb*_AI) | reg };

    // Read from register
    gpio_put(self->CS, 0);
    spi_write_blocking(self->spi_obj, msg, 1);
    sleep_us(SPI_TROTTLING_US);
    num_bytes_read = spi_read_blocking(self->spi_obj, 0, buf, nbytes);
    gpio_put(self->CS, 1);
    sleep_us(SPI_TROTTLING_US);

    return num_bytes_read; 
}

uint8_t tdc7200_read8(tdc7200_obj_t *self, uint8_t reg) {
    uint8_t buf[1];
    tdc7200_read(self, reg, buf, 1);
    return buf[0];
}

uint16_t tdc7200_read16(tdc7200_obj_t *self, uint8_t reg) {
    uint8_t buf[2];
    int readb = tdc7200_read(self, reg | _AI, buf, 2);
    return (((uint16_t)buf[0]) << 8) | buf[1];
}

uint32_t tdc7200_read24(tdc7200_obj_t *self, uint8_t reg) {
    uint8_t buf[3];
    tdc7200_read(self, reg | _AI, buf, 3);
    return (((uint32_t)buf[0]) << 16) | (((uint32_t)buf[1]) << 8) | buf[2];
}

void tdc7200_read_regs24(tdc7200_obj_t *self) {
    //Read all 24-bit chip registers, using auto-increment feature.
    uint8_t buf[40];

    uint result24 = tdc7200_read(self, MINREG24 | _AI, buf, 40);

    #if LOG >= LOG_VERBOSE
    printf("(readregs24) Read %i bytes.\n [", result24);
    for (uint8_t i = 0; i < result24; i++) {
        printf("0x%x, ", buf[i]);
    }
    printf("]\n");
    #endif

    uint i = 0;  // First (0th) byte is always 0, rest are desired values.
    for (uint8_t reg = MINREG24; reg <= MAXREG24; reg++) {	
        // Data comes in MSB first.
        self->reg1[reg] = (((uint32_t)buf[i]) << 16) | (((uint32_t)buf[i+1]) << 8) | buf[i+2];
        #if LOG >= LOG_VERBOSE
        printf("Reg1[0x%x] = %i\n", reg, self->reg1[reg]);
        #endif
        i += 3;
    }
}    

void tdc7200_write8r(tdc7200_obj_t *self, const uint8_t reg, const uint8_t val) {
    tdc7200_write8(self, reg, val);
    uint8_t read = tdc7200_read8(self, reg);
    while (read != val) {
        #if LOG >= LOG_VERBOSE
        printf("(Write8r) Failed to set %x. (%i => %i)\n", reg, val, read);
        #endif
        sleep_us(SPI_TROTTLING_US);
        tdc7200_write8(self, reg, val);
        read = tdc7200_read8(self, reg);
    }
}

void tdc7200_write16r(tdc7200_obj_t *self, const uint8_t reg, const uint16_t val) {
    tdc7200_write16(self, reg, val);
    uint16_t read = tdc7200_read16(self, reg);
    while (read != val) {
        #if LOG >= LOG_VERBOSE
        printf("(Write16r) Failed to set register %x. (%i => %i)\n", reg, val, read);
        #endif
        sleep_us(SPI_TROTTLING_US);
        tdc7200_write16(self, reg, val);
        read = tdc7200_read16(self, reg);
    }
}

tdc7200_obj_t tdc7200_create(spi_inst_t *spi_obj, uint8_t sclk, uint8_t cs, uint8_t din, uint8_t dout, 
                                                uint8_t intb, uint8_t trigg, uint32_t clk_freq) {
    tdc7200_obj_t tdc;

    tdc.spi_obj = spi_obj;
    tdc.SCLK = sclk;
    tdc.DIN = din;
    tdc.DOUT = dout;
    tdc.CS = cs;
    tdc.INTB = intb;
    tdc.TRIGG = trigg;

    tdc.clk_freq = clk_freq;
    tdc.clk_period = 1.0/(double)clk_freq; 
    for (int i = 0; i < MAXREG24+1; i++) { tdc.reg1[i] = 0x00; }
    spi_init(tdc.spi_obj, SPI_SPEED_HZ); // Initialise spi
    //Initialise GPIO pins for SPI communication
    gpio_set_function(tdc.DIN, GPIO_FUNC_SPI);
    gpio_set_function(tdc.DOUT, GPIO_FUNC_SPI);
    gpio_set_function(tdc.SCLK, GPIO_FUNC_SPI);
    // Configure Chip Select
    gpio_init(tdc.CS); // Initialise CS Pin
    gpio_set_dir(tdc.CS, GPIO_OUT); // Set CS as output
    gpio_put(tdc.CS, 1); // Set CS High to indicate no currect SPI communication
    gpio_init(tdc.INTB); // Initialise INTB Pin
    gpio_set_dir(tdc.INTB, GPIO_IN); // Set INTB as input
    gpio_pull_up(tdc.INTB);
    gpio_init(tdc.TRIGG); // Initialise TRIGG Pin
    gpio_set_dir(tdc.TRIGG, GPIO_IN); // Set TRIGG as input

    return tdc;
}

tdc7200_obj_t tdc7200_create_defaults() {
    return tdc7200_create(spi1, 10, 13, 11, 12, 14, 15, 10000000);
}

void tdc7200_configure(tdc7200_obj_t *self, uint32_t clk_freq, bool force_cal, uint8_t meas_mode, bool trigg_edge_falling,
        bool start_edge_falling, bool stop_edge_falling, uint8_t calibration_periods, uint8_t avg_cycles, uint8_t num_stops, 
        uint16_t clock_cntr_stop, uint16_t clock_cntr_ovf, uint32_t timeout_ns, bool retain_state) {
    
    uint8_t reg1[MAXREG24+1];
    uint8_t cf1_state;
    uint8_t cf2_state;
    uint8_t im_state;
    uint16_t clock_cntr_stp;
    uint16_t ovf;

    // Configuration register 1
    if (retain_state) {
        cf1_state = self->reg1[CONFIG1];
    } else {
        self->clk_freq = clk_freq;
        self->clk_period = 1.0/(double)clk_freq;
        #if LOG >= LOG_INFO
        if (clk_freq > CLK_FREQ_MAX) {
            printf("Clock frequency greated then recommended maximum of %i MHz.\n", CLK_FREQ_MAX);
        } else if (clk_freq < CLK_FREQ_MIN) {
            printf("Clock frequency less then recommended minimum of %i MHz.\n", CLK_FREQ_MIN);
        }
        printf("Clock frequency set to %i Hz.\n", self->clk_freq);
        #endif
        cf1_state = 0; // The default after power-on or reset
        self->force_cal = force_cal;
        if (force_cal) {
            cf1_state |= _CF1_FORCE_CAL;
            #if LOG >= LOG_INFO
            printf("Set forced calibration.\n");
            #endif
        }
        self->trigg_falling = trigg_edge_falling;
        if (trigg_edge_falling) {
            cf1_state |= _CF1_TRIGG_EDGE;
            #if LOG >= LOG_INFO
            printf("Set TRIG1 to use falling edge.\n");
            #endif
        }
        self->start_falling = start_edge_falling;
        if (start_edge_falling) {
            cf1_state |= _CF1_START_EDGE;
            #if LOG >= LOG_INFO
            printf("Set START to trigger on falling edge.\n");
            #endif
        }
        self->stop_falling = stop_edge_falling;
        if (stop_edge_falling) {
            cf1_state |= _CF1_STOP_EDGE;
            #if LOG >= LOG_INFO
            printf("Set STOP to trigger on falling edge.\n");
            #endif
        }
        self->meas_mode = meas_mode;
        if (self->meas_mode == 1) {
            cf1_state |= _CF1_MM1; // Does nothing since MM1 == 00.
            #if LOG >= LOG_INFO
            printf("Set measurement mode 1.\n"); // default value
            #endif
        } else if (self->meas_mode == 2) {
            cf1_state |= _CF1_MM2;
            #if LOG >= LOG_INFO
            printf("Set measurement mode 2.\n");
            #endif
        } else {
            cf1_state |= _CF1_MM1;
            self->meas_mode = 1;
            #if LOG >= LOG_INFO
            printf("%i is not a legal measurement mode.\n", meas_mode);
            printf("Defaulting to measurement mode 1.\n");
            #endif
        }
        self->reg1[CONFIG1] = cf1_state;
    }
            
    // Write+read occasionally fails when SPI is overclocked.
    // That is, the value you read back is not what you wrote.
    // Repeat until success.
    // Should usually only take 1 attempt, although there are
    // sometimes clusters of failures immediately after on().
    tdc7200_write8(self, CONFIG1, cf1_state);
    
    uint8_t cf1_read = tdc7200_read8(self, CONFIG1);
    if ((cf1_read == 0) && (cf1_state != 0)) {
        printf("Are you sure the TDC7200 is connected to the Pi's SPI interface?\n");
        exit(1);
    }

//    tdc7200_write8r(self, CONFIG1, cf1_state);

    // Configuration register 2
    if (retain_state) {
        cf2_state = self->reg1[CONFIG2];
    } else {
        cf2_state = 0; // Power-on default is 0b01_000_000
        // Always calibrate for AT LEAST as many cycles as requested.
        // Should probably warn if value is not exact ...
        if (calibration_periods <= 2) {
            cf2_state |= _CF2_CAL_PERS_2;  // No effect since equals 0.
            self->calibration_periods = 2;
        } else if (calibration_periods <= 10) {
            cf2_state |= _CF2_CAL_PERS_10;
            self->calibration_periods = 10;
        } else if (calibration_periods <= 20) {
            cf2_state |= _CF2_CAL_PERS_20;
            self->calibration_periods = 20;
        } else {
            cf2_state |= _CF2_CAL_PERS_40;
            self->calibration_periods = 40;
        }
        #if LOG >= LOG_INFO
        printf("Set %i-clock-period calibration.\n", self->calibration_periods);
        #endif

        if (avg_cycles <= 1) {
            cf2_state |= _CF2_AVG_CYC_1; // No effect since equals 0.
            self->avg_cycles = 1;
            #if LOG >= LOG_INFO
            printf("No averaging.\n"); // default on reset
            #endif
        } else if (avg_cycles <= 2) {
            cf2_state |= _CF2_AVG_CYC_2;
            self->avg_cycles = 2;
            #if LOG >= LOG_INFO
            printf("Averaging over %i measurement cycles.\n", self->avg_cycles);
            #endif
        } else if (avg_cycles <= 4) {
            cf2_state |= _CF2_AVG_CYC_4;
            self->avg_cycles = 4;
            #if LOG >= LOG_INFO
            printf("Averaging over %i measurement cycles.\n", self->avg_cycles);
            #endif
        } else if (avg_cycles <= 8) {
            cf2_state |= _CF2_AVG_CYC_8;                
            self->avg_cycles = 8;
            #if LOG >= LOG_INFO
            printf("Averaging over %i measurement cycles.\n", self->avg_cycles);
            #endif
        } else if (avg_cycles <= 16) {
            cf2_state |= _CF2_AVG_CYC_16;
            self->avg_cycles = 16;
            #if LOG >= LOG_INFO
            printf("Averaging over %i measurement cycles.\n", self->avg_cycles);
            #endif
        } else if (avg_cycles <= 32) {
            cf2_state |= _CF2_AVG_CYC_32;
            self->avg_cycles = 32;
            #if LOG >= LOG_INFO
            printf("Averaging over %i measurement cycles.\n", self->avg_cycles);
            #endif
        } else if (avg_cycles <= 64) {
            cf2_state |= _CF2_AVG_CYC_64;
            self->avg_cycles = 64;
            #if LOG >= LOG_INFO
            printf("Averaging over %i measurement cycles.\n", self->avg_cycles);
            #endif
        } else if (avg_cycles <= 128) {
            cf2_state |= _CF2_AVG_CYC_128;
            self->avg_cycles = 128;
            #if LOG >= LOG_INFO
            printf("Averaging over %i measurement cycles.\n", self->avg_cycles);
            #endif
        } else {
            cf2_state |= _CF2_AVG_CYC_1; // No effect since equals 0.
            self->avg_cycles = 1;
            #if LOG >= LOG_INFO
            printf("%i is not a valid number of cycles to average over, defaulting to no averaging.\n", avg_cycles);        
            #endif
        }

        self->num_stops = num_stops;
        switch (num_stops) {
            case 1:
                cf2_state |= _CF2_NSTOP_1; // No effect since equals 0.
                break;
            case 2:
                cf2_state |= _CF2_NSTOP_2;
                break;
            case 3:
                cf2_state |= _CF2_NSTOP_3;
                break;
            case 4:
                cf2_state |= _CF2_NSTOP_4;
                break;
            case 5:
                cf2_state |= _CF2_NSTOP_5;
                break;
            default:
                // Other codes (for 6, 7, 8) are invalid and give 1.
                cf2_state |= _CF2_NSTOP_1;
                self->num_stops = 1;
                #if LOG >= LOG_INFO
                printf("%i is not a valid number of stop pulses, defaulting to 1.\n", num_stops);
                #endif
        }
        #if LOG >= LOG_INFO
        printf("Set %i stop pulses.\n", self->num_stops);
        #endif
    }

    // Writing occasionally fails. Repeat until success.
    // Should usually only take 1 attempt.
    tdc7200_write8r(self, CONFIG2, cf2_state);

    // Interrupt mask
    if (retain_state) {
        im_state = self->reg1[INT_MASK];
    } else {
        switch (meas_mode) {
            case 1:
                im_state = _IM_COARSE_OVF | _IM_MEASUREMENT;
                break;
            case 2:
                im_state = _IM_CLOCK_OVF | _IM_COARSE_OVF | _IM_MEASUREMENT;
                break;
            default:
                im_state = 0;
        }
        self->reg1[INT_MASK] = im_state;
    }
    tdc7200_write8r(self, INT_MASK, im_state);

    // CLOCK_CNTR_STOP
    clock_cntr_stp = clock_cntr_stop;
    if (retain_state) {
        clock_cntr_stp = self->reg1[CLOCK_CNTR_STOP_MASK];
    } else {
        #if LOG >= LOG_INFO
        printf("Skipping STOP pulses for %i clock periods = %f ns\n", clock_cntr_stp, 1E9 * clock_cntr_stp * self->clk_period);
        #endif
        self->reg1[CLOCK_CNTR_STOP_MASK] = clock_cntr_stp & 0xFFFF;
        #if LOG >= LOG_INFO
        if (self->reg1[CLOCK_CNTR_STOP_MASK] != clock_cntr_stp) {
            printf("clock_cntr_stop %i too large, using %i\n", clock_cntr_stp, self->reg1[CLOCK_CNTR_STOP_MASK]);
        }
        #endif
        self->reg1[CLOCK_CNTR_STOP_MASK_H] = (clock_cntr_stp >> 8) & 0xFF;
        self->reg1[CLOCK_CNTR_STOP_MASK_L] = clock_cntr_stp & 0xFF;
    }
    self->clock_cntr_stop = clock_cntr_stp;
    tdc7200_write16r(self, CLOCK_CNTR_STOP_MASK_H, clock_cntr_stp);

    uint32_t tmout = 0;
    // Set overflow timeout.
    if (retain_state) {
        ovf = self->reg1[CLOCK_CNTR_OVF];
        tmout = (uint32_t)(ovf * (self->clk_period * 1.0E9));
        #if LOG >= LOG_INFO
        printf("Setting overflow timeout from retained state: %u = timeout %u ns.\n", ovf, tmout);
        #endif
    } else if (timeout_ns == 0) {
        ovf = clock_cntr_ovf;
        tmout = (uint32_t)(ovf * (self->clk_period * 1.0E9));;
        #if LOG >= LOG_INFO
        printf("Setting overflow timeout from owf: %u = timeout %u ns.\n", ovf, tmout);
        #endif
    } else {
        tmout = timeout_ns;
        ovf = (uint32_t)((double)timeout_ns / (self->clk_period*1.0E9));
        #if LOG >= LOG_INFO
        printf("Setting overflow timeout from timeout: %u = timeout %u ns\n", ovf, timeout_ns);
        #endif
    }
    self->clock_cntr_ovf = ovf;
    self->timeout = tmout;

    #if LOG >= LOG_INFO
    if ((meas_mode == 2) && (tmout < 2000)) {
        printf("WARNING: Timeout < 2000 nS and meas_mode == 2.\n");
        printf("Maybe measurement mode 1 would be better?\n");
    } else if ((meas_mode == 1) && (tmout > 2000)) {
        printf("WARNING: Timeout > 2000 nS and meas_mode == 1.\n");
        printf("Maybe measurement mode 2 would be better?\n");
    }
    #endif

    if (ovf <= clock_cntr_stop) {
        ovf = clock_cntr_stop + 1;
        #if LOG >= LOG_INFO
        printf("WARNING: clock_cntr_ovf must be greater than clock_cntr_stop.\n");
        printf("otherwise your measurement will stop before it starts.\n");
        printf("Set clock_cntr_ovf to %x\n", ovf);
        #endif
    }
    if (ovf > 0xFFFF) {
        printf("FATAL: clock_cntr_ovf exceeds max of 0xFFFF.\n");
        exit(1);
    }

    self->reg1[CLOCK_CNTR_OVF] = ovf;
    self->reg1[CLOCK_CNTR_OVF_H] = (ovf >> 8) & 0xFF;
    self->reg1[CLOCK_CNTR_OVF_L] = ovf & 0xFF;
    tdc7200_write16r(self, CLOCK_CNTR_OVF_H, ovf);
 }

void tdc7200_configure_defaults(tdc7200_obj_t *self) {
    tdc7200_configure(self, MHZ_12, true, 2, false, false, false, 10, 1, 1, 0, 0xFFFF, 0, false);
}

void tdc7200_reconfigure(tdc7200_obj_t *self) {
    tdc7200_configure(self, self->clk_freq, self->force_cal, self->meas_mode, self->trigg_falling, self->start_falling, self->stop_falling,
        self->calibration_periods, self->avg_cycles, self->num_stops, self->clock_cntr_stop, self->clock_cntr_ovf, self->timeout, false);
}

// returns 0 when edge detected; 1 if pin already in desired state; 2 on timeout
int _wait_for_edge(uint8_t pin, bool falling, uint32_t timeout_us) {
    #if LOG >= LOG_VERBOSE
    printf("Waiting for edge %i at pin %i with timeout %i ... ", falling, pin, timeout_us);
    #endif
    absolute_time_t tmtime = make_timeout_time_us(timeout_us);
    if (falling && (gpio_get(pin) == 0)) {
        #if LOG >= LOG_VERBOSE
        printf("already OK.\n");
        #endif
        return 1;
    } else if (!falling && (gpio_get(pin) != 0)) {
        #if LOG >= LOG_VERBOSE
        printf("already OK.\n");
        #endif
        return 1;
    }
    while (!time_reached(tmtime) && (gpio_get(pin) != (!falling))) {

    }
    bool is_tm = time_reached(tmtime);
    if (is_tm) {
        #if LOG >= LOG_VERBOSE
        printf(" TIMEOUT.\n");
        #endif
    } else {
        #if LOG >= LOG_VERBOSE
        printf("Done.\n");
        #endif
    }
    return (time_reached(tmtime)? 2: 0);
}

tdc7200_meas_t tdc7200_measure(tdc7200_obj_t *self) {
    // Check GPIO state doesn't indicate a measurement is happening.
    tdc7200_meas_t result;
    bool trigg_state = gpio_get(self->TRIGG);
    bool trigg_falling = (self->reg1[CONFIG1] & _CF1_TRIGG_EDGE) > 0;

    if (trigg_state != trigg_falling) {
        #if LOG >= LOG_INFO
        printf("ERROR 12: TRIGG should be %s. Reset the chip by repowering or reconfigure the device.\n", (trigg_falling ? "HIGH" : "LOW"));
        #endif
        tdc7200_reconfigure(self);
        result.error = 12;
        return result;
    }

    // To start measurement, need to set START_MEAS in TDCx_CONFIG1 register.
    bool timed_out = false;
    uint8_t cf1_state = self->reg1[CONFIG1];
    tdc7200_write8(self, CONFIG1, cf1_state | _CF1_START_MEAS);

    // Wait for edge on TRIGG.
    int e = _wait_for_edge(self->TRIGG, 0, (uint32_t)2e6);
    if (e == 2) {
        #if LOG >= LOG_INFO
        printf("ERROR 10: Timed out waiting for TRIGG edge.\n");
        #endif
        result.error = 10;
        return result;
    }
    e = _wait_for_edge(self->INTB, 1, (uint32_t)2e6);
    if (e == 2) {
        #if LOG >= LOG_INFO
        printf("ERROR 7: Timed out waiting for INTB edge.\n");
        #endif
        result.error = 7;
        return result;
    }

    // Read everything in and see what we got.
    tdc7200_read_regs24(self);
    self->reg1[INT_STATUS] = tdc7200_read8(self, INT_STATUS);
    result.int_status = self->reg1[INT_STATUS];
    result.calibration[0] = self->reg1[CALIBRATION1];
    result.calibration[1] = self->reg1[CALIBRATION2];
    if ((result.calibration[1]-result.calibration[0]) == 0) {
        result.error = 5;
        return result;
    }

    result.normLSB = self->clk_period / ((double)(result.calibration[1]-result.calibration[0])/(double)(self->calibration_periods-1) );
    #if LOG >= LOG_VERBOSE
    printf("normLSB: %e (%i, %i)\n", result.normLSB, result.calibration[0], result.calibration[1]);
    #endif
    uint8_t t_regs[6] = { TIME1, TIME2, TIME3, TIME4, TIME5, TIME6 };
    uint8_t c_regs[5] = { CLOCK_COUNT1, CLOCK_COUNT2, CLOCK_COUNT3, CLOCK_COUNT4, CLOCK_COUNT5 };
    result.num_stops = self->num_stops;
    for (int j = 0; j < self->num_stops; j++) {
        result.time[j] = self->reg1[t_regs[j]];
        result.time[j+1] = self->reg1[t_regs[j+1]];
        result.clock_count[j] = self->reg1[c_regs[j]];
        if (self->meas_mode == 1) {
            result.tof[j] = result.normLSB * (double)result.time[j];
        } else if (self->meas_mode == 2) {
            result.tof[j] = (result.normLSB * (double)(result.time[0]-result.time[j+1])) + ((double)result.clock_count[j] * self->clk_period);
        }
    } 
    if ((self->reg1[INT_STATUS] & _IM_CLOCK_OVF) || (self->reg1[INT_STATUS] & _IM_COARSE_OVF)) {
        result.error = 3;
//        return result;
    }
    result.error = 0;
    return result;
}
