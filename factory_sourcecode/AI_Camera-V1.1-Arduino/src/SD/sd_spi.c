#include "esp_err.h"
/*————————————————————————————————————————Header file declaration————————————————————————————————————————*/
#include "sd_spi.h"
/*——————————————————————————————————————Header file declaration end——————————————————————————————————————*/

/*——————————————————————————————————————————Variable declaration—————————————————————————————————————————*/
static sdmmc_card_t *card;
const char mount_point[] = MOUNT_POINT;
/*————————————————————————————————————————Variable declaration end———————————————————————————————————————*/

/*—————————————————————————————————————————Functional function———————————————————————————————————————————*/
esp_err_t write_string_file(const char *filename, char *data)
{
    //SD_INFO("Opening file %s", filename);
    FILE *file = fopen(filename, "w");
    if(!file) 
    {
        //SD_ERROR("Failed to open file for writing string");
        return ESP_FAIL;
    }
    fprintf(file, data);
    fclose(file);
    // SD_INFO("File written");
    return ESP_OK;
}

esp_err_t read_string_file(const char *filename)
{
    //SD_INFO("Reading file %s", filename);
    FILE *file = fopen(filename, "r");
    if(!file)
    {
        //SD_ERROR("Failed to open file for reading string");

        return ESP_FAIL;
    }
    char line[EXAMPLE_MAX_CHAR_SIZE];
    fgets(line, sizeof(line), file);
    fclose(file);

    char *pos = strchr(line,'\n');
    if(pos) 
    {
        *pos = '\0';
        //SD_INFO("Read a line from file: '%s'", line);
    }
    else
        //SD_INFO("Read from file: '%s'", line);
    return ESP_OK;
}

esp_err_t write_file(const char *filename,char *data,size_t size)
{
    size_t success_size=0;
    FILE *file=fopen(filename,"wb");
    if (!file) 
    {
        //SD_ERROR("Failed to open file for writing");
        return ESP_FAIL;
    }
    success_size=fwrite(data,1,size,file);
    if(success_size!=size)
    {
        fclose(file);
        //SD_ERROR("Failed to write file");
        return ESP_FAIL;
    }
    else 
    {
        fclose(file);
        //SD_INFO("File written");
    }
    return ESP_OK;
}

esp_err_t read_file(const char *filename,char *data,size_t size)
{
    size_t success_size=0;
    FILE *file=fopen(filename,"rb");
    if (!file) 
    {
        //SD_ERROR("Failed to open file for reading");
        return ESP_FAIL;
    }
    success_size=fread(data,1,size,file);
    if(success_size!=size)
    {
        fclose(file);
        //SD_ERROR("Failed to read file");
        return ESP_FAIL;
    }
    else 
    {
        fclose(file);
        //SD_INFO("File read success");
    }
    return ESP_OK;
}

esp_err_t read_file2(const char *filename)
{
    size_t success_size=0;
    size_t size=0;
    FILE *file=fopen(filename,"rb");
    if (!file) 
    {
        //SD_ERROR("Failed to open file for reading");
        return ESP_FAIL;
    }
    char buffer[1024];
    while ((success_size=fread(buffer,1,sizeof(buffer),file))>0) 
    {
        size+=success_size;
    }
    fclose(file);
    //SD_INFO("File read success,success size =%d",size);
    return ESP_OK;
}

esp_err_t sd_init()
{
    esp_err_t ret;
    esp_vfs_fat_sdmmc_mount_config_t mount_config = {
#ifdef CONFIG_FORMAT_IF_MOUNT_FAILED
        .format_if_mount_failed = true,
#else
        .format_if_mount_failed = false,
#endif 
        .max_files = 5,
        .allocation_unit_size = 16 * 1024
    };
    
    //SD_INFO("Initializing SD card");
    //SD_INFO("Using SPI peripheral");
    static sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    spi_bus_config_t bus_cfg = {
        .mosi_io_num = PIN_NUM_MOSI,
        .miso_io_num = PIN_NUM_MISO,
        .sclk_io_num = PIN_NUM_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4000,
    };
    ret = spi_bus_initialize(host.slot, &bus_cfg, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK) {
        //SD_ERROR("Failed to initialize bus.");
        return ret;
    }
    sdspi_device_config_t slot_config = {
        .host_id   = host.slot, 
        .gpio_cs   = GPIO_NUM_NC, 
        .gpio_cd   = GPIO_NUM_NC, 
        .gpio_wp   = GPIO_NUM_NC, 
        .gpio_int  = GPIO_NUM_NC,
    };
    //SD_INFO("Mounting filesystem");
#ifdef CONFIG_Need_CS
    slot_config.gpio_cs=PIN_NUM_CS;
#endif 
    ret = esp_vfs_fat_sdspi_mount(mount_point, &host, &slot_config, &mount_config, &card);//Convenience function to get FAT filesystem on SD card registered in VFS.
    if(ret != ESP_OK) 
    {
        //if(ret == ESP_FAIL) 
            //SD_ERROR("Failed to mount filesystem.""If you want the card to be formatted, set the CONFIG_EXAMPLE_FORMAT_IF_MOUNT_FAILED menuconfig option.");
        //else 
            //SD_ERROR("Failed to initialize the card (%s).""Make sure SD card lines have pull-up resistors in place.", esp_err_to_name(ret));
        return ret;
    }
    //SD_INFO("Filesystem mounted");
    sdmmc_card_print_info(stdout, card);
    return ret;
}



esp_err_t format_sd_card()
{
    esp_err_t ret = ESP_OK;
    ret = esp_vfs_fat_sdcard_format(mount_point, card);
    if(ret != ESP_OK) 
    {
        //SD_ERROR("Failed to format FATFS (%s)", esp_err_to_name(ret));
        return ret;
    }
    return ret;
}
/*———————————————————————————————————————Functional function end—————————————————————————————————————————*/