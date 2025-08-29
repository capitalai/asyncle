#ifndef ASYNCLE_CPO_HPP
#define ASYNCLE_CPO_HPP

#include "command.hpp"
#include <type_traits>
#include <utility>

namespace asyncle {

// tag_invoke basis
void tag_invoke();  // ADL anchor

template <class Tag, class... Args>
concept tag_invocable = requires(Tag t, Args&&... args) { tag_invoke(t, std::forward<Args>(args)...); };

template <class Tag, class... Args>
using tag_invoke_result_t = decltype(tag_invoke(std::declval<Tag>(), std::declval<Args>()...));

// CPO: work
struct work_t {
    template <class T, class Cmd, class P>
    requires(cmd_accepts_v<Cmd, P> && tag_invocable<Cmd, T&, P &&>
             && std::same_as<tag_invoke_result_t<Cmd, T&, P &&>, cmd_result_t<Cmd, P>>)
    constexpr auto operator()(T& obj, Cmd cmd, P&& para) const -> cmd_result_t<Cmd, P> {
        return tag_invoke(cmd, obj, std::forward<P>(para));
    }

    template<class T, class Cmd, class P>
    requires ( cmd_accepts_v<Cmd, P> &&
               requires(T& o, Cmd c, P&& p) {
                   { o.work(c, std::forward<P>(p)) } -> std::same_as<cmd_result_t<Cmd,P>>;
               } )
    constexpr auto operator()(T& obj, Cmd cmd, P&& para) const
        -> cmd_result_t<Cmd,P>
    {
        return obj.work(cmd, std::forward<P>(para));
    }
};

inline constexpr work_t work {};

// CPO: can_work
struct can_work_t {
    template<class T, class Cmd>
    requires ( tag_invocable<can_work_t, T&, Cmd> )
    constexpr auto operator()(T& obj, Cmd cmd) const
        -> tag_invoke_result_t<can_work_t, T&, Cmd>
    { return tag_invoke(*this, obj, cmd); }

    template<class T, class Cmd>
    requires ( requires(T& o, Cmd c) {
                   { o.can_work(c) };
               } )
    constexpr auto operator()(T& obj, Cmd cmd) const
        -> decltype(obj.can_work(cmd))
    { return obj.can_work(cmd); }
};

inline constexpr can_work_t can_work {};

// Convenience wrappers that use work with make commands
template<class T, class Obj>
constexpr auto make(T& obj, Obj&& o) 
    -> decltype(work(obj, get_make_command(obj), std::forward<Obj>(o)))
{
    return work(obj, get_make_command(obj), std::forward<Obj>(o));
}

template<class T>
constexpr auto can_make(T& obj) 
    -> decltype(can_work(obj, get_make_command(obj)))
{
    return can_work(obj, get_make_command(obj));
}

// Convenience wrappers that use work with push/take commands
template<class T>
constexpr auto can_push(T& obj) 
    -> decltype(can_work(obj, get_push_command(obj)))
{
    return can_work(obj, get_push_command(obj));
}

template<class T, class Obj>
constexpr auto try_push(T& obj, Obj&& o) 
    -> decltype(work(obj, get_push_command(obj), std::forward<Obj>(o)))
{
    return work(obj, get_push_command(obj), std::forward<Obj>(o));
}

template<class T>
constexpr auto can_take(T& obj) 
    -> decltype(can_work(obj, get_take_command(obj)))
{
    return can_work(obj, get_take_command(obj));
}

template<class T, class Obj>
constexpr auto try_take(T& obj, Obj&& o) 
    -> decltype(work(obj, get_take_command(obj), std::forward<Obj>(o)))
{
    return work(obj, get_take_command(obj), std::forward<Obj>(o));
}

}  // namespace asyncle

#endif
