#include <Arduino.h>
#include <string.h>
#include <OneWire.h>;
#include <DallasTemperature.h>;

// General
#define SERIAL_BAUD 115200
#define MAX_INPUT_LENGTH 255
#define ANALOG_READ_MAX 4095

// Pins
const int PUMPENABLE[] = {12, 25};
const int PUMPPIN1[] = {13, 32};
const int PUMPPIN2[] = {14, 33};
#define ONE_WIRE_BUS 26
#define TURBIDITY_SENSOR 27
#define PH_SENSOR 34

// Variables
char input[MAX_INPUT_LENGTH];
OneWire oneWire(ONE_WIRE_BUS);
DallasTemperature sensors(&oneWire);
double phAcidOffset = -1;
double phAcidOffsetPhValue = -1;
double phNeutralOffset = -1;
double phNeutralOffsetPhValue = -1;
double phBaseOffset = -1;
double phBaseOffsetPhValue = -1;

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
      getTemperature();
    }
    else if (strcmp(command, "getPhOffset") == 0) {
      getPhOffset();
    }
    else if (strcmp(command, "setPhOffset") == 0) {
      char* type = strtok(0, " ");
      char* actualValue = strtok(0, " ");
      setPhOffset(type, actualValue);
    }
    else if (strcmp(command, "getPh") == 0) {
      getPh();
    }
    else if (strcmp(command, "getTurb") == 0) {
      getTurb();
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

void getTemperature() {
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

void setPhOffset(char* type, char* charPhValue) {
  int sensor = analogRead(PH_SENSOR);
  double phValue = atof(charPhValue);
  if (strcmp(type, "acid") == 0) {
    phAcidOffset = sensor;
    phAcidOffsetPhValue = phValue;
    Serial.print("OK");
  }
  else if (strcmp(type, "neutral") == 0) {
    phNeutralOffset = sensor;
    phNeutralOffsetPhValue = phValue;
    Serial.print("OK");
  }
  else if (strcmp(type, "base") == 0) {
    phBaseOffset = sensor;
    phBaseOffsetPhValue = phValue;
    Serial.print("OK");
  }
  else {
    Serial.print("Wrong argument for ph offset type: ");
    Serial.println(type);
  }
}

void getPh() {
  if (phAcidOffset == -1 || phNeutralOffset == -1 || phBaseOffset == -1) {
    Serial.println("Error while reading ph.");
  }
  else {
    double sensor = ANALOG_READ_MAX - analogRead(PH_SENSOR);
    double ph;
    if (sensor < phNeutralOffset) { // check this
      ph = mapDouble(sensor, phAcidOffset, phNeutralOffset, phAcidOffsetPhValue, phNeutralOffsetPhValue);
    }
    else {
      ph = mapDouble(sensor, phNeutralOffset, phBaseOffset, phNeutralOffsetPhValue, phBaseOffsetPhValue);
    }
    Serial.println(ph); 
  }
}

void getTurb() {
  double sensor = analogRead(TURBIDITY_SENSOR);
  Serial.println(sensor / ANALOG_READ_MAX * 100);
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

