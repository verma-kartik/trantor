#pragma once

/**
 * connection.hpp
 *
 * OrmConnection: database-agnostic ORM entrypoint.
 *
 * Replaces heavy std::apply + pack-expansion usage with a small
 * for_each_in_tuple helper to avoid GCC template-instantiation ICEs.
 *
 */

#include <memory>
#include <optional>
#include <sstream>
#include <tuple>
#include <vector>
#include <type_traits>
#include <utility>

#include "interface/idatabase.hpp"
#include "interface/isqldialect.hpp"
#include "result.hpp"
#include "common.hpp"
#include <iostream>

#if defined(__has_include)
# if __has_include(<sqlite3.h>)
#  include <sqlite3.h> // used only to compare column types returned by SQLite adapter
# endif
#endif

namespace trantor {

/* -------------------------
 * tiny tuple-for-each helper
 * -------------------------
 *
 * Calls f(std::get<I>(t)) for each index I.
 */
namespace _tuple_utils {
    template<typename Tuple, typename F, std::size_t... I>
    inline void for_each_in_tuple_impl(const Tuple& t, F&& f, std::index_sequence<I...>) {
        // Expand into left-to-right invocation sequence.
        ( (void)std::invoke(f, std::get<I>(t)), ... );
    }

    template<typename Tuple, typename F>
    inline void for_each_in_tuple(const Tuple& t, F&& f) {
        constexpr std::size_t N = std::tuple_size_v<std::remove_reference_t<Tuple>>;
        for_each_in_tuple_impl(t, std::forward<F>(f), std::make_index_sequence<N>{});
    }
} // namespace _tuple_utils

/**
 * OrmConnection template.
 *
 * Holds a unique_ptr<IDatabaseConnection> and a unique_ptr<ISqlDialect>.
 */
template<class... Table>
requires(sizeof...(Table) > 0)
class OrmConnection {
public:
    OrmConnection(std::unique_ptr<IDatabaseConnection> conn, std::unique_ptr<ISqlDialect> dialect)
        : conn_(std::move(conn)), dialect_(std::move(dialect)) {}

    std::optional<Error> createTables(bool ifNotExist = true) {
        std::array<std::string, sizeof...(Table)> queries{ Table::createTableQuery(*dialect_, ifNotExist)... };
        for (auto &q : queries) {
            std::cout << "[DEBUG] createTableQuery:\n" << q << "\n\n";
            auto err = conn_->execute(q);
            if (err) return err;
        }
        return std::nullopt;
    }

