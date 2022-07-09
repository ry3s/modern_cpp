#include <algorithm>
#include <array>
#include <bitset>
#include <cmath>
#include <iostream>
#include <iterator>
#include <numeric>
#include <random>
#include <tuple>
#include <vector>

int gcd(int m, int n) {
  if (n > m) return gcd(n, m);
  if (n == 0) return m;

  return gcd(n, m % n);
}

void math1() {
  int n;
  std::cin >> n;

  int sum = 0;
  for (int i = 1; i <= n; i++) {
    if (i % 3 == 0 || i % 5 == 0) {
      sum += i;
    }
  }

  std::cout << sum << std::endl;
}

void math2() {
  int m, n;
  std::cin >> m >> n;

  std::cout << gcd(m, n) << std::endl;
}

void math3() {
  int m, n;
  std::cin >> m >> n;

  std::cout << m * n / gcd(m, n) << std::endl;
}

int is_prime(int n) {
  for (int i = 2; i * i <= n; i++) {
    if (n % i == 0) return false;
  }
  return true;
}

void math4() {
  int n;
  std::cin >> n;

  int ans = 0;
  for (int i = n - 1; i > 1; i--) {
    if (is_prime(i)) {
      ans = i;
      break;
    }
  }

  std::cout << ans << std::endl;
}

std::vector<int> enum_primes(int n) {
  std::vector<int> primes;

  int i = 2;
  while (i <= n) {
    bool ok = std::all_of(primes.begin(), primes.end(), [i](int p) { return i % p != 0; });
    if (ok) primes.push_back(i);
  }
  return primes;
}

void math5() {
  int n;
  std::cin >> n;

  auto primes = enum_primes(n);
  std::sort(primes.begin(), primes.end());
  for (int i = 0; i < primes.size() - 1; i++) {
    if (primes[i + 1] - primes[i] == 6) {
      std::cout << "(" << primes[i] << ", " << primes[i + 1] << ")" << std::endl;
    }
  }
}

std::vector<int> enum_divs(int n) {
  std::vector<int> divs;
  for (int i = 1; i * i <= n; i++) {
    if (n % i == 0) {
      divs.push_back(i);
      if (i != n / i) divs.push_back(n / i);
    }
  }
  return divs;
}

void math6() {
  int n;
  std::cin >> n;
  for (int i = 1; i <= n; i++) {
    auto divs = enum_divs(i);
    auto sum = std::accumulate(divs.begin(), divs.end(), 0);
    if (sum > i * 2) {
      std::cout << i << ", " << sum - i * 2 << std::endl;
    }
  }
}

int sum_proper_divisors(const int num) {
  int result = 1;
  const int root = static_cast<int>(std::sqrt(num));
  for (int i = 2; i <= root; i++) {
    if (num % i == 0) {
      result += (i == (num / i) ? i : (i + num / i));
    }
  }
  return result;
}

void print_amicables(const int limit) {
  for (int num = 4; num < limit; num++) {
    if (auto sum1 = sum_proper_divisors(num); sum1 < limit) {
      if (auto sum2 = sum_proper_divisors(sum1); sum2 == num && num != sum1) {
        std::cout << num << ", " << sum1 << std::endl;
      }
    }
  }
}

void print_narcissistics() {
  for (int a = 1; a <= 9; a++) {
    for (int b = 0; b <= 9; b++) {
      for (int c = 0; c <= 9; c++) {
        auto abc = a * 100 + b * 10 + c;
        auto arm = a * a * a + b * b * b + c * c * c;
        if (abc == arm) {
          std::cout << arm << std::endl;
        }
      }
    }
  }
}

std::vector<unsigned long> prime_factors(unsigned long n) {
  std::vector<unsigned long> factors;
  while (n % 2 == 0) {
    factors.push_back(2);
    n /= 2;
  }

  const int root = static_cast<int>(std::sqrt(n));
  for (unsigned long i = 3; i < root; i += 2) {
    while (n % i == 0) {
      factors.push_back(i);
      n /= i;
    }
  }

  if (n > 2) {
    factors.push_back(n);
  }
  return factors;
}

unsigned int gray_encode(const unsigned int n) { return n ^ (n >> 1); }
unsigned int gray_decode(unsigned int gray) {
  for (unsigned int bit = 1U << 31; bit > 1; bit >>= 1) {
    if (gray & bit) gray ^= bit >> 1;
  }
  return gray;
}

void print_graycode_table() {
  auto to_binary = [](unsigned int const value, int const digits) {
    return std::bitset<32>(value).to_string().substr(32 - digits, digits);
  };

  std::cout << "Number\tBinary\tGray\tDecoded" << std::endl;
  std::cout << "------\t------\t----\t-------" << std::endl;
  for (unsigned int n = 0; n < 32; n++) {
    auto encg = gray_encode(n);
    auto decg = gray_decode(encg);
    std::cout << n << "\t" << to_binary(n, 5) << "\t" << to_binary(encg, 5) << "\t" << decg
              << std::endl;
  }
}

std::string to_roman(unsigned int value) {
  std::vector<std::tuple<unsigned int, char const*>> const roman{
      {1000, "M"}, {900, "CM"}, {500, "D"}, {400, "CD"}, {100, "C"}, {90, "XC"}, {50, "L"},
      {40, "XL"},  {10, "X"},   {9, "IX"},  {5, "V"},    {4, "IV"},  {1, "I"}};

  std::string result;
  for (const auto& [num, str] : roman) {
    while (value >= num) {
      result += str;
      value -= num;
    }
  }
  return result;
}

void print_roman_table() {
  for (int i = 1; i <= 100; i++) {
    std::cout << i << "\t" << to_roman(i) << std::endl;
  }
}

std::tuple<unsigned long, long> longest_collatz(const unsigned long limit) {
  long length = 0;
  unsigned long number = 0;

  std::vector<int> cache(limit + 1, 0);
  for (unsigned long i = 2; i <= limit; i++) {
    auto n = i;
    long steps = 0;
    while (n != 1 && n >= i) {
      if ((n % 2) == 0)
        n /= 2;
      else
        n = n * 3 + 1;
      steps++;
    }
    cache[i] = steps + cache[n];

    if (cache[i] > length) {
      length = cache[i];
      number = i;
    }
  }
  return {number, length};
}

template <typename E = std::mt19937, typename D = std::uniform_real_distribution<>>
double compute_pi(E& engine, D& dist, const int samples = 1000000) {
  auto hit = 0;
  for (auto i = 0; i < samples; i++) {
    auto x = dist(engine);
    auto y = dist(engine);
    if (y <= std::sqrt(1 - std::pow(x, 2))) hit++;
  }
  return 4.0 * hit / samples;
}

void output_pi() {
  std::random_device rd;
  auto seed_data = std::array<int, std::mt19937::state_size>{};
  std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
  std::seed_seq seq(std::cbegin(seed_data), std::cend(seed_data));
  auto eng = std::mt19937{seq};
  auto dist = std::uniform_real_distribution<>{0, 1};
  std::cout << compute_pi(eng, dist) << std::endl;
}

bool validate_isbn_10(std::string_view isbn) {
  auto valid = false;
  if (isbn.size() == 10 && std::all_of(std::cbegin(isbn), std::cend(isbn), isdigit)) {
    auto w = 10;
    auto sum =
        std::accumulate(std::cbegin(isbn), std::cend(isbn), 0,
                        [&w](const int total, const char c) { return total + w-- * (c - '0'); });
    valid = !(sum % 11);
  }
  return valid;
}

int main() {}
