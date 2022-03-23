
#include <stdio.h>
#include "pico/stdlib.h"
#include "pico/multicore.h"
#include "hardware/pio.h"
#include "SCPI_parser/Vrekrer_scpi_parser.h"

extern "C" {
    #include "CounterModule/counter_module_v2.h"
}

#define LED 25

SCPI_Parser ts_instrument;

uint32_t ref_freq_hz = 0;
bool tic_enabled = false;
char str[32];

//#define DATA_BUF 256
//double data[DATA_BUF];

uint32_t multiplier(const char *str) {
    switch (str[strlen(str)-1]) {
        case 'k':
        case 'K':
            return 1000;
        case 'm':
        case 'M':
            return 1000000;
        case '%':
            return 100;
    }
    return 1;
}

void identify(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    #ifdef COUNTER_MODULE_V1_H_
    printf("TimeSystem with CounterModule v1,v0.1\n");
    #endif
    #ifdef COUNTER_MODULE_V2_H_
    interface.putchars("TimeSystem with CounterModule v2,v0.2\n");
    #endif
    gpio_put(LED, 0);    
}

void _ack(SCPI_I interface) {
    interface.putchars("OK\n");
}

void scpi_errorhandler(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
  switch(ts_instrument.last_error) {
    case ts_instrument.ErrorCode::NoError:
        printf("No Error\n");
        break;
    case ts_instrument.ErrorCode::UnknownCommand:
        printf("Unknown command received\n");
        break;        
    case ts_instrument.ErrorCode::Timeout:
        printf("Communication timeout error\n");
        break;
    case ts_instrument.ErrorCode::BufferOverflow: 
        printf("Buffer overflow error\n");
        break;
    case ts_instrument.ErrorCode::CommandOverflow: 
        printf("Buffer overflow error\n");
        break;
    case ts_instrument.ErrorCode::InvalidParameter:
        printf("Invalid parameter value received\n");
        break;        
    case ts_instrument.ErrorCode::MissingParameter:
        printf("Missing parameter\n");
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
    case ts_instrument.ErrorCode::InvalidEdgeDef:
        printf("Invalid edge definition (%s). Only RISING | FALLING | 0 | 1 allowed\n", parameters[1]);
        break;        
  }
  ts_instrument.last_error = ts_instrument.ErrorCode::NoError;    
}

#ifdef COUNTER_MODULE_V1_H_
void pdiv_set_freq(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        uint32_t p = atoi(parameters[0]);
        cm_pdiv_set_freq_hz(p);
        printf("Set divider freq to %u\n", p);
    }
    gpio_put(LED, 0);
}

void pdiv_set_plength(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        cm_pdiv_set_pulselength(parameters[0]);
        printf("Set pulse length to %s\n", parameters[0]);
    }
    gpio_put(LED, 0);
}

void pdiv_set_ref(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "XO") == 0)
            cm_pdiv_set_refclock(SOURCE_XO, XOSC_MHZ*MHZ_1);
        else if (strcmp(parameters[0], "EXT") == 0)
            cm_pdiv_set_refclock(SOURCE_EXTCLK, ref_freq_hz);
        else {
            ts_instrument.last_error = ts_instrument.ErrorCode::UnknownSource;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);   
}

void pdiv_delay(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1); 
    if (parameters.Size() > 0) {
        uint32_t p = atoi(parameters[0]);
        cm_pdiv_delay(p);
        printf("Divider delayed by %u cycles \n", p);
    } 
    gpio_put(LED, 0);
}

void pdiv_enable(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);

    if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "ON") == 0 || strcmp(parameters[0], "1") == 0)
            cm_pdiv_enable(true);
        else if (strcmp(parameters[0], "OFF") == 0 || strcmp(parameters[0], "0") == 0)
            cm_pdiv_enable(false);
    }   
    gpio_put(LED, 0);
}

