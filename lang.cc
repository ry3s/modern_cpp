#include <algorithm>
#include <array>
#include <cmath>
#include <cstddef>
#include <initializer_list>
#include <iostream>
#include <iterator>
#include <list>
#include <sstream>
#include <utility>
#include <vector>

class ipv4 {
  std::array<uint8_t, 4> data;

 public:
  constexpr ipv4() : ipv4(0, 0, 0, 0) {}
  constexpr ipv4(const uint8_t a, const uint8_t b, const uint8_t c, const uint8_t d)
      : data{a, b, c, d} {}

  explicit constexpr ipv4(uint32_t a)
      : ipv4(static_cast<uint8_t>((a >> 24) & 0xFF), static_cast<uint8_t>((a >> 16) & 0xFF),
             static_cast<uint8_t>((a >> 8) & 0xFF), static_cast<uint8_t>(a & 0xFF)) {}
  ipv4(ipv4 const& other) noexcept : data(other.data) {}
  ipv4& operator=(ipv4 const& other) noexcept {
    data = other.data;
    return *this;
  }

  std::string to_string() const {
    std::stringstream sstr;
    sstr << *this;
    return sstr.str();
  }

  constexpr uint32_t to_uint32() const {
    return static_cast<uint32_t>(data[0] << 24) | static_cast<uint32_t>(data[1] << 16) |
           static_cast<uint32_t>(data[2] << 8) | static_cast<uint32_t>(data[3]);
  }

  friend std::ostream& operator<<(std::ostream& os, const ipv4& a) {
    os << static_cast<int>(a.data[0]) << '.' << static_cast<int>(a.data[1]) << '.'
       << static_cast<int>(a.data[2]) << '.' << static_cast<int>(a.data[3]);
    return os;
  }

  friend std::istream& operator>>(std::istream& is, ipv4& a) {
    char d1, d2, d3;
    int b1, b2, b3, b4;
    is >> b1 >> d1 >> b2 >> d2 >> b3 >> d3 >> b4;
    if (d1 == '.' && d2 == '.' && d3 == '.') {
      a = ipv4(b1, b2, b3, b4);
    } else {
      is.setstate(std::ios_base::failbit);
    }
    return is;
  }

  ipv4& operator++() noexcept {
    *this = ipv4(1 + to_uint32());
    return *this;
  }

  ipv4 operator++(int) noexcept {
    ipv4 result(*this);
    ++(*this);
    return result;
  }

  friend int operator<=>(const ipv4& lhs, const ipv4& rhs) noexcept {
    if (lhs.data == rhs.data)
      return 0;
    else if (lhs.to_uint32() < rhs.to_uint32())
      return 1;
    else
      return -1;
  }
};

template <class T, size_t R, size_t C>
class array2d {
  using value_type = T;
  using iterator = value_type*;
  using const_iterator = value_type const*;
  std::vector<T> arr;

 public:
  array2d() : arr(R * C) {}
  explicit array2d(std::initializer_list<T> l) : arr(l) {}
  constexpr T* data() noexcept { return arr.data(); }
  constexpr T const* data() const noexcept { return arr.data(); }

  constexpr T& at(size_t const r, size_t const c) { return arr.at(r * C + c); }
  constexpr T const& at(size_t const r, size_t const c) const { return arr.at(r * C + c); }

  constexpr T& operator()(size_t const r, size_t const c) { return arr[r * C + c]; }
  constexpr T const& operator()(size_t const r, size_t const c) const { return arr[r * C + c]; }

  constexpr bool empty() const noexcept { return R == 0 || C == 0; }
  constexpr size_t size(int const rank) const {
    if (rank == 1)
      return R;
    else if (rank == 2)
      return C;
    throw std::out_of_range("Rank is out of range!");
  }

  void fill(T const& value) { std::fill(std::begin(arr), std::end(arr), value); }

