#ifndef _SD_SPI_H_
#define _SD_SPI_H_

#ifdef __cplusplus
extern "C" {
#endif

#include <string.h>
#include "sys/unistd.h"
#include "sys/stat.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"

#define EXAMPLE_MAX_CHAR_SIZE 64
#define MOUNT_POINT "/sdcard"
#define PIN_NUM_MISO  6
#define PIN_NUM_MOSI  8
#define PIN_NUM_CLK   7
#define PIN_NUM_CS    -1

esp_err_t write_string_file(const char *filename, char *data);
esp_err_t read_string_file(const char *filename);
esp_err_t write_file(const char *filename,char *data,size_t size);
esp_err_t read_file(const char *filename,char *data,size_t size);
esp_err_t read_file2(const char *filename);
esp_err_t sd_init();
esp_err_t sd_deinit();
esp_err_t format_sd_card();

#ifdef __cplusplus
}
#endif


#endif