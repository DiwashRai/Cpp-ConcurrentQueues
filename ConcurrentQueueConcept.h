#ifndef QUEUECONCEPT_H
#define QUEUECONCEPT_H

#include <concepts>

/**
 * @brief Concurrent Queue concept
 *
 * Will support the following operations:
 *  - push -> bool
 *  - try_push -> bool
 *  - pop -> bool
 *  - try_pop -> bool
 */
template<typename T>
concept ConcurrentQueue = requires(T queue) {
    { queue.push(std::declval<typename T::value_type>()) } -> std::same_as<bool>;
    { queue.try_push(std::declval<typename T::value_type>()) } -> std::same_as<bool>;
    { queue.pop(std::declval<typename T::value_type&>()) } -> std::same_as<bool>;
    { queue.try_pop(std::declval<typename T::value_type&>()) } -> std::same_as<bool>;
};

#endif //QUEUECONCEPT_H
