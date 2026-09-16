#ifndef YJSP_ADS126_EVAL_CM4_SPI_CONFIG_UTILS_H
#define YJSP_ADS126_EVAL_CM4_SPI_CONFIG_UTILS_H

#include <stdint.h>

int set_spi_mode(int fd, uint8_t * mode_ptr);

int set_spi_bits_per_word(int fd, uint8_t * bits_per_word_ptr);

int set_spi_max_speed_hz(int fd, uint32_t * max_speed_hz_ptr);

#endif //YJSP_ADS126_EVAL_CM4_SPI_CONFIG_UTILS_H