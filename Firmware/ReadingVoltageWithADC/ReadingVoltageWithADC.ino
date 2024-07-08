#define SOLAR_VOLTAGE_ADC A0

#include <SPI.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Fonts/FreeSansBold18pt7b.h>

// TFT display and button variables
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
const int displayWidth = 135;
const int displayHeight = 240;
//unsigned long lastTimeTimeAndDateButtonWasPressed = 0, lastTimeLatLongButtonWasPressed = 0, lastTimeDataButtonWasPressed = 0;
//bool timeAndDateButtonWasPressed = false, latLongButtonWasPressed = false, dataButtonWasPressed = true, clearDisplay = false;

float solarVoltage = 0.0;

void setup() {
  Serial.begin(115200);
  
  // Initialize TFT
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
  pinMode(TFT_I2C_POWER,OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);
  tft.init(displayWidth, displayHeight); // Init ST7789 240x135
  tft.setRotation(3); tft.setFont(&FreeSansBold18pt7b); tft.setCursor(30, 60);
  tft.fillScreen(ST77XX_BLACK);
  tft.println("Solar MPPT\n Charge Controller "); delay(5000);
  tft.fillScreen(ST77XX_BLACK);
}

void loop() {
  solarVoltage = analogRead(SOLAR_VOLTAGE_ADC)/8192.0 * 28.0;
  tft.fillRect(0,0,displayHeight,48,ST77XX_YELLOW); tft.setCursor(0, 24); tft.setTextColor(ST77XX_BLACK); tft.print(solarVoltage); tft.print("V"); Serial.print(solarVoltage); Serial.println("V");
  delay(500);
}
