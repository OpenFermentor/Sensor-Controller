#include <Arduino.h>
#include <string.h>
#include <OneWire.h>;
#include <DallasTemperature.h>;

// General
#define SERIAL_BAUD 115200
#define MAX_INPUT_LENGTH 255
#define ANALOG_READ_MAX 1023

// Pins
const int PUMPENABLE[] = {8, 9};
const int PUMPPIN1[] = {10, 11};
const int PUMPPIN2[] = {12, 13};
#define ONE_WIRE_BUS 2
#define TURBIDITY_SENSOR A2
#define PH_SENSOR A1

// Variables
char input[MAX_INPUT_LENGTH];
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
double phAcidOffset = 627;
int phAcidOffsetPhValue = 4;
double phNeutralOffset = 512;
int phNeutralOffsetPhValue = 7;
double phBaseOffset = 418;
int phBaseOffsetPhValue = 10;

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
  pinMode(TURBIDITY_SENSOR, INPUT);
  pinMode(PH_SENSOR, INPUT);
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
    else if (strcmp(command, "getPhOffset") == 0) {
      getPhOffset();
    }
    else if (strcmp(command, "setPhOffset") == 0) {
      char* type = strtok(0, " ");
      char* phValue = strtok(0, " ");
      char* offset = strtok(0, " ");
      setPhOffset(type, phValue, offset);
    }
    else if (strcmp(command, "getPh") == 0) {
      getPh();
    }
    else if (strcmp(command, "getTurb") == 0) {
      Serial.println(0.00);
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
  double temp = sensors.getTempCByIndex(0);
  if (temp == -127.00) {
    Serial.println("Error while reading temperature.");
  } else {
    Serial.println(temp);
  }
}

void getPhOffset() {
  int sensor = analogRead(PH_SENSOR);
  Serial.println(sensor);
}

void setPhOffset(char* type, char* charPhValue, char* charOffset) {
  double phValue = atof(charPhValue);
  int offset = atof(charOffset);
  if (strcmp(type, "acid") == 0) {
    phAcidOffset = offset;
    phAcidOffsetPhValue = phValue;
    Serial.println("OK");
  }
  else if (strcmp(type, "neutral") == 0) {
    phNeutralOffset = offset;
    phNeutralOffsetPhValue = phValue;
    Serial.println("OK");
  }
  else if (strcmp(type, "base") == 0) {
    phBaseOffset = offset;
    phBaseOffsetPhValue = phValue;
    Serial.println("OK");
  }
  else {
    Serial.print("Wrong argument for ph offset type: ");
    Serial.println(type);
  }
}

void getPh() {
  int sensor = analogRead(PH_SENSOR);
  double ph;
  if (sensor < phNeutralOffset) {
    ph = mapDouble(sensor, phBaseOffset, phNeutralOffset, phBaseOffsetPhValue, phNeutralOffsetPhValue);
  }
  else {
    ph = mapDouble(sensor, phNeutralOffset, phAcidOffset, phNeutralOffsetPhValue, phAcidOffsetPhValue);
  }
  Serial.println(ph);
}

void getTurb() {
  double voltage = (double)analogRead(A0) * 5 / ANALOG_READ_MAX;
  double NTP;
  if (voltage < 2.56) {
    NTP = 3000;
  } else if (voltage > 4.2) {
    NTP = 0;
  } else {
    NTP = -1120.4*voltage*voltage + 5742.3*voltage - 4352.9;
  }
  Serial.println(voltage);
}

void setPumpMultiple(char* pump, char* movements) {
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
  Serial.println("OK");
}

void setPump(int pump, char* state) {
  if (strcmp(state, "0") == 0) {
    turnPumpOff(pump);
  } else if (strcmp(state, "1") == 0) {
    turnPumpForwards(pump);
  } else if (strcmp(state, "2") == 0) {
    turnPumpBackwards(pump);
  } else {
    Serial.print("Wrong argument for pump state: ");
    Serial.println(state);
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

double mapDouble(double x, double in_min, double in_max, double out_min, double out_max) {
  return (x - in_min) * (out_max - out_min) / (in_max - in_min) + out_min;
}
