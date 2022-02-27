#include <stdlib.h>
#include <string.h>
#include "pico/stdio.h"
#include "hardware/timer.h"
#include "scpi.h"

//#include <stdlib.h>

char msg_buffer_[SCPI_BUFFER_LENGTH];
uint8_t message_length_ = 0;
uint64_t time_checker_ = 0;
uint8_t last_error_ = 0;
uint8_t last_cmd_index_ = 0;

char* commands_[SCPI_MAX_COMMANDS];
SCPI_handler_t handlers_[SCPI_MAX_COMMANDS];

uint8_t _scpi_prefix(const char *pre, const char *str) {
    return strncmp(pre, str, strlen(pre)) == 0;
}

int32_t _scpi_parse_param(const char *message, const char *command) {
    return atoi(&message[strlen(command)]);
}

char _getchar_non_blocking() {
    int ch = getchar_timeout_us(0);
    return (ch != PICO_ERROR_TIMEOUT)? (char) ch: '\0';
}

void _call_error_handler() {
    if (handlers_[0] != NULL) {
        (*handlers_[0])(last_error_);
    }
}

char* _scpi_get_message() {
    char ch = _getchar_non_blocking();
    while (ch != '\0') {
        //Read the new char
        msg_buffer_[message_length_] = ch;
        ++message_length_;
        time_checker_ = time_us_64();

        if (message_length_ >= SCPI_BUFFER_LENGTH){
            //Call ErrorHandler due BufferOverflow
            last_error_ = SCPI_ERR_BUFFER_OVERFLOW;
            _call_error_handler();    
            message_length_ = 0;
            return NULL;
        }

        //Test for termination chars (end of the message)
        msg_buffer_[message_length_] = '\0';
        if (strstr(msg_buffer_, scpi_termination_chars) != NULL) {
            //Return the received message
            msg_buffer_[message_length_ - strlen(scpi_termination_chars)] = '\0';
            message_length_ = 0;
            return msg_buffer_;
        }
        ch = _getchar_non_blocking();
    }

    //No more chars avaible yet
    //Return NULL if no message is incomming
    if (message_length_ == 0) return NULL;

    //Check for communication timeout
    if ((time_us_64() - time_checker_) > SCPI_TIMEOUT) {
        //Call ErrorHandler due Timeout
        last_error_ = SCPI_ERR_BUFFER_TIMEOUT;
        _call_error_handler();
        message_length_ = 0;
        return NULL;
    }
  //No errors, be sure to return NULL
  return NULL;
}

void scpi_process_input() {
    char* message = _scpi_get_message();
    if (message != NULL) {
        // NOTE first command with index 0 is the message handler if registered
        for (uint8_t i = 1; i < last_cmd_index_; i++) {
            if (_scpi_prefix(commands_[i], message)) {
                int32_t param = _scpi_parse_param(message, commands_[i]);                
                (*handlers_[i])(param);
            }
        }
    }
}

void scpi_register_command(char *command, SCPI_handler_t caller) {
    last_cmd_index_++;
    commands_[last_cmd_index_] = command;
    handlers_[last_cmd_index_] = caller;
}

void scpi_register_errorhandler(SCPI_handler_t caller) {
    commands_[0] = "ERROR";
    handlers_[0] = caller;
}
