#ifndef UTILS_hpp
#define UTILS_hpp

#include <functional>
#include <optional>

template <typename T, typename U>
std::optional<T> bindOptional(std::optional<U> x, std::function<T(U*)> f) { return x ? std::optional{f(&*x)} : std::nullopt; }

template <typename T, typename U>
std::optional<U> getOpt(std::unordered_map<T,U> xs, T x) {
  try {
    return xs.at(x);
  } catch (std::out_of_range&) {
    return std::nullopt;
  }
}

template <typename R, typename Arg, typename... Args>
std::function<R(Args...)> bindFirst(std::function<R(Arg, Args...)> f, Arg arg) {
  return [f, arg](Args... args) { return f(arg, args...); };
}

template <typename R, typename T, typename... Args>
std::function<R(Args...)> bindMember(R (T::*f)(Args...), T* t) {
  return [f, t](Args... args) { return (t->*f)(args...); };
}

#endif
