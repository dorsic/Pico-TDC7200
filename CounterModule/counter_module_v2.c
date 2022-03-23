#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pico/stdlib.h"
#include "counter_module_v2.h"
#include "tic_gate.pio.h"

#ifdef PDIVCOM_UART
#include "uart.pio.h"
#endif
#ifdef PDIVCOM_SPI
#include "hardware/spi.h"
#endif

tdc7200_obj_t tdc;
bool _tdc_need_reconfigure = false;

uint8_t _cm_meas_mode = 0;
bool _cm_tic_pet_enabled = false;
uint32_t _freq_hz = MHZ_12;
char buf[32];

#ifdef PDIVCOM_UART

uint16_t cm_uart_readline(char *buf) {
    uint32_t st = time_us_32();
    uint16_t n = 0;
    char c = uart_rx_program_getc(UART_PIO, UART_RX_SM);
    while ((c != '\n') && ((time_us_32()-st) < UART_TIMEOUT_US)) {
        if (c != '\0') {
            buf[n]=c;
//            printf("%c", c);
            n++;
            st = time_us_32();
        }
        c = uart_rx_program_getc(UART_PIO, UART_RX_SM);
    }
    buf[n] = '\0';
//    printf("\n");
    if (c != '\n') 
        printf("uart timeout\n");
    return n;
}

void cm_uart_write(const char* message) {
//    printf("%s", message);
    pio_sm_set_enabled(UART_PIO, UART_TX_SM, false);
    pio_sm_restart(UART_PIO, UART_TX_SM);
    pio_sm_set_enabled(UART_PIO, UART_TX_SM, true);
    uart_tx_program_puts(UART_PIO, UART_TX_SM, message);
}

uint16_t cm_uart_writereadline(const char* message, char* buf) {
    pio_sm_set_enabled(UART_PIO, UART_TX_SM, false);
    pio_sm_restart(UART_PIO, UART_TX_SM);
    pio_sm_set_enabled(UART_PIO, UART_TX_SM, true);
    pio_sm_clear_fifos(UART_PIO, UART_RX_SM);
    pio_sm_set_enabled(UART_PIO, UART_RX_SM, false);
    pio_sm_restart(UART_PIO, UART_RX_SM);
    pio_sm_set_enabled(UART_PIO, UART_RX_SM, true);
    uart_tx_program_puts(UART_PIO, UART_TX_SM, message);
    return cm_uart_readline(buf);
}

void cm_initialize_uart() {
    uint offset = pio_add_program(UART_PIO, &uart_tx_program);
    uart_tx_program_init(UART_PIO, UART_TX_SM, offset, PDIVTX_PIN, 9600);
    offset = pio_add_program(UART_PIO, &uart_rx_program);
    uart_rx_program_init(UART_PIO, UART_RX_SM, offset, PDIVRX_PIN, 9600);

    if (0 == cm_uart_writereadline("*IDN?", buf)) {
        printf("WARNING: Cannot communicate with Pico divider.\n");
    }
}
#endif

#ifdef PDIVCOM_SPI
void cm_spi_write(const char* message) {
    gpio_put(PDIVCS_PIN, 0); // Indicate beginning of communication
    spi_write_blocking(SPI, message, strlen(message)); // Send data[]
    gpio_put(PDIVCS_PIN, 1); // Signal end of communication
}

uint16_t cm_spi_writeread(const char* message, char* buf, uint16_t buflen) {
    strcpy(buf, message);
    gpio_put(PDIVCS_PIN, 0); // Indicate beginning of communication
    uint16_t n = spi_write_read_blocking(SPI, message, buf, buflen);
    gpio_put(PDIVCS_PIN, 1); // Signal end of communication
    return n;
}

void cm_initialize_spi() {
    spi_init(SPI, SPI_MHZ * 1000000);    //Initialise GPIO pins for SPI communication
    gpio_set_function(PDIVRX_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PDIVCLK_PIN, GPIO_FUNC_SPI);
    gpio_set_function(PDIVTX_PIN, GPIO_FUNC_SPI);
    // Configure Chip Select
    gpio_init(PDIVCS_PIN); // Initialise CS Pin
    gpio_set_dir(PDIVCS_PIN, GPIO_OUT); // Set CS as output
    gpio_put(PDIVCS_PIN, 1); // Set CS High to indicate no currect SPI communication
}
#endif

