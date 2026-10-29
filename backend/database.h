#pragma once

#include "models.h"

#include <sqlite3.h>

#include <mutex>
#include <string>
#include <utility>
#include <vector>

namespace terrarium {

class Database {
public:
  explicit Database(const char *path);
  ~Database();

  Database(const Database &) = delete;
  Database &operator=(const Database &) = delete;

  void saveMeasurements(float temperature, float humidity, float light);
  std::string getMeasurements(const std::string &type);
  std::string getRecentMeasurements();
  std::string getSetpoints();
  void
  saveSetpoints(const std::vector<std::pair<SetpointField, float>> &setpoints);
  void saveOutput(float value);
  std::string getOutput();

private:
  void execute(const char *sql);
  void initializeSchema();
  void insertMeasurement(const char *type, float value, const char *unit);
  std::string queryMeasurements(const std::string &type);
  std::string queryLatestMeasurement(const char *type, const char *label);
  static std::string columnText(sqlite3_stmt *statement, int column);

  sqlite3 *database_ = nullptr;
  std::mutex mutex_;
};

} // namespace terrarium
