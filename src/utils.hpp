#ifndef UTILS_hpp
#define UTILS_hpp

#include <functional>
#include <optional>
#include <string>
#include <unordered_map>
#include <variant>

template <typename... Fs>
struct overloaded : Fs... {
  using Fs::operator()...;
  overloaded(Fs... fs) : Fs(fs)... {};
};
template <typename... Fs> overloaded(Fs...) -> overloaded<Fs...>;

template <typename T, typename... Ts, typename F, typename... Fs>
void checkVisitTypes(std::variant<T> var, F f) {
  (void)var;
  if (false) {
    f(std::get<T>(var));
  }
}
template <typename T, typename... Ts, typename F, typename... Fs>
void checkVisitTypes(std::variant<T, Ts...> var, F f, Fs... fs) {
  if (false) {
    f(std::get<T>(var));
    checkVisitTypes(std::variant<Ts...>(), fs...);
  }
}

template <typename R, typename... Ts, typename... Fs>
R visit(std::variant<Ts...> var, Fs... fs) {
  checkVisitTypes(var, fs...);
  return std::visit(overloaded(fs...), var);
}

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

template <typename T>
struct Constructor {
  template <typename... Args>
    T operator() (const Args&... args) const { return T(args...); }
};

template <typename R, typename T, typename... Args>
constexpr std::function<R(Args...)> bindMember(R (T::*f)(Args...), T* t) {
  return [f, t](Args... args) { return (t->*f)(args...); };
}

template <typename R, typename Arg, typename... Args>
constexpr std::function<R(Args...)> bindFirst(R (*f)(Arg, Args...), Arg arg) {
  return [f, arg](Args... args) { return (*f)(arg, args...); };
}

template <typename R, typename Arg, typename... Args, typename T>
constexpr std::function<R(Args...)> bindFirst(T f, Arg arg) {
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

constexpr void ignore() {}

template <typename Arg, typename... Args>
constexpr void ignore(Arg arg, Args... args) {
  (void)arg;
  ignore(args...);
}

template <typename R>
struct Const {
  R ret;
  Const(R ret) : ret(ret) {}
  template <typename... Args>
    R operator() (Args... args) const {
      ignore(args...);
      return ret;
    }
};

template <>
struct Const<void> {
  Const() {}
  template <typename... Args>
    void operator() (Args... args) const {
      ignore(args...);
      return;
    }
};

template <typename R, typename... Args>
constexpr std::function<R(Args...)> ConstThrow(std::exception&& e) {
  return [e{std::move(e)}](Args... args) -> R {
    ignore(args...);
    throw e;
  };
}

template <typename R, typename... Args>
const std::function<std::optional<R>(Args...)> ConstNullopt = [](Args... args) -> std::optional<R> {
  ignore(args...);
  return std::nullopt;
};

template <typename T>
const std::function<T(T)> id = [](T t) -> T { return t; };

template <typename T>
const std::function<std::optional<T>(T)> just = [](T t) -> std::optional<T> { return t; };

template <typename R, typename Arg>
const std::function<R(Arg)> static_cast_f = [](Arg arg) -> R { return static_cast<R>(arg); };

template <typename T, typename U, typename... Args>
std::function<U(Args...)> operator^(std::function<U(T)> g, std::function<T(Args...)> f) {
  return [f{std::move(f)}, g{std::move(g)}](Args... args) -> U {
    return g(f(args...));
  };
}

#define parseUnit(unit, str, start) std::size_t unit##Start = start; std::size_t unit##End = str.find(':', unit##Start); std::string unit = str.substr(unit##Start, unit##End - unit##Start)

namespace endian {
  enum endianness {
    little, big
  };
  constexpr endianness native = endianness::little;
}

#endif
