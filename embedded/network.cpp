#include "network.h"

#include "secrets.h"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>

#include <cmath>
#include <cstdlib>

namespace {

constexpr uint32_t kWifiTimeoutMs = 20000;

bool parseSetpoint(const String &payload, const char *name, float &value) {
  const String prefix = String(name) + ":";
  const int start = payload.indexOf(prefix);
  if (start < 0) {
    return false;
  }

  const int valueStart = start + prefix.length();
  int valueEnd = payload.indexOf('\n', valueStart);
  if (valueEnd < 0) {
    valueEnd = payload.length();
  }

  String number = payload.substring(valueStart, valueEnd);
  number.trim();
  if (number.isEmpty()) {
    return false;
  }

  char *end = nullptr;
  const float parsed = strtof(number.c_str(), &end);
  if (end == number.c_str() || !isfinite(parsed)) {
    return false;
  }

  value = parsed;
  return true;
}

bool postForm(const char *path, const String &body) {
  if (!network::connectToWifi()) {
    return false;
  }

  HTTPClient http;
  const String url = String(backendBaseUrl) + path;
  if (!http.begin(url)) {
    Serial.println("Failed to initialize the POST request.");
    return false;
  }

  http.addHeader("Content-Type", "application/x-www-form-urlencoded");
  const int responseCode = http.POST(body);
  http.end();

  if (responseCode < 200 || responseCode >= 300) {
    Serial.print("POST request failed with HTTP status ");
    Serial.println(responseCode);
    return false;
  }
  return true;
}

} // namespace

namespace network {

bool connectToWifi() {
  if (WiFi.status() == WL_CONNECTED) {
    return true;
  }

  WiFi.begin(ssid, password);
  const uint32_t startedAt = millis();
  while (WiFi.status() != WL_CONNECTED &&
         millis() - startedAt < kWifiTimeoutMs) {
    delay(250);
  }

  if (WiFi.status() != WL_CONNECTED) {
    Serial.println("Failed to connect to Wi-Fi.");
    return false;
  }

  Serial.println("Connected to Wi-Fi.");
  Serial.print("Device IP: ");
  Serial.println(WiFi.localIP());
  return true;
}

bool fetchSetpoints(Setpoints &setpoints) {
  if (!connectToWifi()) {
    return false;
  }

  HTTPClient http;
  const String url = String(backendBaseUrl) + "/getSetpoints";
  if (!http.begin(url)) {
    Serial.println("Failed to initialize the setpoint request.");
    return false;
  }

  const int responseCode = http.GET();
  if (responseCode < 200 || responseCode >= 300) {
    Serial.print("Setpoint request failed with HTTP status ");
    Serial.println(responseCode);
    http.end();
    return false;
  }

  const String payload = http.getString();
  http.end();

  Setpoints parsed = setpoints;
  if (!parseSetpoint(payload, "temperature", parsed.temperature) ||
      !parseSetpoint(payload, "humidity", parsed.humidity) ||
      !parseSetpoint(payload, "light", parsed.light)) {
    Serial.println("The backend returned incomplete setpoints.");
    return false;
  }

  setpoints = parsed;
  return true;
}

bool sendMeasurements(const Measurements &measurements) {
  const String body = "temperature=" + String(measurements.temperature, 2) +
                      "&humidity=" + String(measurements.humidity, 2) +
                      "&light=" + String(measurements.light, 2);
  return postForm("/add_measurement", body);
}

bool sendOutput(float output) {
  const String body = "output=" + String(output, 2);
  return postForm("/add_output", body);
}

} // namespace network