void tic_set_start_source(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);   
    if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "CH1") == 0)
            cm_set_start_source(SOURCE_CH1);
        else if (strcmp(parameters[0], "CH2") == 0) 
            cm_set_start_source(SOURCE_CH2);
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
        if (strcmp(parameters[0], "CH2") == 0)
            cm_set_stop_source(SOURCE_CH2);
        else if (strcmp(parameters[0], "DIV") == 0) 
            cm_set_stop_source(SOURCE_DIVCLK);
        else if (strcmp(parameters[0], "REF") == 0) 
            cm_set_stop_source(SOURCE_REFCLK);
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
            cm_set_refclock(SOURCE_INTCLK, ref_freq_hz);
            if (cm_pdiv_get_refclock() == SOURCE_REFCLK) {
                cm_pdiv_set_refclock(SOURCE_REFCLK, ref_freq_hz);
            }
        } else if (strcmp(parameters[0], "EXT") == 0) {
            printf("setting ref clock to Ext @ %d\n", ref_freq_hz);
            cm_set_refclock(SOURCE_EXTCLK, ref_freq_hz);
            if (cm_pdiv_get_refclock() == SOURCE_REFCLK) {
                cm_pdiv_set_refclock(SOURCE_REFCLK, ref_freq_hz);
            }
        } else {
            ts_instrument.last_error = ts_instrument.ErrorCode::UnknownSource;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);
}
#endif

void ref_source(SCPI_C commands, SCPI_P parameters, SCPI_I interface) { 
    gpio_put(LED, 1);
    if (parameters.Size() > 1) {
        ref_freq_hz = atoi(parameters[1]) * multiplier(parameters[1]);
        if (strcmp(parameters[0], "INT") == 0) {
            printf("setting ref clock to Int @ %d\n", ref_freq_hz);
            if (cm_set_refclock(SOURCE_INTCLK, ref_freq_hz))
                _ack(interface);
        } else if (strcmp(parameters[0], "EXT") == 0) {
            printf("setting ref clock to Ext @ %d\n", ref_freq_hz);
            if (cm_set_refclock(SOURCE_EXTCLK, ref_freq_hz))
                _ack(interface);
        } else if (strcmp(parameters[0], "XO") == 0) {
            printf("setting ref clock to XO @ %d\n", ref_freq_hz);
            if (cm_set_refclock(SOURCE_XO, 0)) 
                _ack(interface);
        } else {
            ts_instrument.last_error = ts_instrument.ErrorCode::UnknownSource;
            scpi_errorhandler(commands, parameters, interface);
        }
    } else if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "XO") == 0) {
            printf("setting ref clock to XO\n", 0);
            if (cm_set_refclock(SOURCE_XO, 0))
                _ack(interface);
        } else {
            ts_instrument.last_error = ts_instrument.ErrorCode::MissingParameter;
            scpi_errorhandler(commands, parameters, interface);
        }        
    } else {
        ts_instrument.last_error = ts_instrument.ErrorCode::MissingParameter;
        scpi_errorhandler(commands, parameters, interface);        
    }
    gpio_put(LED, 0);
}

void get_ref_freq(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    cm_forward_and_read("DIV:REF:FREQ?\n", str);
    interface.putchars(str);
}

void tic_set_mode(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        uint8_t v = atoi(parameters[0]);
        if (!cm_tic_set_meas_mode(v)) {
            ts_instrument.last_error = ts_instrument.ErrorCode::InvalidMeasurementMode;
            scpi_errorhandler(commands, parameters, interface);
        } else {
            _ack(interface);
        }
    }
    gpio_put(LED, 0);    
}

void tic_set_calperiods(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        uint8_t v = atoi(parameters[0]);
        if (!cm_tic_set_calperiods(v)) {
            ts_instrument.last_error = ts_instrument.ErrorCode::InvalidCalPeriods;
            scpi_errorhandler(commands, parameters, interface);
        } else {
            _ack(interface);
        }
    }
    gpio_put(LED, 0);
}

void tic_set_calforce(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "ON") == 0 || strcmp(parameters[0], "1") == 0) {
            cm_tic_set_force_calibration(true);
            _ack(interface);
        } else if (strcmp(parameters[0], "OFF") == 0 || strcmp(parameters[0], "0") == 0) {
            cm_tic_set_force_calibration(false);
            _ack(interface);
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
        if (!cm_tic_set_nstops(v)) {
            ts_instrument.last_error = ts_instrument.ErrorCode::InvalidNumberOfStops;
            scpi_errorhandler(commands, parameters, interface);
        } else {
            _ack(interface);
        }
    }
    gpio_put(LED, 0);
}

void tic_enable(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        tic_enabled = (strcmp(parameters[0], "ON") == 0 || strcmp(parameters[0], "1") == 0);
        _ack(interface);
    }
    gpio_put(LED, 0);
}

