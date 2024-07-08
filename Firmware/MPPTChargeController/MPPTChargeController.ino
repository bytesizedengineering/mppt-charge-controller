#define LOAD_ENABLE_BUTTON 0 // pulled high by default
#define BATTERY_INFO_BUTTON 1 // pulled low by default
#define SOLAR_INFO_BUTTON 2 // pulled low by default
#define CHARGING_STATUS 5
#define COMPLETE_STATUS 6
#define CHARGE_EN 9
#define LOAD_EN 11
#define SOLAR_VOLTAGE_ADC A0
#define SOLAR_CURRENT_ADC A1
#define BATTERY_VOLTAGE_ADC A2
#define BATTERY_CURRENT_ADC A3
#define MCP4018_ADDRESS 0x2F
#define UPDATE_DATA_INTERVAL 1000

#include <SPI.h>
#include <Wire.h>
#include <Adafruit_GFX.h>    // Core graphics library
#include <Adafruit_ST7789.h> // Hardware-specific library for ST7789
#include <Fonts/FreeSansBold18pt7b.h>
#include <Fonts/FreeSansBold12pt7b.h>


// TFT display and button variables
Adafruit_ST7789 tft = Adafruit_ST7789(TFT_CS, TFT_DC, TFT_RST);
const int displayWidth = 240;
const int displayHeight = 135;
bool solarInfoButtonWasPressed = true, batteryInfoButtonWasPressed = false, loadEnableButtonWasPressed = false, newHeader = true;
unsigned long lastTimeSolarInfoButtonWasPressed = 0, lastTimeBatteryInfoButtonWasPressed = 0, lastTimeLoadEnableButtonWasPressed = 0;
float solarVoltage = 0.0, previousSolarVoltage = 0.0, solarCurrent = 0.0, previousSolarCurrent = 0.0, solarPower = 0.0, previousSolarPower = 0.0;
float batteryVoltage = 0.0, previousBatteryVoltage = 0.0, batteryCurrent = 0.0, previousBatteryCurrent = 0.0, batteryPower = 0.0;
unsigned long lastTimeSolarWasDisplayed = 0, lastTimeBatteryWasDisplayed = 0, lastTimeStatsWerePrinted = 0;
bool increaseVmp = true;

uint8_t potValue = 64;

// Interrupt Service Routines
void IRAM_ATTR loadEnableButton(){
  if(millis() - lastTimeLoadEnableButtonWasPressed > 250){
    lastTimeLoadEnableButtonWasPressed = millis();
    solarInfoButtonWasPressed = false;
    batteryInfoButtonWasPressed = false;
    loadEnableButtonWasPressed = true;
    newHeader = true;
    //Serial.println("load enable button was pressed.");
  }
}
void IRAM_ATTR batteryInfoButton(){
  if(millis() - lastTimeBatteryInfoButtonWasPressed > 250){
    lastTimeBatteryInfoButtonWasPressed = millis();
    solarInfoButtonWasPressed = false;
    batteryInfoButtonWasPressed = true;
    loadEnableButtonWasPressed = false;
    newHeader = true;
    //Serial.println("battery info button was pressed.");
  }
}
void IRAM_ATTR solarInfoButton(){
  if(millis() - lastTimeSolarInfoButtonWasPressed > 250){
    lastTimeSolarInfoButtonWasPressed = millis();
    solarInfoButtonWasPressed = true;
    batteryInfoButtonWasPressed = false;
    loadEnableButtonWasPressed = false;
    newHeader = true;
    //Serial.println("solar info button was pressed.");
  }
}

void setup() {
  Serial.begin(115200);
  Wire.begin();
  // Initialize buttons
  pinMode(LOAD_ENABLE_BUTTON, INPUT_PULLUP);
  pinMode(BATTERY_INFO_BUTTON, INPUT_PULLDOWN);
  pinMode(SOLAR_INFO_BUTTON, INPUT_PULLDOWN);


  attachInterrupt(digitalPinToInterrupt(LOAD_ENABLE_BUTTON), loadEnableButton, FALLING);
  attachInterrupt(digitalPinToInterrupt(BATTERY_INFO_BUTTON), batteryInfoButton, RISING);
  attachInterrupt(digitalPinToInterrupt(SOLAR_INFO_BUTTON), solarInfoButton, RISING);

  // Initialize TFT
  pinMode(TFT_BACKLITE, OUTPUT);
  digitalWrite(TFT_BACKLITE, HIGH);
  pinMode(TFT_I2C_POWER,OUTPUT);
  digitalWrite(TFT_I2C_POWER, HIGH);
  delay(10);
  tft.init(displayHeight, displayWidth); // Init ST7789 240x135
  tft.setRotation(1);
  tft.fillScreen(ST77XX_BLACK);
  //tft.println("Solar MPPT\n Charge Controller "); delay(5000); tft.fillScreen(ST77XX_BLACK);

  pinMode(CHARGING_STATUS, INPUT);
  pinMode(COMPLETE_STATUS, INPUT);
  pinMode(CHARGE_EN, OUTPUT); digitalWrite(CHARGE_EN, LOW);
  pinMode(LOAD_EN, OUTPUT); digitalWrite(LOAD_EN, HIGH);





}

