
#include <stdio.h>
#include "pico/stdlib.h"
#include "hardware/pio.h"
#include "scpi.h"
#include "CounterModule_v1/counter_module_v1.h"

#define LED 25

typedef void (*SCPI_caller_t)(uint32_t param);

void identify(int32_t param) {
  printf("TimeSystem with CounterModule v1,v0.1\n");
}

void led_on(int32_t param) {
  gpio_put(LED, 1);
}

void led_off(int32_t param) {
  gpio_put(LED, 0);
}

void pdiv_set_freq(int32_t param) {
    gpio_put(LED, 1);
    if (param > 0) {
        cm_pdiv_set_freq_hz(param);
        printf("Set divider freq to %s\n", param);
    }
    gpio_put(LED, 0);
}

void pdiv_set_plength(int32_t param) {
    gpio_put(LED, 1);
    if (param > 0) {
        cm_pdiv_set_plength_ns(param);
        printf("Set pulse length to %s\n", param);
    }
    gpio_put(LED, 0);
}

void pdiv_slide(int32_t param) {
    gpio_put(LED, 1);
    if (param > 0) {
        cm_pdiv_slide(param);
        printf("Divider postponed by %s cycles \n", param);
    }
    gpio_put(LED, 1);
}

void pdiv_on(int32_t param) {
    gpio_put(LED, 1);
    printf("Enabling divider.\n");
    cm_pdiv_on();
    gpio_put(LED, 0);
}

void pdiv_off(int32_t param) {
    gpio_put(LED, 1);
    printf("Disabling divider.\n");
    cm_pdiv_off();
    gpio_put(LED, 0);
}

void scpi_errorhandler(int32_t param) {
  switch(param){
    case SCPI_ERR_BUFFER_OVERFLOW: 
      printf("Buffer overflow error\n");
      break;
    case SCPI_ERR_BUFFER_TIMEOUT:
      printf("Communication timeout error\n");
      break;
    case SCPI_ERR_UNKNOWN:
      printf("Unknown command received\n");
      break;
  }
}

void initialize_scpi() {
    scpi_register_command("*IDN?", &identify);
    scpi_register_errorhandler(&scpi_errorhandler);
}

int main() {
    stdio_init_all();
    sleep_ms(3000);
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
    gpio_put(LED, 1);

    sleep_ms(1000);
    gpio_put(LED, 0);
    sleep_ms(1000);
    gpio_put(LED, 1);
    sleep_ms(1000);
    gpio_put(LED, 0);

    printf("Awaiting messages in 1s...\n");
//    cm_initialize();

    initialize_scpi();
    sleep_ms(1000);
 //   tdc7200_meas_t msmt;

    while(true) {
        ts_instrument.ProcessInput(SCPI_Interface(), "\n");    
//        scpi_process_input();
    }

/*
        gpio_put(LED, 0);
        msmt = cm_measure();
        gpio_put(LED, 1);
        if (msmt.error == 0) {
            for (int i = 0; i < msmt.num_stops; i++) {
                printf("%0.3f\tCh%s\n", msmt.tof[i]*1e9, "A");
            }
        } else {
            printf("error: %i, num_stops: %i, normLSB: %e\n", msmt.error, msmt.num_stops, msmt.normLSB);
        }
    }
    */
}