    // find: returns Maybe<std::optional<T>> (variant<Error, optional<T>>)
    template<typename T, typename PrimaryKeyType>
    Maybe<std::optional<T>> find(const PrimaryKeyType& id) {
        using table_t = typename TableForClass<T>::type;

        static_assert(table_t::hasPrimaryKey, "Cannot execute a find on a table without a primary key");
        static_assert(std::is_convertible_v<PrimaryKeyType, typename table_t::PrimaryKey::MemberType>,
                      "Primary key type does not match the type specified in the definition of the table");

        auto q = table_t::findQuery(*dialect_);
        auto [stmtPtr, perr] = conn_->prepare(q);
        if (perr) return *perr;
        auto stmt = std::move(stmtPtr);

        // bind primary key
        using pk_t = typename table_t::PrimaryKey::MemberType;
        if constexpr (IsArithmetic<pk_t>()) {
            if constexpr (std::is_floating_point_v<std::remove_cvref_t<pk_t>>) {
                if (auto e = stmt->bindDouble(1, static_cast<double>(id))) return *e;
            } else {
                if (auto e = stmt->bindInt(1, static_cast<long long>(id))) return *e;
            }
        } else if constexpr (IsString<pk_t>()) {
            if (auto e = stmt->bindText(1, std::string(id))) return *e;
        } else {
            return Error("Unsupported PK type for binding");
        }

        if (auto e = stmt->step()) return *e;
        if (stmt->done()) {
            // no rows
            return std::nullopt;
        }

        if (stmt->columnCount() != table_t::numberOfColumns) {
            return Error("Unexpected number of columns returned by find query");
        }

        T record;
        std::optional<Error> lastError;
        int columnIdx = 0;

        // We iterate an *instance* of the columns tuple so we can refer to each column type as a value.
        typename table_t::columns_t columns{};
        _tuple_utils::for_each_in_tuple(columns, [&](const auto& column_inst) {
            if (lastError) return;
            using column_t = std::remove_reference_t<decltype(column_inst)>;

            // READ into either public member or use setter
            if constexpr (column_t::publicColumn) {
                using member_t = typename column_t::MemberType;
                if constexpr (IsArithmetic<member_t>()) {
                    // numeric
#if defined(SQLITE_INTEGER)
                    if (stmt->columnType(columnIdx) == SQLITE_INTEGER) {
                        long long iVal = 0;
                        if (auto e = stmt->getInt(columnIdx, iVal)) { lastError = e; return; }
                        if constexpr (std::is_floating_point_v<member_t>) {
                            column_t::getter(record) = static_cast<member_t>(static_cast<double>(iVal));
                        } else {
                            column_t::getter(record) = static_cast<member_t>(iVal);
                        }
                    } else if (stmt->columnType(columnIdx) == SQLITE_FLOAT) {
                        double dVal = 0;
                        if (auto e = stmt->getDouble(columnIdx, dVal)) { lastError = e; return; }
                        column_t::getter(record) = static_cast<member_t>(dVal);
                    } else
#endif
                    {
                        // fallback: try text to numeric conversion
                        std::string txt;
                        if (auto e = stmt->getText(columnIdx, txt)) { lastError = e; return; }
                        if constexpr (std::is_floating_point_v<member_t>) {
                            column_t::getter(record) = static_cast<member_t>(std::stod(txt));
                        } else {
                            column_t::getter(record) = static_cast<member_t>(std::stoll(txt));
                        }
                    }
                } else if constexpr (IsString<member_t>()) {
                    std::string txt;
                    if (auto e = stmt->getText(columnIdx, txt)) { lastError = e; return; }
                    column_t::getter(record) = txt;
                } else {
                    // container / blob
                    std::vector<uint8_t> blob;
                    if (auto e = stmt->getBlob(columnIdx, blob)) { lastError = e; return; }
                    using m_t = typename column_t::MemberType;
                    if constexpr (is_basic_string<m_t>::value) {
                        column_t::getter(record) = std::string(blob.begin(), blob.end());
                    } else if constexpr (is_vector<m_t>::value) {
                        m_t v;
                        v.resize(blob.size());
                        memcpy(v.data(), blob.data(), blob.size());
                        column_t::getter(record) = std::move(v);
                    } else {
                        lastError = Error("Unsupported container type for column read");
                        return;
                    }
                }
            } else {
                // private column: read into temp then setter
                using m_t = typename column_t::MemberType;
                if constexpr (IsArithmetic<m_t>()) {
#if defined(SQLITE_INTEGER)
                    if (stmt->columnType(columnIdx) == SQLITE_INTEGER) {
                        long long iVal = 0;
                        if (auto e = stmt->getInt(columnIdx, iVal)) { lastError = e; return; }
                        if constexpr (std::is_floating_point_v<m_t>) {
                            m_t v = static_cast<m_t>(static_cast<double>(iVal));
                            column_t::setter(record, v);
                        } else {
                            m_t v = static_cast<m_t>(iVal);
                            column_t::setter(record, v);
                        }
                    } else if (stmt->columnType(columnIdx) == SQLITE_FLOAT) {
                        double dVal = 0;
                        if (auto e = stmt->getDouble(columnIdx, dVal)) { lastError = e; return; }
                        column_t::setter(record, static_cast<m_t>(dVal));
                    } else
#endif
                    {
                        std::string txt;
                        if (auto e = stmt->getText(columnIdx, txt)) { lastError = e; return; }
                        if constexpr (std::is_floating_point_v<m_t>) {
                            column_t::setter(record, static_cast<m_t>(std::stod(txt)));
                        } else {
                            column_t::setter(record, static_cast<m_t>(std::stoll(txt)));
                        }
                    }
                } else if constexpr (IsString<m_t>()) {
                    std::string txt;
                    if (auto e = stmt->getText(columnIdx, txt)) { lastError = e; return; }
                    column_t::setter(record, txt);
                } else {
                    std::vector<uint8_t> blob;
                    if (auto e = stmt->getBlob(columnIdx, blob)) { lastError = e; return; }
                    if constexpr (is_basic_string<m_t>::value) {
                        m_t str(blob.begin(), blob.end());
                        column_t::setter(record, str);
                    } else if constexpr (is_vector<m_t>::value) {
                        m_t v;
                        v.resize(blob.size());
                        memcpy(v.data(), blob.data(), blob.size());
                        column_t::setter(record, v);
                    } else {
                        lastError = Error("Unsupported container type for column read");
                        return;
                    }
                }
            }

            ++columnIdx;
        });

        if (lastError) return *lastError;
        return record;
    }

