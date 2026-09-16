cmake_minimum_required(VERSION 3.31)
project(yjsp_ads126_eval C)

set(CMAKE_C_STANDARD 17)

add_executable(yjsp_ads126_eval
        main.c
        cm4_spi_config_utils.c
        ads1261.c
)
