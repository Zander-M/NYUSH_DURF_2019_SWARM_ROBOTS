deps_config := \
	/home/zdrm/esp/esp-idf/components/app_trace/Kconfig \
	/home/zdrm/esp/esp-idf/components/aws_iot/Kconfig \
	/home/zdrm/esp/esp-idf/components/bt/Kconfig \
	/home/zdrm/esp/esp-idf/components/driver/Kconfig \
	/home/zdrm/esp/esp-idf/components/esp32/Kconfig \
	/home/zdrm/esp/esp-idf/components/esp_adc_cal/Kconfig \
	/home/zdrm/esp/esp-idf/components/esp_event/Kconfig \
	/home/zdrm/esp/esp-idf/components/esp_http_client/Kconfig \
	/home/zdrm/esp/esp-idf/components/esp_http_server/Kconfig \
	/home/zdrm/esp/esp-idf/components/ethernet/Kconfig \
	/home/zdrm/esp/esp-idf/components/fatfs/Kconfig \
	/home/zdrm/esp/esp-idf/components/freemodbus/Kconfig \
	/home/zdrm/esp/esp-idf/components/freertos/Kconfig \
	/home/zdrm/esp/esp-idf/components/heap/Kconfig \
	/home/zdrm/esp/esp-idf/components/libsodium/Kconfig \
	/home/zdrm/esp/esp-idf/components/log/Kconfig \
	/home/zdrm/esp/esp-idf/components/lwip/Kconfig \
	/home/zdrm/esp/esp-idf/components/mbedtls/Kconfig \
	/home/zdrm/esp/esp-idf/components/mdns/Kconfig \
	/home/zdrm/esp/esp-idf/components/mqtt/Kconfig \
	/home/zdrm/esp/esp-idf/components/nvs_flash/Kconfig \
	/home/zdrm/esp/esp-idf/components/openssl/Kconfig \
	/home/zdrm/esp/esp-idf/components/pthread/Kconfig \
	/home/zdrm/esp/esp-idf/components/spi_flash/Kconfig \
	/home/zdrm/esp/esp-idf/components/spiffs/Kconfig \
	/home/zdrm/esp/esp-idf/components/tcpip_adapter/Kconfig \
	/home/zdrm/esp/esp-idf/components/vfs/Kconfig \
	/home/zdrm/esp/esp-idf/components/wear_levelling/Kconfig \
	/home/zdrm/esp/esp-idf/components/bootloader/Kconfig.projbuild \
	/home/zdrm/esp/esp-idf/components/esptool_py/Kconfig.projbuild \
	/home/zdrm/Desktop/DURF/Project/Code/motor_test/main/Kconfig.projbuild \
	/home/zdrm/esp/esp-idf/components/partition_table/Kconfig.projbuild \
	/home/zdrm/esp/esp-idf/Kconfig

include/config/auto.conf: \
	$(deps_config)

ifneq "$(IDF_CMAKE)" "n"
include/config/auto.conf: FORCE
endif

$(deps_config): ;
