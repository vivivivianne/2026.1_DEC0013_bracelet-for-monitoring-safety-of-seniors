file(GLOB my_srcs *.c *.h)
idf_component_register(SRCS "${my_srcs}"
                    INCLUDE_DIRS "."
                    REQUIRES esp_driver_i2c
                    REQUIRES esp_driver_gpio)
