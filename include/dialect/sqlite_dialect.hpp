#pragma once

#include "interface/isqldialect.hpp"
#include <sstream>

namespace trantor {

struct SqliteDialect : public ISqlDialect {
    std::string typeName(sql_type_t t) const override {
        return sqlTypesStr(t); // uses your consteval sqlTypesStr
    }

    std::string renderColumnDefinition(const std::string &name,
                                       sql_type_t sqlType,
                                       const std::string &constraints,
                                       bool isPrimaryKey,
                                       bool autoInc) const override {
        std::ostringstream ss;
        ss << escapeIdentifier(name) << " " << typeName(sqlType);
        if (isPrimaryKey) {
            ss << " PRIMARY KEY";
            if (autoInc && sqlType == sql_type_t::INTEGER) {
                ss << " " << autoIncrementKeyword();
            }
        }
        if (!constraints.empty()) {
            ss << " " << constraints;
        }
        return ss.str();
    }

    std::string escapeIdentifier(const std::string &ident) const override {
        return std::string("`") + ident + "`";
    }

    std::string autoIncrementKeyword() const override {
        return "AUTOINCREMENT";
    }
};
} // namespace trantor
