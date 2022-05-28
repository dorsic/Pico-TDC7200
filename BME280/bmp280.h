#include "bmp2_defs.h"

static struct bmp2_dev dev;
static struct bmp2_config conf;

void bme280_initialize();

struct bmp2_data bme280_get_data();

int8_t bmp280_get_status();
