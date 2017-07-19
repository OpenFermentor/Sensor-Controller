#include <Arduino.h>
#include <string.h>
#include <OneWire.h>;
#include <DallasTemperature.h>;

// General
#define SERIAL_BAUD 9600
#define MAX_INPUT_LENGTH 255

// Pins
#define PUMPENABLE 21
#define PUMPPIN1 22
#define PUMPPIN2 23
#define ONE_WIRE_BUS 5

// Variables
char input[MAX_INPUT_LENGTH];
OneWire oneWire(ONE_WIRE_BUS); 
DallasTemperature sensors(&oneWire);

void setup() {
  Serial.begin(SERIAL_BAUD);
  pinMode(PUMPENABLE, OUTPUT);
  digitalWrite(PUMPENABLE, LOW);
  pinMode(PUMPPIN1, OUTPUT);
  digitalWrite(PUMPPIN1, LOW);
  pinMode(PUMPPIN2, OUTPUT);
  digitalWrite(PUMPPIN2, LOW);
}

void loop() {
  if (Serial.available() > 0) {
    byte terminatorAt = Serial.readBytes(input, MAX_INPUT_LENGTH);
    while (input[terminatorAt-1] == '\r' || input[terminatorAt-1] == '\n') {
      terminatorAt--;
    }
    input[terminatorAt] = 0;
    char* command = strtok(input, " ");
    char* parameter = strtok(0, " ");
    if (strcmp(command, "getTemp") == 0) {
      getTemperature();
    }
    else if (strcmp(command, "getPH") == 0) {
      Serial.println("Not yet implemented");
    }
    else if (strcmp(command, "setPump") == 0) {
      setPumpMultiple(parameter);
    }
    else {
      Serial.println("Unrecognized Command");
    }
  }
}

void setPumpMultiple(char* parameter) {
  char* movement = strtok_r(parameter, ",", &parameter);
  while (movement != 0) {
    char* pumpState = strtok_r(movement, ":", &movement);
    char* nextDelay = strtok_r(0, ":", &movement);
    setPump(pumpState);
    if (nextDelay != 0) {
      delay(atoi(nextDelay));
    }
    movement = strtok_r(0, ",", &parameter);
  }
  Serial.println("OK");
}

void setPump(char* state) {
  if (strcmp(state, "0") == 0) {
    turnOffPump();
  } else if (strcmp(state, "1") == 0) {
    turnPumpForwards();
  } else if (strcmp(state, "2") == 0) {
    turnPumpBackwards();
  } else {
    Serial.print("Wrong argument for pump state: ");
    Serial.println(state);
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

void turnOffPump() {
  digitalWrite(PUMPENABLE, LOW);
  digitalWrite(PUMPPIN1, LOW);
  digitalWrite(PUMPPIN2, LOW);
}

void turnPumpForwards() {
  digitalWrite(PUMPPIN2, LOW);
  digitalWrite(PUMPPIN1, HIGH);
  digitalWrite(PUMPENABLE, HIGH);
}

void turnPumpBackwards() {
  digitalWrite(PUMPPIN1, LOW);
  digitalWrite(PUMPPIN2, HIGH);
  digitalWrite(PUMPENABLE, HIGH);
}

