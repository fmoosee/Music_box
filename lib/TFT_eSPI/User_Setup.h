#define USER_SETUP_ID 27
#define ST7796_DRIVER

// Pinos SPI atualizados
#define TFT_MOSI  23
#define TFT_SCLK  18
#define TFT_CS    5     // Movido do 15 para o 5
#define TFT_DC    16
#define TFT_RST   17
#define TFT_MISO  19

// Fontes
#define LOAD_GLCD
#define LOAD_FONT2
#define LOAD_FONT4
#define LOAD_FONT6
#define LOAD_FONT7
#define LOAD_FONT8
#define LOAD_GFXFF
#define SMOOTH_FONT

// Frequência estável para o ST7796
#define SPI_FREQUENCY       27000000L   // 27 MHz para escrita de pixels
#define SPI_READ_FREQUENCY  20000000L   // 20 MHz para leitura