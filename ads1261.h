#ifndef ADS1261_H
#define ADS1261_H

#include <stdbool.h>
#include <stdint.h>

#define NOP 0x00
#define RESET 0x06
#define START 0x08
#define STOP 0x0A

#define RDATA 0x12
#define SYOCAL 0x16
#define GANCAL 0x17
#define SFOCAL 0x19

#define RREG 0x20
#define WREG 0x40

#define LOCK 0xF2
#define UNLOCK 0xF5

// Second byte registers for the register commands
typedef enum {
    ADS1261_REG_ID       = 0x00,
    ADS1261_REG_STATUS   = 0x01,
    ADS1261_REG_MODE0    = 0x02,
    ADS1261_REG_MODE1    = 0x03,
    ADS1261_REG_MODE2    = 0x04,
    ADS1261_REG_MODE3    = 0x05,
    ADS1261_REG_REF      = 0x06,
    ADS1261_REG_OFCAL0   = 0x07,
    ADS1261_REG_OFCAL1   = 0x08,
    ADS1261_REG_OFCAL2   = 0x09,
    ADS1261_REG_FSCAL0   = 0x0A,
    ADS1261_REG_FSCAL1   = 0x0B,
    ADS1261_REG_FSCAL2   = 0x0C,
    ADS1261_REG_IMUX     = 0x0D,
    ADS1261_REG_IMAG     = 0x0E,
    ADS1261_REG_RESERVED = 0x0F,
    ADS1261_REG_PGA      = 0x10,
    ADS1261_REG_INPMUX   = 0x11,
    ADS1261_REG_INPBIAS  = 0x12,
} ads1261_register_t;

#define CMD_WRITE_REG(reg) (WREG | (reg & 0x1F))
#define CMD_READ_REG(reg) (RREG | (reg & 0x1F))

typedef enum {
    ADS1261_AINCOM  = 0x0,
    ADS1261_AIN0    = 0x1,
    ADS1261_AIN1    = 0x2,
    ADS1261_AIN2    = 0x3,
    ADS1261_AIN3    = 0x4,
    ADS1261_AIN4    = 0x5,
    ADS1261_AIN5    = 0x6,
    ADS1261_AIN6    = 0x7,
    ADS1261_AIN7    = 0x8,
    ADS1261_AIN8    = 0x9,
    ADS1261_AIN9    = 0xA,
    ADS1261_TMP     = 0xB,
    ADS1261_ASUP    = 0xC,
    ADS1261_DSUP    = 0xD,
    ADS1261_DISCONNECT = 0xE,
    ADS1261_VCOM     = 0xF,
} ads1261_input_mux_t;

typedef enum {
    GOOD = 0,
    SPI_ERROR = 1,
    REFLECTION_ERROR = 2,
    MSG_BOUNDS_ERROR = 3
} ads1261_error_code_t;

#define MAX_TRANSACTION_LEN_BYTES 8

typedef struct {
    uint8_t tx[MAX_TRANSACTION_LEN_BYTES];
    uint8_t rx[MAX_TRANSACTION_LEN_BYTES];
    uint8_t len;
} ads1261_spi_transaction_record_t;

typedef ads1261_error_code_t (*ads1261_phyx_transact_ptr) (ads1261_spi_transaction_record_t * trans);

bool get_input_mux(ads1261_input_mux_t pos, ads1261_input_mux_t neg, uint8_t * res);

ads1261_error_code_t ads1261_read_reg(ads1261_phyx_transact_ptr spi_dev, uint8_t reg, uint8_t * value, ads1261_spi_transaction_record_t * trans);

ads1261_error_code_t ads1261_write_reg(ads1261_phyx_transact_ptr spi_dev, uint8_t reg, uint8_t value, ads1261_spi_transaction_record_t * trans);

#endif //ADS1261_H