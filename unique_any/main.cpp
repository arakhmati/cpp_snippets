
#include <any>
#include <array>
#include <iostream>

template <typename ...T>
inline constexpr bool always_false_v = false;

struct unique_any final {
    static constexpr std::size_t ALIGNMENT = 32;
    static constexpr std::size_t MAX_STORAGE_SIZE = 256;
    using storage_t = std::array<std::byte, MAX_STORAGE_SIZE>;

    template <typename Type, typename BaseType = std::decay_t<Type>>
    unique_any(Type&& object) :
        pointer{new(&type_erased_storage) BaseType{std::forward<Type>(object)}},
        delete_storage{[](storage_t& self) { reinterpret_cast<BaseType*>(&self)->~BaseType(); }},
        move_storage{[](storage_t& self, void* other) -> void* {
            if constexpr (std::is_move_constructible_v<BaseType>) {
                return new (&self) BaseType{std::move(*reinterpret_cast<BaseType*>(other))};
            } else {
                static_assert(always_false_v<BaseType>);
            }
        }}
        {
        static_assert(sizeof(BaseType) <= MAX_STORAGE_SIZE);
        static_assert(ALIGNMENT % alignof(BaseType) == 0);
    }

    void destruct() noexcept {
        if (this->pointer) {
            this->delete_storage(this->type_erased_storage);
        }
        this->pointer = nullptr;
    }

    unique_any(const unique_any& other) = delete;
    unique_any& operator=(const unique_any& other) = delete;

    unique_any(unique_any&& other) :
        pointer{other.pointer ? other.move_storage(this->type_erased_storage, other.pointer) : nullptr},
        delete_storage{other.delete_storage},
        move_storage{other.move_storage} {}

    unique_any& operator=(unique_any&& other) {
        if (other.pointer != this->pointer) {
            this->destruct();
            this->pointer = nullptr;
            if (other.pointer) {
                this->pointer = other.move_storage(this->type_erased_storage, other.pointer);
            }
            this->delete_storage = other.delete_storage;
            this->move_storage = other.move_storage;
        }
        return *this;
    }

    ~unique_any() { this->destruct(); }

    template<typename T>
    T& get() {
        return  *reinterpret_cast<T*>(&type_erased_storage);
    }
    
    template<typename T>
    const T& get() const {
        return  *reinterpret_cast<const T*>(&type_erased_storage);
    }

   private:
    alignas(ALIGNMENT) void* pointer = nullptr;
    alignas(ALIGNMENT) storage_t type_erased_storage;

    void (*delete_storage)(storage_t&) = nullptr;
    void* (*move_storage)(storage_t& storage, void*) = nullptr;
};


struct NonCopyable {

    int value;

    NonCopyable(int value) : value{value} {}

    NonCopyable(const NonCopyable &other) = delete;
    NonCopyable &operator=(const NonCopyable &other) = delete;

    NonCopyable(NonCopyable &&other) = default;
    NonCopyable& operator=(NonCopyable &&other) = default;
};

int main(int argc, char** argv) {
    unique_any any = NonCopyable{5};
    std::cout << any.get<NonCopyable>().value << std::endl;
    any = 23.5f;
    std::cout << any.get<float>() << std::endl;
    return 0;
}
