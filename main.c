#include <fcntl.h>
#include <linux/spi/spidev.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <linux/types.h>
#include <gpiod.h>
#include <inttypes.h>

#include "ads1261.h"
#include "cm4_spi_config_utils.h"

#define ADC_SPI_PIPE "/dev/spidev4.0"
#define SPI_SPEED_HZ (10000000U)
#define SAMPLE_CAPACITY (65536)

typedef struct {
    uint64_t timestamp_ns;
    unsigned long edge_seq;
    uint32_t val;
} sample_struct_t;

sample_struct_t samples[SAMPLE_CAPACITY];
size_t count = 0;

static int ADS1261_SPI_DEV_4_DEVICE_0_FD;
ads1261_error_code_t ads1261_device_0_spi_transfer(ads1261_spi_transaction_record_t * const trans);

ads1261_error_code_t base_spi_transfer(int fd, ads1261_spi_transaction_record_t * const trans);

ads1261_error_code_t print_ads1261_spi_transaction(const ads1261_spi_transaction_record_t * trans);

static struct gpiod_line_request * request_drdy(void) {
    static unsigned int offset = 22;
    struct gpiod_chip * chip = gpiod_chip_open("/dev/gpiochip0");
    if (!chip) {
        return NULL;
    }
    struct gpiod_line_settings *settings = gpiod_line_settings_new();
    struct gpiod_line_config *config = gpiod_line_config_new();
    struct gpiod_line_request *request = NULL;

    if (settings && config &&
        gpiod_line_settings_set_direction(
            settings, GPIOD_LINE_DIRECTION_INPUT) == 0 &&
        gpiod_line_settings_set_edge_detection(
            settings, GPIOD_LINE_EDGE_FALLING) == 0 &&
        gpiod_line_config_add_line_settings(
            config, &offset, 1, settings) == 0) {
        request = gpiod_chip_request_lines(chip, NULL, config);
            }

    gpiod_line_config_free(config);
    gpiod_line_settings_free(settings);
    gpiod_chip_close(chip);
    return request;
}

