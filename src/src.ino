#include <Arduino.h>
#include <string.h>
#include <Wire.h>
#include <OneWire.h>;
#include <DallasTemperature.h>;

// General
#define SERIAL_BAUD 115200
#define MAX_INPUT_LENGTH 255
#define ANALOG_READ_MAX 1023
#define PH_I2C_ADDRESS 99

// Pins
const byte PUMPENABLE[] = {5, 6, 3};
const byte PUMPPIN1[] = {8, 10, 12};
const byte PUMPPIN2[] = {9, 11, 13};
#define ONE_WIRE_BUS 2

// Variables
char input[MAX_INPUT_LENGTH];
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
float temperature;

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PUMPENABLE[0], OUTPUT);
  digitalWrite(PUMPENABLE[0], LOW);
  pinMode(PUMPENABLE[1], OUTPUT);
  digitalWrite(PUMPENABLE[1], LOW);
  pinMode(PUMPPIN1[0], OUTPUT);
  digitalWrite(PUMPPIN1[0], LOW);
  pinMode(PUMPPIN1[1], OUTPUT);
  digitalWrite(PUMPPIN1[1], LOW);
  pinMode(PUMPPIN2[0], OUTPUT);
  digitalWrite(PUMPPIN2[0], LOW);
  pinMode(PUMPPIN2[1], OUTPUT);
  digitalWrite(PUMPPIN2[1], LOW);
  Wire.begin();
}

void loop() {
  if (Serial.available() > 0) {
    byte terminatorAt = Serial.readBytes(input, MAX_INPUT_LENGTH);
    while (input[terminatorAt-1] == '\r' || input[terminatorAt-1] == '\n') {
      terminatorAt--;
    }
    input[terminatorAt] = 0;
    char* command = strtok(input, " ");
    if (strcmp(command, "getTemp") == 0) {
      getTemp();
    }
    else if (strcmp(command, "calibratePh") == 0) {
      char* type = strtok(0, " ");
      calibratePh(type);
    }
    else if (strcmp(command, "getPh") == 0) {
      getPh();
    }
    else if (strcmp(command, "setPump") == 0) {
      char* pump = strtok(0, " ");
      char* movements = strtok(0, " ");
      setPumpMultiple(pump, movements);
    }
    else {
      Serial.println("Unrecognized Command");
    }
  }
}

void getTemp() {
  sensors.requestTemperatures();
  float readValue = sensors.getTempCByIndex(0);
  if (readValue == -127.00) {
    Serial.println("ERROR");
  } else {
    Serial.println(readValue);
    temperature = readValue;
  }
}

void calibratePh(char* type) {
  if (strcmp(type, "acid") == 0) {
    sendI2C(PH_I2C_ADDRESS, "Cal,low,4.00", 900);
    Serial.println("OK");
  } else if (strcmp(type, "neutral") == 0) {
    sendI2C(PH_I2C_ADDRESS, "Cal,mid,7.00", 900);
    Serial.println("OK");
  } else if (strcmp(type, "base") == 0) {
    sendI2C(PH_I2C_ADDRESS, "Cal,high,10.00", 900);
    Serial.println("OK");
  } else {
    Serial.println("ERROR");
  }
}

void getPh() {  
  char temperatureChar[10];
  dtostrf(temperature,3,2,temperatureChar);
  char temperatureCorrectionCommand[20] = "";
  sprintf(temperatureCorrectionCommand, "T,%s", temperatureChar);
  sendI2C(PH_I2C_ADDRESS, temperatureCorrectionCommand, 300);
  Serial.println(sendI2C(PH_I2C_ADDRESS, "R", 900));
}

void setPumpMultiple(char* pump, char* movements) {
  Serial.println("OK");
  char* movement = strtok_r(movements, ",", &movements);
  while (movement != 0) {
    char* pumpState = strtok_r(movement, ":", &movement);
    char* nextDelay = strtok_r(0, ":", &movement);
    setPump(atoi(pump), pumpState);
    if (nextDelay != 0) {
      delay(atoi(nextDelay));
    }
    movement = strtok_r(0, ",", &movements);
  }
}

void setPump(byte pump, char* state) {
  if (strcmp(state, "0") == 0) {
    turnPumpOff(pump);
  } else if (strcmp(state, "1") == 0) {
    turnPumpForwards(pump);
  } else if (strcmp(state, "2") == 0) {
    turnPumpBackwards(pump);
  } else {
    Serial.print("ERROR");
  }
}

void turnPumpOff(int pump) {
  digitalWrite(PUMPENABLE[pump], LOW);
  digitalWrite(PUMPPIN1[pump], LOW);
  digitalWrite(PUMPPIN2[pump], LOW);
}

void turnPumpForwards(int pump) {
  digitalWrite(PUMPPIN2[pump], LOW);
  digitalWrite(PUMPPIN1[pump], HIGH);
  digitalWrite(PUMPENABLE[pump], HIGH);
}

void turnPumpBackwards(int pump) {
  digitalWrite(PUMPPIN1[pump], LOW);
  digitalWrite(PUMPPIN2[pump], HIGH);
  digitalWrite(PUMPENABLE[pump], HIGH);
}

char* sendI2C(int address, char* data, int readDelay) {
  Wire.beginTransmission(address);
  Wire.write(data);
  Wire.endTransmission();
  delay(readDelay);
  Wire.requestFrom(address, 20, 1);
  byte code = Wire.read();
  if (code != 1) {
    Wire.endTransmission();
    return "ERROR";
  }
  char response[20];
  byte i = 0;
  while (Wire.available()) {
    char in_char = Wire.read();
    response[i] = in_char;
    i++;
    if (in_char == 0) {
      Wire.endTransmission();
      break;
    }
  }
  return response;
}
