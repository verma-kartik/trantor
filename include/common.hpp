#pragma once

#include <functional>
#include <string>
#include <optional>
#include <variant>

namespace trantor
{
    /**
     * @enum LogLevel
     *
     * @brief Describes various levels of Log types
     *
     */
    enum class LogLevel{
        error = 0,
        warning = 1,
        info = 2,
        debug = 3
    };
    
    using Logger = std::function<void(LogLevel, const std::string)>;

    struct Error{
        using ErrStringProvider = std::function<std::string(int)>;

        std::string err_msg;
        int result_code;
        ErrStringProvider provider; 

        Error(const std::string err,
            int result_code = 0,
            ErrStringProvider provider = nullptr) : err_msg(err), result_code(result_code), provider(provider) {}

        /**
        * @brief Stream insertion operator to output an Error.
        */

        friend std::ostream& operator<<(std::ostream &out, const Error &e) {
            out << e.err_msg;
            if (e.result_code != 0) {
                if (e.provider) {
                    out << ": " << e.provider(e.result_code);
                } else {
                    out << ": code=" << e.result_code;
                }
            }
            return out;
        }

    };

    template<typename T>
    using Maybe = std::variant<Error, T>;

    /**
     * @brief A templated structure to store a fixed-length character array.
     *
     * This structure provides a way to store a fixed-length character array of size N.
     * It is particularly useful for storing the names of tables and columns.
     *
     * The constructor accepts a reference to a string literal (const char (&str)[N]),
     * ensuring only compile-time string literals can be passed.
     *
     * @tparam N The size of the fixed-length character array.
     */
    template<size_t N>
    struct FixedLengthString {
        constexpr FixedLengthString(const char (&str)[N]) {
            std::copy_n(str, N, value);
        }
        char value[N]{};
    };

    template<bool... T>
    static constexpr bool AnyOf = (... || T);

    template<bool... T>
    static constexpr bool AllOf = (... && T);

    /*
    TYPE TRAITS
    */

    //By default, it inherits from std::false_type → meaning "No, T is not a std::vector".
    template<typename T>
    struct is_vector : std::false_type  { };

    template<typename T, typename A>
    struct is_vector<std::vector<T, A>> : std::true_type {};

    template<typename T>
    struct is_basic_string : std::false_type  { };

    template<typename CharT, typename Traits, typename Allocator>
    struct is_basic_string<std::basic_string<CharT, Traits, Allocator>> : std::true_type {};

    template<typename T>
    struct is_string : std::false_type  { };

    template<typename CharT, typename Traits, typename Allocator>
    struct is_string<std::basic_string<CharT, Traits, Allocator>> : std::true_type {};

    template<typename T>
    struct is_array : std::false_type  { };

    template<typename T, auto s>
    struct is_array<std::array<T, s>> : std::true_type {};

    template<typename T>
    struct is_optional : std::false_type  { };

    template<typename T>
    struct is_optional<std::optional<T>> : std::true_type {};

    template<typename T>
    struct remove_optional : std::type_identity<T> {};

    template<typename T>
    struct remove_optional<std::optional<T>> : std::type_identity<T> {};


    /**
     * @brief Checks whether a given type is an arithmetic type.
     * An arithmetic type is one that represents a numeric value
     * and supports arithmetic operations.
     *
     * @tparam T The type to be checked.
     * @return `true` if the type is an arithmetic type, `false` otherwise.
     */
    template<typename T>
    static constexpr bool IsArithmetic() {
        using plain = remove_optional<std::remove_cvref_t<T>>::type;
        return std::is_arithmetic_v<plain>;
    }

    template<typename T>
    concept ArithmeticT = IsArithmetic<T>();


    /**
     * @brief Checks whether a given type is an optional type.
     *
     * @tparam T The type to be checked.
     * @return `true` if the type is an optional type, `false` otherwise.
     */
    template<typename T>
    static constexpr bool IsOptional() {
        using plain = std::remove_cvref_t<T>;
        return is_optional<plain>::value;
    }

    template<typename T>
    concept OptionalT = IsOptional<T>();



    /**
     * @brief Checks whether a given type is a continuous container type.
     * A continuous container is one that can store a sequence of
     * elements and provides a continuous memory layout.
     *
     * @tparam T    The type to be checked.
     * @return `true` if the type is a continuous container type, `false` otherwise.
     */
    template<typename T>
    static constexpr bool IsContinuousContainer() {
        using plain = remove_optional<std::remove_cvref_t<T>>::type;
        return is_vector<plain>::value || is_basic_string<plain>::value || is_array<plain>::value;
    }

    template<typename T>
    concept ContinuousContainer = IsContinuousContainer<T>();


    template <typename T>
    static constexpr bool IsString() {
        using plain = remove_optional<std::remove_cvref_t<T>>::type;
        return is_string<plain>::value;
    }

    template<typename T>
    concept StringT = IsString<T>();

    // unique tuple https://stackoverflow.com/a/57528226
    namespace _unique_tuple_detail {
        template <typename T, typename... Ts>
        struct unique : std::type_identity<T> {};

        template <typename... Ts, typename U, typename... Us>
        struct unique<std::tuple<Ts...>, U, Us...>
                : std::conditional_t<(std::is_same_v<U, Ts> || ...)
                        , unique<std::tuple<Ts...>, Us...>
                        , unique<std::tuple<Ts..., U>, Us...>> {};
    };

    template <typename... Ts>
    using unique_tuple = typename _unique_tuple_detail::unique<std::tuple<>, Ts...>::type;

    template<bool... T>
    struct IndexOfFirst {
    private:
        static constexpr int _impl() {
            constexpr std::array<bool, sizeof...(T)> a{T...};
            const auto it = std::find(a.begin(), a.end(), true);

            // As we are in constant expression, we will have compilation error.
            if (it == a.end()) return -1;

            return std::distance(a.begin(), it);
        }
    public:
        static constexpr int value = _impl();
    };

} // namespace trantor
