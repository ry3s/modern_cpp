#include <algorithm>
#include <array>
#include <cstddef>
#include <iostream>
#include <limits>
#include <map>
#include <memory>
#include <random>
#include <sstream>
#include <stdexcept>
#include <string>
#include <type_traits>

class PasswordValidator {
 public:
  virtual bool validate(std::string_view password) = 0;
  virtual ~PasswordValidator() = default;
};

class LengthValidator final : public PasswordValidator {
 public:
  explicit LengthValidator(unsigned int min_length) : length_(min_length) {}
  virtual bool validate(std::string_view password) override { return password.length() >= length_; }

 private:
  const unsigned int length_;
};

class PasswordValidatorDecorator : public PasswordValidator {
 public:
  explicit PasswordValidatorDecorator(std::unique_ptr<PasswordValidator> validator)
      : inner_(std::move(validator)) {}
  virtual bool validate(std::string_view password) override { return inner_->validate(password); }

 private:
  std::unique_ptr<PasswordValidator> inner_;
};

class DigitPasswordValidator : public PasswordValidatorDecorator {
 public:
  explicit DigitPasswordValidator(std::unique_ptr<PasswordValidator> validator)
      : PasswordValidatorDecorator(std::move(validator)) {}

  virtual bool validate(std::string_view password) override {
    if (!PasswordValidatorDecorator::validate(password)) return false;
    return password.find_first_of("0123456789") != std::string::npos;
  }
};

class CasePasswordValidator : public PasswordValidatorDecorator {
 public:
  explicit CasePasswordValidator(std::unique_ptr<PasswordValidator> validator)
      : PasswordValidatorDecorator(std::move(validator)) {}

  virtual bool validate(std::string_view password) override {
    if (!PasswordValidatorDecorator::validate(password)) return false;

    const bool haslower = std::any_of(password.cbegin(), password.cend(), islower);
    const bool hasupper = std::any_of(password.cbegin(), password.cend(), isupper);
    return haslower && hasupper;
  }
};

void test_password_validator() {
  auto validator1 = std::make_unique<DigitPasswordValidator>(std::make_unique<LengthValidator>(8));

  assert(validator1->validate("abc123!@#"));
  assert(!validator1->validate("abcde!@#"));
}

class PasswordGenerator {
 public:
  virtual std::string generate() = 0;

  virtual std::string allowed_chars() const = 0;
  virtual size_t length() const = 0;
  virtual void add(std::unique_ptr<PasswordGenerator> generator) = 0;

  virtual ~PasswordGenerator() = default;
};

class BasicPasswordGenerator : public PasswordGenerator {
  const size_t length_;

 public:
  explicit BasicPasswordGenerator(const size_t length) noexcept : length_(length) {}

  virtual std::string generate() override { throw std::runtime_error("not implemented"); }

  virtual void add(std::unique_ptr<PasswordGenerator>) override {
    throw std::runtime_error("not implemented");
  }

  virtual size_t length() const noexcept override final { return length_; }
};

class DigitGenerator : public BasicPasswordGenerator {
 public:
  explicit DigitGenerator(const size_t length) noexcept : BasicPasswordGenerator(length) {}

  virtual std::string allowed_chars() const override { return "0123456789"; }
};

class SymbolGenerator : public BasicPasswordGenerator {
 public:
  explicit SymbolGenerator(const size_t length) noexcept : BasicPasswordGenerator(length) {}

  virtual std::string allowed_chars() const override { return "!@#$%^&*(){}[]?<>"; }
};

class UpperLetterGenerator : public BasicPasswordGenerator {
 public:
  explicit UpperLetterGenerator(const size_t length) noexcept : BasicPasswordGenerator(length) {}

  virtual std::string allowed_chars() const override { return "ABCDEFGHIJKLMNOPQRSTUVXYZ"; }
};

class LowerLetterGenerator : public BasicPasswordGenerator {
 public:
  explicit LowerLetterGenerator(const size_t length) noexcept : BasicPasswordGenerator(length) {}

