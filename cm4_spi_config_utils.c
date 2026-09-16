#include "cm4_spi_config_utils.h"
#include <sys/ioctl.h>
#include <linux/spi/spidev.h>


int set_spi_mode(int fd, uint8_t * mode_ptr) {
    return ioctl(fd, SPI_IOC_WR_MODE, mode_ptr);
}

int set_spi_bits_per_word(int fd, uint8_t * bits_per_word_ptr) {
    return ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, bits_per_word_ptr);
}

int set_spi_max_speed_hz(int fd, uint32_t * max_speed_hz_ptr) {
    return ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, max_speed_hz_ptr);
}
