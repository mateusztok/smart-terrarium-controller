#pragma once

#include "database.h"

#include <pistache/router.h>

#include <functional>
#include <string>

namespace terrarium {

class MeasurementHandler {
public:
  explicit MeasurementHandler(Database &database);
  void setupRoutes(Pistache::Rest::Router &router);

private:
  void handleAddMeasurement(const Pistache::Rest::Request &request,
                            Pistache::Http::ResponseWriter response);
  void handleTemperature(const Pistache::Rest::Request &request,
                         Pistache::Http::ResponseWriter response);
  void handleHumidity(const Pistache::Rest::Request &request,
                      Pistache::Http::ResponseWriter response);
  void handleLight(const Pistache::Rest::Request &request,
                   Pistache::Http::ResponseWriter response);
  void handleRecentMeasurements(const Pistache::Rest::Request &request,
                                Pistache::Http::ResponseWriter response);
  void handleGetSetpoints(const Pistache::Rest::Request &request,
                          Pistache::Http::ResponseWriter response);
  void handleGetOutputData(const Pistache::Rest::Request &request,
                           Pistache::Http::ResponseWriter response);
  void handleSetPoints(const Pistache::Rest::Request &request,
                       Pistache::Http::ResponseWriter response);
  void handleOutput(const Pistache::Rest::Request &request,
                    Pistache::Http::ResponseWriter response);

  static void send(Pistache::Http::ResponseWriter &response,
                   Pistache::Http::Code code, const std::string &body);
  static void sendDatabaseResponse(Pistache::Http::ResponseWriter &response,
                                   const std::function<std::string()> &query);

  Database &database_;
};

} // namespace terrarium