  virtual std::string allowed_chars() const override { return "abcdefghijklmnopqrstuvxyz"; }
};

class CompositePasswordGenerator : public PasswordGenerator {
  virtual std::string allowed_chars() const override {
    throw std::runtime_error("not implemented");
  }
  virtual size_t length() const override { throw std::runtime_error("not implemented"); }

 public:
  CompositePasswordGenerator() {
    auto seed_data = std::array<int, std::mt19937::state_size>{};
    std::generate(seed_data.begin(), seed_data.end(), std::ref(rd_));
    std::seed_seq seq(seed_data.cbegin(), seed_data.cend());
    eng_.seed(seq);
  }

  virtual std::string generate() override {
    std::string password;
    for (auto& generator : generators_) {
      std::string chars = generator->allowed_chars();
      std::uniform_int_distribution<> ud(0, chars.length() - 1);

      for (size_t i = 0; i < generator->length(); i++) {
        password += chars[ud(eng_)];
      }
    }
    std::shuffle(password.begin(), password.end(), eng_);

    return password;
  }

  virtual void add(std::unique_ptr<PasswordGenerator> generator) override {
    generators_.push_back(std::move(generator));
  }

 private:
  std::random_device rd_;
  std::mt19937 eng_;
  std::vector<std::unique_ptr<PasswordGenerator>> generators_;
};

void test_password_generator() {
  CompositePasswordGenerator gen;
  gen.add(std::make_unique<SymbolGenerator>(2));
  gen.add(std::make_unique<DigitGenerator>(2));
  gen.add(std::make_unique<UpperLetterGenerator>(2));
  gen.add(std::make_unique<LowerLetterGenerator>(4));

  const auto password = gen.generate();
  std::cout << password << std::endl;
}

enum class SexType { Female, Male };

class SocialNumberGenerator {
 protected:
  virtual int sex_digit(const SexType sex) const noexcept = 0;
  virtual int next_random(const unsigned year, const unsigned month, const unsigned day) = 0;
  virtual int modulo_value() const noexcept = 0;

  SocialNumberGenerator(const int min, const int max) : ud_(min, max) {
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size>{};
    std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
    std::seed_seq seq(seed_data.cbegin(), seed_data.cend());
    eng_.seed(seq);
  }

 public:
  std::string generate(const SexType sex, const unsigned year, const unsigned month,
                       const unsigned day) {
    std::stringstream ss;

    ss << sex_digit(sex);
    ss << year << month << day;
    ss << next_random(year, month, day);
    const auto number = ss.str();

    auto index = number.length();
    const auto sum =
        std::accumulate(number.begin(), number.end(), 0U, [&index](const unsigned s, const char c) {
          return s + static_cast<unsigned>(index-- * (c - '0'));
        });
    const auto rest = sum % modulo_value();
    ss << modulo_value() - rest;
    return ss.str();
  }

  virtual ~SocialNumberGenerator() = default;

 protected:
  std::map<unsigned, int> cache_;
  std::mt19937 eng_;
  std::uniform_int_distribution<> ud_;
};

class SoutheriaSocialNumberGenerator final : public SocialNumberGenerator {
 public:
  SoutheriaSocialNumberGenerator() : SocialNumberGenerator(1000, 9999) {}

 protected:
  virtual int sex_digit(const SexType sex) const noexcept override {
    if (sex == SexType::Female)
      return 1;
    else
      return 2;
  }

  virtual int next_random(const unsigned year, const unsigned month, const unsigned day) override {
    const auto key = year * 10000 + month * 100 + day;
    while (true) {
      const auto number = ud_(eng_);
      const auto pos = cache_.find(number);
      if (pos == std::end(cache_)) {
        cache_[key] = number;
        return number;
      }
    }
  }

  virtual int modulo_value() const noexcept override { return 11; }
};

class NortheriaSocialNumberGenerator final : public SocialNumberGenerator {
 public:
  NortheriaSocialNumberGenerator() : SocialNumberGenerator(10000, 99999) {}

