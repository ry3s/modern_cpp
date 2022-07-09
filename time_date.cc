#include <chrono>
#include <iostream>
#include <stdexcept>
#include <thread>
#include <utility>

#include "iso_week.h"

template <typename Time = std::chrono::microseconds,
          typename Clock = std::chrono::high_resolution_clock>
struct perf_timer {
  template <typename F, typename... Args>
  static Time duration(F&& f, Args... args) {
    auto start = Clock::now();
    std::invoke(std::forward<F>(f), std::forward<Args>(args)...);
    auto end = Clock::now();
    return std::chrono::duration_cast<Time>(end - start);
  }
};

void f() {
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(2s);
}

void g(const int a, const int b) {
  using namespace std::chrono_literals;
  std::this_thread::sleep_for(1s);
}

void test_perf_timer() {
  auto t1 = perf_timer<std::chrono::microseconds>::duration(f);
  auto t2 = perf_timer<std::chrono::microseconds>::duration(g, 1, 2);
  auto total = std::chrono::duration<double, std::nano>(t1 + t2).count();
  std::cout << total << std::endl;
}

inline int number_of_days(const std::chrono::sys_days& first, const std::chrono::sys_days& last) {
  return (last - first).count();
}

void test_number_of_days() {
  using namespace std::chrono_literals;
  auto diff = number_of_days(2016y/7/23, 15d/5/2017);
  std::cout << diff << std::endl;
}

unsigned week_day(std::chrono::sys_days date) {
  std::chrono::year_month_weekday date_weekday{date};

  auto weekday2int = [](std::chrono::weekday wd) {
    if (wd == std::chrono::Sunday) return 1;
    if (wd == std::chrono::Monday) return 2;
    if (wd == std::chrono::Tuesday) return 3;
    if (wd == std::chrono::Wednesday) return 4;
    if (wd == std::chrono::Thursday) return 5;
    if (wd == std::chrono::Friday) return 6;
    if (wd == std::chrono::Saturday) return 7;
    throw std::runtime_error("Not reachable!");
  };
  return weekday2int(date_weekday.weekday());
}


void test_week_day() {
  using namespace std::chrono_literals;
  auto date = 2022y/6/30;
  std::cout << week_day(date) << std::endl;
}

int day_of_year(const std::chrono::year_month_day& date) {
  auto first_date = std::chrono::year_month_day{
    std::chrono::year{date.year()}, std::chrono::month{1}, std::chrono::day{0}};
  return (std::chrono::sys_days{ date } - std::chrono::sys_days{ first_date }).count();
}

unsigned int calendar_week(const std::chrono::year_month_day& date) {
  return day_of_year(date) / 7;
}

int main() { test_week_day(); }
