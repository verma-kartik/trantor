#pragma once

#include "common.hpp"
#include "constraints.hpp"
#include <sstream>
#include <tuple>
#include <optional>
#include <type_traits>

namespace trantor{

    namespace _constraint_t_detail{
        template<typename MemberT, typename... C>
        struct constraints_t {
            using type = unique_tuple<C...>;
        };

        // template <typename MemberT, typename... C>
        // requires (not is_optional<MemberT>::value)
        // struct constraints_t<MemberT, C...> {
        //     using type = unique_tuple<column_constraint::NotNull<>, C...>;
        // };

        // specialization: add NotNull automatically if type not optional
        template <typename MemberT, typename... C>
        requires (!is_optional<MemberT>::value && !(column_constraint::is_not_null_v<C> || ...))
        struct constraints_t<MemberT, C...> {
            using type = unique_tuple<column_constraint::NotNull<>, C...>;
        };

        // turn tuple-of-constraints into a single SQL fragment (no trailing space/comma)
        static inline std::string constraintCreationQuery(auto constraints) {
            std::stringstream ss;
            bool first = true;
            std::apply([&](const auto&... c) {
                (([&]{
                    auto fragment = std::string(c);
                    if (!fragment.empty()) {
                        if (!first) ss << " ";
                        ss << fragment;
                        first = false;
                    }
                }()), ...);
            }, constraints);

            return ss.str();
        }
    }

    // Helper: extract types from member-function or member-pointer
    template<typename T>
    struct resolve_function_ptr_types : std::false_type {
        using argType = std::false_type;
        using returnType = void;
        using klass = std::false_type;
    };

    // setter: void (C::*)(A) and cv/ref variants
    template<typename C, typename A>
    struct resolve_function_ptr_types<void (C::*)(A)> {
        using argType = A;
        using returnType = void;
        using klass = C;
    };
    template<typename C, typename A>
    struct resolve_function_ptr_types<void (C::*)(A) &> : resolve_function_ptr_types<void (C::*)(A)> {};
    template<typename C, typename A>
    struct resolve_function_ptr_types<void (C::*)(A) &&> : resolve_function_ptr_types<void (C::*)(A)> {};
    template<typename C, typename A>
    struct resolve_function_ptr_types<void (C::*)(A) const> : std::false_type {}; // setter shouldn't be const-qualified

    // getter: R (C::*)() and const variant
    template<typename R, typename C>
    struct resolve_function_ptr_types<R (C::*)()> {
        using argType = std::false_type;
        using returnType = R;
        using klass = C;
    };
    template<typename R, typename C>
    struct resolve_function_ptr_types<R (C::*)() const> {
        using argType = std::false_type;
        using returnType = R;
        using klass = C;
    };

    // pointer-to-member: R C::*
    template<typename R, typename C>
    struct resolve_function_ptr_types<R C::*> {
        using argType = std::false_type;
        using returnType = void;
        using klass = std::false_type;
    };


    /**
     * ColumnP: column backed by getter/setter methods.
     *
     * Usage:
     *   ColumnP<"col", &MyClass::getX, &MyClass::setX, Constraint...>
     *
     * Both Getter and Setter are required (no default setter).
     */
    template <trantor::FixedLengthString columnName, auto Getter, auto Setter, class... Constraint>
    class ColumnP {
    private:
        using SetterResolved = resolve_function_ptr_types<decltype(Setter)>;
        using GetterResolved = resolve_function_ptr_types<decltype(Getter)>;

        static_assert(!std::is_same<typename SetterResolved::argType, std::false_type>::value,
                      "ColumnP: Setter must be a pointer to a non-const member function with signature void C::(Arg)");
        static_assert(std::is_same<typename SetterResolved::returnType, void>::value,
                      "ColumnP: Setter must return void");

        static_assert(std::is_same<typename GetterResolved::argType, std::false_type>::value,
                      "ColumnP: Getter must be a pointer to a member function taking no arguments");
        static_assert(!std::is_same<typename GetterResolved::returnType, void>::value,
                      "ColumnP: Getter must return a non-void type");

        static_assert(std::is_same<typename GetterResolved::returnType, typename SetterResolved::argType>::value,
                      "ColumnP: Getter return type must match Setter parameter type");

    public:
        static constexpr bool publicColumn = false;
        using MemberType = typename SetterResolved::argType;
        using ObjectClass = typename SetterResolved::klass;
        static constexpr bool isPrimaryKey = AnyOf<column_constraint::ConstraintIsPrimaryKey<Constraint>::value...>;
        using constraints_t = typename _constraint_t_detail::constraints_t<MemberType, Constraint...>::type;

        static constexpr const char *name() {
            return columnName.value;
        }

        static constexpr trantor::sql_type_t SQLMemberType = trantor::MemberTypeToSqlType<MemberType>::value;
        static constexpr bool isAutoIncColumn =
                AnyOf<column_constraint::ConstraintIsPrimaryKey<Constraint>::value...> &&
                SQLMemberType == sql_type_t::INTEGER;

        static auto &getter(auto &obj) {
            return (obj.*Getter)();
        }

        static void setter(auto &obj, auto arg) {
            (obj.*Setter)(arg);
        }

        static std::string constraintCreationQuery() {
            return _constraint_t_detail::constraintCreationQuery(constraints_t{});
        }
    };


    /**
     * Column: column backed by a pointer-to-data-member.
     *
     * Usage:
     *   Column<"col", &MyClass::member, Constraint...>
     */
    template <trantor::FixedLengthString columnName, auto M, class... Constraint>
    class Column{
    private:
        template<typename T>
        struct find_column_type : std::false_type {
            using type = std::false_type;
        };

        template<typename R, typename C, class A>
        struct find_column_type<R (C::*)(A)> { using type = std::false_type; };

        template<typename R, typename C>
        struct find_column_type<R C::*> {
            using type = R;
            using klass = C;
        };

    public:
        static constexpr bool publicColumn = true;
        using MemberType = typename find_column_type<decltype(M)>::type;

        static_assert(!std::is_same<MemberType, std::false_type>::value,
                      "Column: template argument must be a pointer to a class member");

        using ObjectClass = typename find_column_type<decltype(M)>::klass;
        using constraints_t = typename _constraint_t_detail::constraints_t<MemberType, Constraint...>::type;
        static constexpr sql_type_t SQLMemberType = MemberTypeToSqlType<MemberType>::value;
        static constexpr bool isPrimaryKey = AnyOf<column_constraint::ConstraintIsPrimaryKey<Constraint>::value...>;
        static constexpr bool isAutoIncColumn = AnyOf<column_constraint::ConstraintIsPrimaryKey<Constraint>::value...> && SQLMemberType == sql_type_t::INTEGER;

        static constexpr const char* name(){
            return columnName.value;
        }

        static auto& getter(auto& obj){
            return obj.*M;
        };

        static void setter(auto& obj, auto arg){
            obj.*M = arg;
        };

        static std::string constraintCreationQuery() {
            return _constraint_t_detail::constraintCreationQuery(constraints_t{});
        }
    };
} // namespace trantor
