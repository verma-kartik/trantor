#pragma once

/**
 * @file types.hpp
 * @brief Contains utility functions and type traits related to SQL operations.
 */

#include <type_traits>
#include <string_view>

namespace trantor{

    enum class sql_type_t{
        INTEGER,
        TEXT,
        BLOB,
        NUMERIC,
        REAL
    };

    /**
     * NOTE: Changed from `consteval` to `constexpr` so dialect code can call this at runtime.
     * `constexpr` still allows compile-time evaluation where possible.
     */
    static constexpr const char* sqlTypesStr(sql_type_t t) {
        switch (t) {
                case sql_type_t::INTEGER: return "INTEGER";
                case sql_type_t::TEXT:    return "TEXT";
                case sql_type_t::BLOB:    return "BLOB";
                case sql_type_t::NUMERIC: return "NUMERIC";
                case sql_type_t::REAL:    return "REAL";
                default:                  return "not a supported type";
        }
    }

    enum class order_t{
        NONE,
        ASC,
        DESC
    };

    static constexpr const char* orderStr(order_t t){
        switch (t) {
            case order_t::NONE: return "";
            case order_t::ASC:  return "ASC";
            case order_t::DESC: return "DESC";
            default:            return "order not supported";
        }
    }

    // Forward dependency: remove_optional, IsArithmetic, IsString, IsContinuousContainer
    // are assumed available from your common.hpp. MemberTypeToSqlType relies on them.
    template<typename T>
    struct MemberTypeToSqlType {
    private:
        static constexpr sql_type_t findType() {
            using type = typename remove_optional<T>::type;
            if constexpr (IsArithmetic<type>()) {
                if (std::is_floating_point_v<type>) {
                    return sql_type_t::REAL;
                } else {
                    return sql_type_t::INTEGER;
                }
            } else if constexpr (IsString<type>()) {
                return sql_type_t::TEXT;
            } else if constexpr (IsContinuousContainer<type>()) {
                return sql_type_t::BLOB;
            } else {
                static_assert(std::is_same_v<T, std::false_type>,
                              "Member type is not convertible to an sql type. "
                              "Please use only arithmetic types or continuous containers, "
                              "or std::optional containing either type");
            }
        }
    public:
        static constexpr sql_type_t value = findType();
    };

    // === Extension example: map custom types (uncomment & adapt as needed) ===
    /*
    // Example: map time_point to INTEGER (unix epoch) — choose appropriate SQL type
    #include <chrono>
    template<>
    struct MemberTypeToSqlType<std::chrono::system_clock::time_point> {
        static constexpr sql_type_t value = sql_type_t::INTEGER;
    };
    */

} // namespace trantor
