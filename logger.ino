#include <LowPower.h>
#include <SPI.h>
#include <SD.h>
#include "DFRobot_SHT20.h"
#include "MAX44009.h"
#include <Wire.h>

#define WORKINGPIN 8
#define ERROR_WARNING_PIN 9

unsigned long timerCount = 0;
unsigned long prevMillis = 0;
int interval = 600000;
float elapsedTime = 0;

DFRobot_SHT20 sht20(&Wire, 0x40);
MAX44009 light;

void(* resetFunc)(void) = 0;

void setup() {
  Serial.begin(115200);
  sht20.initSHT20();
  sht20.checkSHT20();
  analogReference(INTERNAL);
  pinMode(WORKINGPIN, OUTPUT);
  pinMode(ERROR_WARNING_PIN, OUTPUT);
  Wire.begin();
  //initialize light sensor, if not found sleep and blink warning light
  if(light.begin()){
    sensorError();
  }
  
  //initialize SD card, if not found, reset arduino
  if (!SD.begin(10)) {
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
    resetFunc();
  }
  File dataFile = SD.open("sensor.txt", FILE_WRITE);
  if(dataFile){
    dataFile.println("Logger Initialized\n");
    dataFile.close(); 
  }    
}

//print to serial monitor
void serialOut(float voltage, float lux, float temp, float hum){
  Serial.print("VOLTAGE: ");
  Serial.println(voltage);
  Serial.print("Light (lux): ");
  Serial.println(lux);
  Serial.print("Temp: "); Serial.print(temp); Serial.println(" C");
  Serial.print("Hum: "); Serial.print(hum); Serial.println("% rH\n");
}
// write data to file
void fileOut(File dataFile, float voltage, float lux, float temp, float hum){
  dataFile.print("Volts: ");
  dataFile.println(voltage);
  dataFile.print("Light (lux): ");
  dataFile.println(lux);
  dataFile.print("Temp: "); dataFile.print(temp); dataFile.println(" C");
  dataFile.print("Hum: "); dataFile.print(hum); dataFile.println("% rH\n");
}
//if voltage goes below 2.6, then pause logging and blink warning light every 8 seconds
void lowVoltageBatteryWARNING(){
  int count = 0;
  while(1){
    LowPower.powerDown(SLEEP_4S, ADC_OFF, BOD_OFF);
    if(count %2 == 0){
      digitalWrite(ERROR_WARNING_PIN, HIGH);
      delay(100);
      digitalWrite(ERROR_WARNING_PIN, LOW);
    }
    count++;
  }
}
// loop and blink light every 4 seconds
void sensorError(){
  int count = 0;
  while(1){
      LowPower.powerDown(SLEEP_2S, ADC_OFF, BOD_OFF);
      if(count %2 == 0){
        digitalWrite(ERROR_WARNING_PIN, HIGH);
        delay(100);
        digitalWrite(ERROR_WARNING_PIN, LOW);
      }
      count++;
    }
}

void loop() {
  //sleep 10 minutes
  for(int i = 0; i < 75; i++){
    LowPower.powerDown(SLEEP_8S, ADC_OFF, BOD_OFF);
    if(i % 2 == 0){
      digitalWrite(WORKINGPIN, HIGH);
      delay(3);
      digitalWrite(WORKINGPIN, LOW);
    }
    timerCount++;
  }

  unsigned long currentMillis = millis();
  float hum = sht20.readHumidity();
  float temp = sht20.readTemperature();
  float lux = light.get_lux();
  int sensorValue = analogRead(A1); //read the A0 pin value
  float voltage = sensorValue * (1.1 / 1023.00) * 11;
    
  //check voltage level 
  if (voltage < 2.6 && timerCount > 1){
    File dataFile = SD.open("sensor.txt", FILE_WRITE);
    if(dataFile){
      dataFile.println("VOLTAGE < 2.6v, REPLACE BATTERIES!");
      dataFile.close(); 
    }        
    lowVoltageBatteryWARNING();
  }
  File dataFile = SD.open("sensor.txt", FILE_WRITE);
  if(dataFile){
    currentMillis = currentMillis + (timerCount * 8000);
    if ((unsigned long)(currentMillis - prevMillis) >= interval) {
      elapsedTime++;
      float hours = (elapsedTime * 10) / 60;
      dataFile.print(hours);
      dataFile.println(" hours have passed...");
      prevMillis = currentMillis;
    }
    fileOut(dataFile, voltage, lux, temp, hum);
  //serialOut(voltage, lux, temp, hum);
    dataFile.close();
  }
  else{
    Serial.print("ERROR OPENING FILE\n");
  }  
}
