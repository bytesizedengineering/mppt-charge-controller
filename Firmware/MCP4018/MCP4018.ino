#define DOWN_BUTTON 1 // pulled low by default
#define UP_BUTTON 2 // pulled low by default
#define MCP4018_ADDRESS 0x2F

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789

uint8_t potValue = 80;
unsigned long lastTimePotValueDisplayed = 0;

// TFT display and button variables
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
const int displayWidth = 240;
const int displayHeight = 135;
unsigned long lastTimeDownButtonWasPressed = 0, lastTimeUpButtonWasPressed = 0;

// Interrupt Service Routines
void IRAM_ATTR downButton(){
  if(millis() - lastTimeDownButtonWasPressed > 250){
    lastTimeDownButtonWasPressed = millis();
    potValue--;
    //Serial.println("down button was pressed.");
  }
}
void IRAM_ATTR upButton(){
  if(millis() - lastTimeUpButtonWasPressed > 250){
    lastTimeUpButtonWasPressed = millis();
    potValue++;
    //Serial.println("up button was pressed.");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  
  pinMode(DOWN_BUTTON, INPUT_PULLDOWN);
  pinMode(UP_BUTTON, INPUT_PULLDOWN);
  attachInterrupt(digitalPinToInterrupt(DOWN_BUTTON), downButton, RISING);
  attachInterrupt(digitalPinToInterrupt(UP_BUTTON), upButton, RISING);
  
  // Initialize TFT
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
  pinMode(TFT_I2C_POWER,OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);
  tft.init(displayHeight, displayWidth); // Init ST7789 240x135
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  
}

void loop() {
  setPotentiometerValue(potValue);
  
  if(millis() - lastTimePotValueDisplayed > 1000){
    lastTimePotValueDisplayed = millis();
    tft.fillRect(0,0,displayWidth,displayHeight/4,ST77XX_BLACK);
    tft.setCursor(0,0);
    tft.print(potValue);
  }
  
}
void setPotentiometerValue(uint8_t value) {
  if (value > 127) value = 127; // Ensure the value is within the valid range
  Wire.beginTransmission(MCP4018_ADDRESS);
  Wire.write(value);
  Wire.endTransmission();
}