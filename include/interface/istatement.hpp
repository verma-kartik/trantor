#pragma once

#include <optional>
#include <string>
#include <vector>
#include "common.hpp"

namespace trantor {

/**
 * Minimal statement interface used by the ORM.
 *
 * Implementations should be zero-cost wrappers around native prepared-stmt objects.
 */
struct IStatement {
    virtual ~IStatement() = default;

    // Binding helpers (1-based index)
    virtual std::optional<Error> bindInt(size_t idx, long long v) = 0;
    virtual std::optional<Error> bindDouble(size_t idx, double v) = 0;
    virtual std::optional<Error> bindText(size_t idx, const std::string& v) = 0;
    virtual std::optional<Error> bindBlob(size_t idx, const void* data, size_t len) = 0;
    virtual std::optional<Error> bindNull(size_t idx) = 0;

    // Step / reset
    virtual std::optional<Error> step() = 0;
    virtual std::optional<Error> reset() = 0;

    // Query state helpers
    virtual bool done() const = 0;
    virtual int columnCount() const = 0;
    virtual int columnType(int idx) const = 0; // DB-specific constants (e.g., SQLITE_INTEGER)

    // Typed column read helpers (idx is 0-based)
    virtual std::optional<Error> getInt(int idx, long long &out) = 0;
    virtual std::optional<Error> getDouble(int idx, double &out) = 0;
    virtual std::optional<Error> getText(int idx, std::string &out) = 0;
    virtual std::optional<Error> getBlob(int idx, std::vector<uint8_t> &out) = 0;

    // Optional: get raw pointer to underlying native stmt (for advanced use)
    virtual void* native_handle() { return nullptr; }
};
} // namespace trantor
