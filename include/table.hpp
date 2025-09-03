#pragma once

/**
    * @file table.hpp
    * @brief Contains routines for creating tables and querying them.
    *
    * NOTE: refactored to produce dialect-aware SQL strings.
    */

#include "common.hpp"
#include "column.hpp"
#include "types.hpp"
#include <algorithm>
#include <iostream>
#include <string>
#include <sstream>
#include <tuple>
#include "interface/isqldialect.hpp"

namespace trantor{

    template <typename Column, typename Table>
    concept ColumnBelongingToClass = std::is_same<typename Column::ObjectClass, Table>::value;

    template <FixedLengthString tableName, typename T, ColumnBelongingToClass<T>... Column>
    class Table{
    private:
        template<typename... C>
        struct FindPrimaryKey : std::false_type {};

        template<typename... C>
        requires AnyOf<Column::isPrimaryKey...>
        struct FindPrimaryKey<C...> : std::false_type {
            static constexpr int primaryKeyIndex = IndexOfFirst<C::isPrimaryKey...>::value;
            using type = typename std::tuple_element<primaryKeyIndex, std::tuple<Column...>>::type;
        };

        // helper to generate column definitions using dialect
        static std::string generateColumnDefs(const ISqlDialect &dialect) {
            std::ostringstream ss;
            std::apply([&](const auto&... a) {
                (([&]{
                    using col_t = std::remove_reference_t<decltype(a)>;
                    auto name = col_t::name();
                    auto sqlType = col_t::SQLMemberType;
                    auto constraints = col_t::constraintCreationQuery();
                    bool isPK = col_t::isPrimaryKey;
                    bool autoInc = col_t::isAutoIncColumn;
                    ss << "\t" << dialect.renderColumnDefinition(name, sqlType, constraints, isPK, autoInc) << ",\n";
                }()), ...);
            }, ColumnTuples{});
            auto s = ss.str();
            if (s.size() >= 2) s.erase(s.end()-2, s.end()); // remove trailing comma+newline
            return s;
        }

        // generate column name list (used in INSERT)
        static std::string generateColumnsList(const ISqlDialect &dialect) {
            std::ostringstream ss;
            bool first = true;
            std::apply([&](const auto&... a) {
                (([&]{
                    using col_t = std::remove_reference_t<decltype(a)>;
                    if constexpr (!col_t::isAutoIncColumn) {
                        if (!first) ss << ", ";
                        ss << dialect.escapeIdentifier(col_t::name());
                        first = false;
                    }
                }()), ...);
            }, ColumnTuples{});
            return ss.str();
        }

        // generate a parameterized VALUES clause with placeholders for binding
        static std::string generateInsertPlaceholders() {
            std::ostringstream ss;
            bool first = true;
            std::apply([&](const auto&... a) {
                (([&] {
                    using col_t = std::remove_reference_t<decltype(a)>;
                    if constexpr (!col_t::isAutoIncColumn) {
                        if (!first) ss << ", ";
                        ss << "?";
                        first = false;
                    }
                }()), ...);
            }, ColumnTuples{});
            return ss.str();
        }

        // generate VALUES read (not used when binding) - better to use placeholders + binding
    public:
        static constexpr auto numberOfColumns = sizeof...(Column);
        static constexpr bool hasPrimaryKey = AnyOf<Column::isPrimaryKey...>;
        using PrimaryKey = typename FindPrimaryKey<Column...>::type;
        using ObjectClass = T;
        using ColumnTuples = std::tuple<Column...>;
        using columns_t = ColumnTuples;

        static constexpr std::string columnName(int index){
            int i = 0;
            std::string name;
            ((name = i == index ? Column::name() : name, i++), ...);
            return name;
        }

        static std::string createTableQuery(const ISqlDialect &dialect, bool ifNotExist) {
            std::ostringstream query;
            query << "CREATE TABLE ";
            if (ifNotExist) query << "IF NOT EXISTS ";
            query << dialect.escapeIdentifier(tableName.value) << " (\n";
            query << generateColumnDefs(dialect);
            query << "\n );";
            return query.str();
        }

        static std::string findQuery(const ISqlDialect &dialect) {
            std::ostringstream ss;
            ss << "SELECT * FROM " << dialect.escapeIdentifier(tableName.value) << " WHERE "
               << dialect.escapeIdentifier(PrimaryKey::name()) << " = ?;";
            return ss.str();
        }

        static std::string insertQuery(const ISqlDialect &dialect, const ObjectClass&) {
            std::ostringstream ss;
            ss << "INSERT INTO " << dialect.escapeIdentifier(tableName.value) << " (";
            ss << generateColumnsList(dialect);
            ss << ") VALUES (" << generateInsertPlaceholders() << ");";
            return ss.str();
        }

        static std::string insertQueries(const ISqlDialect &dialect, const std::vector<ObjectClass>& objects) {
            // default: fallback to many single INSERTs executed in a transaction by the caller
            std::ostringstream ss;
            for (const auto &obj : objects) {
                ss << insertQuery(dialect, obj) << "\n";
            }
            return ss.str();
        }

        static std::string deleteQuery(const ISqlDialect &dialect) {
            std::ostringstream ss;
            ss << "DELETE FROM " << dialect.escapeIdentifier(tableName.value)
               << " WHERE " << dialect.escapeIdentifier(PrimaryKey::name()) << " = ?;";
            return ss.str();
        }
    };
} // namespace trantor
