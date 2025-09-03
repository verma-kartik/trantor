#pragma once
#include <string>
#include "types.hpp"

namespace trantor {

/**
 * SQL dialect interface. Implement for each DB backend to handle type names,
 * identifier escaping and special keywords (autoinc, serial, etc).
 */
struct ISqlDialect {
    virtual ~ISqlDialect() = default;

    // Convert trantor::sql_type_t to backend type name
    virtual std::string typeName(sql_type_t t) const = 0;

    // Render column definition: name + type + constraints + PK/AUTO
    virtual std::string renderColumnDefinition(const std::string &name,
                                               sql_type_t sqlType,
                                               const std::string &constraints,
                                               bool isPrimaryKey,
                                               bool autoInc) const = 0;

    // Escape an identifier (table/column name)
    virtual std::string escapeIdentifier(const std::string &ident) const = 0;

    // Return an autoincrement token (if backend supports it) or empty
    virtual std::string autoIncrementKeyword() const = 0;
};
} // namespace trantor
