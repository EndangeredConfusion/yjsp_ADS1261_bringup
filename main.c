#include <errno.h>
#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <linux/types.h>
#include <stdbool.h>
#include "adc_commands.h"


#define ADC_SPI_PIPE "/dev/spidev4.0"
#define SPI_SPEED_HZ (10000000U)
#define TX_BASE_CMD_LEN (2)
#define MAX_TRANSACTION_LEN_BYTES 8

typedef enum {
    GOOD = 0,
    SPI_ERROR = 1,
    REFLECTION_ERROR = 2,
    MSG_BOUNDS_ERROR = 3
} ads1261_error_code_t;

typedef struct {
    uint8_t tx[MAX_TRANSACTION_LEN_BYTES];
    uint8_t rx[MAX_TRANSACTION_LEN_BYTES];
    uint8_t len;
} spi_transaction_record_t;

ads1261_error_code_t ads1261_read_reg(int fd, uint8_t reg, uint8_t * value, spi_transaction_record_t * trans);
ads1261_error_code_t ads1261_write_reg(int fd, uint8_t reg, uint8_t value, spi_transaction_record_t * trans);

ads1261_error_code_t print_ads1261_spi_transaction(const spi_transaction_record_t * trans);

int set_spi_mode(int fd, uint8_t * mode_ptr) {
    return ioctl(fd, SPI_IOC_WR_MODE, mode_ptr);
}

int set_spi_bits_per_word(int fd, uint8_t * bits_per_word_ptr) {
    return ioctl(fd, SPI_IOC_WR_BITS_PER_WORD, bits_per_word_ptr);
}

int set_spi_max_speed_hz(int fd, uint32_t * max_speed_hz_ptr) {
    return ioctl(fd, SPI_IOC_WR_MAX_SPEED_HZ, max_speed_hz_ptr);
}

int spi_transfer(int fd, const uint8_t *tx, uint8_t *rx, size_t len);

int main(void) {
    printf("And now, we begin...\n\n");

    int fd = open(ADC_SPI_PIPE, O_RDWR);

    if (fd < 0) {
        perror("Failed to open SPI pipe");
    }

    uint8_t mode = SPI_MODE_1;
    uint8_t bits_per_word = 8;
    uint32_t max_speed_hz = SPI_SPEED_HZ;
    if (set_spi_mode(fd, &mode) < 0) {
        perror("Failed to set SPI mode");
    }
    if (set_spi_bits_per_word(fd, &bits_per_word) < 0) {
        perror("Failed to set SPI bits per word");
    }
    if (set_spi_max_speed_hz(fd, &max_speed_hz) < 0) {
        perror("Failed to set SPI max speed");
    }

    spi_transaction_record_t read_device_id_transaction_record;
    uint8_t id_reg_val;
    ads1261_read_reg(fd, ADS1261_REG_ID, &id_reg_val, &read_device_id_transaction_record);
    printf("Read device ID:\n");
    print_ads1261_spi_transaction(&read_device_id_transaction_record);

    // turn vlven1 on (GPIO24) (output is 24V), with off output is ~4.3V
    // read V-FB (AIN6)
    // V-FB ~=~ 10k/(10k + 100k) * OUT = .09*OUT = [0.396, 2.16]
    // MUX from AIN6 to AINCOM or AIN1 on gain of 1
    // INPMUX

    printf("\nSet device to pos AIN6, neg AIN1:\n");
    uint8_t inpmux = 0;
    if (!get_input_mux(ADS1261_AIN6, ADS1261_AINCOM, &inpmux)) {
        perror("Invalid input mux config request.");
    }
    spi_transaction_record_t mux_write_transaction_record;
    ads1261_write_reg(fd, ADS1261_REG_INPMUX, inpmux, &mux_write_transaction_record);
    print_ads1261_spi_transaction(&mux_write_transaction_record);

    printf("\nSet device external gain to passthrough:\n");

    spi_transaction_record_t pga_write_transaction_record;
#define PGA_PASSTHROUGH_ON (0b1 << 7)
    ads1261_write_reg(fd, ADS1261_REG_PGA, PGA_PASSTHROUGH_ON, &pga_write_transaction_record);
    print_ads1261_spi_transaction(&pga_write_transaction_record);

    printf("\nSet device to use internal 2.5V reference:\n");
    spi_transaction_record_t ref_write_record;
#define REF_INTERNAL_2_5V (0b1 << 4)
    ads1261_write_reg(fd, ADS1261_REG_REF, REF_INTERNAL_2_5V, &ref_write_record);
    print_ads1261_spi_transaction(&ref_write_record);


    printf("\nSet device to SR 14400, Filter: FIR:\n");
    spi_transaction_record_t mode0_write_record;
#define MODE0_14400_SR_VAL (01101)
#define MODE0_14400_SR_OFFSET (3)
#define MODE0_FIR_VAL (100)
#define MODE0_FIR_OFFSET (0)
#define MODE0_TEST_VAL (MODE0_14400_SR_VAL<<MODE0_14400_SR_OFFSET) || (MODE0_FIR_VAL<<MODE0_FIR_OFFSET)
    ads1261_write_reg(fd, ADS1261_REG_MODE0, MODE0_TEST_VAL, &mode0_write_record);
    print_ads1261_spi_transaction(&mode0_write_record);


    printf("\nRead device SR:\n");
#define IS_SR_READY(SR) (SR & (0b1 << 2))
    spi_transaction_record_t read_sr_record;
    uint8_t read_sr_val;
    ads1261_read_reg(fd, ADS1261_REG_STATUS, &read_sr_val, &read_sr_record);
    print_ads1261_spi_transaction(&read_sr_record);


    return 0;
}

