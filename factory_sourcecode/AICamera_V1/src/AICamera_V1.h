#ifndef __AICamera_V2__
#define __AICamera_V2__

#ifdef __cplusplus
extern "C" {
#endif

/* IIC  */
#define I2C_SCL                     15
#define I2C_SDA                     16
#define TP_TRST                     12
#define TP_INT                      11

/*SD card*/
#define SPI_NSS_PIN                 -1
#define SPI_MISO_PIN                6
#define SPI_SCK_PIN                 7
#define SPI_MOSI_PIN                8

/* Screen */
#define LCD_DC_PIN                  9
#define LCD_CS_PIN                  10
#define LCD_SCK_PIN                 14
#define LCD_MOSI_PIN                13
#define TP_SDA_PIN                  I2C_SDA
#define TP_SCL_PIN                  I2C_SCL
#define TP_TRST                     12
#define TP_INT                      11

/*Camera*/
#define CAMERA_PWDN_PIN             -1
#define CAMERA_RESET_PIN            -1
#define CAMERA_XCLK_PIN             39 
#define CAMERA_SCL_PIN              40   
#define CAMERA_SDA_PIN              41 
#define CAMERA_D0_PIN               20
#define CAMERA_D1_PIN               18
#define CAMERA_D2_PIN               17
#define CAMERA_D3_PIN               19
#define CAMERA_D4_PIN               21
#define CAMERA_D5_PIN               47
#define CAMERA_D6_PIN               38
#define CAMERA_D7_PIN               46
#define CAMERA_VSYNC_PIN            42
#define CAMERA_HREF_PIN             45   
#define CAMERA_PCLK_PIN             48 

static const uint16_t screenWidth=240;
static const uint16_t screenHeight=284;
static lv_disp_draw_buf_t draw_buf;
static lv_disp_drv_t disp_drv; 
static lv_indev_drv_t indev_drv;    

#ifdef __cplusplus
}
#endif

#endif