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
#define SECONDARY_PSU_STATUS_READ_THRESHOLD 512

// Pins
const byte PUMP_ENABLE[] = {5, 6, 3};
const byte PUMP_PIN1[] = {8, 10, 12};
const byte PUMP_PIN2[] = {9, 11, 13};
#define ONE_WIRE_BUS 2
#define SECONDARY_PSU_STATUS_PIN A0

// Variables
char input[MAX_INPUT_LENGTH];
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
// DEFAULT TEMPERATURE FOR PH CALIBRATION
float temperature = 25;

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PUMP_ENABLE[0], OUTPUT);
  digitalWrite(PUMP_ENABLE[0], LOW);
  pinMode(PUMP_ENABLE[1], OUTPUT);
  digitalWrite(PUMP_ENABLE[1], LOW);
  pinMode(PUMP_PIN1[0], OUTPUT);
  digitalWrite(PUMP_PIN1[0], LOW);
  pinMode(PUMP_PIN1[1], OUTPUT);
  digitalWrite(PUMP_PIN1[1], LOW);
  pinMode(PUMP_PIN2[0], OUTPUT);
  digitalWrite(PUMP_PIN2[0], LOW);
  pinMode(PUMP_PIN2[1], OUTPUT);
  digitalWrite(PUMP_PIN2[1], LOW);
  Wire.begin();
  // AVOID WRONG READS AND INITIALIZE TEMPERATURE FOR PH CALIBRATION
  getTemp();
  getTemp();
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
      Serial.println(getTemp());
    }
    else if (strcmp(command, "calibratePh") == 0) {
      char* type = strtok(0, " ");
      Serial.println(calibratePh(type));
    }
    else if (strcmp(command, "getPh") == 0) {
      Serial.println(getPh());
    }
    else if (strcmp(command, "getStatus") == 0) {
      Serial.println(getStatus());
    }
    else if (strcmp(command, "setPump") == 0) {
      char* pump = strtok(0, " ");
      char* movements = strtok(0, " ");
      Serial.println(setPumpMultiple(pump, movements));
    }
    else {
      Serial.println("Unrecognized Command");
    }
  }
}

char* getTemp() {
  sensors.requestTemperatures();
  float readValue = sensors.getTempCByIndex(0);
  if (readValue == -127.00) {
    return "ERROR";
  } else {
    temperature = readValue;
    char temperatureChar[10];
    sprintf(temperatureChar, "%f", readValue);
    return temperatureChar;
  }
}

char* calibratePh(char* type) {
  if (strcmp(type, "acid") == 0) {
    sendI2C(PH_I2C_ADDRESS, "Cal,low,4.00", 900);
    return "OK";
  } else if (strcmp(type, "neutral") == 0) {
    sendI2C(PH_I2C_ADDRESS, "Cal,mid,7.00", 900);
    return "OK";
  } else if (strcmp(type, "base") == 0) {
    sendI2C(PH_I2C_ADDRESS, "Cal,high,10.00", 900);
    return "OK";
  } else {
    return "ERROR";
  }
}

char* getPh() {
  char temperatureChar[10];
  dtostrf(temperature, 3, 2, temperatureChar);
  char temperatureCorrectionCommand[20] = "";
  sprintf(temperatureCorrectionCommand, "T,%s", temperatureChar);
  sendI2C(PH_I2C_ADDRESS, temperatureCorrectionCommand, 300);
  char* readValue = sendI2C(PH_I2C_ADDRESS, "R", 900);
  if (strcmp(readValue, "0.00") == 0) {
    return "ERROR";
  } else {
    return readValue;
  }
}

char* getStatus() {
  char* phStatus = getPhStatus();
  char* tempStatus = getTempStatus();
  char* secondaryPSUStatus = getSecondaryPSUStatus();
  char statusChar[100];
  strcpy(statusChar, "SecondaryPSU:");
  strcat(statusChar, secondaryPSUStatus);
  strcat(statusChar, ",PHSENSOR:");
  strcat(statusChar, phStatus);
  strcat(statusChar, ",TEMPSENSOR:");
  strcat(statusChar, tempStatus);
  return statusChar;
}

char* getSecondaryPSUStatus() {
  int value = analogRead(SECONDARY_PSU_STATUS_PIN);
  if (value < SECONDARY_PSU_STATUS_READ_THRESHOLD) {
    return "OFF";
  } else {
    return "ON";
  }
}

char* getPhStatus() {
  char* ph = getPh();
  if (strcmp(ph, "ERROR") == 0) {
    return "OFF";
  } else {
    return "ON";
  }
}

char* getTempStatus() {
  char* temp = getTemp();
  if (strcmp(temp, "ERROR") == 0) {
    return "OFF";
  } else {
    return "ON";
  }
}

char* setPumpMultiple(char* pump, char* movements) {
  char* secondaryPSUStatus = getSecondaryPSUStatus();
  if (strcmp(secondaryPSUStatus, "OFF") == 0) {
    return "ERROR";
  } else {
    return "OK";
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
}

char* setPump(byte pump, char* state) {
  if (strcmp(state, "0") == 0) {
    turnPumpOff(pump);
  } else if (strcmp(state, "1") == 0) {
    turnPumpForwards(pump);
  } else if (strcmp(state, "2") == 0) {
    turnPumpBackwards(pump);
  }
}

void turnPumpOff(int pump) {
  digitalWrite(PUMP_ENABLE[pump], LOW);
  digitalWrite(PUMP_PIN1[pump], LOW);
  digitalWrite(PUMP_PIN2[pump], LOW);
}

void turnPumpForwards(int pump) {
  digitalWrite(PUMP_PIN2[pump], LOW);
  digitalWrite(PUMP_PIN1[pump], HIGH);
  digitalWrite(PUMP_ENABLE[pump], HIGH);
}

void turnPumpBackwards(int pump) {
  digitalWrite(PUMP_PIN1[pump], LOW);
  digitalWrite(PUMP_PIN2[pump], HIGH);
  digitalWrite(PUMP_ENABLE[pump], HIGH);
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
