#include "controllers.h"

#include <Arduino.h>
#include <PID_v1.h>

#include <cmath>

namespace {

constexpr uint8_t kLightPin = 13;
constexpr uint8_t kTemperaturePin = 12;
constexpr uint8_t kHumidityPin = 27;
constexpr float kLightDeadZone = 5.0f;
constexpr float kHumidityDeadZone = 1.0f;
constexpr float kHumidityZone = 10.0f;
constexpr unsigned long kHumidityDelayMs = 5000;
constexpr double kTemperatureKp = 31.15;
constexpr double kTemperatureTi = 0.3;
constexpr double kTemperatureTd = 0.0;

float lightPower = 0;
bool humidityIsOn = false;
unsigned long humidityStartTime = 0;
double temperatureInput = 0;
double temperatureOutputValue = 0;
double temperatureSetpoint = 0;
PID temperaturePid(&temperatureInput, &temperatureOutputValue,
                   &temperatureSetpoint, kTemperatureKp, kTemperatureTi,
                   kTemperatureTd, DIRECT);

void updateLight(float measurement, float setpoint) {
  const float difference = setpoint - measurement;
  if (difference > kLightDeadZone) {
    lightPower += difference > 20.0f ? 5.0f : 1.0f;
  } else if (difference < -kLightDeadZone) {
    lightPower -= std::fabs(difference) > 20.0f ? 5.0f : 1.0f;
  } else {
    return;
  }

  lightPower = constrain(lightPower, 0.0f, 255.0f);
  analogWrite(kLightPin, static_cast<int>(lightPower));

  Serial.print("Light power: ");
  Serial.print(lightPower);
  Serial.print(" Light setpoint: ");
  Serial.println(setpoint);
}

void updateTemperature(float measurement, float setpoint) {
  temperatureInput = measurement;
  temperatureSetpoint = setpoint;
  temperaturePid.Compute();
  analogWrite(kTemperaturePin, static_cast<int>(temperatureOutputValue));

  Serial.print("Temperature power (PWM): ");
  Serial.print(temperatureOutputValue);
  Serial.print(" Temperature setpoint: ");
  Serial.println(setpoint);
}

void updateHumidity(float measurement, float setpoint) {
  const unsigned long currentTime = millis();
  if (measurement < setpoint - kHumidityZone) {
    digitalWrite(kHumidityPin, LOW);
    humidityIsOn = true;
    Serial.println("Humidity ON");
  } else if (measurement > setpoint - kHumidityZone &&
             measurement < setpoint - kHumidityDeadZone) {
    if (currentTime - humidityStartTime > kHumidityDelayMs) {
      digitalWrite(kHumidityPin, humidityIsOn ? HIGH : LOW);
      humidityStartTime = currentTime;
      humidityIsOn = !humidityIsOn;
    }
  } else if (measurement > setpoint - kHumidityDeadZone) {
    digitalWrite(kHumidityPin, HIGH);
    humidityIsOn = false;
    Serial.print("Humidity OFF, setpoint: ");
    Serial.println(setpoint);
  }
}

} // namespace

namespace controllers {

void setup() {
  pinMode(kHumidityPin, OUTPUT);
  digitalWrite(kHumidityPin, HIGH);
  Serial.println("Humidity setup complete");

  pinMode(kLightPin, OUTPUT);
  analogWrite(kLightPin, 0);
  Serial.println("Light setup complete");

  pinMode(kTemperaturePin, OUTPUT);
  analogWrite(kTemperaturePin, 0);
  temperaturePid.SetMode(AUTOMATIC);
  temperaturePid.SetOutputLimits(0, 255);
  Serial.println("Temperature setup complete");
}

void update(const Measurements &measurements, const Setpoints &setpoints) {
  updateTemperature(measurements.temperature, setpoints.temperature);
  updateHumidity(measurements.humidity, setpoints.humidity);
  updateLight(measurements.light, setpoints.light);
}

float temperatureOutput() {
  return static_cast<float>(temperatureOutputValue);
}

} // namespace controllers
