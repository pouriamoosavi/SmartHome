#include "Keypad_I2C.h"
#include <Keypad.h>
#include <Wire.h>
#include <LiquidCrystal_I2C.h>

const byte LCD_I2C_ADDR = 0x24;
const byte KEYPAD_I2C_ADDR = 0x20;

LiquidCrystal_I2C lcd(LCD_I2C_ADDR, 20, 4);

char hexaKeys[4][3] = {
  {'1', '2', '3'},
  {'4', '5', '6'},
  {'7', '8', '9'},
  {'*', '0', '#'}
};

byte rowPins[] = {3,4,5,6}; 
byte colPins[] = {2,1,0}; 

Keypad_I2C mkp(makeKeymap(hexaKeys), rowPins, colPins, 4, 3, KEYPAD_I2C_ADDR); 

const uint8_t TEMP_MENU = 1;
const uint8_t HUMI_MENU = 2;
const uint8_t LIGHT_MENU = 3;

String inputMessage;

boolean isInMainMenu;
int8_t desiredMenu = 0;
int8_t desiredTemp = 0;
int8_t desiredHumi = 0;
int8_t desiredLight = 0;

int8_t desiredTempTemp = 0;
int8_t desiredHumiTemp = 0;
int8_t desiredLightTemp = 0;

int8_t* mkpPointTo;
boolean mkpIsReady;

const int COOLER_ENGINE_DELAY = 1000;

unsigned long coolerPumpStart = 0;
uint8_t coolerPumpOn = 0;
uint8_t coolerEngineOn = 0;
uint8_t coolerEngineFastOn = 0;
uint8_t heaterOn = 0;

const uint8_t COOLER_PUMP_PIN = 5;
const uint8_t COOLER_ENGINE_PIN = 4;
const uint8_t COOLER_ENGINE_FAST_PIN = 3;
const uint8_t HEATER_PIN = 2;

const uint8_t TEMP_SENSOR_PIN = 3;

void setup() {
  Serial.begin(9600);
  Wire.begin();
  mkp.begin();
  
  lcd.init();
  lcd.backlight();

  pinMode(COOLER_PUMP_PIN, OUTPUT);
  pinMode(COOLER_ENGINE_PIN, OUTPUT);
  pinMode(COOLER_ENGINE_FAST_PIN, OUTPUT);
  pinMode(HEATER_PIN, OUTPUT);
  
  resetState();
}

void loop() {
  
  if(isInMainMenu) {
    getMenuFromMkp();

    if(mkpIsReady) {
      isInMainMenu = false;
      
      if(desiredMenu == TEMP_MENU) {
        inputMessage = "Enter temp: (" + String(desiredTemp) + ")";
        mkpPointTo = &desiredTempTemp;
        
      } else if (desiredMenu == HUMI_MENU) {
        inputMessage = "Enter humidity: (" + String(desiredHumi) + ")";
        mkpPointTo = &desiredHumiTemp;
        
      } else if (desiredMenu == LIGHT_MENU) {
        inputMessage = "Enter light: (" + String(desiredLight) + ")";
        mkpPointTo = &desiredLightTemp;
      }
      
      mkpIsReady = false;
      showOtherMenu();
    }
    
  } else {
    if(mkpIsReady) {
      desiredTemp = desiredTempTemp;
      desiredHumi = desiredHumiTemp;
      desiredLight = desiredLightTemp;
      resetState();
    } else {
      getIntFromMkp(true);
    }
  }

  checkTemp();

  delay(10);
  
}

void getIntFromMkp(boolean echo) {
  char key = mkp.getKey();
  if(key) {
    if(echo) {
      lcd.print(key); 
    }
    if(key == '*') {
      mkpIsReady = true;
    } else if (key == '#') {
      resetState();
    } else {
      *mkpPointTo = (*mkpPointTo * 10) + (key-'0');
      mkpIsReady = false;
    }
  }
}

void getMenuFromMkp() {
  char key = mkp.getKey();
  if(key) {
    *mkpPointTo = key-'0';
    mkpIsReady = true;
  }
}

void showMainMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  String lcdLineOne = "1- Set temp (" + String(desiredTemp) + ")";
  lcd.print(lcdLineOne);
  lcd.setCursor(0,1);
  String lcdLineTwo = "2- Set humi (" + String(desiredHumi) + ")";
  lcd.print(lcdLineTwo);
  lcd.setCursor(0,2);
  String lcdLineThree = "3- Set light (" + String(desiredLight) + ")";
  lcd.print(lcdLineThree);
}

void showOtherMenu() {
  lcd.clear();
  lcd.setCursor(0,0);
  lcd.print(inputMessage);
  lcd.setCursor(0,1);
  lcd.print("(* Save) (# Cancel)");
  lcd.setCursor(0,2);
}

void resetState() {
  desiredTempTemp = 0;
  desiredHumiTemp = 0;
  desiredLightTemp = 0;

  showMainMenu();
  mkpPointTo = &desiredMenu;
  mkpIsReady = false;
  isInMainMenu = true;
}

int getCurrentTemp() {
  return analogRead(TEMP_SENSOR_PIN)*500/1024;
}

void checkTemp() {
  int currentTemp = getCurrentTemp();
  
  if(currentTemp >= desiredTemp+1) {
    heaterOn = 0;
        
    if(!coolerPumpOn && !coolerEngineOn) {
      coolerPumpStart = millis();
      coolerPumpOn = 1;
    }

    if(coolerPumpOn && (millis() - coolerPumpStart > COOLER_ENGINE_DELAY)
      && !coolerEngineOn) {
        coolerEngineOn = 1;
    }
    
  }
  
  if (currentTemp >= desiredTemp+2) {
    heaterOn = 0;
        
    if(coolerEngineOn && !coolerEngineFastOn) {
      coolerEngineFastOn = 1;
    }

  } 
  
  if (currentTemp <= desiredTemp-1) {
    
    coolerPumpOn = 0;
    coolerEngineOn = 0;
    coolerEngineFastOn = 0;
    
    if(!heaterOn) {
       heaterOn = 1;
    }
  }

  if(currentTemp == desiredTemp) {
    coolerPumpOn = 0;
    coolerEngineOn = 0;
    coolerEngineFastOn = 0;
    heaterOn = 0;
  }

  digitalWrite(COOLER_PUMP_PIN, coolerPumpOn);
  digitalWrite(COOLER_ENGINE_PIN, coolerEngineOn);
  digitalWrite(COOLER_ENGINE_FAST_PIN, coolerEngineFastOn);
  digitalWrite(HEATER_PIN, heaterOn);
}
