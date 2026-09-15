#include "ads1261.h"

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