void tic_set_edge(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    uint8_t src = 0;
    bool falling = false;
    if (parameters.Size() > 1) {
        if (strcmp(parameters[0], "START") == 0) {
            src = SOURCE_START;
        } else if (strcmp(parameters[0], "STOP") == 0) {
            src = SOURCE_STOP;
        } else {
            ts_instrument.last_error = ts_instrument.ErrorCode::UnknownSource;
            scpi_errorhandler(commands, parameters, interface);
            gpio_put(LED, 0);
            return;
        }
        if (strcmp(parameters[1], "RISING") == 0 || strcmp(parameters[1], "1") == 0) {
            falling = false;
        } else if (strcmp(parameters[1], "FALLING") == 0 || strcmp(parameters[1], "0") == 0) {
            falling = true;
        } else {
            ts_instrument.last_error = ts_instrument.ErrorCode::InvalidEdgeDef;
            scpi_errorhandler(commands, parameters, interface);
            gpio_put(LED, 0);
            return;
        }
        if (cm_tic_set_edge(src, falling))
            _ack(interface);
    } else {
        ts_instrument.last_error = ts_instrument.ErrorCode::MissingParameter;
        scpi_errorhandler(commands, parameters, interface);
    }
    gpio_put(LED, 0);
}


void tic_pet_enable(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    gpio_put(LED, 1);
    if (parameters.Size() > 0) {
        if (strcmp(parameters[0], "ON") == 0 || strcmp(parameters[0], "1") == 0) {
            if (cm_tic_pet_enable(true))
                _ack(interface);
        } else if (strcmp(parameters[0], "OFF") == 0 || strcmp(parameters[0], "0") == 0) {
            if (cm_tic_pet_enable(false))
                _ack(interface);
        } else {
            ts_instrument.last_error = ts_instrument.ErrorCode::InvalidForceCalibration;
            scpi_errorhandler(commands, parameters, interface);
        }
    }
    gpio_put(LED, 0);
}

void read_pet(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    sprintf(str, "%d", cm_get_pet());
    interface.putchars(str);
}

void _serialize_command(char* str, SCPI_C commands, SCPI_P parameters) {
    sprintf(str, "");
    for (uint8_t i = 0; i < commands.Size(); i++)
        sprintf(str, "%s:%s", str, commands[i]);
    
    for (uint8_t i = 0; i < parameters.Size(); i++)
        sprintf(str, "%s %s", str, parameters[i]);
    sprintf(str, "%s\n", str);
}

void cm_fwd_w(SCPI_C commands, SCPI_P parameters, SCPI_I interface) {
    #ifdef COUNTER_MODULE_V2_H_
    _serialize_command(str, commands, parameters);
    cm_forward_and_read(str, str);
    interface.putchars(str);
    #endif
}

void initialize_scpi() {
    ts_instrument.RegisterCommand("*IDN?", &identify);
    ts_instrument.RegisterCommand(":CONFigure:REFerence", &ref_source);
    ts_instrument.RegisterCommand(":REFerence:FREQuency?", &get_ref_freq);
    ts_instrument.SetCommandTreeBase(":TIC");
        ts_instrument.RegisterCommand(":ENABle", &tic_enable);
        ts_instrument.RegisterCommand(":MODE", &tic_set_mode);
        ts_instrument.RegisterCommand(":CALPeriods", &tic_set_calperiods);
        ts_instrument.RegisterCommand(":CALForce", &tic_set_calforce);
        ts_instrument.RegisterCommand(":STOP", &cm_fwd_w);
        ts_instrument.RegisterCommand(":STARt", &cm_fwd_w);
        ts_instrument.RegisterCommand(":NSTOps", &tic_set_nstops);
        ts_instrument.RegisterCommand(":IMPedance", &cm_fwd_w);      // param <50 | 0> | <1M | 1>
        ts_instrument.RegisterCommand(":EDGe", &tic_set_edge);      // params <CH1 | CH2> | <<RISING | 1> | <FALLING | 0>>
        ts_instrument.RegisterCommand(":PET?", &read_pet);      // params <CH1 | CH2> | <<RISING | 1> | <FALLING | 0>>
        ts_instrument.RegisterCommand(":PET:ENABle", &tic_pet_enable);      // params <CH1 | CH2> | <<RISING | 1> | <FALLING | 0>>
    ts_instrument.SetCommandTreeBase(":DIVider");
        ts_instrument.RegisterCommand(":ENABle", &cm_fwd_w);
        ts_instrument.RegisterCommand(":DELay", &cm_fwd_w);
        ts_instrument.RegisterCommand(":FREQuency", &cm_fwd_w);
        ts_instrument.RegisterCommand(":PULSelength", &cm_fwd_w);
        ts_instrument.RegisterCommand(":SYNChronization", &cm_fwd_w);
    ts_instrument.SetCommandTreeBase(":DIVider:TRIGger");        
        ts_instrument.RegisterCommand(":ENABle", &cm_fwd_w);             // param <ON | 1> | <OFF | 0>
        ts_instrument.RegisterCommand(":DELay", &cm_fwd_w);               // param delay in clock cycles
        ts_instrument.RegisterCommand(":FREQuency", &cm_fwd_w);        // param freq in Hz
        ts_instrument.RegisterCommand(":PULSelength", &cm_fwd_w);   // param <high portion in ns> | <p%>
        ts_instrument.RegisterCommand(":SYNChronization", &cm_fwd_w);
    ts_instrument.SetErrorHandler(&scpi_errorhandler);
}

