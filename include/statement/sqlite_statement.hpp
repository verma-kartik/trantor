#pragma once

#include "interface/istatement.hpp"
#include <sqlite3.h>
#include <optional>
#include <string>
#include <vector>
#include <cstring>

namespace trantor {

class SqliteStatement final : public IStatement {
public:
    SqliteStatement(sqlite3_stmt* s, Logger logger = nullptr) : stmt_(s), logger_(logger) {}
    ~SqliteStatement() override {
        if (stmt_) sqlite3_finalize(stmt_);
    }

    std::optional<Error> bindInt(size_t idx, long long v) override {
        int rc = sqlite3_bind_int64(stmt_, static_cast<int>(idx), v);
        if (rc != SQLITE_OK) return Error("sqlite bind int failed", rc);
        return std::nullopt;
    }

    std::optional<Error> bindDouble(size_t idx, double v) override {
        int rc = sqlite3_bind_double(stmt_, static_cast<int>(idx), v);
        if (rc != SQLITE_OK) return Error("sqlite bind double failed", rc);
        return std::nullopt;
    }

    std::optional<Error> bindText(size_t idx, const std::string& v) override {
        int rc = sqlite3_bind_text(stmt_, static_cast<int>(idx), v.c_str(), (int)v.size(), SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) return Error("sqlite bind text failed", rc);
        return std::nullopt;
    }

    std::optional<Error> bindBlob(size_t idx, const void* data, size_t len) override {
        int rc = sqlite3_bind_blob(stmt_, static_cast<int>(idx), data, static_cast<int>(len), SQLITE_TRANSIENT);
        if (rc != SQLITE_OK) return Error("sqlite bind blob failed", rc);
        return std::nullopt;
    }

    std::optional<Error> bindNull(size_t idx) override {
        int rc = sqlite3_bind_null(stmt_, static_cast<int>(idx));
        if (rc != SQLITE_OK) return Error("sqlite bind null failed", rc);
        return std::nullopt;
    }

    std::optional<Error> step() override {
        int rc = sqlite3_step(stmt_);
        if (rc != SQLITE_ROW && rc != SQLITE_DONE) {
            return Error("sqlite step failed", rc);
        }
        done_ = (rc == SQLITE_DONE);
        return std::nullopt;
    }

    std::optional<Error> reset() override {
        int rc = sqlite3_reset(stmt_);
        if (rc != SQLITE_OK) return Error("sqlite reset failed", rc);
        rc = sqlite3_clear_bindings(stmt_);
        if (rc != SQLITE_OK) return Error("sqlite clear_bindings failed", rc);
        done_ = false;
        return std::nullopt;
    }

    bool done() const override { return done_; }
    int columnCount() const override { return sqlite3_column_count(stmt_); }
    int columnType(int idx) const override { return sqlite3_column_type(stmt_, idx); }

    std::optional<Error> getInt(int idx, long long &out) override {
        out = sqlite3_column_int64(stmt_, idx);
        return std::nullopt;
    }
    std::optional<Error> getDouble(int idx, double &out) override {
        out = sqlite3_column_double(stmt_, idx);
        return std::nullopt;
    }
    std::optional<Error> getText(int idx, std::string &out) override {
        const unsigned char* txt = sqlite3_column_text(stmt_, idx);
        out = txt ? reinterpret_cast<const char*>(txt) : std::string{};
        return std::nullopt;
    }
    std::optional<Error> getBlob(int idx, std::vector<uint8_t> &out) override {
        const void* data = sqlite3_column_blob(stmt_, idx);
        int len = sqlite3_column_bytes(stmt_, idx);
        if (!data || len <= 0) {
            out.clear();
            return std::nullopt;
        }
        out.resize(len);
        memcpy(out.data(), data, len);
        return std::nullopt;
    }

    void* native_handle() override { return stmt_; }

private:
    sqlite3_stmt* stmt_ = nullptr;
    Logger logger_ = nullptr;
    bool done_ = false;
};
} // namespace trantor
