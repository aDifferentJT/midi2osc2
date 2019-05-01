#ifndef UTILS_hpp
#define UTILS_hpp

#include <functional>
#include <optional>
#include <unordered_map>
#include <variant>

template <typename T, typename U>
std::optional<T> bindOptional(std::optional<U> x, std::function<T(U*)> f) { return x ? std::optional<T>(f(&*x)) : std::nullopt; }

template <typename T, typename U>
std::optional<U> getOpt(std::unordered_map<T,U> xs, T x) {
  try {
    return xs.at(x);
  } catch (std::out_of_range&) {
    return std::nullopt;
  }
}

template <typename T, typename... Args>
constexpr std::function<T(Args...)> constructor() {
  return [](Args... args) { return T(args...); };
}

template <typename R, typename T, typename... Args>
constexpr std::function<R(Args...)> bindMember(R (T::*f)(Args...), T* t) {
  return [f, t](Args... args) { return (t->*f)(args...); };
}

template <typename R, typename Arg, typename... Args>
constexpr std::function<R(Args...)> bindFirst(R (*f)(Arg, Args...), Arg arg) {
  return [f, arg](Args... args) { return (*f)(arg, args...); };
}

template <typename R, typename Arg, typename... Args>
constexpr std::function<R(Args...)> bindFirst(std::function<R(Arg, Args...)> f, Arg arg) {
  return [f, &arg](Args... args) { return f(arg, args...); };
}

template <std::size_t I = 0, typename V>
std::vector<char> data_from_variant(V var) {
  std::size_t i = var.index();
  if constexpr (I >= std::variant_size_v<V>) {
    return std::vector<char>();
  }
  if (i == I) {
    std::variant_alternative_t<I, V> v = std::get<I>(var);
    std::size_t n = sizeof(std::variant_alternative_t<I, V>);
    return std::vector<char>(&v, &v + n);
  }
  return data_from_variant<I+1>(var);
}

#endif