  void swap(array2d& other) noexcept { arr.swap(other.arr); }

  const_iterator begin() const { return arr.data(); }
  const_iterator end() const { return arr.data() + arr.size(); }
  iterator begin() { return arr.data(); }
  iterator end() { return arr.data() + arr.size(); }
};

template <typename T>
T minimum(T const a, T const b) {
  return a < b ? a : b;
}

template <typename T1, typename... T>
T1 minimum(T1 a, T... args) {
  return minimum(a, minimum(args...));
}

template <typename C, typename... Args>
void push_back(C& c, Args&&... args) {
  (c.push_back(args), ...);
}

void print_variadic_push_back() {
  std::vector<int> v;
  push_back(v, 21, 2, 3, 4);
  std::copy(v.cbegin(), v.cend(), std::ostream_iterator<int>(std::cout, " "));

  std::list<int> l;
  push_back(l, 1, 2, 3, 4);
  std::copy(l.cbegin(), l.cend(), std::ostream_iterator<int>(std::cout, " "));
}

template <class C, class T>
bool contains(C const& c, T const& value) {
  return std::cend(c) != std::find(std::cbegin(c), std::cend(c), value);
}

template <class C, class... T>
bool contains_any(C const& c, T&&... value) {
  return (... || contains(c, value));
}

template <class C, class... T>
bool contains_all(C const& c, T&&... value) {
  return (... && contains(c, value));
}

template <class C, class... T>
bool contains_none(C const& c, T&&... value) {
  return !contains_any(c, std::forward<T>(value)...);
}

template <typename Traits>
class unique_handle {
  using pointer = typename Traits::pointer;
  pointer value_;

 public:
  unique_handle(unique_handle const&) = delete;
  unique_handle& operator=(unique_handle const&) = delete;

  explicit unique_handle(pointer value = Traits::invalid()) noexcept : value_(value) {}

  unique_handle(unique_handle&& other) noexcept : value_(other.release()) {}

  unique_handle& operator=(unique_handle&& other) noexcept {
    if (this != &other) reset(other.release());
    return *this;
  }

  ~unique_handle() noexcept { Traits::close(value_); }

  explicit operator bool() const noexcept { return value_ != Traits::invalid(); }

  pointer get() const noexcept { return value_; }

  pointer release() noexcept {
    auto value = value_;
    value_ = Traits::invalid();
    return value;
  }

  bool reset(pointer value = Traits::invalid()) noexcept {
    if (value_ != value) {
      Traits::close(value_);
      value_ = value;
    }
    return static_cast<bool>(*this);
  }

  void swap(unique_handle<Traits>& other) noexcept { std::swap(value_, other.value_); }
};

template <typename Traits>
void swap(unique_handle<Traits>& left, unique_handle<Traits>& right) noexcept {
  left.swap(right);
}

template <typename Traits>
bool operator==(unique_handle<Traits> const& left, unique_handle<Traits> const& right) noexcept {
  return left.get() == right.get();
}

struct null_handle_trais {
  using pointer = void*;
  static pointer invalid() noexcept { return nullptr; }
  static void close(pointer value) noexcept { free(value); }
};

bool are_equal(double const d1, double const d2, double const eps = 0.001) {
  return std::fabs(d1 - d2) < eps;
}

namespace temperature {
enum class scale { celsius, fahrenheit, kelvin };

template <scale S>
class quantity {
  double const amount;

