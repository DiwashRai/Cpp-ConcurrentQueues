#ifndef QUEUETYPETRAITS_H
#define QUEUETYPETRAITS_H

#include <type_traits>

template<typename T>
struct is_bounded : std::false_type {};

template<typename T>
constexpr bool is_bounded_v = is_bounded<T>::value;

#endif //QUEUETYPETRAITS_H
