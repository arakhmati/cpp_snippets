# reflection

Reflection for printing and hashing the structs that implement `attribute_names` and `attibute_values` methods

[Demo](https://godbolt.org/z/M84Kdn1sK)

```cpp
#include <cassert>
#include <sstream>
#include <experimental/type_traits>

template <class>
inline constexpr bool always_false_v = false;

template <char... chars>
using attribute_name_t = std::integer_sequence<char, chars...>;

template <typename T, T... chars>
constexpr attribute_name_t<chars...> operator""_attr() { return { }; }

template <char... chars>
std::ostream& operator<<(std::ostream& os, const attribute_name_t<chars...>& object) {
    ((os << chars), ...);
    return os;
}

template <typename T, typename = std::void_t<>>
struct is_std_hashable : std::false_type {};

template <typename T>
struct is_std_hashable<T, std::void_t<decltype(std::declval<std::hash<T>>()(std::declval<T>()))>> : std::true_type {};

template <typename T>
constexpr bool is_std_hashable_v = is_std_hashable<T>::value;

template <typename T>
using has_attribute_names_t = decltype(std::declval<T>().attribute_names);

template <typename T>
using has_attribute_values_t = decltype(std::declval<T>().attribute_values());

template <typename T>
constexpr bool supports_compile_time_attributes_v = std::experimental::is_detected_v<has_attribute_names_t, T> and
                                                    std::experimental::is_detected_v<has_attribute_values_t, T>;

template<typename T>
inline constexpr std::size_t get_num_attributes() {
    return std::tuple_size_v<decltype(T::attribute_names)>;
}

template <typename T>
typename std::enable_if_t<supports_compile_time_attributes_v<T>, std::ostream>& operator<<(
    std::ostream& os, const T& object) {
    constexpr auto num_attributes = get_num_attributes<T>();

    os << "(";

    [&os, &object]<size_t... Ns>(std::index_sequence<Ns...>) {
        ([&os, &object] {
            const auto& attribute = std::get<Ns>(object.attribute_values());
            using attribute_t = std::decay_t<decltype(attribute)>;


            os << std::get<Ns>(object.attribute_names);
            os << "=";

            if constexpr (std::is_same_v<attribute_t, bool>) {
                os << (attribute ? "true" : "false");
            } else {
                os << attribute;
            }


            if constexpr (Ns < num_attributes - 1) {
                os << ",";
            }
        }
        (), ...);
    }(std::make_index_sequence<num_attributes>{});

    os << ")";
    return os;
}

template<typename T>
constexpr inline std::size_t hash_object(const T& object) {
    constexpr auto num_attributes = get_num_attributes<T>();

    std::size_t seed = 0;
    [&object, &seed]<size_t... Ns>(std::index_sequence<Ns...>) {
        (
            [&object, &seed] {
            const auto& attribute = std::get<Ns>(object.attribute_values());
                std::size_t hash;
                if constexpr (is_std_hashable_v<std::decay_t<decltype(attribute)>>) {
                    hash = std::hash<std::decay_t<decltype(attribute)>>{}(attribute);
                } else {
                    hash = hash_object(attribute);
                }
                seed = hash + 0x9e3779b9 + (seed << 6) + (seed >> 2);
            }(), 
            ...
        );
    }(std::make_index_sequence<num_attributes>{});
    return seed;
}


struct memory_config_t {

    int memory_layout;
    bool buffer_type;

    static constexpr auto attribute_names = std::make_tuple(
        "memory_layout",
        "buffer_type"_attr
    );

    constexpr auto attribute_values () const {
        return std::forward_as_tuple(
            memory_layout,
            buffer_type
        );
    }
};


struct operation_t {

    std::string name;
    int param;
    memory_config_t memory_config;
    std::string output_dtype;

    static constexpr auto attribute_names = std::make_tuple(
        "name"_attr,
        "param"_attr,
        "memory_config"_attr,
        "output_dtype"_attr
    );

    constexpr auto attribute_values () const {
        return std::forward_as_tuple(
            name,
            param,
            memory_config,
            output_dtype
        );
    }
};


int main (int argc, char**argv) {

    auto operation = operation_t{"matmul", argc, memory_config_t{5, false}, "bfloat16"};
    auto& memory_config = operation.memory_config;

    {
        std::stringstream ss;
        ss << operation;
        assert(ss.str() == "(name=matmul,param=1,memory_config=(memory_layout=5,buffer_type=false),output_dtype=bfloat16)");
    }

    {
        std::stringstream ss;
        ss << operation.memory_config;
        assert(ss.str() == "(memory_layout=5,buffer_type=false)");
    }

    assert(hash_object(operation) == 7910179190541357550);
    assert(hash_object(operation.memory_config) == 173201934248);

    return 0;
}
```