void loop() {
  //read solar voltage and current, and calculate power
  solarVoltage = (analogReadMilliVolts(SOLAR_VOLTAGE_ADC)*535/36.0)/1000.0 + 0.06; // voltage divider resistor values on solar input
  solarVoltage = previousSolarVoltage*0.99 + solarVoltage*0.01;
  previousSolarVoltage = solarVoltage;

  solarCurrent = (analogReadMilliVolts(SOLAR_CURRENT_ADC)*20000/3300.0-10000)/1000.0; // Using a +/-10A current sensor running on 3.3V VCC
  solarCurrent = previousSolarCurrent*0.95 + solarCurrent*0.05;
  previousSolarCurrent = solarCurrent;

  solarPower = solarVoltage*solarCurrent;
  setVmp();

  //read battery voltage and current and calculate power
  batteryVoltage = (analogReadMilliVolts(BATTERY_VOLTAGE_ADC)*599/100.0)/1000.0 + 0.4; // voltage divider resistor values on battery output
  batteryVoltage = previousBatteryVoltage*0.99 + batteryVoltage*0.01;
  previousBatteryVoltage = batteryVoltage;

  batteryCurrent = (analogReadMilliVolts(BATTERY_CURRENT_ADC)*20000/3300.0-10000)/1000.0; // Using a +/-10A current sensor running on 3.3V VCC
  batteryCurrent = previousBatteryCurrent*0.95 + batteryCurrent*0.05;
  previousBatteryCurrent = batteryCurrent;

  batteryPower = batteryVoltage*batteryCurrent;

  float controllerEfficiency = batteryPower/solarPower;
  

  if(millis() - lastTimeStatsWerePrinted > UPDATE_DATA_INTERVAL){
    lastTimeStatsWerePrinted = millis();
    Serial.print("Solar Input: ");
    Serial.print(solarVoltage); Serial.print("V\t");
    Serial.print(solarCurrent); Serial.print("A\t");
    Serial.print(solarPower); Serial.print("W\t");
    Serial.print("Battery Output: ");
    Serial.print(batteryVoltage); Serial.print("V\t");
    Serial.print(batteryCurrent); Serial.print("A\t");
    Serial.print(batteryPower); Serial.print("W\t");
    Serial.print("Controller Effeciency: "); Serial.print(controllerEfficiency*100); Serial.println("%");
    
        
  }

  if(loadEnableButtonWasPressed){
    if(newHeader){
      displayHeader(ST77XX_GREEN, "load enable");
      newHeader = false;
    }
  }
  if(batteryInfoButtonWasPressed){
    if(newHeader){
      displayHeader(ST77XX_RED, "Battery Output");
      newHeader = false;
    }
    if(millis() - lastTimeBatteryWasDisplayed > UPDATE_DATA_INTERVAL){
      lastTimeBatteryWasDisplayed = millis();
      displayData("battery");
    }
  }
  if(solarInfoButtonWasPressed){
    if(newHeader){
      displayHeader(ST77XX_YELLOW, "Solar Input");
      newHeader = false;
    }
    if(millis() - lastTimeSolarWasDisplayed > UPDATE_DATA_INTERVAL){
      lastTimeSolarWasDisplayed = millis();
      displayData("solar");
    }
  }
}

void displayHeader(uint16_t color, String headerText){
  tft.fillRect(0,0,displayWidth,displayHeight/3 - 1,color);
  tft.setTextColor(ST77XX_BLACK); tft.setFont(&FreeSansBold12pt7b);
  tft.setCursor(0, 30);
  tft.print(headerText);
}

void displayData(String source){
  tft.fillRect(0,displayHeight/3,displayWidth,displayHeight*2/3,ST77XX_BLACK);
  tft.setTextColor(ST77XX_WHITE); tft.setFont(&FreeSansBold18pt7b);
  tft.setCursor(0, 75);
  if(source == "solar"){
    tft.print(solarVoltage); tft.print("V  ");
    tft.print(solarCurrent); tft.println("A");
    tft.print(solarPower);   tft.print("W  ");
  }
  else if(source == "battery"){
    tft.print(batteryVoltage); tft.print("V  ");
    tft.print(batteryCurrent); tft.println("A");
    tft.print(batteryPower);   tft.print("W  ");
  }
  else{
    tft.println("invalid data source");
  }
  tft.println(potValue);
}

void setPotentiometerValue() {
  Wire.beginTransmission(MCP4018_ADDRESS);
  Wire.write(potValue);
  Wire.endTransmission();
}

void setVmp(){
  if(solarPower > previousSolarPower){
    if(increaseVmp) potValue++;
    else potValue--;
  }
  else{
    if(increaseVmp){
      increaseVmp = false;
      potValue--;
    }
    else{
      increaseVmp = true;
      potValue++;
    }
  }
  previousSolarPower = solarPower;
  potValue = constrain(potValue, 0, 127);
  setPotentiometerValue();
}