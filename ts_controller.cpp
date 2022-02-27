
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "SCPI_parser/Vrekrer_scpi_parser.h"

extern "C" {
    #include "CounterModule_v1/counter_module_v1.h"
}

#define LED 25

SCPI_Parser ts_instrument;

uint32_t ref_freq_hz = 0;
bool tic_enabled = false;

#define DATA_BUF 256
double data[DATA_BUF];

void identify(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
  printf("TimeSystem with CounterModule v1,v0.1\n");
}

void scpi_errorhandler(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
  switch(ts_instrument.last_error) {
    case ts_instrument.ErrorCode::BufferOverflow: 
        printf("Buffer overflow error\n");
        break;
    case ts_instrument.ErrorCode::Timeout:
        printf("Communication timeout error\n");
        break;
    case ts_instrument.ErrorCode::UnknownCommand:
        printf("Unknown command received\n");
        break;
    case ts_instrument.ErrorCode::NoError:
        printf("No Error\n");
        break;
    case ts_instrument.ErrorCode::UnknownSource:
        printf("Unknown source\n");
        break;
    case ts_instrument.ErrorCode::InvalidMeasurementMode:
        printf("Invalid TIC measurement mode (%s). Only 1 | 2 valid.\n", parameters[0]);
        break;
    case ts_instrument.ErrorCode::InvalidCalPeriods:
        printf("Invalid TIC calibration periods (%s). Only 2 | 10 | 20 | 40 valid.\n", parameters[0]);
        break;
    case ts_instrument.ErrorCode::InvalidForceCalibration:
        printf("Invalid TIC force calibration setting (%s). Only ON | OFF.\n", parameters[0]);
        break;
    case ts_instrument.ErrorCode::InvalidNumberOfStops:
        printf("Invalid number of TIC stops (%s). Only 1 | 2 | 3 | 4 | 5.\n", parameters[0]);
        break;        
  }
  ts_instrument.last_error = ts_instrument.ErrorCode::NoError;    
}

void pdiv_set_freq(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        uint32_t p = atoi(parameters[0]);
        cm1_pdiv_set_freq_hz(p);
        printf("Set divider freq to %u\n", p);
    }
    gpio_put(LED, 0);
}

void pdiv_set_plength(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        uint32_t p = atoi(parameters[0]);
        cm1_pdiv_set_plength_ns(p);
        printf("Set pulse length to %u\n", p);
    }
    gpio_put(LED, 0);
}

void pdiv_set_ref(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "TCXO") == 0)
            cm1_pdiv_set_refclock(SOURCE_TCXO, XOSC_MHZ*MHZ_1);
        else if (strcmp(parameters[0], "EXT") == 0)
            cm1_pdiv_set_refclock(SOURCE_EXTCLK, ref_freq_hz);
        else {
            ts_instrument.last_error = ts_instrument.ErrorCode::UnknownSource;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);
}

void pdiv_slide(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        uint32_t p = atoi(parameters[0]);
        cm1_pdiv_slide(p);
        printf("Divider postponed by %u cycles \n", p);
    }
    gpio_put(LED, 0);
}

void pdiv_onoff(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "ON") == 0)
            cm1_pdiv_on();
        else if (strcmp(parameters[0], "OFF") == 0)
            cm1_pdiv_off();
    }
    gpio_put(LED, 0);
}

void tic_set_start_source(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "CHA") == 0)
            cm1_set_start_source(SOURCE_CHA);
        else if (strcmp(parameters[0], "CHB") == 0) 
            cm1_set_start_source(SOURCE_CHB);
        else {
            ts_instrument.last_error = ts_instrument.ErrorCode::UnknownSource;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);
}

void tic_set_stop_source(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "CHB") == 0)
            cm1_set_stop_source(SOURCE_CHB);
        else if (strcmp(parameters[0], "DIV") == 0) 
            cm1_set_stop_source(SOURCE_DIVCLK);
        else if (strcmp(parameters[0], "REF") == 0) 
            cm1_set_stop_source(SOURCE_REFCLK);
        else {
            ts_instrument.last_error = ts_instrument.ErrorCode::UnknownSource;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);
}

void ref_source(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 1) {
        ref_freq_hz = atoi(parameters[1]);
        if (strcmp(parameters[0], "INT") == 0) {
            printf("setting ref clock to Int @ %d\n", ref_freq_hz);
            cm1_set_refclock(SOURCE_INTCLK, ref_freq_hz);
            if (cm1_pdiv_get_refclock() == SOURCE_REFCLK) {
                cm1_pdiv_set_refclock(SOURCE_REFCLK, ref_freq_hz);
            }
        } else if (strcmp(parameters[0], "EXT") == 0) {
            printf("setting ref clock to Ext @ %d\n", ref_freq_hz);
            cm1_set_refclock(SOURCE_EXTCLK, ref_freq_hz);
            if (cm1_pdiv_get_refclock() == SOURCE_REFCLK) {
                cm1_pdiv_set_refclock(SOURCE_REFCLK, ref_freq_hz);
            }
        } else {
            ts_instrument.last_error = ts_instrument.ErrorCode::UnknownSource;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);
}

