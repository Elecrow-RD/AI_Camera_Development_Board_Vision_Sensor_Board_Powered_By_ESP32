#ifndef _ELECROW_SD_H
#define _ELECROW_SD_H

#include <SPI.h>
#include <SD.h>

class ELECROW_SD
{
  public:

    enum SD_Save_Result { SAVE_OK, ERR_OPEN, ERR_WRITE, ERR_NOT_INSERTED };

    ELECROW_SD(int SPI_SCK,int SPI_MISO,int SPI_MOSI,int SPI_NSS);

    ~ELECROW_SD(){};
    
    bool SD_init(void);

    void listDir(fs::FS & fs, const char *dirname, uint8_t levels);

    void createDir(fs::FS & fs, const char *path);

    void removeDir(fs::FS & fs, const char *path);

    void createFile(fs::FS & fs,const char *path);

    void readFile(fs::FS & fs, const char *path);

    void writeFile(fs::FS & fs, const char *path, const char *message);

    void appendFile(fs::FS & fs, const char *path, const char *message);

    void renameFile(fs::FS & fs, const char *path1, const char *path2);

    void deleteFile(fs::FS & fs, const char *path);

    SD_Save_Result save_rgb565_to_bmp(fs::FS &fs,
                               const char *path,
                               const uint16_t *rgb565,
                               int width,
                               int height);
    bool read_jpg_from_fs(fs::FS &fs, const char *path, uint8_t **out_buf, size_t *out_len);

    void SD_test(void);

    void SD_End();

  protected:

  private:
    int SD_SCK_PIN; 

    int SD_MISO_PIN;

    int SD_MOSI_PIN;

    int SD_NSS_PIN;
};

#endif