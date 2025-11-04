#pragma once

/**
 * @file concepts.h
 * @brief Defines type concepts used for validating Promise and Future value types.
 */

#include <type_traits>

namespace rts::core::concepts {

    /**
     * @brief A type is a valid Promise value if it is move-constructible, destructible, and not a reference.
     */
    template<typename T>
    concept PromiseValue =
            std::is_move_constructible_v<T> &&
            std::is_destructible_v<T> &&
            !std::is_reference_v<T>;

    /**
     * @brief A type is a valid Future value if it is copy-constructible, move-constructible, or void.
     */
    template<typename T>
    concept FutureValue =
            (std::is_copy_constructible_v<T> &&
             std::is_move_constructible_v<T>) ||
            std::is_void_v<T>;
} // namespace rts::core::concepts
