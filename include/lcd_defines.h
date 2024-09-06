// Define GPIOs for the RGB interface (adjust based on the schematic)
#define GPIO_LCD_VSYNC      41
#define GPIO_LCD_HSYNC      39
#define GPIO_LCD_DE         40
#define GPIO_LCD_PCLK       0
#define GPIO_LCD_BACKLIGHT  2

#define GPIO_LCD_R3         45
#define GPIO_LCD_R4         48
#define GPIO_LCD_R5         47
#define GPIO_LCD_R6         21
#define GPIO_LCD_R7         14

#define GPIO_LCD_G2         5
#define GPIO_LCD_G3         6
#define GPIO_LCD_G4         7
#define GPIO_LCD_G5         15
#define GPIO_LCD_G6         16
#define GPIO_LCD_G7         4

#define GPIO_LCD_B3         8
#define GPIO_LCD_B4         3
#define GPIO_LCD_B5         46
#define GPIO_LCD_B6         9
#define GPIO_LCD_B7         1

#define LCD_WIDTH  800
#define LCD_HEIGHT 480

#define LCD_PIN_D0 GPIO_LCD_B3
#define LCD_PIN_D1 GPIO_LCD_B4
#define LCD_PIN_D2 GPIO_LCD_B5
#define LCD_PIN_D3 GPIO_LCD_B6
#define LCD_PIN_D4 GPIO_LCD_B7

#define LCD_PIN_D5 GPIO_LCD_G2
#define LCD_PIN_D6 GPIO_LCD_G3
#define LCD_PIN_D7 GPIO_LCD_G4
#define LCD_PIN_D8 GPIO_LCD_G5
#define LCD_PIN_D9 GPIO_LCD_G6
#define LCD_PIN_D10 GPIO_LCD_G7

#define LCD_PIN_D11 GPIO_LCD_R3
#define LCD_PIN_D12 GPIO_LCD_R4
#define LCD_PIN_D13 GPIO_LCD_R5
#define LCD_PIN_D14 GPIO_LCD_R6
#define LCD_PIN_D15 GPIO_LCD_R7
