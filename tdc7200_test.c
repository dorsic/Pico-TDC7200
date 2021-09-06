
#include <stdlib.h>
#include <stdio.h>
#include "pico/stdlib.h"
#include "TDC7200/tdc7200.h"

#define LED 25

int main() {
    stdio_init_all();
    sleep_ms(3000);
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
    gpio_put(LED, 1);
    printf("Configuring TDC7200 for measurements...\n");
    tdc7200_obj_t tdc = tdc7200_create_defaults();
   // void tdc7200_configure(tdc7200_obj_t *self, uint32_t clk_freq, bool force_cal, uint8_t meas_mode, bool trigg_falling,
   //     bool falling, uint8_t calibration_periods, uint8_t avg_cycles, uint8_t num_stops, 
   //     uint16_t clock_cntr_stop, uint16_t clock_cntr_ovf, uint32_t timeout, bool retain_state)
    tdc7200_configure(&tdc, MHZ_10, 
        true,   // force_cal
        2,      // meas_mode
        true,   // trigg_edge_falling;  set to true if you see on the osciloscope LOW on initialising measurement rising edge on start pulse and falling edge on initialising second measurement
        true,  // start_edge_falling   // NOTE the invertors on the ChA input
        false,  // stop_edge_falling
        40,     // calibration_periods
        1,      // avg_cycles
        1,      // num_stops
        0,      // clock_cntr_stop
        0,      // clock_cntr_ovf 
        (uint32_t)10E9, // timeout_ns
        false   // retain_state
    );
//    tdc7200_configure_defaults(&tdc);
    printf("Starting measurements in 1s...\n");
    sleep_ms(1000);
    tdc7200_meas_t msmt;
    while(true) {
        gpio_put(LED, 0);
        msmt = tdc7200_measure(&tdc);
        gpio_put(LED, 1);
        if (msmt.error == 0) {
            for (int i = 0; i < msmt.num_stops; i++) {
                printf("time0\t%i\ttime%i\t%i\tclock%i\t%i\tnormLBS\t%e\ttof%i\t%0.3f\tns\n", msmt.time[0], i+1, msmt.time[i+1], i+1, msmt.clock_count[i], msmt.normLSB, i+1, msmt.tof[i]*1e9);
            }
        } else {
            printf("error: %i, num_stops: %i, normLSB: %e\n", msmt.error, msmt.num_stops, msmt.normLSB);
        }
    }
}