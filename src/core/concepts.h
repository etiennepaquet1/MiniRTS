#pragma once
#include <type_traits>

namespace rts::concepts {

    template<typename T>
    concept PromiseValue =
        std::is_move_constructible_v<T> &&
        std::is_destructible_v<T> &&
        !std::is_reference_v<T>;

}