ads1261_error_code_t base_spi_transfer(int fd, spi_transaction_record_t * const trans) {
    if (trans->len > MAX_TRANSACTION_LEN_BYTES) {
        return MSG_BOUNDS_ERROR;
    }
    struct spi_ioc_transfer spi_ioc_transfer = {
        .tx_buf = (uintptr_t)trans->tx,
        .rx_buf = (uintptr_t)trans->rx,
        .len = (uint32_t)trans->len,
    };
    if (ioctl(fd, SPI_IOC_MESSAGE(1), &spi_ioc_transfer) < 0) {
        perror("SPI transfer failed");
        return SPI_ERROR;
    }
    return GOOD;
}

ads1261_error_code_t ads1261_read_reg(int fd, uint8_t reg, uint8_t * value, spi_transaction_record_t * const trans) {
    enum { MESSAGE_LENGTH = 3 };
    _Static_assert(
        MESSAGE_LENGTH <= MAX_TRANSACTION_LEN_BYTES,
        "RREG transaction exceeds transaction buffer"
    );
    trans->tx[0] = CMD_READ_REG(reg);
    trans->tx[1] = 0;
    trans->tx[2] = 0;
    trans->len = MESSAGE_LENGTH;
    int transfer_code = base_spi_transfer(fd, trans);
    if (transfer_code != GOOD) {
        return transfer_code;
    }
    if (trans->tx[0] != trans->rx[1]) {
        return REFLECTION_ERROR;
    }
    *value = trans->rx[2];
    return GOOD;
}

ads1261_error_code_t ads1261_write_reg(int fd, uint8_t reg, uint8_t value, spi_transaction_record_t * trans) {
    enum { MESSAGE_LENGTH = 2 };
    _Static_assert(
        MESSAGE_LENGTH <= MAX_TRANSACTION_LEN_BYTES,
        "WREG transaction exceeds transaction buffer"
    );
    trans->tx[0] = CMD_WRITE_REG(reg);
    trans->tx[1] = value;
    trans->len = MESSAGE_LENGTH;
    int transfer_code = base_spi_transfer(fd, trans);
    if (transfer_code != GOOD) {
        return transfer_code;
    }
    if (trans->tx[0] != trans->rx[1]) {
        return REFLECTION_ERROR;
    }
    return GOOD;
}

bool get_input_mux(ads1261_input_mux_t pos, ads1261_input_mux_t neg, uint8_t * res) {
    if ((pos > 0xF) || (neg > 0xF)) {
        return false;
    }
    *res = ((pos & 0xF) << 4 | (neg & 0xF));
    return true;
}

ads1261_error_code_t print_ads1261_spi_transaction(const spi_transaction_record_t * trans) {
    if (trans->len > MAX_TRANSACTION_LEN_BYTES) {
        return MSG_BOUNDS_ERROR;
    }
    printf("TX: ");
    for (int i = 0; i < trans->len; i++) {
        printf("%X\t", trans->tx[i]);
    }
    printf("\nRX: ");
    for (int i = 0; i < trans->len; i++) {
        printf("%X\t", trans->rx[i]);
    }
    printf("\n");
    return GOOD;
}
