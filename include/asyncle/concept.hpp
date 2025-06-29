#ifndef ASYNCLE_VAL_HPP
#define ASYNCLE_VAL_HPP

#include <concepts>

namespace asyncle {

template <typename T, typename U = int>
concept val = std::convertible_to<T, U>;

}

#endif
