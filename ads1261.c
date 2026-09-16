#include "ads1261.h"

#define SHIFT_LEFT_ONE_BYTE(bits) (bits << 8)
#define SHIFT_LEFT_TWO_BYTE(bits) (bits << 16)

#define TWO_POW_23 (8388608)

ads1261_error_code_t ads1261_read_reg(ads1261_phyx_transact_ptr spi_dev, uint8_t reg, uint8_t * value, ads1261_spi_transaction_record_t * const trans) {
    enum { MESSAGE_LENGTH = 3 };
    _Static_assert(
        MESSAGE_LENGTH <= MAX_TRANSACTION_LEN_BYTES,
        "RREG transaction exceeds transaction buffer"
    );
    trans->tx[0] = CMD_READ_REG(reg);
    trans->tx[1] = 0;
    trans->tx[2] = 0;
    trans->len = MESSAGE_LENGTH;
    const int transfer_code = spi_dev(trans);
    if (transfer_code != GOOD) {
        return transfer_code;
    }
    if (trans->tx[0] != trans->rx[1]) {
        return REFLECTION_ERROR;
    }
    *value = trans->rx[2];
    return GOOD;
}

ads1261_error_code_t ads1261_write_reg(ads1261_phyx_transact_ptr spi_dev, uint8_t reg, uint8_t value, ads1261_spi_transaction_record_t * trans) {
    enum { MESSAGE_LENGTH = 2 };
    _Static_assert(
        MESSAGE_LENGTH <= MAX_TRANSACTION_LEN_BYTES,
        "WREG transaction exceeds transaction buffer"
    );
    trans->tx[0] = CMD_WRITE_REG(reg);
    trans->tx[1] = value;
    trans->len = MESSAGE_LENGTH;
    const int transfer_code = spi_dev(trans);
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

ads1261_error_code_t ads1261_read_cmd_read_data(ads1261_phyx_transact_ptr spi_dev, uint32_t * raw_val, ads1261_spi_transaction_record_t * trans) {
    enum { MESSAGE_LENGTH = 5 };
    _Static_assert(
    MESSAGE_LENGTH <= MAX_TRANSACTION_LEN_BYTES,
        "RDATA transaction exceeds transaction buffer"
    );
    trans->tx[0] = RDATA;
    trans->tx[1] = 0;
    trans->tx[2] = 0;
    trans->tx[3] = 0;
    trans->tx[4] = 0;
    trans->len = MESSAGE_LENGTH;
    const int transfer_code = spi_dev(trans);
    if (transfer_code != GOOD) {
        return transfer_code;
    }
    if (trans->tx[0] != trans->rx[1]) {
        return REFLECTION_ERROR;
    }
    *raw_val = SHIFT_LEFT_TWO_BYTE(trans->rx[2]) | SHIFT_LEFT_ONE_BYTE(trans->rx[3]) | trans->rx[4];
    return GOOD;
}

ads1261_error_code_t ads1261_cmd_start(ads1261_phyx_transact_ptr spi_dev, ads1261_spi_transaction_record_t * trans) {
    enum { MESSAGE_LENGTH = 2 };
    _Static_assert(
    MESSAGE_LENGTH <= MAX_TRANSACTION_LEN_BYTES,
        "START cmd transaction exceeds transaction buffer"
    );
    trans->tx[0] = START;
    trans->tx[1] = 0;
    trans->len = MESSAGE_LENGTH;
    const int transfer_code = spi_dev(trans);
    if (transfer_code != GOOD) {
        return transfer_code;
    }
    if (trans->tx[0] != trans->rx[1]) {
        return REFLECTION_ERROR;
    }
    return GOOD;
}

ads1261_error_code_t ads1261_cmd_stop(ads1261_phyx_transact_ptr spi_dev, ads1261_spi_transaction_record_t * trans) {
    enum { MESSAGE_LENGTH = 2 };
    _Static_assert(
    MESSAGE_LENGTH <= MAX_TRANSACTION_LEN_BYTES,
        "STOP cmd transaction exceeds transaction buffer"
    );
    trans->tx[0] = STOP;
    trans->tx[1] = 0;
    trans->len = MESSAGE_LENGTH;
    const int transfer_code = spi_dev(trans);
    if (transfer_code != GOOD) {
        return transfer_code;
    }
    if (trans->tx[0] != trans->rx[1]) {
        return REFLECTION_ERROR;
    }
    return GOOD;
}

static inline int32_t ads1261_sign_extend_raw_data_read(uint32_t raw_data) {
    raw_data &= 0x00FFFFFF;
    // 24 bit raw data -> bit 23 is the sign bit
    if (raw_data & (0b1 << 23)) {
        // sign extend (make all the new bits high)
        raw_data |= (0xFF000000);
    }
    return (int32_t)raw_data;
}

double ads1261_decode_voltage(uint32_t raw_data, double vref, double gain) {
    int32_t signed_value = ads1261_sign_extend_raw_data_read(raw_data);
    return ((double)signed_value / (TWO_POW_23)) * (vref / gain);
}

ads1261_error_code_t ads1261_cmd_reset(ads1261_phyx_transact_ptr spi_dev, ads1261_spi_transaction_record_t * trans) {
    enum { MESSAGE_LENGTH = 2 };
    _Static_assert(
    MESSAGE_LENGTH <= MAX_TRANSACTION_LEN_BYTES,
        "RESET cmd transaction exceeds transaction buffer"
    );
    trans->tx[0] = RESET;
    trans->tx[1] = 0;
    trans->len = MESSAGE_LENGTH;
    const int transfer_code = spi_dev(trans);
    if (transfer_code != GOOD) {
        return transfer_code;
    }
    if (trans->tx[0] != trans->rx[1]) {
        return REFLECTION_ERROR;
    }
    return GOOD;
}