int main(void) {
    ADS1261_SPI_DEV_4_DEVICE_0_FD = open(ADC_SPI_PIPE, O_RDWR);
    if (ADS1261_SPI_DEV_4_DEVICE_0_FD < 0) {
        perror("Failed to open SPI pipe");
    }

    uint8_t mode = SPI_MODE_1;
    uint8_t bits_per_word = 8;
    uint32_t max_speed_hz = SPI_SPEED_HZ;
    if (set_spi_mode(ADS1261_SPI_DEV_4_DEVICE_0_FD, &mode) < 0) {
        perror("Failed to set SPI mode");
    }
    if (set_spi_bits_per_word(ADS1261_SPI_DEV_4_DEVICE_0_FD, &bits_per_word) < 0) {
        perror("Failed to set SPI bits per word");
    }
    if (set_spi_max_speed_hz(ADS1261_SPI_DEV_4_DEVICE_0_FD, &max_speed_hz) < 0) {
        perror("Failed to set SPI max speed");
    }

    printf("Reset device to clear registers:\n");
    ads1261_spi_transaction_record_t reset_device_transaction_record;
    ads1261_cmd_reset(ads1261_device_0_spi_transfer, &reset_device_transaction_record);
    print_ads1261_spi_transaction(&reset_device_transaction_record);

    printf("\nRead device ID:\n");
    ads1261_spi_transaction_record_t read_device_id_transaction_record;
    uint8_t id_reg_val;
    ads1261_read_reg(ads1261_device_0_spi_transfer, ADS1261_REG_ID, &id_reg_val, &read_device_id_transaction_record);
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
    ads1261_spi_transaction_record_t mux_write_transaction_record;
    ads1261_write_reg(ads1261_device_0_spi_transfer, ADS1261_REG_INPMUX, inpmux, &mux_write_transaction_record);
    print_ads1261_spi_transaction(&mux_write_transaction_record);

    printf("\nSet device external gain to passthrough:\n");

    ads1261_spi_transaction_record_t pga_write_transaction_record;
#define PGA_PASSTHROUGH_ON (0b1 << 7)
    ads1261_write_reg(ads1261_device_0_spi_transfer, ADS1261_REG_PGA, PGA_PASSTHROUGH_ON, &pga_write_transaction_record);
    print_ads1261_spi_transaction(&pga_write_transaction_record);

    printf("\nSet device to use internal 2.5V reference:\n");
    ads1261_spi_transaction_record_t ref_write_record;
#define REF_INTERNAL_2_5V (0b1 << 4)
    ads1261_write_reg(ads1261_device_0_spi_transfer, ADS1261_REG_REF, REF_INTERNAL_2_5V, &ref_write_record);
    print_ads1261_spi_transaction(&ref_write_record);

    printf("\nSet device (MODE 0) to SR 40k, Filter: FIR:\n");
    ads1261_spi_transaction_record_t mode0_write_record;
#define MODE0_40_000_SR_VAL (0b11111)
#define MODE0_14400_SR_OFFSET (3)
#define MODE0_FIR_VAL (0b100)
#define MODE0_FIR_OFFSET (0)
#define MODE0_TEST_VAL (MODE0_40_000_SR_VAL<<MODE0_14400_SR_OFFSET) | (MODE0_FIR_VAL<<MODE0_FIR_OFFSET)
    ads1261_write_reg(&ads1261_device_0_spi_transfer, ADS1261_REG_MODE0, MODE0_TEST_VAL, &mode0_write_record);
    print_ads1261_spi_transaction(&mode0_write_record);

    printf("\nSet device (MODE 1) to 50us conversion start delay:\n");
    ads1261_spi_transaction_record_t mode1_write_record;
#define MODE1_VAL_50US (0001)
    ads1261_write_reg(&ads1261_device_0_spi_transfer, ADS1261_REG_MODE1, MODE1_VAL_50US, &mode1_write_record);
    print_ads1261_spi_transaction(&mode1_write_record);

    printf("\nRead device SR:\n");
#define IS_SR_READY(SR) (SR & (0b1 << 2))
    ads1261_spi_transaction_record_t read_sr_record;
    uint8_t read_sr_val;
    ads1261_read_reg(ads1261_device_0_spi_transfer, ADS1261_REG_STATUS, &read_sr_val, &read_sr_record);
    print_ads1261_spi_transaction(&read_sr_record);

    printf("\nReset the was reset bit in SR:\n");
    const uint8_t RESET_SR_BITS = 0x00;
    ads1261_write_reg(ads1261_device_0_spi_transfer, ADS1261_REG_STATUS, RESET_SR_BITS, &read_sr_record);
    print_ads1261_spi_transaction(&read_sr_record);

    printf("\nRead device SR again:\n");
    ads1261_read_reg(ads1261_device_0_spi_transfer, ADS1261_REG_STATUS, &read_sr_val, &read_sr_record);
    print_ads1261_spi_transaction(&read_sr_record);

    struct gpiod_line_request *drdy = request_drdy();
    if (!drdy) {
        perror("request GPIO22 DRDY");
        return 1;
    }

    struct gpiod_edge_event_buffer *events = gpiod_edge_event_buffer_new(1);
    if (!events) {
        perror("allocate GPIO event buffer");
        gpiod_line_request_release(drdy);
        return 1;
    }
    ads1261_spi_transaction_record_t transaction;
    printf("\n\nStarting adc\n");
    if (ads1261_cmd_start(ads1261_device_0_spi_transfer, &transaction) != GOOD) {
        fprintf(stderr, "Failed to start conversions\n");
        gpiod_edge_event_buffer_free(events);
        gpiod_line_request_release(drdy);
        return 1;
    }

    while (count < SAMPLE_CAPACITY) {
        /* Blocks until GPIO22 has a falling edge. */
        int num_events = gpiod_line_request_read_edge_events(drdy, events, 1);
        if (num_events < 0) {
            perror("wait for DRDY");
            break;
        }
        struct gpiod_edge_event *event = gpiod_edge_event_buffer_get_event(events, 0);
        uint64_t timestamp_ns = gpiod_edge_event_get_timestamp_ns(event);
        unsigned long seq = gpiod_edge_event_get_line_seqno(event);

        uint32_t raw;
        if (ads1261_read_cmd_read_data(ads1261_device_0_spi_transfer, &raw, &transaction) != GOOD) {
            fprintf(stderr, "Failed to read ADC data\n");
            break;
        }

        samples[count++] = (sample_struct_t){timestamp_ns, seq, raw};
    };
    FILE *out = stdout;
    fprintf(out, "drdy_ns,edge_seq,raw\n");
    for (size_t i = 0; i < count; ++i) {
        fprintf(out, "%" PRIu64 ",%lu,%" PRIu32 "\n",
                samples[i].timestamp_ns, samples[i].edge_seq, ads1261_decode_voltage(samples[i].val, 2.5, 1));
    }
    return 0;

}

ads1261_error_code_t ads1261_device_0_spi_transfer(ads1261_spi_transaction_record_t * const trans) {
    return base_spi_transfer(ADS1261_SPI_DEV_4_DEVICE_0_FD, trans);
}

ads1261_error_code_t base_spi_transfer(int fd, ads1261_spi_transaction_record_t * const trans) {
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

ads1261_error_code_t print_ads1261_spi_transaction(const ads1261_spi_transaction_record_t * trans) {
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