 public:
  constexpr explicit quantity(double const a) : amount(a) {}
  explicit operator double() const { return amount; }
};

template <scale S>
inline bool operator==(quantity<S> const& left, quantity<S> const& right) {
  return are_equal(static_cast<double>(left), static_cast<double>(right));
}

template <scale S>
inline bool operator!=(quantity<S> const& left, quantity<S> const& right) {
  return !(left == right);
}

template <scale S>
inline bool operator<(quantity<S> const& left, quantity<S> const& right) {
  return static_cast<double>(left) < static_cast<double>(right);
}

template <scale S>
inline bool operator>(quantity<S> const& left, quantity<S> const& right) {
  return right < left;
}

template <scale S>
inline bool operator<=(quantity<S> const& left, quantity<S> const& right) {
  return !(left > right);
}

template <scale S>
inline bool operator>=(quantity<S> const& left, quantity<S> const& right) {
  return !(left < right);
}

template <scale S>
constexpr quantity<S> operator+(quantity<S> const& left, quantity<S> const& right) {
  return quantity<S>(static_cast<double>(left) + static_cast<int>(right));
}

template <scale S>
constexpr quantity<S> operator-(quantity<S> const& left, quantity<S> const& right) {
  return quantity<S>(static_cast<double>(left) - static_cast<int>(right));
}

template <scale S, scale R>
struct conversion_traits {
  static double convert(double const value) = delete;
};

template <>
struct conversion_traits<scale::celsius, scale::fahrenheit> {
  static double convert(double const value) { return (value * 9) / 5 + 32; }
};

template <>
struct conversion_traits<scale::fahrenheit, scale::celsius> {
  static double convert(double const value) { return (value - 32) * 5 / 9; }
};

template <>
struct conversion_traits<scale::celsius, scale::kelvin> {
  static double convert(double const value) { return value + 273.15; }
};

template <>
struct conversion_traits<scale::kelvin, scale::celsius> {
  static double convert(double const value) { return value - 273.15; }
};

template <>
struct conversion_traits<scale::fahrenheit, scale::kelvin> {
  static double convert(double const value) { return (value + 459.67) * 5 / 9; }
};

template <>
struct conversion_traits<scale::kelvin, scale::fahrenheit> {
  static double convert(double const value) { return (value * 9) / 5 - 459.67; }
};

template <scale R, scale S>
constexpr quantity<R> temperature_cast(quantity<S> const q) {
  return quantity<R>(conversion_traits<S, R>::convert(static_cast<double>(q)));
}
}  // namespace temperature

namespace temperature::temperature_scale_literals {
constexpr quantity<scale::celsius> operator""_deg(long double const amount) {
  return quantity<scale::celsius>{static_cast<double>(amount)};
}
constexpr quantity<scale::fahrenheit> operator""_f(long double const amount) {
  return quantity<scale::fahrenheit>{static_cast<double>(amount)};
}
constexpr quantity<scale::kelvin> operator""_k(long double const amount) {
  return quantity<scale::kelvin>{static_cast<double>(amount)};
}
}  // namespace temperature::temperature_scale_literals

void test_temperature() {
  using namespace temperature;
  using namespace temperature_scale_literals;

  auto t1{36.5_deg};
  auto t2{79.0_f};
  auto t3{100.0_k};
  {
    auto tf = temperature_cast<scale::fahrenheit>(t1);
    auto tc = temperature_cast<scale::celsius>(tf);
    assert(t1 == tc);
  }

  {
    auto tk = temperature_cast<scale::kelvin>(t1);
    auto tc = temperature_cast<scale::celsius>(tk);
    assert(t1 == tc);
  }

  {
    auto tc = temperature_cast<scale::celsius>(t2);
    auto tf = temperature_cast<scale::fahrenheit>(tc);
    assert(t2 == tf);
  }

  {
    auto tk = temperature_cast<scale::kelvin>(t2);
    auto tf = temperature_cast<scale::fahrenheit>(tk);
    assert(t2 == tf);
  }

  {
    auto tc = temperature_cast<scale::celsius>(t3);
    auto tk = temperature_cast<scale::kelvin>(tc);
    assert(t3 == tk);
  }

  {
    auto tf = temperature_cast<scale::fahrenheit>(t3);
    auto tk = temperature_cast<scale::kelvin>(tf);
    assert(t3 == tk);
  }
}
int main() { test_temperature(); }