 protected:
  virtual int sex_digit(const SexType sex) const noexcept override {
    if (sex == SexType::Female)
      return 9;
    else
      return 7;
  }

  virtual int next_random(const unsigned year, const unsigned month, const unsigned day) override {
    const auto key = year * 10000 + month * 100 + day;
    while (true) {
      const auto number = ud_(eng_);
      const auto pos = cache_.find(number);
      if (pos == std::end(cache_)) {
        cache_[key] = number;
        return number;
      }
    }
  }

  virtual int modulo_value() const noexcept override { return 11; }
};

class SocialNumberGeneratorFactory {
 public:
  SocialNumberGeneratorFactory() {
    generators_["northeria"] = std::make_unique<NortheriaSocialNumberGenerator>();
    generators_["southeria"] = std::make_unique<SoutheriaSocialNumberGenerator>();
  }

  SocialNumberGenerator* get_generator(std::string country) const {
    auto it = generators_.find(country.c_str());
    if (it != generators_.end()) return it->second.get();

    throw std::runtime_error("invalid country");
  }

 private:
  std::map<std::string, std::unique_ptr<SocialNumberGenerator>> generators_;
};

void test_social_number_generator() {
  SocialNumberGeneratorFactory factory;

  auto sn1 = factory.get_generator("northeria")->generate(SexType::Female, 2017, 12, 25);
  auto ss1 = factory.get_generator("southeria")->generate(SexType::Female, 2017, 12, 25);

  std::cout << sn1 << "\n";
  std::cout << ss1 << "\n";
}

class Role {
 public:
  virtual double approval_limit() const noexcept = 0;
  virtual ~Role() = default;
};

class EmployeeRole : public Role {
 public:
  virtual double approval_limit() const noexcept override { return 1000.0; }
};

class TeamManagerRole : public Role {
 public:
  virtual double approval_limit() const noexcept override { return 10000.0; }
};

class DepartmentManagerRole : public Role {
 public:
  virtual double approval_limit() const noexcept override { return 100000.0; }
};

class PresidentRole : public Role {
 public:
  virtual double approval_limit() const noexcept override {
    return std::numeric_limits<double>::max();
  }
};

struct Expense {
  const double amount;
  const std::string description;
  Expense(const double amount, std::string_view desc) : amount(amount), description(desc) {}
};

class Employee {
 public:
  Employee(std::string_view name, std::unique_ptr<Role> own_role)
      : name_(name), own_role_(std::move(own_role)) {}

  void set_direct_manager(std::shared_ptr<Employee> manager) { direct_manager_ = manager; }

  void approve(const Expense& e) {
    if (e.amount <= own_role_->approval_limit())
      std::cout << name_ << " approved expense '" << e.description << "', cost = " << e.amount
                << std::endl;
    else if (direct_manager_ != nullptr)
      direct_manager_->approve(e);
  }

 private:
  const std::string name_;
  const std::unique_ptr<Role> own_role_;
  std::shared_ptr<Employee> direct_manager_;
};

void test_expense() {
  auto john = std::make_shared<Employee>("john smith", std::make_unique<EmployeeRole>());
  auto robert = std::make_shared<Employee>("robert booth", std::make_unique<TeamManagerRole>());
  auto david = std::make_shared<Employee>("david jones", std::make_unique<DepartmentManagerRole>());
  auto cecil = std::make_shared<Employee>("cecil williamson", std::make_unique<PresidentRole>());

  john->set_direct_manager(robert);
  robert->set_direct_manager(david);
  david->set_direct_manager(cecil);

  john->approve(Expense{500, "magazins"});
  john->approve(Expense{5000, "hotel accomodation"});
  john->approve(Expense{50000, "conference costs"});
  john->approve(Expense{200000, "new lorry"});
}

struct DiscountType {
  virtual double discount_percent(const double price, const double quantity) const noexcept = 0;
  virtual ~DiscountType() = default;
};

struct FixedDiscount final : public DiscountType {
  explicit FixedDiscount(const double discount) noexcept : discount(discount) {}
  virtual double discount_percent(const double, const double) const noexcept { return discount; }

