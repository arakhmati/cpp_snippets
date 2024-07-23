# JSON

Force the struct to have common path for getting and setting an attribute

[Demo](https://godbolt.org/z/jrG6zrnTx)

```cpp
#include <https://raw.githubusercontent.com/boost-ext/reflect/main/reflect>

#include <iostream>
#include <optional>
#include <filesystem>

struct config_t {
    struct attributes_t {
        int a;
        float b;
        std::filesystem::path root_report_path;
        std::optional<std::filesystem::path> report_name;
    };

  private:
    attributes_t attributes;

  public:

    config_t(auto&& ... args) : attributes{std::forward<decltype(args)>(args)...} {}

    template <reflect::fixed_string name>
        requires requires { reflect::get<name>(std::declval<attributes_t>()); }
    const auto get() const {
        return reflect::get<name>(this->attributes);
    }

    template <std::size_t index>
    const auto get() const {
        return reflect::get<index>(this->attributes);
    }

    template <reflect::fixed_string name>
        requires(name == reflect::fixed_string{"report_path"})
    const std::optional<std::filesystem::path> get() const {
        if (this->attributes.report_name.has_value()) {
            auto hash = std::hash<std::string>{}(this->attributes.report_name.value());
            return this->attributes.root_report_path / std::to_string(hash);
        }
        return std::nullopt;
    }

    template <
        reflect::fixed_string name,
        typename T = std::decay_t<decltype(reflect::get<name>(std::declval<attributes_t>()))>>
    void set(const T& value) {
        reflect::get<name>(this->attributes) = value;
        this->validate(name);
    }

    template <
        std::size_t index,
        typename T = std::decay_t<decltype(reflect::get<index>(std::declval<attributes_t>()))>>
    void set(const T& value) {
        reflect::get<index>(this->attributes) = value;
        this->validate(reflect::member_name<index, attributes_t>());
    }

    void validate(std::string_view name) {}
};

void print_config_using_names(const config_t& config) {
    std::cout << "Config: ";
    std::cout << config.get<"a">() << ", ";
    std::cout << config.get<"b">() << ", ";
    std::cout << config.get<"root_report_path">().c_str() << ", ";
    std::cout << config.get<"report_name">()->c_str() << ", ";
    std::cout << config.get<"report_path">()->c_str();
    std::cout << std::endl;
}

void print_config_using_indices(const config_t& config) {
    std::cout << "Config: ";
    std::cout << config.get<0>() << ", ";
    std::cout << config.get<1>() << ", ";
    std::cout << config.get<2>().c_str() << ", ";
    std::cout << config.get<3>()->c_str() << ", ";
    std::cout << config.get<"report_path">()->c_str();
    std::cout << std::endl;
}


int main () {

    config_t config{4, 6.3f, "root", "default"};
    print_config_using_names(config);

    config.set<"a">(5);
    config.set<"b">(8.5f);
    config.set<"report_name">("fgh");
    config.set<"root_report_path">("abc");
    print_config_using_names(config);

    config.set<0>(9);
    config.set<1>(3.4);
    config.set<2>("qwerty");
    config.set<3>("jkl");
    print_config_using_indices(config);

    return 0;
}
```
