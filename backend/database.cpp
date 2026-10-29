#include "database.h"

#include <array>
#include <ctime>
#include <iomanip>
#include <memory>
#include <sstream>
#include <stdexcept>

namespace terrarium {
namespace {

using SqlStatement = std::unique_ptr<sqlite3_stmt, decltype(&sqlite3_finalize)>;

std::string currentTimestamp() {
  const std::time_t now = std::time(nullptr);
  std::tm localTime{};
  localtime_r(&now, &localTime);

  std::ostringstream timestamp;
  timestamp << std::put_time(&localTime, "%Y-%m-%d %H:%M:%S");
  return timestamp.str();
}

SqlStatement prepareStatement(sqlite3 *database, const char *sql) {
  sqlite3_stmt *statement = nullptr;
  if (sqlite3_prepare_v2(database, sql, -1, &statement, nullptr) != SQLITE_OK) {
    throw std::runtime_error(sqlite3_errmsg(database));
  }
  return SqlStatement(statement, sqlite3_finalize);
}

void checkSqliteResult(sqlite3 *database, int result) {
  if (result != SQLITE_OK && result != SQLITE_DONE && result != SQLITE_ROW) {
    throw std::runtime_error(sqlite3_errmsg(database));
  }
}

} // namespace

Database::Database(const char *path) {
  if (sqlite3_open(path, &database_) != SQLITE_OK) {
    const std::string error =
        database_ ? sqlite3_errmsg(database_) : "SQLite open failed.";
    if (database_) {
      sqlite3_close(database_);
      database_ = nullptr;
    }
    throw std::runtime_error(error);
  }

  initializeSchema();
}

Database::~Database() {
  if (database_) {
    sqlite3_close(database_);
  }
}

void Database::saveMeasurements(float temperature, float humidity,
                                float light) {
  const std::array<std::pair<const char *, std::pair<float, const char *>>, 3>
      values = {{
          {"temperature", {temperature, "°C"}},
          {"humidity", {humidity, "%"}},
          {"light", {light, "lux"}},
      }};

  std::lock_guard<std::mutex> lock(mutex_);
  execute("BEGIN IMMEDIATE;");
  try {
    for (const auto &[type, measurement] : values) {
      insertMeasurement(type, measurement.first, measurement.second);
    }
    execute("COMMIT;");
  } catch (...) {
    execute("ROLLBACK;");
    throw;
  }
}

std::string Database::getMeasurements(const std::string &type) {
  std::lock_guard<std::mutex> lock(mutex_);
  return queryMeasurements(type);
}

std::string Database::getRecentMeasurements() {
  std::lock_guard<std::mutex> lock(mutex_);
  std::string response;
  for (const auto &field : kSetpointFields) {
    response += queryLatestMeasurement(field.name, field.name);
  }
  return response.empty() ? "No data!\n" : response;
}

std::string Database::getSetpoints() {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto statement = prepareStatement(
      database_, "SELECT type, value, unit FROM Setpoints ORDER BY id;");
  std::string response;

  int result = SQLITE_OK;
  while ((result = sqlite3_step(statement.get())) == SQLITE_ROW) {
    response += columnText(statement.get(), 0) + ": " +
                std::to_string(sqlite3_column_double(statement.get(), 1)) +
                " " + columnText(statement.get(), 2) + "\n";
  }
  checkSqliteResult(database_, result);
  return response.empty() ? "No data!\n" : response;
}

void Database::saveSetpoints(
    const std::vector<std::pair<SetpointField, float>> &setpoints) {
  std::lock_guard<std::mutex> lock(mutex_);
  execute("BEGIN IMMEDIATE;");
  try {
    const auto statement = prepareStatement(
        database_,
        "INSERT INTO Setpoints (type, value, unit) VALUES (?, ?, ?) "
        "ON CONFLICT(type) DO UPDATE SET value = excluded.value, unit = "
        "excluded.unit;");

    for (const auto &[field, value] : setpoints) {
      sqlite3_reset(statement.get());
      sqlite3_clear_bindings(statement.get());
      checkSqliteResult(
          database_,
          sqlite3_bind_text(statement.get(), 1, field.name, -1, SQLITE_STATIC));
      checkSqliteResult(database_,
                        sqlite3_bind_double(statement.get(), 2, value));
      checkSqliteResult(
          database_,
          sqlite3_bind_text(statement.get(), 3, field.unit, -1, SQLITE_STATIC));
      checkSqliteResult(database_, sqlite3_step(statement.get()));
    }
    execute("COMMIT;");
  } catch (...) {
    execute("ROLLBACK;");
    throw;
  }
}

void Database::saveOutput(float value) {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto statement = prepareStatement(
      database_, "INSERT INTO output_data (timestamp, value) VALUES (?, ?);");
  const std::string timestamp = currentTimestamp();
  checkSqliteResult(database_,
                    sqlite3_bind_text(statement.get(), 1, timestamp.c_str(), -1,
                                      SQLITE_TRANSIENT));
  checkSqliteResult(database_, sqlite3_bind_double(statement.get(), 2, value));
  checkSqliteResult(database_, sqlite3_step(statement.get()));
}

std::string Database::getOutput() {
  std::lock_guard<std::mutex> lock(mutex_);
  const auto statement =
      prepareStatement(database_, "SELECT timestamp, value FROM output_data "
                                  "ORDER BY id DESC LIMIT 500;");
  std::string response;

  int result = SQLITE_OK;
  while ((result = sqlite3_step(statement.get())) == SQLITE_ROW) {
    response += "Timestamp: " + columnText(statement.get(), 0) + ", Value: " +
                std::to_string(sqlite3_column_double(statement.get(), 1)) +
                "\n";
  }
  checkSqliteResult(database_, result);
  return response.empty() ? "No data!\n" : response;
}

void Database::execute(const char *sql) {
  char *error = nullptr;
  if (sqlite3_exec(database_, sql, nullptr, nullptr, &error) != SQLITE_OK) {
    const std::string message = error ? error : sqlite3_errmsg(database_);
    sqlite3_free(error);
    throw std::runtime_error(message);
  }
}

void Database::initializeSchema() {
  std::lock_guard<std::mutex> lock(mutex_);
  execute("CREATE TABLE IF NOT EXISTS sensor_data ("
          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
          "timestamp TEXT NOT NULL, value REAL NOT NULL, "
          "type TEXT NOT NULL, unit TEXT NOT NULL);"
          "CREATE TABLE IF NOT EXISTS output_data ("
          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
          "timestamp TEXT NOT NULL, value REAL NOT NULL);"
          "CREATE TABLE IF NOT EXISTS Setpoints ("
          "id INTEGER PRIMARY KEY AUTOINCREMENT, "
          "type TEXT NOT NULL UNIQUE, value REAL NOT NULL DEFAULT 0.0, "
          "unit TEXT NOT NULL DEFAULT '');"
          "INSERT OR IGNORE INTO Setpoints (type, value, unit) VALUES "
          "('temperature', 25.0, '°C'), "
          "('humidity', 40.0, '%'), "
          "('light', 100.0, 'lux');");
}

void Database::insertMeasurement(const char *type, float value,
                                 const char *unit) {
  const auto statement = prepareStatement(
      database_, "INSERT INTO sensor_data (timestamp, value, type, unit) "
                 "VALUES (?, ?, ?, ?);");
  const std::string timestamp = currentTimestamp();
  checkSqliteResult(database_,
                    sqlite3_bind_text(statement.get(), 1, timestamp.c_str(), -1,
                                      SQLITE_TRANSIENT));
  checkSqliteResult(database_, sqlite3_bind_double(statement.get(), 2, value));
  checkSqliteResult(database_, sqlite3_bind_text(statement.get(), 3, type, -1,
                                                 SQLITE_STATIC));
  checkSqliteResult(database_, sqlite3_bind_text(statement.get(), 4, unit, -1,
                                                 SQLITE_STATIC));
  checkSqliteResult(database_, sqlite3_step(statement.get()));
}

std::string Database::queryMeasurements(const std::string &type) {
  const auto statement = prepareStatement(
      database_, "SELECT timestamp, value, unit FROM sensor_data "
                 "WHERE type = ? ORDER BY id DESC LIMIT 500;");
  checkSqliteResult(database_,
                    sqlite3_bind_text(statement.get(), 1, type.c_str(), -1,
                                      SQLITE_TRANSIENT));

  std::string response;
  int result = SQLITE_OK;
  while ((result = sqlite3_step(statement.get())) == SQLITE_ROW) {
    response += "Timestamp: " + columnText(statement.get(), 0) + ", Value: " +
                std::to_string(sqlite3_column_double(statement.get(), 1)) +
                " " + columnText(statement.get(), 2) + "\n";
  }
  checkSqliteResult(database_, result);
  return response.empty() ? "No data!\n" : response;
}

std::string Database::queryLatestMeasurement(const char *type,
                                             const char *label) {
  const auto statement = prepareStatement(
      database_, "SELECT timestamp, value, unit FROM sensor_data "
                 "WHERE type = ? ORDER BY id DESC LIMIT 1;");
  checkSqliteResult(database_, sqlite3_bind_text(statement.get(), 1, type, -1,
                                                 SQLITE_STATIC));

  const int result = sqlite3_step(statement.get());
  if (result == SQLITE_DONE) {
    return std::string(label) + " - No data!\n";
  }
  checkSqliteResult(database_, result);
  return std::string(label) +
         " - Timestamp: " + columnText(statement.get(), 0) + ", Value: " +
         std::to_string(sqlite3_column_double(statement.get(), 1)) + " " +
         columnText(statement.get(), 2) + "\n";
}

std::string Database::columnText(sqlite3_stmt *statement, int column) {
  const auto *value = sqlite3_column_text(statement, column);
  return value ? reinterpret_cast<const char *>(value) : "";
}

} // namespace terrarium