 private:
  const double discount;
};

struct VolumeDicount final : public DiscountType {
  VolumeDicount(const double quantity, const double discount) noexcept
      : discount(discount), min_quantity(quantity) {}
  virtual double discount_percent(const double, const double quantity) const noexcept {
    return quantity >= min_quantity ? discount : 0.0;
  }

 private:
  const double discount;
  const double min_quantity;
};

struct PriceDiscount : public DiscountType {
  PriceDiscount(const double price, const double discount) noexcept
      : discount(discount), min_total_price(price) {}
  virtual double discount_percent(const double price, const double quantity) const noexcept {
    return price * quantity >= min_total_price ? discount : 0.0;
  }

 private:
  const double discount;
  const double min_total_price;
};

struct AmountDiscount : public DiscountType {
  AmountDiscount(const double price, const double discount) noexcept
      : discount(discount), min_total_price(price) {}
  virtual double discount_percent(const double price, const double) const noexcept {
    return price >= min_total_price ? discount : 0.0;
  }

 private:
  const double discount;
  const double min_total_price;
};

struct DefaultDiscount : public DiscountType {
  virtual double discount_percent(const double, const double) const noexcept { return 0.0; }
};

struct Customer {
  std::string name;
  DiscountType* discount;
};

enum class ArticleUnit { Piece, Kg, Meter, Sqmeter, Cmeter, Liter };

struct Article {
  int id;
  std::string name;
  double price;
  ArticleUnit unit;
  DiscountType* discount;
};

struct OrderLine {
  Article product;
  int quantity;
  DiscountType* discount;
};

struct Order {
  int id;
  Customer* buyer;
  std::vector<OrderLine> lines;
  DiscountType* discount;
};

struct PriceCalculator {
  virtual double calculate_price(const Order& o) = 0;
  virtual ~PriceCalculator() = default;
};

struct CumulativePriceCalculator : public PriceCalculator {
  virtual double calculate_price(const Order& o) override {
    double price = 0.0;
    for (auto ol : o.lines) {
      double line_price = ol.product.price * ol.quantity;

      line_price *= (1.0 - ol.product.discount->discount_percent(ol.product.price, ol.quantity));
      line_price *= (1.0 - ol.discount->discount_percent(ol.product.price, ol.quantity));
      line_price *= (1.0 - o.buyer->discount->discount_percent(ol.product.price, ol.quantity));

      price += line_price;
    }

    if (o.discount != nullptr) price *= (1.0 - o.discount->discount_percent(price, 0.0));

    return price;
  }
};

inline bool are_equal(const double d1, const double d2, const double diff = 0.001) {
  return std::abs(d1 - d2) <= diff;
}

void test_discount() {
  FixedDiscount d1(0.1);
  VolumeDicount d2(10, 0.15);
  PriceDiscount d3(100, 0.05);
  AmountDiscount d4(100, 0.05);
  DefaultDiscount d0;

  Customer c1{"default", &d0};
  Customer c2{"john", &d1};
  Customer c3{"joane", &d3};

  Article a1{1, "pen", 5, ArticleUnit::Piece, &d0};
  Article a2{2, "expensive pen", 15, ArticleUnit::Piece, &d1};
  Article a3{3, "scissors", 10, ArticleUnit::Piece, &d2};

  CumulativePriceCalculator calc;

  Order o1{101, &c1, {{a1, 1, &d0}}, &d0};
  assert(are_equal(calc.calculate_price(o1), 5.0));

  Order o3{103, &c1, {{a2, 1, &d0}}, &d0};
  assert(are_equal(calc.calculate_price(o3), 13.5));

  Order o6{106, &c1, {{a3, 15, &d0}}, &d0};
  assert(are_equal(calc.calculate_price(o6), 127.5));

  Order o9{109, &c3, {{a2, 20, &d1}}, &d4};
  assert(are_equal(calc.calculate_price(o9), 219.3075));
}

int main() { test_discount(); }
