#include "sensors.h"

#include <Arduino.h>
#include <Wire.h>

namespace {

constexpr uint8_t kBh1750Address = 0x23;
constexpr uint8_t kBh1750ContinuousHighResolutionMode = 0x10;
constexpr uint8_t kAht10Address = 0x38;
constexpr uint8_t kAht10InitializeCommand = 0xE1;
constexpr uint8_t kAht10TriggerMeasurementCommand = 0xAC;
constexpr uint8_t kAht10SoftResetCommand = 0xBA;

bool initializeAht10() {
  Wire.beginTransmission(kAht10Address);
  Wire.write(kAht10SoftResetCommand);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  delay(20);

  Wire.beginTransmission(kAht10Address);
  Wire.write(kAht10InitializeCommand);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  delay(50);

  if (Wire.requestFrom(kAht10Address, static_cast<uint8_t>(1)) != 1) {
    return false;
  }

  const uint8_t status = Wire.read();
  return (status & 0x68) == 0x08;
}

bool initializeBh1750() {
  Wire.beginTransmission(kBh1750Address);
  Wire.write(kBh1750ContinuousHighResolutionMode);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  delay(200);
  return true;
}

bool readAht10(Measurements &measurements) {
  Wire.beginTransmission(kAht10Address);
  Wire.write(kAht10TriggerMeasurementCommand);
  Wire.write(0x33);
  Wire.write(0x00);
  if (Wire.endTransmission() != 0) {
    return false;
  }
  delay(80);

  if (Wire.requestFrom(kAht10Address, static_cast<uint8_t>(6)) != 6) {
    return false;
  }

  uint8_t data[6];
  for (uint8_t &byte : data) {
    byte = Wire.read();
  }

  const uint32_t humidityRaw = (static_cast<uint32_t>(data[1]) << 12) |
                               (static_cast<uint32_t>(data[2]) << 4) |
                               (data[3] >> 4);
  const uint32_t temperatureRaw =
      (static_cast<uint32_t>(data[3] & 0x0F) << 16) |
      (static_cast<uint32_t>(data[4]) << 8) | data[5];

  measurements.humidity = static_cast<float>(humidityRaw) * 100.0f / 1048576.0f;
  measurements.temperature =
      static_cast<float>(temperatureRaw) * 200.0f / 1048576.0f - 50.0f;
  return true;
}

bool readBh1750(float &light) {
  if (Wire.requestFrom(kBh1750Address, static_cast<uint8_t>(2)) != 2) {
    return false;
  }

  const uint16_t raw = (static_cast<uint16_t>(Wire.read()) << 8) | Wire.read();
  light = static_cast<float>(raw) / 1.2f;
  return true;
}

} // namespace

namespace sensors {

void setup() {
  if (!initializeAht10()) {
    Serial.println("Failed to initialize the AHT10 sensor.");
  }
  if (!initializeBh1750()) {
    Serial.println("Failed to initialize the BH1750 sensor.");
  }
}

bool readMeasurements(Measurements &measurements) {
  Measurements current = measurements;
  const bool airSensorAvailable = readAht10(current);
  const bool lightSensorAvailable = readBh1750(current.light);

  if (!airSensorAvailable) {
    Serial.println("Failed to read the AHT10 sensor.");
  }
  if (!lightSensorAvailable) {
    Serial.println("Failed to read the BH1750 sensor.");
  }

  if (!airSensorAvailable || !lightSensorAvailable) {
    return false;
  }

  measurements = current;
  return true;
}

} // namespace sensors
