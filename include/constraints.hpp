#pragma once

/**
 * @file constraint.hpp
 * @brief Column constraint utilities for the ORM.
 *
 * Constraints are stored as *types* (default-constructible). Column code
 * constructs a tuple of them and prints them to SQL using operator<<.
 *
 * Notes:
 *  - If you need a runtime-dependent constraint (e.g. DEFAULT from a runtime variable),
 *    implement a custom constraint type and ensure you create/insert it *manually* into
 *    SQL generation (the current system assumes compile-time/default-constructed constraints).
 */

#include <string>
#include <ostream>
#include <type_traits>
#include <utility>

namespace trantor::column_constraint {

/* ------------------------------
 * Basic constraints
 * ------------------------------ */

template<typename Dummy = void>
struct NotNull {
    static constexpr bool isPrimaryKey = false;
    constexpr NotNull() noexcept = default;
    explicit operator std::string() const { return "NOT NULL"; }
};

// --- TRAITS FOR DETECTING CONSTRAINT TYPES ---
template <typename T>
struct is_not_null : std::false_type {};

template <>
struct is_not_null<NotNull<>> : std::true_type {};

template <typename T>
inline constexpr bool is_not_null_v = is_not_null<T>::value;


template<typename Dummy = void>
struct Unique {
    static constexpr bool isPrimaryKey = false;
    constexpr Unique() noexcept = default;
    explicit operator std::string() const { return "UNIQUE"; }
};

/**
 * PrimaryKey
 *
 * A bare PRIMARY KEY marker. Whether it implies AUTOINCREMENT / SERIAL / AUTO_INCREMENT
 * is a dialect decision. Column::isAutoIncColumn already checks SQL type + this flag.
 */
template<typename Dummy = void>
struct PrimaryKey {
    static constexpr bool isPrimaryKey = true;
    constexpr PrimaryKey() noexcept = default;
    explicit operator std::string() const { return "PRIMARY KEY"; }
};

/* ------------------------------
 * DefaultValue for compile-time numeric defaults
 * ------------------------------
 *
 * Usage:
 *   using MyDefault = DefaultValue<42>;
 *
 * This supports arithmetic NTTP (integers/floats). For string defaults or
 * complex defaults prefer a custom constraint type (see Raw below).
 */
template<auto V>
struct DefaultValue {
    static constexpr bool isPrimaryKey = false;
    constexpr DefaultValue() noexcept = default;

    // Only arithmetic supported here for simplicity
    static_assert(std::is_arithmetic_v<decltype(V)>, "DefaultValue only supports arithmetic NTTPs");

    explicit operator std::string() const {
        if constexpr (std::is_floating_point_v<decltype(V)>) {
            return std::string("DEFAULT ") + std::to_string((long double)V);
        } else {
            return std::string("DEFAULT ") + std::to_string((long long)V);
        }
    }
};

/* ------------------------------
 * Raw: base for custom constraints
 * ------------------------------
 *
 * If you need a constraint that the header can't represent (e.g. CHECK, DEFAULT 'str'),
 * implement a tiny constraint type that implements operator std::string().
 *
 * Example:
 *
 *   struct CheckAge {
 *     static constexpr bool isPrimaryKey = false;
 *     explicit operator std::string() const { return "CHECK(age > 0)"; }
 *   };
 *
 * Then pass CheckAge to Column's template parameter pack.
 */
struct Raw {
    static constexpr bool isPrimaryKey = false;
    Raw() noexcept = default;
    explicit Raw(const char* s) : v(s ? s : "") {}
    explicit Raw(std::string s) : v(std::move(s)) {}
    operator std::string() const { return v; }

private:
    std::string v;
};

/* ------------------------------
 * Trait: ConstraintIsPrimaryKey
 * ------------------------------
 *
 * SFINAE-friendly trait that returns true if the constraint type exposes
 * a compile-time `isPrimaryKey` boolean member.
 */
template<typename C, typename = void>
struct ConstraintIsPrimaryKey {
    static constexpr bool value = false;
};

template<typename C>
struct ConstraintIsPrimaryKey<C, std::void_t<decltype(C::isPrimaryKey)>> {
    static constexpr bool value = C::isPrimaryKey;
};

/* ------------------------------
 * Generic stream operator for constraints
 * ------------------------------
 *
 * Many constraint types above have `operator std::string()`. This generic
 * overload prints such constraints. It participates in overload resolution
 * only if `C` is convertible to std::string.
 */
// template for types convertible to std::string BUT NOT std::string itself
template<typename C>
auto operator<<(std::ostream &os, const C &c)
    -> std::enable_if_t<
           !std::is_same_v<std::decay_t<C>, std::string> &&
            std::is_convertible_v<C, std::string>,
           std::ostream &>
{
    os << static_cast<std::string>(c);
    return os;
}

/* ------------------------------
 * Helpful aliases (optional)
 * ------------------------------ */

using NotNull_t = NotNull<>;
using Unique_t = Unique<>;
using PrimaryKey_t = PrimaryKey<>;

} // namespace trantor::column_constraint
