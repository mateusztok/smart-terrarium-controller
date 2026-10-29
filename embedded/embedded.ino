#include "controllers.h"
#include "network.h"
#include "sensors.h"

#include <Arduino.h>
#include <Wire.h>

namespace {

constexpr uint8_t kMeasurementsPerUpload = 20;
constexpr uint32_t kMeasurementIntervalMs = 100;

Measurements measurements;
Setpoints setpoints;

void printMeasurements(const Measurements &current) {
  Serial.print("Temperature: ");
  Serial.print(current.temperature);
  Serial.print(" C, humidity: ");
  Serial.print(current.humidity);
  Serial.print(" %, light: ");
  Serial.print(current.light);
  Serial.println(" lux");
}

} // namespace

void setup() {
  Serial.begin(115200);
  Wire.begin();
  controllers::setup();
  sensors::setup();
  network::connectToWifi();
}

void loop() {
  if (!network::fetchSetpoints(setpoints)) {
    Serial.println("Failed to retrieve setpoints. Keeping previous values.");
  }

  bool hasValidMeasurement = false;
  for (uint8_t i = 0; i < kMeasurementsPerUpload; ++i) {
    if (sensors::readMeasurements(measurements)) {
      controllers::update(measurements, setpoints);
      printMeasurements(measurements);
      hasValidMeasurement = true;
    }
    delay(kMeasurementIntervalMs);
  }

  if (hasValidMeasurement) {
    network::sendMeasurements(measurements);
    network::sendOutput(controllers::temperatureOutput());
  }
}
