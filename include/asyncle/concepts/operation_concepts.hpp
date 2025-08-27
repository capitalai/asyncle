#ifndef ASYNCLE_OPERATION_CONCEPTS_HPP
#define ASYNCLE_OPERATION_CONCEPTS_HPP

#include "basic_concepts.hpp"
#include "utility_concepts.hpp"
#include "value_concepts.hpp"

namespace asyncle {

template <typename T, typename U>
concept result = testable<T> && can_get_value<U>;

template <typename T, typename Obj>
concept can_push = object<Obj> && requires(T t, Obj obj) {
    { t.can_push() } -> checkable;
    { t.try_push(obj) } -> testable;
};

template <typename T, typename Obj>
concept can_take = object<Obj> && requires(T t, Obj obj) {
    { t.can_take() } -> checkable;
    { t.try_take(obj) } -> testable;
};

template <typename T, typename Obj>
concept can_work = object<Obj> && requires(T t, Obj obj) {
    { t.can_work() } -> checkable;
    { t.work(obj) } -> testable;
};

template <typename T, typename Obj, typename R>
concept can_make = object<Obj> && requires(T t, Obj obj) {
    { t.can_make() } -> checkable;
    { t.make(obj) } -> result<R>;
};

}  // namespace asyncle

#endif