void cm_forward(const char* message) {
    #ifdef PDIVCOM_UART
    cm_uart_write(message);
    #endif
    #ifdef PDIVCOM_SPI
    cm_spi_write(message);
    #endif
}

uint16_t cm_forward_and_read(const char* message, char* buf) {
    #ifdef PDIVCOM_UART
    return cm_uart_writereadline(message, buf);
    #endif
    #ifdef PDIVCOM_SPI
    cm_spi_write(message);
    #endif
}

void cm_initialize_com() {
    #ifdef PDIVCOM_UART
    cm_initialize_uart();
    #endif    
    #ifdef PDIVCOM_SPI
    cm_initialize_spi();
    #endif    
}

void cm_initialize() {
    // init GPIOs

    // init PicoDIV SPI communication
    cm_initialize_com();

    gpio_init(SWCHIN1_PIN);
    gpio_set_dir(SWCHIN1_PIN, GPIO_OUT);
    gpio_put(SWCHIN1_PIN, 1);       // CHA

    gpio_init(SWCHIN2_PIN);
    gpio_set_dir(SWCHIN2_PIN, GPIO_OUT);
    gpio_put(SWCHIN2_PIN, 1);       // CHB

    gpio_init(SWCLKSEL1_PIN);
    gpio_set_dir(SWCLKSEL1_PIN, GPIO_OUT);
    gpio_put(SWCLKSEL1_PIN, 1);       //    RefClK

    gpio_init(SWCLKSEL2_PIN);
    gpio_set_dir(SWCLKSEL2_PIN, GPIO_OUT);
    gpio_put(SWCLKSEL2_PIN, 1);       //    ExtClK


    // init TDC
    printf("Configuring TDC7200 for measurements...\n");
    tdc = tdc7200_create(spi1, SCLK_PIN, CSB_PIN, DIN_PIN, DOUT_PIN, INTB_PIN, TRIGG_PIN, MHZ_10);

    tdc7200_configure(&tdc, MHZ_10, 
        true,   // force_cal
        1,      // meas_mode
        true,   // trigg_edge_falling;  set to true if you see on the osciloscope LOW on initialising measurement rising edge on start pulse and falling edge on initialising second measurement
        false,  // start_edge_falling
        false,  // stop_edge_falling
        10,     // calibration_periods
        1,      // avg_cycles
        1,      // num_stops
        0,      // clock_cntr_stop
        0,      // coarse_cnt_ovf
        0,      // clock_cntr_ovf 
        (uint32_t)MODE1_TIMEOUT_NS, // timeout_ns
        false   // retain_state
    );

    uint offset = pio_add_program(GATE_PIO, &tic_gate_program);
    tic_gate_program_init(GATE_PIO, GATE_SM, offset, GATE_PIN);
    pio_sm_set_enabled(GATE_PIO, GATE_SM, true);

}

// PicoDIV functions

// TDC7200

// valid source INTCLK | EXTCLK | XO
bool cm_set_refclock(uint8_t source, uint32_t freq_hz) {
    if (source != SOURCE_INTCLK && source != SOURCE_EXTCLK && source != SOURCE_XO) {
        printf("Invalid ref clock source for Counter Module. (%u).", source);
        return false;
    }
    char msg[32];
    if (source == SOURCE_XO) {
        gpio_put(SWCLKSEL1_PIN, true);
        gpio_put(SWCLKSEL2_PIN, false);
        sprintf(msg, ":DIV:REF XO\n");
    }
    if (source == SOURCE_EXTCLK) {
        gpio_put(SWCLKSEL1_PIN, true);
        gpio_put(SWCLKSEL2_PIN, true);
        sprintf(msg, ":DIV:REF EXT,%d\n", freq_hz);
    }
    if (source == SOURCE_INTCLK) {
        gpio_put(SWCLKSEL1_PIN, false);
        gpio_put(SWCLKSEL2_PIN, true);
        sprintf(msg, ":DIV:REF INT,%d\n", freq_hz);
    }
    cm_forward_and_read(msg, msg);
    bool ret = strcmp(msg, "OK") == 0;
    if (ret)
        _freq_hz = (source != SOURCE_XO)? freq_hz: MHZ_12;
    return ret;
}

