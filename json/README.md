# reflection

Convert C++ objects to and from JSON

[Demo](https://godbolt.org/z/PhchGEMGb)

```cpp
#include <https://raw.githubusercontent.com/boost-ext/reflect/main/reflect>
#include <nlohmann/json.hpp>

#include <map>
#include <string_view>
#include <vector>
#include <variant>

using std::literals::operator""sv;

struct some_type {
    std::variant<std::optional<int>, float> a; 
    std::optional<float> b;
};

struct another_type {
    int c;
    float d;
};


struct composite_type {
    std::variant<some_type, another_type> attribute;
};

static_assert(std::is_aggregate_v<some_type>);
static_assert(std::is_aggregate_v<another_type>);
static_assert(std::is_aggregate_v<composite_type>);

template <typename... Ts>
[[nodiscard]] std::variant<Ts...> map_index_to_variant(std::size_t i, std::variant<Ts...>) {
    assert(i < sizeof...(Ts));
    static constexpr std::variant<Ts...> table[] = {Ts{}...};
    return table[i];
}

namespace json {

template<typename T>
struct to_json_t;

nlohmann::json to_json(const auto& object) {
    return to_json_t<std::decay_t<decltype(object)>>{}(object);
}

template<typename T>
struct from_json_t;


template<typename T>
T from_json(const nlohmann::json& json_object) {
    return from_json_t<T>{}(json_object);
}


template <typename T>
    requires std::is_integral_v<T> or std::is_floating_point_v<T> or std::is_enum_v<T>
struct to_json_t<T> {
    nlohmann::json operator()(T object) noexcept { return object; }
};

template <typename T>
    requires std::is_integral_v<T> or std::is_floating_point_v<T> or std::is_enum_v<T>
struct from_json_t<T> {
    T operator()(const nlohmann::json& json_object) noexcept { return json_object.get<T>(); }
};


template <>
struct to_json_t<const char*> {
    nlohmann::json operator()(const char* object) noexcept { return object; }
};

template <>
struct from_json_t<const char*> {
    const char* operator()(const nlohmann::json& json_object) { 
        throw std::runtime_error("Cannot load const char* from JSON");
    }
};

template <>
struct to_json_t<std::string> {
    nlohmann::json operator()(const std::string& object) noexcept { return object; }
};


template <>
struct from_json_t<std::string> {
    std::string operator()(const nlohmann::json& json_object) noexcept { return json_object.get<std::string>(); }
};


template<typename T>
struct to_json_t<std::vector<T>> {
    nlohmann::json operator()(const std::vector<T>& object) {
        nlohmann::json json_array = nlohmann::json::array();
        for (const auto& element : object) {
            json_array.push_back(to_json(element));
        }
        return json_array;
    }

};

template <typename T>
struct from_json_t<std::vector<T>> {
    std::vector<T> operator()(const nlohmann::json& json_object) noexcept {
        std::vector<T> vector;
        for (const auto& element : json_object) {
            vector.push_back(from_json<T>(element));
        }
        return vector;
    }
};

template<typename K, typename V>
struct to_json_t<std::map<K, V>> {
    nlohmann::json operator()(const std::map<K, V>& object) {
        nlohmann::json json_object = nlohmann::json::object();
        for (const auto& [key, value] : object) {
            json_object[to_json(key)] = to_json(value);
        }
        return json_object;
    }
};

template<typename K, typename V>
struct from_json_t<std::map<K, V>> {
    std::map<K, V> operator()(const nlohmann::json& json_object) {
        std::map<K, V> object;
        for (const auto& [key, value] : json_object.items()) {
            object[from_json<K>(key)] = from_json<V>(value);
        }
        return object;
    }
};


template <typename T>
struct to_json_t<std::optional<T>> {
    nlohmann::json operator()(const std::optional<T>& optional) noexcept {
        if (optional.has_value()) {
            return to_json(optional.value());
        } else {
            return nullptr;
        }
    }
};

template <typename T>
struct from_json_t<std::optional<T>> {
    std::optional<T> operator()(const nlohmann::json& json_object) noexcept {
        if (json_object.is_null()) {
            return std::nullopt;
        } else {
            return from_json<T>(json_object);
        }
    }
};

template <typename... Ts>
struct to_json_t<std::variant<Ts...>> {
    nlohmann::json operator()(const std::variant<Ts...>& variant) noexcept {
        return std::visit([index = variant.index()](const auto& value) -> nlohmann::json { 
            nlohmann::json json_object = nlohmann::json::object();
            return {{"index", index}, {"value", to_json(value)}}; 
            },
        variant);
    }
};

template <class variant_t, std::size_t I = 0>
variant_t variant_from_index(std::size_t index, const nlohmann::json& json_object) {
    if constexpr(I >= std::variant_size_v<variant_t>)
        throw std::runtime_error{"Variant index " + std::to_string(I + index) + " out of bounds"};
    else
        return index == 0
            ? from_json<std::variant_alternative_t<I, variant_t>>(json_object)
            : variant_from_index<variant_t, I + 1>(index - 1, json_object);
}

template <typename... Ts>
struct from_json_t<std::variant<Ts...>> {
    std::variant<Ts...> operator()(const nlohmann::json& json_object) {
        auto index = json_object["index"].get<std::size_t>();
        return variant_from_index<std::variant<Ts...>>(index, json_object["value"]);
    }
};

template <typename... Ts>
struct to_json_t<std::tuple<Ts...>> {
    nlohmann::json operator()(const std::tuple<Ts...>& tuple) noexcept {
        nlohmann::json json_array = nlohmann::json::array();
        [&tuple, &json_array]<std::size_t... Ns>(std::index_sequence<Ns...>) {
            (
                [&tuple, &json_array] {
                    const auto& element = std::get<Ns>(tuple);
                    json_array.push_back(to_json(element));
                }(),
                ...);
        }(std::make_index_sequence<sizeof...(Ts)>{});
        return json_array;
    }
};



template <typename... Ts>
struct from_json_t<std::tuple<Ts...>> {
    std::tuple<Ts...> operator()(const nlohmann::json& json_object) noexcept {
        std::tuple<Ts...> tuple;
        [&tuple, &json_object]<std::size_t... Ns>(std::index_sequence<Ns...>) {
            (
                [&tuple, &json_object] {
                    const auto& element = json_object[Ns];
                    std::get<Ns>(tuple) = from_json<Ts>(element);
                }(),
                ...);
        }(std::make_index_sequence<sizeof...(Ts)>{});
        return tuple;
    }
};


template<typename T>
concept Reflectable = (std::is_aggregate_v<std::decay_t<T>> and requires {
                                 reflect::for_each(
                                     [](auto I) { }, std::declval<T>());
                             });

template <typename T>
    requires Reflectable<T>
struct to_json_t<T> {
    nlohmann::json operator()(const T& object) {
        nlohmann::json json_object = nlohmann::json::object();
        reflect::for_each([&json_object, &object](auto I) {
            json_object.emplace(reflect::member_name<I>(object), to_json(reflect::get<I>(object)));
        }, object);
        return json_object; 
    }

};

template <typename T>
    requires Reflectable<T>
struct from_json_t<T> {
    T operator()(const nlohmann::json& json_object) noexcept {
        T object;
        reflect::for_each(
            [&object, &json_object](auto I) {
                const auto& attribute_name = reflect::member_name<I>(object);
                const auto& attribute = reflect::get<I>(object);
                if (json_object.contains(attribute_name)) {
                    reflect::get<I>(object) = from_json<std::decay_t<decltype(attribute)>>(json_object[attribute_name]);
                }
            },
            object);
        return object;
    }
};

template <typename T>
struct to_json_t {
    nlohmann::json operator()(const T& optional) noexcept { return "Unsupported type"; }
};



} // namespace json 


#include <iostream>
int main() {
    auto object =  std::tuple{
        std::tuple{std::string{"first"}, std::vector{some_type{.a=1.0f, .b=5}, some_type{std::nullopt, 2}}},
        std::tuple{std::string{"second"}, (another_type{.c = 42, .d = 123.0f}), (composite_type{some_type{3, 2}})}
    };
    nlohmann::json serialized_object = json::to_json(object);
    std::cout << serialized_object.dump(4) << std::endl;

    auto deserialized_object = json::from_json<decltype(object)>(serialized_object);

    std::cout << json::to_json(deserialized_object).dump(4) << std::endl;

    assert(serialized_object.dump(4) == json::to_json(deserialized_object).dump(4));

    return 0;
}
```
