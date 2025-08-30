#ifndef ASYNCLE_OPERATION_CONCEPTS_HPP
#define ASYNCLE_OPERATION_CONCEPTS_HPP

#include "../base/command.hpp"
#include "../base/cpo.hpp"
#include "basic_concepts.hpp"
#include "error_concepts.hpp"
#include "utility_concepts.hpp"
#include "value_concepts.hpp"

namespace asyncle {

template <typename T, typename U>
concept result = testable<T> && can_get_value<U>;

// CPO-based workable concept
template <typename T, typename Cmd, typename Para>
concept workable =
  object<Cmd> && is_command<Cmd> && object<Para> && cmd_accepts_v<Cmd, Para> && requires(T& t, Cmd cmd, Para&& para) {
      { asyncle::can_work(t, cmd) } -> checkable;
      { asyncle::work(t, cmd, std::forward<Para>(para)) } -> std::same_as<cmd_result_t<Cmd, Para>>;
  };

// CPO-based concepts using work with convenience wrappers
template <typename T, typename Obj>
concept makeable = object<Obj> && requires(T& t, Obj&& obj) {
    { asyncle::can_make(t) } -> checkable;
    { asyncle::make(t, std::forward<Obj>(obj)) };
};

template <typename T, typename Obj>
concept pushable = object<Obj> && requires(T& t, Obj&& obj) {
    { asyncle::can_push(t) } -> checkable;
    { asyncle::try_push(t, std::forward<Obj>(obj)) };
};

template <typename T, typename Obj>
concept takeable = object<Obj> && requires(T& t, Obj&& obj) {
    { asyncle::can_take(t) } -> checkable;
    { asyncle::try_take(t, std::forward<Obj>(obj)) };
};

}  // namespace asyncle

#endif
