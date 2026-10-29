#include "measurement_handler.h"

#include "form_data.h"
#include "models.h"

#include <array>
#include <cmath>
#include <iostream>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace terrarium {
namespace {

void addCorsHeaders(Pistache::Http::ResponseWriter &response) {
  response.headers().add<Pistache::Http::Header::AccessControlAllowOrigin>("*");
  response.headers().add<Pistache::Http::Header::AccessControlAllowMethods>(
      "GET, POST, OPTIONS");
  response.headers().add<Pistache::Http::Header::AccessControlAllowHeaders>(
      "Content-Type");
}

} // namespace

MeasurementHandler::MeasurementHandler(Database &database)
    : database_(database) {}

void MeasurementHandler::setupRoutes(Pistache::Rest::Router &router) {
  using namespace Pistache::Rest;
  Routes::Post(router, "/add_measurement",
               Routes::bind(&MeasurementHandler::handleAddMeasurement, this));
  Routes::Get(router, "/temperature",
              Routes::bind(&MeasurementHandler::handleTemperature, this));
  Routes::Get(router, "/humidity",
              Routes::bind(&MeasurementHandler::handleHumidity, this));
  Routes::Get(router, "/light",
              Routes::bind(&MeasurementHandler::handleLight, this));
  Routes::Get(
      router, "/recentMeasurements",
      Routes::bind(&MeasurementHandler::handleRecentMeasurements, this));
  Routes::Get(router, "/getSetpoints",
              Routes::bind(&MeasurementHandler::handleGetSetpoints, this));
  Routes::Get(router, "/getOutput",
              Routes::bind(&MeasurementHandler::handleGetOutputData, this));
  Routes::Post(router, "/setSetpoints",
               Routes::bind(&MeasurementHandler::handleSetPoints, this));
  Routes::Post(router, "/add_output",
               Routes::bind(&MeasurementHandler::handleOutput, this));
}

void MeasurementHandler::handleAddMeasurement(
    const Pistache::Rest::Request &request,
    Pistache::Http::ResponseWriter response) {
  if (request.method() != Pistache::Http::Method::Post) {
    send(response, Pistache::Http::Code::Method_Not_Allowed,
         "HTTP method not allowed.");
    return;
  }

  const std::string body = request.body();
  float temperature;
  float humidity;
  float light;
  try {
    temperature = parseNumericValue(getFormValue(body, "temperature"));
    humidity = parseNumericValue(getFormValue(body, "humidity"));
    light = parseNumericValue(getFormValue(body, "light"));
  } catch (const std::exception &error) {
    std::cerr << "Invalid measurement data: " << error.what() << std::endl;
    send(response, Pistache::Http::Code::Bad_Request,
         "Invalid measurement data.");
    return;
  }

  try {
    database_.saveMeasurements(std::round(temperature * 100.0f) / 100.0f,
                               std::round(humidity), std::round(light));
    send(response, Pistache::Http::Code::Ok, "Measurements saved.\n");
  } catch (const std::exception &error) {
    std::cerr << "Failed to save measurements: " << error.what() << std::endl;
    send(response, Pistache::Http::Code::Internal_Server_Error,
         "Internal server error.");
  }
}

void MeasurementHandler::handleTemperature(
    const Pistache::Rest::Request &, Pistache::Http::ResponseWriter response) {
  sendDatabaseResponse(
      response, [&] { return database_.getMeasurements("temperature"); });
}

void MeasurementHandler::handleHumidity(
    const Pistache::Rest::Request &, Pistache::Http::ResponseWriter response) {
  sendDatabaseResponse(response,
                       [&] { return database_.getMeasurements("humidity"); });
}

void MeasurementHandler::handleLight(const Pistache::Rest::Request &,
                                     Pistache::Http::ResponseWriter response) {
  sendDatabaseResponse(response,
                       [&] { return database_.getMeasurements("light"); });
}

void MeasurementHandler::handleRecentMeasurements(
    const Pistache::Rest::Request &, Pistache::Http::ResponseWriter response) {
  sendDatabaseResponse(response,
                       [&] { return database_.getRecentMeasurements(); });
}

void MeasurementHandler::handleGetSetpoints(
    const Pistache::Rest::Request &, Pistache::Http::ResponseWriter response) {
  sendDatabaseResponse(response, [&] { return database_.getSetpoints(); });
}

void MeasurementHandler::handleGetOutputData(
    const Pistache::Rest::Request &, Pistache::Http::ResponseWriter response) {
  sendDatabaseResponse(response, [&] { return database_.getOutput(); });
}

void MeasurementHandler::handleSetPoints(
    const Pistache::Rest::Request &request,
    Pistache::Http::ResponseWriter response) {
  if (request.method() != Pistache::Http::Method::Post) {
    send(response, Pistache::Http::Code::Method_Not_Allowed,
         "HTTP method not allowed.");
    return;
  }

  try {
    const std::string body = request.body();
    std::vector<std::pair<SetpointField, float>> values;
    std::string message = "Setpoints saved:\n";

    for (const auto &field : kSetpointFields) {
      const std::string value = getFormValue(body, field.name);
      if (value.empty()) {
        continue;
      }
      values.emplace_back(field, parseNumericValue(value));
      message +=
          std::string(field.name) + ": " + value + " " + field.unit + "\n";
    }

    if (values.empty()) {
      send(response, Pistache::Http::Code::Bad_Request,
           "At least one setpoint is required.");
      return;
    }

    database_.saveSetpoints(values);
    send(response, Pistache::Http::Code::Ok, message);
  } catch (const std::invalid_argument &error) {
    std::cerr << "Invalid setpoint value: " << error.what() << std::endl;
    send(response, Pistache::Http::Code::Bad_Request,
         "Invalid setpoint value.");
  } catch (const std::exception &error) {
    std::cerr << "Failed to update setpoints: " << error.what() << std::endl;
    send(response, Pistache::Http::Code::Internal_Server_Error,
         "Internal server error.");
  }
}

void MeasurementHandler::handleOutput(const Pistache::Rest::Request &request,
                                      Pistache::Http::ResponseWriter response) {
  if (request.method() != Pistache::Http::Method::Post) {
    send(response, Pistache::Http::Code::Method_Not_Allowed,
         "HTTP method not allowed.");
    return;
  }

  float value;
  try {
    value = parseNumericValue(getFormValue(request.body(), "output"));
  } catch (const std::exception &error) {
    std::cerr << "Invalid output value: " << error.what() << std::endl;
    send(response, Pistache::Http::Code::Bad_Request, "Invalid output value.");
    return;
  }

  try {
    database_.saveOutput(value);
    send(response, Pistache::Http::Code::Ok, "Output value saved.");
  } catch (const std::exception &error) {
    std::cerr << "Failed to save output value: " << error.what() << std::endl;
    send(response, Pistache::Http::Code::Internal_Server_Error,
         "Internal server error.");
  }
}

void MeasurementHandler::send(Pistache::Http::ResponseWriter &response,
                              Pistache::Http::Code code,
                              const std::string &body) {
  addCorsHeaders(response);
  response.send(code, body, MIME(Text, Plain));
}

void MeasurementHandler::sendDatabaseResponse(
    Pistache::Http::ResponseWriter &response,
    const std::function<std::string()> &query) {
  try {
    send(response, Pistache::Http::Code::Ok, query());
  } catch (const std::exception &error) {
    std::cerr << "Database request failed: " << error.what() << std::endl;
    send(response, Pistache::Http::Code::Internal_Server_Error,
         "Internal server error.");
  }
}

} // namespace terrarium