    // insert: binds non-auto-inc columns using prepared statement
    template<class T>
    std::optional<Error> insert(const T& record) {
        using table_t = typename TableForClass<T>::type;

        auto q = table_t::insertQuery(*dialect_, record);
        auto [stmtPtr, perr] = conn_->prepare(q);
        if (perr) return perr;
        auto stmt = std::move(stmtPtr);

        int bindIndex = 1;
        std::optional<Error> err;

        // Iterate columns and bind non-auto-inc ones
        typename table_t::columns_t columns{};
        _tuple_utils::for_each_in_tuple(columns, [&](const auto& column_inst) {
            if (err) return;
            using column_t = std::remove_reference_t<decltype(column_inst)>;
            if constexpr (!column_t::isAutoIncColumn) {
                using member_t = typename column_t::MemberType;

                if constexpr (IsArithmetic<member_t>()) {
                    auto val = column_t::getter(const_cast<T&>(record));
                    if constexpr (std::is_floating_point_v<std::remove_cvref_t<member_t>>) {
                        err = stmt->bindDouble(bindIndex++, static_cast<double>(val));
                    } else {
                        err = stmt->bindInt(bindIndex++, static_cast<long long>(val));
                    }
                } else if constexpr (IsString<member_t>()) {
                    auto s = column_t::getter(const_cast<T&>(record));
                    err = stmt->bindText(bindIndex++, s);
                } else if constexpr (IsContinuousContainer<member_t>()) {
                    auto &cont = column_t::getter(const_cast<T&>(record));
                    using value_type_t = typename std::remove_reference_t<decltype(cont)>::value_type;
                    const void* data = static_cast<const void*>(cont.data());
                    size_t len = cont.size() * sizeof(value_type_t);
                    err = stmt->bindBlob(bindIndex++, data, len);
                } else {
                    err = Error("Unsupported column type for insert binding");
                }
            }
        });

        if (err) return err;
        if (auto e = stmt->step()) return e;
        if (!stmt->done()) return Error("Insert did not finish");
        if (auto e = stmt->reset()) return e;
        return std::nullopt;
    }

    // deleteRecord by primary key
    template<class T, typename PrimaryKeyType>
    std::optional<Error> deleteRecord(const PrimaryKeyType& id) {
        using table_t = typename TableForClass<T>::type;

        static_assert(table_t::hasPrimaryKey, "Cannot execute a delete on a table without a primary key");

        auto q = table_t::deleteQuery(*dialect_);
        auto [stmtPtr, perr] = conn_->prepare(q);
        if (perr) return perr;
        auto stmt = std::move(stmtPtr);

        using pk_t = typename table_t::PrimaryKey::MemberType;
        if constexpr (IsArithmetic<pk_t>()) {
            if constexpr (std::is_floating_point_v<pk_t>) {
                if (auto e = stmt->bindDouble(1, (double)id)) return e;
            } else {
                if (auto e = stmt->bindInt(1, (long long)id)) return e;
            }
        } else if constexpr (IsString<pk_t>()) {
            if (auto e = stmt->bindText(1, std::string(id))) return e;
        } else {
            return Error("Unsupported PK type for delete");
        }

        if (auto e = stmt->step()) return e;
        return std::nullopt;
    }

private:
    std::unique_ptr<IDatabaseConnection> conn_;
    std::unique_ptr<ISqlDialect> dialect_;

    template<class C>
    struct TableForClass {
        static constexpr int idx = IndexOfFirst<std::is_same<C, typename Table::ObjectClass>::value...>::value;
        static_assert(idx >= 0, "Connection does not contain any table matching the type T");
        using type = typename std::tuple_element<idx, std::tuple<Table...>>::type;
    };
};

} // namespace trantor
