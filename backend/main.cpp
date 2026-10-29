#include "database.h"
#include "measurement_handler.h"

#include <pistache/endpoint.h>
#include <pistache/router.h>

#include <cstdlib>
#include <iostream>
#include <memory>
#include <stdexcept>

namespace {

constexpr int kDefaultPort = 9080;
constexpr int kWorkerThreads = 2;

} // namespace

int main() {
  const char *configuredPath = std::getenv("TERRARIUM_DB_PATH");
  const char *databasePath = configuredPath ? configuredPath : "sensor_data.db";
  const char *configuredPort = std::getenv("TERRARIUM_PORT");

  try {
    const int port = configuredPort ? std::stoi(configuredPort) : kDefaultPort;
    if (port < 1 || port > 65535) {
      throw std::invalid_argument(
          "TERRARIUM_PORT must be between 1 and 65535.");
    }

    terrarium::Database database(databasePath);
    terrarium::MeasurementHandler measurementHandler(database);
    Pistache::Http::Endpoint endpoint(
        Pistache::Address(Pistache::Ipv4::any(), Pistache::Port(port)));
    endpoint.init(Pistache::Http::Endpoint::options()
                      .threads(kWorkerThreads)
                      .flags(Pistache::Tcp::Options::ReuseAddr));

    Pistache::Rest::Router router;
    measurementHandler.setupRoutes(router);
    endpoint.setHandler(router.handler());

    std::cout << "Starting backend on port " << port << " using database "
              << databasePath << std::endl;
    endpoint.serve();
  } catch (const std::exception &error) {
    std::cerr << "Backend startup failed: " << error.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
