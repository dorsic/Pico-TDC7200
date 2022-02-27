#ifndef SCPI_H_
#define SCPI_H_

#include <inttypes.h>

#define SCPI_BUFFER_LENGTH 64
#define SCPI_TIMEOUT 10000
#define SCPI_MAX_COMMANDS 20

#define SCPI_ERR_BUFFER_OVERFLOW 1
#define SCPI_ERR_BUFFER_TIMEOUT 2
#define SCPI_ERR_UNKNOWN 9
#define scpi_termination_chars "\n"
#define INT_MIN -2147483648

typedef void (*SCPI_handler_t)(int32_t param);

void scpi_process_input();

void scpi_register_command(char *command, SCPI_handler_t caller);

void scpi_register_errorhandler(SCPI_handler_t caller);


#endif //SCPI_H_