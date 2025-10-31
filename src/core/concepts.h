#pragma once
#include <type_traits>

namespace core::concepts {

    template<typename T>
    concept PromiseValue =
        std::is_move_constructible_v<T> &&
        std::is_destructible_v<T> &&
        !std::is_reference_v<T>;


    template<typename T>
    concept FutureValue =
            std::is_copy_constructible_v<T> && std::is_move_constructible_v<T>
            || std::is_void_v<T>;
}
