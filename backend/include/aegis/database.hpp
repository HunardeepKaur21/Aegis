#pragma once
#include <sqlite3.h>
#include <crow.h>
#include <functional>
#include <mutex>
#include <string>

namespace aegis {

// Thin RAII wrapper over SQLite. Rows are returned as Crow JSON so routes can send them directly.
class Database {
public:
    explicit Database(const std::string& path);
    ~Database();
    Database(const Database&) = delete;
    Database& operator=(const Database&) = delete;

    void execScript(const std::string& sql);
    long long scalarInt(const std::string& sql);
    crow::json::wvalue rows(const std::string& sql);   // JSON array of objects
    crow::json::wvalue firstRow(const std::string& sql); // JSON object, or null if empty
    bool hasTable(const std::string& name);
    // Runs a SELECT and calls fn once per row (fn reads columns from the statement).
    void query(const std::string& sql, const std::function<void(sqlite3_stmt*)>& fn);
    int changes() const;                              // rows changed by the last write
    static std::string quote(const std::string& s);   // SQL string literal with ' escaped

private:
    sqlite3* db_ = nullptr;
    std::recursive_mutex mutex_;  // one connection shared by Crow's worker threads
};

std::string readFile(const std::string& path);

}  // namespace aegis