uint32_t cm_get_pet() {
    uint16_t n;
    n = cm_forward_and_read(":TIC:PET?\n", buf);
//    printf("Read %d chars (%s)\n", n, buf);
    if (n>0) {
        return atoi(buf);
    } else {
        return -1;
    }    
}


tdc7200_meas_t cm_tic_measure() {
    if (_tdc_need_reconfigure) {
        tdc7200_reconfigure(&tdc);
        _tdc_need_reconfigure = false;
    }
    if (_cm_meas_mode == 3 && _cm_tic_pet_enabled) {        
        cm_forward_and_read(":TIC:STAR CH1\n", buf);
//        cm_forward_and_read(":TIC:STOP REF\n", buf);
        tdc7200_meas_t r1 = tdc7200_measure(&tdc);
        cm_forward_and_read(":TIC:STAR CH2\n", buf);
        tdc7200_meas_t r2 = tdc7200_measure(&tdc);        
        uint32_t clk = cm_get_pet();
//        printf("r1.t1=%.3f r1.t2=%.3f clk=%d r2.t1=%.3f r2.t2=%.3f \n", r1.tof[0]*1.0e9, r1.tof[1]*1.0e9, clk, r2.tof[0]*1.0e9, r2.tof[1]*1.0e9);
//        printf("r1.t1=%.3f clk=%d r2.t1=%.3f \n", r1.tof[0]*1.0e9, clk, r2.tof[0]*1.0e9);
        r1.tof[0] = r1.tof[0] - r2.tof[0] + (double)clk/(double)_freq_hz*2.0;
        r1.clock_count[0] = clk;
        return r1;
    } else {
        return tdc7200_measure(&tdc);
    }
}

bool cm_tic_set_meas_mode(uint8_t measurement_mode) {
    if ((measurement_mode-1) > 2) {
        #ifdef DEBUG
            printf("Invalid measurement mode (%d).\n", measurement_mode);
        #endif
        return false;
    }
    _cm_meas_mode = measurement_mode;
    tdc.meas_mode = measurement_mode % 2;
    tdc.timeout = measurement_mode==1? MODE1_TIMEOUT_NS : MODE2_TIMEOUT_NS;
    tdc.clock_cntr_ovf = 0;
    tdc.coarse_cntr_ovf = 0;
    _tdc_need_reconfigure = true;
    return true;
}

bool cm_tic_set_calperiods(uint8_t periods) {
    if (periods == 2 || periods == 10 || periods == 20 || periods == 40) {
        tdc.calibration_periods = periods;
        _tdc_need_reconfigure = true;
    } else {
        #ifdef DEBUG
            printf("Invalid calibration periods (%d).\n", periods);
        #endif
        return false;
    }
    return true;
}

bool cm_tic_set_force_calibration(bool force) {
    tdc.force_cal = force;
    _tdc_need_reconfigure = true;
    return true;
}

bool cm_tic_set_nstops(uint8_t nstops) {
    if ((nstops > 0) && (nstops < 6)) {
        tdc.num_stops = nstops;
        _tdc_need_reconfigure = true;
        return true;
    }
    #ifdef DEBUG
        printf("Invalid number of stops (%d).\n", nstops);
    #endif
    return false;
}

bool cm_tic_set_edge(uint8_t channel, bool falling) {
    if (channel == SOURCE_START) {
        tdc.start_falling = falling;
        _tdc_need_reconfigure = true;
    } else if (channel != SOURCE_STOP) {
        tdc.stop_falling = falling;
        _tdc_need_reconfigure = true;
    } else {
        #ifdef DEBUG
        #endif
        return false;
    }
    return true;
}

bool cm_tic_pet_enable(bool on) {
    char ret[4];
    if (on) {
        _cm_tic_pet_enabled = true;
        cm_forward_and_read(":TIC:PET:ENAB 1\n", ret);
        pio_sm_set_enabled(GATE_PIO, GATE_SM, true);
    } else {
        _cm_tic_pet_enabled = false;
        cm_forward_and_read(":TIC:PET:ENAB 0\n", ret);
        pio_sm_set_enabled(GATE_PIO, GATE_SM, false);
    }
    return strcmp(ret, "OK") == 0;
}

bool cm_tic_pet_enabled() {
    return _cm_tic_pet_enabled;
}

uint8_t cm_get_meas_mode() {
    return _cm_meas_mode;
}