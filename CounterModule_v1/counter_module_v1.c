#include <stdio.h>
#include <string.h>
#include "pico/stdlib.h"
#include "hardware/spi.h"
#include "counter_module_v1.h"
#include "uart.pio.h"

tdc7200_obj_t tdc;
bool _tdc_need_reconfigure = false;

void cm1_write_spi(spi_inst_t *spi, const uint8_t cs, const uint8_t reg, const uint32_t data) {
    uint8_t msg[5];

    msg[0] = 0x00 || reg;
    msg[1] = (data >> 24);
    msg[2] = (data >> 16) & 0xFF;
    msg[3] = (data >> 8) & 0xFF;
    msg[4] = data & 0xFF;

    gpio_put(cs, 0);
    spi_write_blocking(spi, msg, 2);
    gpio_put(cs, 1);
}

uint32_t cm1_read_spi(spi_inst_t *spi, const uint8_t cs, const uint8_t reg) {
    uint8_t msg = 0x80 | (0 << 6) | reg;
    uint8_t data[4];

    gpio_put(cs, 0);
    spi_write_blocking(spi, &msg, 1);
    uint8_t br = spi_read_blocking(spi, 0, data, 4);
    gpio_put(cs, 1);

    return (data[0] << 24) + (data[1] << 16) + (data[2] << 8) + data[3];
}

uint16_t cm1_uart_readline() {
    uint32_t st = time_us_32();
    uint16_t n = 0;
    char c = uart_rx_program_getc(pio0, 1);
    while ((c != '\n') && ((time_us_32()-st) < UART_TIMEOUT_US)) {
        if (c != '\0') {
            printf("%s", c);
            n++;
        }
        c = uart_rx_program_getc(pio0, 1);
    }
    printf("\n");
    if (c != '\n') 
        printf("uart timeout\n");
    return n;
}

void initialize_uart() {
    PIO pio = pio0;
    uint sm = 0;
    uint offset = pio_add_program(pio, &uart_tx_program);
    uart_tx_program_init(pio, sm, offset, 17, 9600);

    sm = 1;
    offset = pio_add_program(pio, &uart_rx_program);
    uart_rx_program_init(pio, sm, offset, 19, 9600);

    uart_tx_program_puts(pio, 0, "*IDN?");    
    if (0 == cm1_uart_readline()) {
        printf("WARNING: Cannot communicate with Pico divider.\n");
    }
}

void cm1_initialize() {
    // init GPIOs

    // init PicoDIV SPI communication
    initialize_uart();

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
        2,     // calibration_periods
        1,      // avg_cycles
        1,      // num_stops
        0,      // clock_cntr_stop
        0,      // clock_cntr_ovf 
        (uint32_t)1E5, // timeout_ns
        false   // retain_state
    );
}

// valid source CHA | CHB
void cm1_set_start_source(uint8_t source) {
    if (source != SOURCE_CHA && source != SOURCE_CHB) {
        printf("Invalid start source for Counter Module. (%u).", source);
        return;
    }
    gpio_put(SWCHIN1_PIN, (source == SOURCE_CHA));
}

// valid source CHB | DIVCLK | REFCLK
void cm1_set_stop_source(uint8_t source) {
    if (source != SOURCE_CHB && source != SOURCE_DIVCLK && source != SOURCE_REFCLK) {
        printf("Invalid stop source for Counter Module. (%u).", source);
        return;
    }
    if (source == SOURCE_CHB) {
        gpio_put(SWCHIN2_PIN, (source == SOURCE_CHB));
    } else {
        gpio_put(SWCHIN2_PIN, (source == SOURCE_CHB));
        gpio_put(SWCLKSEL1_PIN, (source == SOURCE_REFCLK));
    }
}

// valid source INTCLK | EXTCLK
void cm1_set_refclock(uint8_t source, uint32_t freq_hz) {
    if (source != SOURCE_INTCLK && source != SOURCE_EXTCLK) {
        printf("Invalid ref clock source for Counter Module. (%u).", source);
        return;
    }
    gpio_put(SWCLKSEL2_PIN, (source == SOURCE_EXTCLK));
}

// PicoDIV functions

uint8_t cm1_pdiv_get_refclock() {
 //   return cm1_read_spi(pdiv_spi, PDIVCS_PIN, 0x01) & 0xFF;
    return 0;
}

