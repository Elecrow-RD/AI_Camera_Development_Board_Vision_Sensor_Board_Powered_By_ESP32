#include <stdint.h>
#include "elecrow_sd.h"

extern uint8_t sd_state;
float percentage;
ELECROW_SD::ELECROW_SD(int SPI_SCK,int SPI_MISO,int SPI_MOSI,int SPI_NSS)
{
  SD_SCK_PIN=SPI_SCK;
  SD_MISO_PIN= SPI_MISO;
  SD_MOSI_PIN=SPI_MOSI;
  SD_NSS_PIN=SPI_NSS;
}
void ELECROW_SD::SD_End(void)
{
  SD.end();
}


bool ELECROW_SD::SD_init(void)
{
  Serial.print("Initializing SD card...");
  SPI.begin(SD_SCK_PIN,SD_MISO_PIN,SD_MOSI_PIN,SD_NSS_PIN);
  if(!SD.begin()) 
  {
    Serial.println("initialization failed. Things to check:");
    Serial.println("1. is a card inserted?");
    Serial.println("2. is your wiring correct?");
    Serial.println("3. did you change the chipSelect pin to match your shield or module?");
    Serial.println("Note: press reset button on the board and reopen this Serial Monitor after fixing your issue!");

    while (true);
    return false;
  }
  else{
    
    return true;
  }
    
  Serial.println("initialization done.");
}

