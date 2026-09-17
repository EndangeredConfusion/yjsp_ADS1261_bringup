cmake_minimum_required(VERSION 3.31)
project(yjsp_ads126_eval C)

set(CMAKE_C_STANDARD 17)

find_package(PkgConfig REQUIRED)
pkg_check_modules(GPIOD REQUIRED IMPORTED_TARGET libgpiod>=2.0)

add_executable(yjsp_ads126_eval
        main.c
        cm4_spi_config_utils.c
        ads1261.c
)

target_link_libraries(yjsp_ads126_eval PRIVATE PkgConfig::GPIOD)