// valid source TCXO | REFCLK
void cm1_pdiv_set_refclock(uint8_t source, uint32_t freq_hz) {
    if (source != SOURCE_TCXO && source != SOURCE_EXTCLK) {
        printf("Invalid ref clock source for PicoDIV. (%u)", source);
        return;
    }
    char d[32];
    if (source == SOURCE_EXTCLK) {
        pio_sm_drain_tx_fifo(pio0, 0);
        if (freq_hz == 0)
            sprintf(d, "CONF:REF:EXT\n");
        else 
            sprintf(d, "CONF:REF:EXT %d\n", freq_hz);
        pio_sm_set_enabled(pio0, 0, false);
        pio_sm_restart(pio0, 0);
        pio_sm_set_enabled(pio0, 0, true);
        uart_tx_program_puts(pio0, 0, d);
        cm1_uart_readline();
    } else if (source == SOURCE_TCXO) {
        pio_sm_drain_tx_fifo(pio0, 0);
        sprintf(d, "CONF:REF:TCXO\n");
        pio_sm_set_enabled(pio0, 0, false);
        pio_sm_restart(pio0, 0);
        pio_sm_set_enabled(pio0, 0, true);
        uart_tx_program_puts(pio0, 0, d);
        cm1_uart_readline();
    }
}

uint32_t cm1_pdiv_get_refclock_freq() {
    return 0;
}

void cm1_pdiv_set_freq_hz(uint32_t freq_hz) {
    char d[32];
    sprintf(d, "CONF:DIV:FREQ %u\n", freq_hz);
    pio_sm_restart(pio0, 0);
    uart_tx_program_puts(pio0, 0, d);
    printf("%s", d);
}

uint32_t cm1_pdiv_get_freq_hz() {
    return 0;
}

void cm1_pdiv_set_plength_ns(uint32_t plength_ns) {
    char d[32];
    sprintf(d, "CONF:DIV:PULS %u\n", plength_ns);
    pio_sm_restart(pio0, 0);
    uart_tx_program_puts(pio0, 0, d);
}

uint32_t cm1_pdiv_get_plength_ns() {
    return 0;
}


void cm1_pdiv_slide(uint32_t n_refclock_cycles) {
    char d[32];
    sprintf(d, "DIV:SLID %u\n", n_refclock_cycles);
    pio_sm_set_enabled(pio0, 0, false);
    pio_sm_restart(pio0, 0);
    pio_sm_set_enabled(pio0, 0, true);
    uart_tx_program_puts(pio0, 0, d);
}

void cm1_pdiv_on() {
    char d[32];
    pio_sm_drain_tx_fifo(pio0, 0);
    sprintf(d, "DIV:ENAB ON\n");
    pio_sm_set_enabled(pio0, 0, false);
    pio_sm_restart(pio0, 0);
    pio_sm_set_enabled(pio0, 0, true);
    uart_tx_program_puts(pio0, 0, d);
    cm1_uart_readline();
}

void cm1_pdiv_off() {
    char d[32];
    pio_sm_drain_tx_fifo(pio0, 0);
    sprintf(d, "DIV:ENAB OFF\n");
    pio_sm_set_enabled(pio0, 0, false);
    pio_sm_restart(pio0, 0);
    pio_sm_set_enabled(pio0, 0, true);
    uart_tx_program_puts(pio0, 0, d);
}


// TDC7200

tdc7200_meas_t cm1_tic_measure() {
    if (_tdc_need_reconfigure) {
        tdc7200_reconfigure(&tdc);
        _tdc_need_reconfigure = false;
    }
    return tdc7200_measure(&tdc);
}

bool cm1_tic_set_meas_mode(uint8_t measurement_mode) {
    if ((measurement_mode-1) > 1) {
        #ifdef DEBUG
            printf("Invalid measurement mode (%d).\n", measurement_mode);
        #endif
        return false;
    }
    tdc.meas_mode = measurement_mode;
    _tdc_need_reconfigure = true;
    return true;
}

bool cm1_tic_set_calperiods(uint8_t periods) {
    if (periods == 2 || periods == 10 || periods == 20 || periods == 40) {
        tdc.calibration_periods = periods;
        _tdc_need_reconfigure = true;
        return true;
    } else {
        #ifdef DEBUG
            printf("Invalid calibration periods (%d).\n", periods);
        #endif
        return false;
    }
}

bool cm1_tic_set_force_calibration(bool force) {
    tdc.force_cal = force;
    _tdc_need_reconfigure = true;
}

bool cm1_tic_set_nstops(uint8_t nstops) {
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