void tic_set_mode(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        uint8_t v = atoi(parameters[0]);
        if (!cm1_tic_set_meas_mode(v)) {
            ts_instrument.last_error = ts_instrument.ErrorCode::InvalidMeasurementMode;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);    
}

void tic_set_calperiods(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        uint8_t v = atoi(parameters[0]);
        if (!cm1_tic_set_calperiods(v)) {
            ts_instrument.last_error = ts_instrument.ErrorCode::InvalidCalPeriods;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);
}

void tic_set_calforce(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "ON") == 0) {
            cm1_tic_set_force_calibration(true);
        } else if (strcmp(parameters[0], "OFF") == 0) {
            cm1_tic_set_force_calibration(false); 
        } else {
            ts_instrument.last_error = ts_instrument.ErrorCode::InvalidForceCalibration;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);
}

void tic_set_nstops(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {        
        uint8_t v = atoi(parameters[0]);
        if (!cm1_tic_set_nstops(v)) {
            ts_instrument.last_error = ts_instrument.ErrorCode::InvalidNumberOfStops;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);
}

void tic_enable(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        tic_enabled = (strcmp(parameters[0], "ON") == 0);
    }
    gpio_put(LED, 0);
}

void initialize_scpi() {
    ts_instrument.RegisterCommand("*IDN?", &identify);
    ts_instrument.RegisterCommand("DIVider:SLIDe", &pdiv_slide);
    ts_instrument.RegisterCommand("DIVider:ENABle", &pdiv_onoff);
    ts_instrument.RegisterCommand("TIC:ENABle", &tic_enable);
    ts_instrument.RegisterCommand("CONFigure:REFerence", &ref_source);
    ts_instrument.SetCommandTreeBase("CONFigure:DIVider");
        ts_instrument.RegisterCommand(":FREQuency", &pdiv_set_freq);
        ts_instrument.RegisterCommand(":PULSelength", &pdiv_set_plength);
        ts_instrument.RegisterCommand(":REFerence", &pdiv_set_ref);
    ts_instrument.SetCommandTreeBase("CONFigure:TIC");
        ts_instrument.RegisterCommand(":MODE", &tic_set_mode);
        ts_instrument.RegisterCommand(":CALPeriods", &tic_set_calperiods);
        ts_instrument.RegisterCommand(":CALForce", &tic_set_calforce);
        ts_instrument.RegisterCommand(":STOP", &tic_set_stop_source);
        ts_instrument.RegisterCommand(":STARt", &tic_set_start_source);
        ts_instrument.RegisterCommand(":NSTOps", &tic_set_nstops);
    ts_instrument.SetErrorHandler(&scpi_errorhandler);
}

void core1_main() {
    while(true) {
        ts_instrument.ProcessInput(SCPI_Interface(), "\r\n");
    }
}

int main() {
    stdio_init_all();
    sleep_ms(3000);
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
    gpio_put(LED, 1);

    printf("Awaiting messages in 1s...\n");
    cm1_initialize();
    initialize_scpi();
    multicore_launch_core1(core1_main);
    sleep_ms(1000);

    tdc7200_meas_t msmt;

    uint16_t j = 0;
    while(true) {
        //ts_instrument.ProcessInput(SCPI_Interface(), "\n");
        if (tic_enabled) {
            gpio_put(LED, 0);
            msmt = cm1_tic_measure();
            gpio_put(LED, 1);
            if (msmt.error == 0) {
                for (int i = 0; i < msmt.num_stops; i++) {
                    printf("%0.3f\t", msmt.tof[i]*1e9);
                }
                printf("\n");
            } else {
                printf("error: %i, num_stops: %i, normLSB: %e\n", msmt.error, msmt.num_stops, msmt.normLSB);
            }
/*
            if (j == DATA_BUF) {
                gpio_put(LED, 0);
                for (int i = 0; i < DATA_BUF; i++) {
                    printf("%0.3f\n", data[i]*1e9);
                }
                j = 0;
                gpio_put(LED, 1);
            } 
            msmt = cm1_tic_measure();
            if (msmt.error == 0) {
                data[j] = msmt.tof[0];
                j++;
            } else {
                printf("error: %i, num_stops: %i, normLSB: %e\n", msmt.error, msmt.num_stops, msmt.normLSB);
            } */
        }
    }
}
