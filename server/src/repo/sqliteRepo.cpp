#include "repo/sqliteRepo.h"

#include <sqlite3.h>

#include <iostream>
#include <string>

namespace simpleelo::server::repo {
namespace {

constexpr const char* kSchemaSql =
    "CREATE TABLE IF NOT EXISTS kv_state ("
    "  key TEXT PRIMARY KEY,"
    "  value TEXT NOT NULL"
    ");";

constexpr const char* kStateKey = "server_state";

bool ensureSchema(sqlite3* db) {
  char* errMsg = nullptr;
  const int rc = sqlite3_exec(db, kSchemaSql, nullptr, nullptr, &errMsg);
  if (rc != SQLITE_OK) {
    std::cerr << "sqlite ensureSchema failed: " << (errMsg ? errMsg : "unknown") << std::endl;
    sqlite3_free(errMsg);
    return false;
  }
  return true;
}

}  // namespace

bool loadState(const std::string& dbPath, nlohmann::json& state, bool& found) {
  found = false;
  state = nlohmann::json::object();

  sqlite3* db = nullptr;
  if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
    if (db != nullptr) {
      std::cerr << "sqlite open failed: " << sqlite3_errmsg(db) << std::endl;
      sqlite3_close(db);
    }
    return false;
  }

  if (!ensureSchema(db)) {
    sqlite3_close(db);
    return false;
  }

  sqlite3_stmt* stmt = nullptr;
  const char* sql = "SELECT value FROM kv_state WHERE key = ?1 LIMIT 1;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "sqlite prepare(load) failed: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_close(db);
    return false;
  }

  sqlite3_bind_text(stmt, 1, kStateKey, -1, SQLITE_STATIC);
  const int stepRc = sqlite3_step(stmt);
  if (stepRc == SQLITE_ROW) {
    const unsigned char* text = sqlite3_column_text(stmt, 0);
    if (text != nullptr) {
      try {
        state = nlohmann::json::parse(reinterpret_cast<const char*>(text));
        found = true;
      } catch (const std::exception& ex) {
        std::cerr << "sqlite state json parse failed: " << ex.what() << std::endl;
        sqlite3_finalize(stmt);
        sqlite3_close(db);
        return false;
      }
    }
  } else if (stepRc != SQLITE_DONE) {
    std::cerr << "sqlite step(load) failed: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_finalize(stmt);
    sqlite3_close(db);
    return false;
  }

  sqlite3_finalize(stmt);
  sqlite3_close(db);
  return true;
}

bool saveState(const std::string& dbPath, const nlohmann::json& state) {
  sqlite3* db = nullptr;
  if (sqlite3_open(dbPath.c_str(), &db) != SQLITE_OK) {
    if (db != nullptr) {
      std::cerr << "sqlite open failed: " << sqlite3_errmsg(db) << std::endl;
      sqlite3_close(db);
    }
    return false;
  }

  if (!ensureSchema(db)) {
    sqlite3_close(db);
    return false;
  }

  char* beginErr = nullptr;
  if (sqlite3_exec(db, "BEGIN IMMEDIATE TRANSACTION;", nullptr, nullptr, &beginErr) != SQLITE_OK) {
    std::cerr << "sqlite begin failed: " << (beginErr ? beginErr : "unknown") << std::endl;
    sqlite3_free(beginErr);
    sqlite3_close(db);
    return false;
  }

  sqlite3_stmt* stmt = nullptr;
  const char* sql = "INSERT INTO kv_state(key, value) VALUES(?1, ?2) "
                    "ON CONFLICT(key) DO UPDATE SET value=excluded.value;";
  if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
    std::cerr << "sqlite prepare(save) failed: " << sqlite3_errmsg(db) << std::endl;
    sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
    sqlite3_close(db);
    return false;
  }

  const std::string serialized = state.dump();
  sqlite3_bind_text(stmt, 1, kStateKey, -1, SQLITE_STATIC);
  sqlite3_bind_text(stmt, 2, serialized.c_str(), static_cast<int>(serialized.size()), SQLITE_TRANSIENT);

  bool ok = true;
  if (sqlite3_step(stmt) != SQLITE_DONE) {
    std::cerr << "sqlite step(save) failed: " << sqlite3_errmsg(db) << std::endl;
    ok = false;
  }

  sqlite3_finalize(stmt);

  if (ok) {
    if (sqlite3_exec(db, "COMMIT;", nullptr, nullptr, nullptr) != SQLITE_OK) {
      std::cerr << "sqlite commit failed: " << sqlite3_errmsg(db) << std::endl;
      ok = false;
    }
  }
  if (!ok) {
    sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
  }

  sqlite3_close(db);
  return ok;
}

}  // namespace simpleelo::server::repo
