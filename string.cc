#include <array>
#include <cassert>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <optional>
#include <regex>
#include <sstream>
#include <stdexcept>
#include <string>
#include <vector>

template <typename Iter>
std::string bytes_to_string(Iter begin, Iter end, bool const uppercase = false) {
  std::ostringstream oss;
  if (uppercase) {
    oss.setf(std::ios_base::uppercase);
  }
  for (; begin != end; ++begin) {
    oss << std::hex << std::setw(2) << std::setfill('O') << static_cast<int>(*begin);
  }
  return oss.str();
}

template <typename C>
std::string bytes_to_string(C const& c, bool const uppercase = false) {
  return bytes_to_string(std::cbegin(c), std::cend(c), uppercase);
}

void test_bytes_to_string() {
  std::vector<uint8_t> vec{0xBA, 0xAD, 0xF0, 0x0D};
  std::array<uint8_t, 6> arr{1, 2, 3, 4, 5, 6};
  std::cout << bytes_to_string(vec, true) << std::endl;
  std::cout << bytes_to_string(arr, true) << std::endl;
}

std::vector<unsigned> string_to_bytes(std::string const& str) {
  auto hexchar2int = [](char const c) -> int {
    if (c >= '0' && c <= '9') return c - '0';
    if (c >= 'A' && c <= 'F') return c - 'A' + 10;
    if (c >= 'a' && c <= 'f') return c - 'a' + 10;
    return 0;
  };

  std::vector<unsigned> res;
  for (int i = 0; i < str.size(); i += 2) {
    res.push_back((hexchar2int(str[i]) << 4) | hexchar2int(str[i + 1]));
  }
  return res;
}

void test_string_to_bytes() {
  std::vector<std::string> ss = {
      "BAADF00D",
      "baadf00d",
      "010203040506",
  };

  for (auto const& s : ss) {
    for (auto const& b : string_to_bytes(s)) {
      std::cout << std::hex << std::setw(2) << std::setfill('0') << b;
    }
    std::cout << std::endl;
  }
}

std::string to_titlecase(std::string const& str) {
  std::string res;
  res.resize(str.size());
  bool prev_is_space = true;
  for (int i = 0; i < str.size(); i++) {
    if (prev_is_space) {
      res[i] = toupper(str[i]);
      prev_is_space = false;
    } else {
      if ((res[i] = str[i]) == ' ') prev_is_space = true;
    }
  }
  return res;
}

void test_titlecase() {
  auto s = "the c++ challenger";
  std::cout << to_titlecase(s) << std::endl;
}

std::string concat(const std::vector<std::string>& strs, const std::string delimiter) {
  std::string res = "";
  if (strs.empty()) return "";
  for (const auto& s : strs) {
    res += s;
    res += delimiter;
  }
  res.pop_back();
  return res;
}

void test_concat() {
  std::vector<std::string> sample = {"this", "is", "an", "example"};
  std::cout << concat(sample, " ") << std::endl;
}

std::vector<std::string> split(const std::string input, const std::string delimiters) {
  std::vector<std::string> res;

  std::string s = "";
  for (const auto ch : input) {
    if (delimiters.find(ch) != std::string::npos) {
      if (!s.empty()) res.push_back(s);
      s = "";
    } else {
      s += ch;
    }
  }
  if (!s.empty()) res.push_back(s);
  return res;
}

void test_split() {
  std::string s = "this is an example";
  auto res = split(s, ",.! ");
  for (auto s : res) {
    std::cout << s << " ";
  }
  std::cout << std::endl;
}