void core1_main() {
    uint32_t tm = time_us_32();
    bool on = false;
    while(true) {
        ts_instrument.ProcessInput(SCPI_Interface(), "\n");
        if ((time_us_32() - tm) > 50000) {
            on != on;
            gpio_put(LED, on);
            tm = time_us_32();
        }
    }
}

int main() {
    stdio_init_all();
    sleep_ms(2000);
    gpio_init(LED);
    gpio_set_dir(LED, GPIO_OUT);
    gpio_put(LED, 1);

    printf("Awaiting messages in 1s...\n");
    cm_initialize();
    initialize_scpi();
    multicore_launch_core1(core1_main);
    sleep_ms(1000);

    tdc7200_meas_t msmt;

    uint16_t j = 0;
    while(true) {
        if (tic_enabled) {
            gpio_put(LED, 0);
            if (cm_get_meas_mode() == 3 & !cm_tic_pet_enabled()) {
/*                cm_forward_and_read("DIV:TRIG:FREQ 1000000\n", str);
                if (strcmp(str, "OK") != 0)
                    printf("Error configuring mode 3 measurement (setting trig freq)\n");
                cm_forward_and_read("DIV:TRIG:PULS 50%\n", str);
                if (strcmp(str, "OK") != 0)
                    printf("Error configuring mode 3 measurement (setting trig pulselength)\n");
                cm_forward_and_read("DIV:TRIG:SYNC 0\n", str);
                if (strcmp(str, "OK") != 0)
                    printf("Error configuring mode 3 measurement (setting trig sync)\n");
                cm_forward_and_read("DIV:TRIG:ENAB 1\n", str);
                if (strcmp(str, "OK") != 0)
                    printf("Error configuring mode 3 measurement (enabling trig)\n");
*/                cm_forward_and_read("TIC:STOP REF\n", str);
                if (strcmp(str, "OK") != 0)
                    printf("Error configuring mode 3 measurement (settig stop signal)\n");
                if (!cm_tic_pet_enable(true))
                    printf("Error configuring mode 3 measurement (PET initialization)\n");
                if (!cm_tic_set_nstops(1))
                    printf("Error configuring mode 3 measurement (number of stops)\n");
            } else if (cm_get_meas_mode() != 3 & cm_tic_pet_enabled()) {
                cm_tic_pet_enable(false);
                cm_forward_and_read("DIV:TRIG:ENAB 0\n", str);
            }
            msmt = cm_tic_measure();
            gpio_put(LED, 1);
            if (msmt.error == 0) {
                for (int i = 0; i < msmt.num_stops; i++) {
                    printf("%0.3f\t", msmt.tof[i]*1.0e9);
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
            msmt = cm_tic_measure();
            if (msmt.error == 0) {
                data[j] = msmt.tof[0];
                j++;
            } else {
                printf("error: %i, num_stops: %i, normLSB: %e\n", msmt.error, msmt.num_stops, msmt.normLSB);
            } */
        }
    }
}
