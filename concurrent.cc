#include <algorithm>
#include <array>
#include <chrono>
#include <future>
#include <iostream>
#include <iterator>
#include <mutex>
#include <numeric>
#include <random>
#include <string>
#include <string_view>
#include <thread>
#include <utility>
#include <vector>
#include <queue>

template <typename RandomAccessIterator, typename F>
void ptransform(RandomAccessIterator begin, RandomAccessIterator end, F&& f) {
  auto size = std::distance(begin, end);
  if (size <= 10000) {
    std::transform(begin, end, begin, std::forward<F>(f));
    return;
  }

  std::vector<std::thread> threads;
  const int num_thread = std::thread::hardware_concurrency();
  auto first = begin;
  auto last = first;
  size /= num_thread;
  for (int i = 0; i < num_thread; i++) {
    first = last;
    if (i == num_thread - 1)
      last = end;
    else
      std::advance(last, size);

    threads.emplace_back(
        [first, last, &f]() { std::transform(first, last, first, std::forward<F>(f)); });
  }
  for (auto& t : threads) t.join();
}

void test_ptransform() {
  std::vector<int> data(1000'000);
  ptransform(data.begin(), data.end(), [](const int e) { return e * e; });
}

template <typename Iterator, typename F>
auto pprocess(Iterator begin, Iterator end, F&& f) {
  auto size = std::distance(begin, end);
  if (size <= 10000) {
    return std::forward<F>(f)(begin, end);
  }

  const int num_thread = std::thread::hardware_concurrency();
  std::vector<std::thread> threads;
  std::vector<typename std::iterator_traits<Iterator>::value_type> mins(num_thread);

  auto first = begin;
  auto last = first;
  size /= num_thread;
  for (int i = 0; i < num_thread; i++) {
    first = last;
    if (i == num_thread - 1)
      last = end;
    else
      std::advance(last, size);

    threads.emplace_back(
        [first, last, &f, &r = mins[i]]() { r = std::forward<F>(f)(first, last); });
  }

  for (auto& t : threads) t.join();

  return std::forward<F>(f)(mins.begin(), mins.end());
}

template <typename Iterator>
auto pmin(Iterator begin, Iterator end) {
  return pprocess(begin, end, [](auto b, auto e) { return *std::min_element(b, e); });
}

template <typename Iterator>

auto pmax(Iterator begin, Iterator end) {
  return pprocess(begin, end, [](auto b, auto e) { return *std::max_element(b, e); });
}

void test_parallel_minmax() {
  const size_t count = 1000'000;
  std::vector<long> data(count);
  std::iota(data.begin(), data.end(), 1);

  auto rmin = pmin(data.cbegin(), data.cend());
  auto rmax = pmax(data.cbegin(), data.cend());
  std::cout << rmin << " " << rmax << "\n";
}

template <typename Iterator, typename F>
auto async_pprocess(Iterator begin, Iterator end, F&& f) {
  auto size = std::distance(begin, end);
  if (size <= 10000) {
    return std::forward<F>(f)(begin, end);
  }

  const int num_task = std::thread::hardware_concurrency();
  std::vector<std::future<typename std::iterator_traits<Iterator>::value_type>> tasks;

  auto first = begin;
  auto last = first;
  size /= num_task;
  for (int i = 0; i < num_task; i++) {
    first = last;
    if (i == num_task - 1)
      last = end;
    else
      std::advance(last, size);

    tasks.emplace_back(std::async(std::launch::async,
                                  [first, last, &f]() { return std::forward<F>(f)(first, last); }));
  }

  std::vector<typename std::iterator_traits<Iterator>::value_type> mins(num_task);
  for (auto& t : tasks) {
    mins.push_back(t.get());
  }

  return std::forward<F>(f)(mins.begin(), mins.end());
}

template <typename Iterator>
auto async_pmin(Iterator begin, Iterator end) {
  return pprocess(begin, end, [](auto b, auto e) { return *std::min_element(b, e); });
}

template <typename Iterator>

auto async_pmax(Iterator begin, Iterator end) {
  return pprocess(begin, end, [](auto b, auto e) { return *std::max_element(b, e); });
}

void test_async_minmax() {
  const size_t count = 1000'000;
  std::vector<long> data(count);
  std::iota(data.begin(), data.end(), 1);

  auto rmin = async_pmin(data.cbegin(), data.cend());
  auto rmax = async_pmax(data.cbegin(), data.cend());
  std::cout << rmin << " " << rmax << "\n";
}

template <typename RandomAccessIterator>
void pquicksort(RandomAccessIterator first, RandomAccessIterator last) {
  if (last - first < 2) return;

  const auto pivot_value = *first;
  const auto p =
      std::stable_partition(first, last, [pivot_value](auto x) { return x < pivot_value; });
  if (last - first <= 1000000) {
    pquicksort(first, p);
    pquicksort(p + 1, last);
  } else {
    auto f1 = std::async(std::launch::async, [first, p]() { pquicksort(first, p); });
    auto f2 = std::async(std::launch::async, [last, p]() { pquicksort(p + 1, last); });
    f1.wait();
    f2.wait();
  }
}

void test_pquicksort() {
  std::random_device rd;
  std::mt19937 mt;
  auto seed_data = std::array<int, std::mt19937::state_size>{};
  std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
  std::seed_seq seq(seed_data.cbegin(), seed_data.cend());
  mt.seed(seq);
  std::uniform_int_distribution<> ud(1, 1000000);

  const size_t count = 100;
  std::vector<long> data(count);
  std::generate_n(data.begin(), count, [&mt, &ud]() { return ud(mt); });

  pquicksort(data.begin(), data.end());
  std::cout << std::is_sorted(data.cbegin(), data.cend()) << "\n";
}

class Logger {
 protected:
  Logger() = default;

 public:
  static Logger& instance() {
    static Logger logger;
    return logger;
  }

  Logger(const Logger&) = delete;  // copy constructor
  Logger& operator=(const Logger&) = delete;

  void log(const std::string_view message) {
    std::lock_guard<std::mutex> lock(mt_);
    std::cout << "LOG: " << message << std::endl;
  }

 private:
  std::mutex mt_;
};

void test_logger() {
  std::vector<std::thread> threads;
  for (int i = 1; i <= 7; i++) {
    threads.emplace_back([i]() {
      std::random_device rd;
      std::mt19937 mt(rd());
      std::uniform_int_distribution<> ud(100, 1000);

      Logger::instance().log("thread " + std::to_string(i) + " started");
      std::this_thread::sleep_for(std::chrono::milliseconds(ud(mt)));
      Logger::instance().log("thread " + std::to_string(i) + " finished");
    });
  }

  for (auto& t : threads) t.join();
}

class TicketingMachine {
 public:
  explicit TicketingMachine(const int start) : first_ticket_(start), last_ticket_(start) {}

  int next() { return last_ticket_++; }
  int last() const { return last_ticket_; }

  void reset() { last_ticket_ = first_ticket_; }

 private:
  int first_ticket_;
  int last_ticket_;
};

class Customer {
 public:
  explicit Customer(const int no) : number_(no) {}

  int ticket_number() const noexcept { return number_; }

 private:
  int number_;
  friend bool operator<(const Customer& left, const Customer& right);
};

bool operator<(const Customer& left, const Customer& right) { return left.number_ > right.number_; }

void test_customer_service() {
  std::priority_queue<Customer> customers;
  bool office_open = true;
  std::mutex mt;
  std::condition_variable cv;

  std::vector<std::thread> desks;
  for (int i = 1; i <= 3; i++) {
    desks.emplace_back([i, &office_open, &mt, &cv, &customers]() {
      std::random_device rd;
      auto seed_data = std::array<int, std::mt19937::state_size>{};
      std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
      std::seed_seq seq(seed_data.cbegin(), seed_data.cend());
      std::mt19937 eng(seq);
      std::uniform_int_distribution<> ud(2000, 3000);

      Logger::instance().log("desk " + std::to_string(i) + " open");

      while (office_open || !customers.empty()) {
        std::unique_lock<std::mutex> locker(mt);

        cv.wait_for(locker, std::chrono::seconds(1), [&customers]() { return !customers.empty(); });

        if (!customers.empty()) {
          const auto c = customers.top();
          customers.pop();

          Logger::instance().log("[-] desk " + std::to_string(i) + " handling customer "
                                 + std::to_string(c.ticket_number()));
          Logger::instance().log("[=] queue size:  " + std::to_string(customers.size()));

          locker.unlock();
          cv.notify_one();

          std::this_thread::sleep_for(std::chrono::milliseconds(ud(eng)));

          Logger::instance().log("[ ] desk " + std::to_string(i) + " done with customer "
                                 + std::to_string(c.ticket_number()));

        }
      }
      Logger::instance().log("desk " + std::to_string(i) + " closed");
    });
  }

  std::thread store([&office_open, &customers, &cv]() {
    TicketingMachine tm(100);
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size>{};
    std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
    std::seed_seq seq(seed_data.cbegin(), seed_data.cend());
    std::mt19937 eng(seq);
    std::uniform_int_distribution<> ud(200, 500);

    for (int i = 1; i <= 25; i++) {
      Customer c(tm.next());
      customers.push(c);

      Logger::instance().log("[+] new customer with ticket " + std::to_string(c.ticket_number()));
      Logger::instance().log("[=] queue size: " + std::to_string(customers.size()));

      cv.notify_one();
      std::this_thread::sleep_for(std::chrono::milliseconds(ud(eng)));
    }
    office_open = false;
  });

  store.join();
  for (auto& desk : desks) desk.join();
}

int main() { test_customer_service(); }