std::string longest_palindrome(std::string s) {
  std::vector<int> radius(s.size());

  int i = 0, j = 0;
  while (i < s.size()) {
    while (i - j >= 0 && i + j < s.size() && s[i - j] == s[i + j]) {
      j++;
    }
    radius[i] = j;
    int k = 1;
    while (i - k >= 0 && i + k < s.size() && k + radius[i - k] < j) {
      radius[i + k] = radius[i - k];
      k++;
    }
    i += k;
    j -= k;
  }

  int max_center = 0;
  int max_length = 0;
  for (int i = 0; i < radius.size(); i++) {
    if (radius[i] > max_length) {
      max_center = i;
      max_length = radius[i];
    }
  }

  std::string res = "";
  for (int i = max_center - max_length + 1; i < max_center + max_length; i++) {
    res += s[i];
  }
  return res;
}

void test_longest_palindrome() {
  std::cout << longest_palindrome("sahararahnide") << std::endl;
  std::cout << longest_palindrome("level") << std::endl;
  std::cout << longest_palindrome("s") << std::endl;
}

bool validate_number_plate_format(std::string input) {
  std::regex re(R"([A-Z]{3}-[A-Z]{2} \d{3,4})");
  return std::regex_match(input.c_str(), re);
}

void test_validate_nubmer_plate_format() {
  std::vector<std::string> strings = {
      "ABC-DE 123",
      "ABC-DE 1234",
      "ABC-DE 12345",
      "abc-de 1234",
  };
  for (auto s : strings) {
    std::cout << validate_number_plate_format(s) << std::endl;
  }
}

std::vector<std::string> extract_license_plate_numbers(const std::string& input) {
  std::regex re(R"(([A-Z]{3}-[A-Z]{2} \d{3,4})*)");

  std::smatch match;
  std::vector<std::string> results;
  for (auto i = std::sregex_iterator(std::cbegin(input), std::cend(input), re);
       i != std::sregex_iterator(); i++) {
    if ((*i)[1].matched) results.push_back(i->str());
  }
  return results;
}

struct uri_parts {
  std::string protocol;
  std::string domain;
  std::optional<int> port;
  std::optional<std::string> path;
  std::optional<std::string> query;
  std::optional<std::string> fragment;
};

std::optional<uri_parts> parse_uri(std::string uri) {
  std::regex re(R"(^(\w+):\/\/([\w.-]+)(:(\d+))?)"
                R"(([\w\/\.]+)?(\?([\w=&]*)(#?(\w+))?)?$)");

  auto matches = std::smatch{};
  if (std::regex_match(uri, matches, re)) {
    if (matches[1].matched && matches[2].matched) {
      uri_parts parts;
      parts.protocol = matches[1].str();
      parts.domain = matches[2].str();
      if (matches[4].matched) parts.port = std::stoi(matches[4]);
      if (matches[5].matched) parts.path = matches[5];
      if (matches[7].matched) parts.query = matches[7];
      if (matches[9].matched) parts.fragment = matches[9];

      return parts;
    }
  }
  return {};
}

void test_parse_uri() {
  auto p1 = parse_uri("https://packt.com");
  assert(p1.has_value());
  assert(p1->protocol == "https");
  assert(p1->domain == "packt.com");
  assert(!p1->port.has_value());
  assert(!p1->path.has_value());
  assert(!p1->query.has_value());
  assert(!p1->fragment.has_value());

  auto p2 = parse_uri("https://bbc.com:80/en/index.html?lite=true#ui");
  assert(p2.has_value());
  assert(p2->protocol == "https");
  assert(p2->domain == "bbc.com");
  assert(p2->port == 80);
  assert(p2->path == "/en/index.html");
  assert(p2->query == "lite=true");
  assert(p2->fragment == "ui");
}

std::string convert_date_format(const std::string& input) {
  std::regex re(R"(([0-9]{2})(\.|-)([0-9]{2})(\.|-)([0-9]{4}))");
  return std::regex_replace(input.c_str(), re, "$5-$3-$1");
}

void test_convert_date_format() {
  using namespace std::string_literals;
  std::cout << convert_date_format("today is 01.12.2017!"s) << std::endl;
}

int main() { test_convert_date_format(); }