void ELECROW_SD::listDir(fs::FS & fs, const char *dirname, uint8_t levels)
{
  Serial.printf("Listing directory: %s\n", dirname);

  File root = fs.open(dirname);
  if (!root)
  {
    Serial.println("Failed to open directory");
    return;
  }
  if (!root.isDirectory())
  {
    Serial.println("Not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file)
  {
    if (file.isDirectory())
    {
      Serial.print("  DIR : ");
      Serial.println(file.name());
      if (levels)
      {
        listDir(fs, file.name(), levels - 1);
      }
    }
    else
    {
      Serial.print("  FILE: ");
      Serial.print(file.name());
      Serial.print("  SIZE: ");
      Serial.println(file.size());
    }
    file = root.openNextFile();
  }
}

void ELECROW_SD::createDir(fs::FS & fs, const char *path)
{
  Serial.printf("Creating Dir: %s\n", path);
  if (fs.mkdir(path))
  {
    Serial.println("Dir created");
  }
  else
  {
    Serial.println("mkdir failed");
  }
}

void ELECROW_SD::removeDir(fs::FS & fs, const char *path)
{
  Serial.printf("Removing Dir: %s\n", path);
  if (fs.rmdir(path))
  {
    Serial.println("Dir removed");
  }
  else
  {
    Serial.println("rmdir failed");
  }
}

void ELECROW_SD::createFile(fs::FS & fs,const char *path)
{
  Serial.printf("Creating File: %s\n", path);
  File file = fs.open(path);
  if (file)
  {
    Serial.println("File created");
    file.close();
  }
  else
  {
    Serial.println("created file failed");
  }
}

void ELECROW_SD::readFile(fs::FS & fs, const char *path)
{
  Serial.printf("Reading file: %s\n", path);

  File file = fs.open(path);
  if (!file)
  {
    Serial.println("Failed to open file for reading");
    return;
  }

  Serial.print("Read from file: ");
  while (file.available())
  {
    Serial.write(file.read());
  }
  file.close();
}

void ELECROW_SD::writeFile(fs::FS & fs, const char *path, const char *message)
{
  Serial.printf("Writing file: %s\n", path);

  File file = fs.open(path, FILE_WRITE);
  if (!file)
  {
    Serial.println("Failed to open file for writing");
    return;
  }
  if (file.print(message))
  {
    Serial.println("File written");
  }
  else
  {
    Serial.println("Write failed");
  }
  file.close();
}

void ELECROW_SD::appendFile(fs::FS & fs, const char *path, const char *message)
{
  Serial.printf("Appending to file: %s\n", path);

  File file = fs.open(path, FILE_APPEND);
  if (!file)
  {
    Serial.println("Failed to open file for appending");
    return;
  }
  if (file.print(message))
  {
    Serial.println("Message appended");
  }
  else
  {
    Serial.println("Append failed");
  }
  file.close();
}

void ELECROW_SD::renameFile(fs::FS & fs, const char *path1, const char *path2)
{
  Serial.printf("Renaming file %s to %s\n", path1, path2);
  if (fs.rename(path1, path2))
  {
    Serial.println("File renamed");
  }
  else
  {
    Serial.println("Rename failed");
  }
}

void ELECROW_SD::deleteFile(fs::FS & fs, const char *path)
{
  Serial.printf("Deleting file: %s\n", path);
  if (fs.remove(path))
  {
    Serial.println("File deleted");
  }
  else
  {
    Serial.println("Delete failed");
  }
}

ELECROW_SD::SD_Save_Result ELECROW_SD::save_rgb565_to_bmp(fs::FS &fs,
                               const char *path,
                               const uint16_t *rgb565,
                               int width,
                               int height)
{
    if (!rgb565 || width <= 0 || height <= 0)
        return ERR_WRITE;

    File file = fs.open(path, FILE_WRITE);
    if (!file)
        return ERR_OPEN;

    const uint32_t fileHeaderSize = 14;
    const uint32_t infoHeaderSize = 40;
    const uint32_t maskSize       = 12;
    const uint32_t headerSize     = fileHeaderSize + infoHeaderSize + maskSize;

    const uint32_t imageSize = width * height * 2;
    const uint32_t fileSize  = headerSize + imageSize;

    uint8_t header[66] = {0};

    // ===== BITMAPFILEHEADER =====
    header[0] = 'B';
    header[1] = 'M';
    *(uint32_t*)&header[2]  = fileSize;
    *(uint32_t*)&header[10] = headerSize;   // !!! 66

    // ===== BITMAPINFOHEADER =====
    *(uint32_t*)&header[14] = infoHeaderSize;
    *(int32_t*)&header[18]  = width;
    *(int32_t*)&header[22]  = height;       // 正值：bottom-up
    *(uint16_t*)&header[26] = 1;
    *(uint16_t*)&header[28] = 16;           // RGB565
    *(uint32_t*)&header[30] = 3;            // BI_BITFIELDS
    *(uint32_t*)&header[34] = imageSize;

    // ===== RGB565 MASKS =====
    *(uint32_t*)&header[54] = 0xF800;  // Red
    *(uint32_t*)&header[58] = 0x07E0;  // Green
    *(uint32_t*)&header[62] = 0x001F;  // Blue

    // 写头
    file.write(header, sizeof(header));

    // BMP 像素必须从底行开始
    for (int y = height - 1; y >= 0; y--) {
        file.write((uint8_t *)(rgb565 + y * width),
                   width * 2);
    }

    file.close();
    return SAVE_OK;
}

void ELECROW_SD::SD_test()
{

  /*
    uint8_t cardType = SD.cardType();
    uint64_t cardSize = SD.cardSize() / (1024 * 1024);
    Serial.print("SD Card Type: ");
    if (cardType == CARD_NONE)
    {
      Serial.println("No SD card attached");
    }
    else if (cardType == CARD_MMC)
    {
      Serial.println("MMC");
    }
    else if (cardType == CARD_SD)
    {
      Serial.println("SDSC");
    }
    else if (cardType == CARD_SDHC)
    {
      Serial.println("SDHC");
    }
    else
    {
      Serial.println("UNKNOWN");
    }
    Serial.printf("SD Card Size: %lluMB\n", cardSize);
    
    listDir(SD, "/", 0);
    createDir(SD, "/mydir");
    listDir(SD, "/", 0);
    removeDir(SD, "/mydir");
    writeFile(SD, "/hello.txt", "Hello ");
    appendFile(SD, "/hello.txt", "World!\n");
    readFile(SD, "/hello.txt");
    deleteFile(SD, "/foo.txt");
    renameFile(SD, "/hello.txt", "/foo.txt");
    readFile(SD, "/foo.txt");
    
    Serial.printf("Total space: %lluMB\n", SD.totalBytes() / (1024 * 1024));
    Serial.printf("Used space: %lluMB\n", SD.usedBytes() / (1024 * 1024));
*/
    uint64_t total = SD.totalBytes() / (1024 * 1024);
    uint64_t used =  SD.usedBytes() / (1024 * 1024);
    float used_percent = ((float)used / (float)total) * 100.0;
    percentage = 100.0 - used_percent;

    //Serial.printf("percentage = %.2f\n", percentage);

    //Serial.println(percentage);

}

