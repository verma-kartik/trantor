#pragma once

#include <memory>
#include <optional>
#include <string>
#include <utility>
#include "common.hpp"

namespace trantor {
struct IStatement;

/**
 * Minimal database-agnostic connection interface.
 */
struct IDatabaseConnection {
    virtual ~IDatabaseConnection() = default;

    // Execute a non-prepared SQL string (no binding).
    virtual std::optional<Error> execute(const std::string& sql) = 0;

    // Prepare a statement; return (statement, optional error). On error, first is null and second is Error.
    virtual std::pair<std::unique_ptr<IStatement>, std::optional<Error>>
    prepare(const std::string& sql) = 0;

    // Transactions
    virtual std::optional<Error> beginTransaction() = 0;
    virtual std::optional<Error> commitTransaction() = 0;
    virtual std::optional<Error> rollbackTransaction() = 0;

    Logger logger = nullptr;
};
} // namespace trantor
