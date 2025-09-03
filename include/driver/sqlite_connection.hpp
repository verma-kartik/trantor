#pragma once
#include <sqlite3.h>
#include <memory>
#include <string>
#include "interface/idatabase.hpp"
#include "statement/sqlite_statement.hpp"
#include "common.hpp"
#include <utility>

namespace trantor {

class SqliteConnection final : public IDatabaseConnection {
public:
    static Maybe<std::unique_ptr<SqliteConnection>> create(const char* fileName,
                                                           int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE,
                                                           Logger logger = nullptr) {
        sqlite3* db_handle = nullptr;
        int result = sqlite3_open_v2(fileName, &db_handle, flags, nullptr);
        if (result != SQLITE_OK || !db_handle) {
            return Error("Unable to open sqlite connection", result);
        }
        return std::unique_ptr<SqliteConnection>(new SqliteConnection(db_handle, logger));
    }

    ~SqliteConnection() override {
        if (db_) {
            int rc = sqlite3_close_v2(db_);
            if (rc != SQLITE_OK) {
                if (logger_) {
                    logger_(LogLevel::error, "Unable to close sqlite connection");
                    logger_(LogLevel::error, sqlite3_errstr(rc));
                }
            }
        }
    }

    std::optional<Error> execute(const std::string &sql) override {
        char* errMsg = nullptr;
        int rc = sqlite3_exec(db_, sql.c_str(), nullptr, nullptr, &errMsg);
        if (rc != SQLITE_OK) {
            std::string msg = errMsg ? errMsg : "sqlite exec failed";
            if (errMsg) sqlite3_free(errMsg);
            return Error(msg.c_str(), rc);
        }
        return std::nullopt;
    }

    std::pair<std::unique_ptr<IStatement>, std::optional<Error>>
    prepare(const std::string &sql) override {
        sqlite3_stmt* stmt = nullptr;
        int rc = sqlite3_prepare_v2(db_, sql.c_str(), -1, &stmt, nullptr);
        if (rc != SQLITE_OK) {
            return {nullptr, Error("Unable to prepare statement", rc)};
        }
        return {std::make_unique<SqliteStatement>(stmt, logger_), std::nullopt};
    }

    std::optional<Error> beginTransaction() override { return execute("BEGIN;"); }
    std::optional<Error> commitTransaction() override { return execute("COMMIT;"); }
    std::optional<Error> rollbackTransaction() override { return execute("ROLLBACK;"); }

private:
    SqliteConnection(sqlite3* db, Logger logger) : db_(db), logger_(logger) { if(!logger_) logger_ = [](auto...){ }; }
    sqlite3* db_{nullptr};
    Logger logger_{nullptr};
};
} // namespace trantor
