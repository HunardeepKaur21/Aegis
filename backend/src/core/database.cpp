#include "aegis/database.hpp"
#include <fstream>
#include <sstream>
#include <stdexcept>

namespace aegis {

Database::Database(const std::string& path) {
    if (sqlite3_open(path.c_str(), &db_) != SQLITE_OK) {
        std::string msg = sqlite3_errmsg(db_);
        sqlite3_close(db_);
        throw std::runtime_error("Cannot open database: " + msg);
    }
    sqlite3_exec(db_, "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr);
}

Database::~Database() { if (db_) sqlite3_close(db_); }

void Database::execScript(const std::string& sql) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    char* err = nullptr;
    if (sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        throw std::runtime_error("SQL error: " + msg);
    }
}

static crow::json::wvalue rowToJson(sqlite3_stmt* st) {
    crow::json::wvalue obj;
    for (int i = 0; i < sqlite3_column_count(st); ++i) {
        const char* name = sqlite3_column_name(st, i);
        switch (sqlite3_column_type(st, i)) {
            case SQLITE_INTEGER: obj[name] = static_cast<long long>(sqlite3_column_int64(st, i)); break;
            case SQLITE_FLOAT:   obj[name] = sqlite3_column_double(st, i); break;
            case SQLITE_NULL:    obj[name] = nullptr; break;
            default:
                obj[name] = std::string(reinterpret_cast<const char*>(sqlite3_column_text(st, i)));
        }
    }
    return obj;
}

crow::json::wvalue Database::rows(const std::string& sql) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &st, nullptr) != SQLITE_OK)
        throw std::runtime_error(std::string("SQL error: ") + sqlite3_errmsg(db_));
    std::vector<crow::json::wvalue> out;
    while (sqlite3_step(st) == SQLITE_ROW) out.push_back(rowToJson(st));
    sqlite3_finalize(st);
    return crow::json::wvalue::list(out);
}

crow::json::wvalue Database::firstRow(const std::string& sql) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &st, nullptr) != SQLITE_OK)
        throw std::runtime_error(std::string("SQL error: ") + sqlite3_errmsg(db_));
    crow::json::wvalue out;  // null by default
    if (sqlite3_step(st) == SQLITE_ROW) out = rowToJson(st);
    sqlite3_finalize(st);
    return out;
}

long long Database::scalarInt(const std::string& sql) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &st, nullptr) != SQLITE_OK)
        throw std::runtime_error(std::string("SQL error: ") + sqlite3_errmsg(db_));
    long long v = 0;
    if (sqlite3_step(st) == SQLITE_ROW) v = sqlite3_column_int64(st, 0);
    sqlite3_finalize(st);
    return v;
}

bool Database::hasTable(const std::string& name) {
    return scalarInt("SELECT COUNT(*) FROM sqlite_master WHERE type='table' AND name='" + name + "'") > 0;
}

void Database::query(const std::string& sql, const std::function<void(sqlite3_stmt*)>& fn) {
    std::lock_guard<std::recursive_mutex> lock(mutex_);
    sqlite3_stmt* st = nullptr;
    if (sqlite3_prepare_v2(db_, sql.c_str(), -1, &st, nullptr) != SQLITE_OK)
        throw std::runtime_error(std::string("SQL error: ") + sqlite3_errmsg(db_));
    while (sqlite3_step(st) == SQLITE_ROW) fn(st);
    sqlite3_finalize(st);
}

int Database::changes() const { return sqlite3_changes(db_); }

std::string Database::quote(const std::string& s) {
    std::string out = "'";
    for (char c : s) { if (c == '\'') out += '\''; out += c; }
    return out + "'";
}

std::string readFile(const std::string& path) {
    std::ifstream f(path);
    if (!f) throw std::runtime_error("Cannot read file: " + path);
    std::stringstream ss;
    ss << f.rdbuf();
    return ss.str();
}

}  // namespace aegis
