#include <algorithm>
#include <array>
#include <cassert>
#include <chrono>
#include <cstddef>
#include <functional>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <map>
#include <memory>
#include <mutex>
#include <numeric>
#include <random>
#include <set>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <type_traits>
#include <utility>
#include <vector>

template <typename T, class Compare = std::less<typename std::vector<T>::value_type>>
class PriorityQueue {
  using value_type = typename std::vector<T>::value_type;
  using size_type = typename std::vector<T>::size_type;
  using reference = typename std::vector<T>::reference;
  using const_reference = typename std::vector<T>::const_reference;

 public:
  bool empty() const noexcept { return data_.empty(); }
  size_type size() const noexcept { return data_.size(); }

  void push(const value_type& value) {
    data_.push_back(value);
    std::push_heap(std::begin(data_), std::end(data_), compare_);
  }

  void pop() {
    std::pop_heap(std::begin(data_), std::end(data_), compare_);
    data_.pop_back();
  }

  const_reference top() const { return data_.front(); }

 private:
  std::vector<T> data_;
  Compare compare_;
};

void test_priority_queue() {
  PriorityQueue<int> q;
  for (int i : {1, 5, 3, 1, 13, 21, 8}) {
    q.push(i);
  }

  assert(!q.empty());
  assert(q.size() == 7);

  while (!q.empty()) {
    std::cout << q.top() << std::endl;
    q.pop();
  }
}

template <typename T>
class RingBufferIterator;

template <typename T>
class RingBuffer {
  using const_iterator = RingBufferIterator<T>;

 public:
  RingBuffer() = delete;
  explicit RingBuffer(const size_t size) : data_(size) {}

  void clear() noexcept {
    head_ = -1;
    size_ = 0;
  }

  bool empty() const noexcept { return size_ == 0; }
  bool full() const noexcept { return size_ == data_.size(); }
  size_t capacity() const noexcept { return data_.size(); }
  size_t size() const noexcept { return size_; }

  void push(const T x) {
    head_ = next_pos();
    data_[head_] = x;
    if (size_ < data_.size()) size_++;
  }

  T pop() {
    if (empty()) throw std::runtime_error("Empty buffer");

    size_t pos = first_pos();
    size_--;
    return data_[pos];
  }

  const_iterator begin() const { return const_iterator(*this, first_pos(), empty()); }

  const_iterator end() const { return const_iterator(*this, next_pos(), true); }

 private:
  std::vector<T> data_;
  size_t head_ = -1;
  size_t size_ = 0;

  size_t next_pos() const noexcept { return size_ == 0 ? 0 : (head_ + 1) % data_.size(); }

  size_t first_pos() const noexcept {
    return size_ == 0 ? 0 : (head_ + data_.size() - size_ + 1) % data_.size();
  }

  friend class RingBufferIterator<T>;
};

template <typename T>
class RingBufferIterator {
  using self_type = RingBufferIterator;
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;
  using const_pointer = const T*;
  using iterator_category = std::random_access_iterator_tag;
  using defference_type = ptrdiff_t;

 public:
  RingBufferIterator(const RingBuffer<T>& buf, const size_t pos, const bool last)
      : buffer_(buf), index_(pos), last_(last) {}

  self_type& operator++() {
    if (last_) throw std::out_of_range("Iterator cannot be incremented past the end of range.");
    index_ = (index_ + 1) % buffer_.data_.size();
    last_ = index_ == buffer_.next_pos();
    return *this;
  }

  self_type operator++(int) {
    self_type tmp = *this;
    ++*this;
    return tmp;
  }

  bool operator==(const self_type& other) const {
    assert(compatible(other));
    return index_ == other.index_ && last_ == other.last_;
  }

  bool operator!=(const self_type& other) const { return !(*this == other); }

  const_reference operator*() const { return buffer_.data_[index_]; }

  const_pointer operator->() const { return std::addressof(operator*()); }

 private:
  bool compatible(const self_type& other) const { return &buffer_ == &other.buffer_; }

  const RingBuffer<T>& buffer_;
  size_t index_;
  bool last_;
};

void test_ring_buffer() {
  RingBuffer<int> rbuf(5);
  for (int x : {1, 2, 3, 4}) {
    rbuf.push(x);
  }

  int x = rbuf.pop();
  assert(x == 1);
  for (int x : {5, 6, 7, 8}) {
    rbuf.push(x);
  }
  for (auto x : rbuf) {
    std::cout << x << " ";
  }
  std::cout << std::endl;
  x = rbuf.pop();
  x = rbuf.pop();
  for (auto x : rbuf) {
    std::cout << x << " ";
  }
  std::cout << std::endl;
}

template <typename T>
class DoubleBuffer {
  using value_type = T;
  using reference = T&;
  using const_reference = const T&;
  using pointer = T*;

 public:
  explicit DoubleBuffer(const size_t size) : rdbuf_(size), wrbuf_(size) {}

  size_t size() const noexcept { return rdbuf_.size(); }

  void write(const T* const ptr, const size_t size) {
    std::unique_lock<std::mutex> lock(mt_);
    auto length = std::min(size, wrbuf_.size());
    std::copy(ptr, ptr + length, std::begin(wrbuf_));
    wrbuf_.swap(rdbuf_);
  }

  template <class Output>
  void read(Output it) const {
    std::unique_lock<std::mutex> lock(mt_);
    std::copy(std::cbegin(rdbuf_), std::cend(rdbuf_), it);
  }

  pointer data() const {
    std::unique_lock<std::mutex> lock(mt_);
    return rdbuf_.data();
  }

  reference operator[](const size_t pos) {
    std::unique_lock<std::mutex> lock(mt_);
    return rdbuf_[pos];
  }

  const_reference operator[](const size_t pos) const {
    std::unique_lock<std::mutex> lock(mt_);
    return rdbuf_[pos];
  }

  void swap(DoubleBuffer other) {
    std::swap(rdbuf_, other.rdbuf_);
    std::swap(wrbuf_, other.wrbuf_);
  }

 private:
  std::vector<T> rdbuf_;
  std::vector<T> wrbuf_;
  mutable std::mutex mt_;
};

template <typename T>
void print_buffer(const DoubleBuffer<T>& buf) {
  buf.read(std::ostream_iterator<T>(std::cout, " "));
  std::cout << std::endl;
}

void test_double_buffer() {
  DoubleBuffer<int> buf(10);

  std::thread t([&buf]() {
    for (int i = 1; i < 1000; i += 10) {
      int data[10] = {};
      std::iota(std::begin(data), std::end(data), i);
      buf.write(data, std::size(data));
    }

    using namespace std::chrono_literals;
    std::this_thread::sleep_for(100ms);
  });

  auto start = std::chrono::system_clock::now();
  do {
    print_buffer(buf);
    using namespace std::chrono_literals;
    std::this_thread::sleep_for(150ms);
  } while (
      std::chrono::duration_cast<std::chrono::seconds>(std::chrono::system_clock::now() - start)
          .count() < 12);

  t.join();
}

template <typename T>
std::vector<std::pair<T, int>> find_most_frequent(const std::vector<T>& input) {
  std::map<T, int> counts;
  for (const auto& x : input) counts[x]++;

  auto maxelem = std::max_element(counts.cbegin(), counts.cend(),
                                  [](const auto& x, const auto& y) { return x.second < y.second; });

  std::vector<std::pair<T, int>> result;
  std::copy_if(counts.cbegin(), counts.cend(), std::back_inserter(result),
               [maxelem](const auto& kv) { return kv.second == maxelem->second; });

  return result;
}

void test_find_most_frequent() {
  auto input = std::vector<int>{1, 1, 3, 5, 8, 13, 3, 5, 8, 8, 5};
  auto result = find_most_frequent(input);
  for (const auto& [x, count] : result) {
    std::cout << x << ": " << count << std::endl;
  }
}

std::map<char, double> analyze_text(std::string_view text) {
  std::map<char, unsigned long long> counts;
  for (char ch = 'a'; ch <= 'z'; ch++) {
    counts[ch] = 0.;
  }

  for (auto ch : text) {
    if (isalpha(ch)) counts[tolower(ch)]++;
  }

  auto total =
      std::accumulate(counts.cbegin(), counts.cend(), 0ULL,
                      [](const unsigned long long sum, const auto& kv) { return sum + kv.second; });

  std::map<char, double> frequencies;
  for (const auto& [ch, count] : counts) {
    frequencies[ch] = (100. * count) / static_cast<double>(total);
  }

  return frequencies;
}

void test_analyze_text() {
  std::string text =
      R"(Lorem ipsum dolor sit amet, consectetur adipiscing elit, )"
      R"(sed do eiusmod tempor incididunt ut labore et dolore magna aliqua. )"
      R"(Ut enim ad minim veniam, quis nostrud exercitation ullamco laboris nisi ut aliquip ex ea commodo consequat. )"
      R"(Duis aute irure dolor in reprehenderit in voluptate velit esse cillum dolore eu fugiat nulla pariatur. )"
      R"(Excepteur sint occaecat cupidatat non proident, sunt in culpa qui officia deserunt mollit anim id est laborum.)";
  auto result = analyze_text(text);
  for (const auto& [ch, rate] : result) {
    std::cout << ch << ": " << std::fixed << std::setw(5) << std::setfill(' ')
              << std::setprecision(4) << rate << std::endl;
  }
}

std::vector<std::string> filter_phone_nubmers(const std::vector<std::string>& numbers,
                                              const std::string& country_code) {
  std::vector<std::string> result;
  std::copy_if(numbers.cbegin(), numbers.cend(), std::back_inserter(result),
               [country_code](std::string number) {
                 return number.starts_with("+" + country_code) || number.starts_with(country_code);
               });
  return result;
}

void test_filter_phone_numbers() {
  std::vector<std::string> numbers{
      "07555 123456", "07555 123456", "+44 07555 123456", "44 07555 123456", "7555 12345",
  };
  auto result = filter_phone_nubmers(numbers, "44");
  for (const auto& number : result) {
    std::cout << number << std::endl;
  }
}

std::vector<std::string> convert_phone_numbers(const std::vector<std::string>& numbers,
                                               const std::string& country_code) {
  std::vector<std::string> result;
  std::transform(numbers.cbegin(), numbers.cend(), std::back_inserter(result),
                 [country_code](std::string number) {
                   number.erase(std::remove_if(number.begin(), number.end(),
                                               [](const char ch) { return isspace(ch); }));
                   if (number == "") return number;

                   if (number.starts_with("0"))
                     return "+++" + country_code + number.substr(1);
                   else if (number.starts_with(country_code))
                     return "+" + number;
                   else if (number.starts_with("+" + country_code))
                     return number;
                   else
                     return "+" + country_code + number;
                 });
  return result;
}

void test_convert_phone_numbers() {
  std::vector<std::string> numbers{
      "07555 123456", "07555 123456", "+44 07555 123456", "44 07555 123456", "7555 12345",
  };
  auto result = convert_phone_numbers(numbers, "44");
  for (const auto& number : result) {
    std::cout << number << std::endl;
  }
}

void print_permutations(std::string input) {
  std::sort(input.begin(), input.end());
  do {
    std::cout << input << std::endl;
  } while (std::next_permutation(input.begin(), input.end()));
}

void next_perm_recursive(std::string input, std::string perm) {
  if (input.empty()) {
    std::cout << perm << std::endl;
    return;
  }

  for (size_t i = 0; i < input.size(); i++) {
    next_perm_recursive(input.substr(1), perm + input[0]);
    std::rotate(input.begin(), input.begin() + 1, input.end());
  }
}

void print_permutations_recursive(std::string input) { next_perm_recursive(input, ""); }

void test_print_permutations() {
  std::string input = "main";
  print_permutations(input);
  std::cout << std::endl;
  print_permutations_recursive(input);
}

double truncated_mean(std::vector<int> values, const double percentage) {
  std::sort(values.begin(), values.end());
  auto remove_count = static_cast<size_t>(values.size() * percentage + 0.5);

  values.erase(values.begin(), values.begin() + remove_count);
  values.erase(values.end() - remove_count, values.end());

  auto total = std::accumulate(values.cbegin(), values.cend(), 0ULL);
  return static_cast<double>(total) / values.size();
}

void test_truncated_mean() {
  std::vector<std::vector<int>> movies{
      {10, 9, 10, 9, 9, 8, 7, 10, 5, 9, 9, 8},
      {10, 5, 7, 8, 9, 8, 9, 10, 10, 5, 9, 8, 10},
      {10, 10, 10, 9, 3, 8, 8, 9, 6, 4, 7, 10},
  };

  for (auto& values : movies) {
    std::cout << truncated_mean(values, 0.05) << std::endl;
  }
}

template <typename Input, typename Output>
void pairwise(Input begin, Input end, Output result) {
  for (auto it = begin; it != end;) {
    const auto v1 = *it++;
    if (it == end) break;
    const auto v2 = *it++;
    result++ = std::make_pair(v1, v2);
  }
}

template <typename T>
std::vector<std::pair<T, T>> pairwise(const std::vector<T>& range) {
  std::vector<std::pair<T, T>> result;
  pairwise(std::cbegin(range), std::cend(range), std::back_inserter(result));
  return result;
}

void test_pairwise() {
  std::vector<int> v{1, 1, 3, 5, 8, 13, 21};
  auto result = pairwise(v);
  for (const auto& [v1, v2] : result) {
    std::cout << v1 << " " << v2 << std::endl;
  }
}

template <typename T, typename U, typename S>
void zip(T begin1, T end1, U begin2, U end2, S result) {
  auto it1 = begin1;
  auto it2 = begin2;
  while (it1 != end1 && it2 != end2) {
    result++ = std::make_pair(*it1++, *it2++);
  }
}

template <typename S, typename T>
std::vector<std::pair<S, T>> zip(std::vector<S>& range1, std::vector<T>& range2) {
  std::vector<std::pair<S, T>> result;
  zip(range1.cbegin(), range1.cend(), range2.cbegin(), range2.cend(), std::back_inserter(result));
  return result;
}

void test_zip() {
  std::vector<int> v1{1, 2, 3, 4, 5, 6, 7, 8, 9};
  std::vector<int> v2{1, 1, 3, 5, 8, 13, 21};
  auto result = zip(v1, v2);
  for (const auto v : result) {
    std::cout << v.first << " " << v.second << std::endl;
  }
}

template <typename T, typename A, typename F,
          typename R = typename std::decay<typename std::result_of<typename std::decay<F>::type&(
              typename std::vector<T, A>::const_reference)>::type>::type>
std::vector<R> select(const std::vector<T, A>& c, F&& f) {
  std::vector<R> v;
  std::transform(std::cbegin(c), std::cend(c), std::back_inserter(v), std::forward<F>(f));
  return v;
}

template <class RandomIt>
RandomIt partition(RandomIt first, RandomIt last) {
  auto pivot = *first;
  auto i = first + 1;
  auto j = last - 1;
  while (i <= j) {
    while (i <= j && *i <= pivot) i++;
    while (i <= j && *j > pivot) j--;
    if (i < j) std::iter_swap(i, j);
  }
  std::iter_swap(i - 1, first);
  return i - 1;
}

template <class RandomIt>
void quicksort(RandomIt first, RandomIt last) {
  if (first >= last) return;

  auto p = partition(first, last);
  quicksort(first, p);
  quicksort(p + 1, last);
}

void test_quicksort() {
  std::vector<int> v{1, 5, 3, 8, 6, 2, 9, 7, 4};
  quicksort(v.begin(), v.end());
  for (const auto x : v) {
    std::cout << x << " ";
  }
  std::cout << '\n';
}

template <typename Vertex = int, typename Weight = double>
class Graph {
 public:
  using vertex_type = Vertex;
  using weight_type = Weight;
  using neighbor_type = std::pair<Vertex, Weight>;
  using neighbor_list_type = std::vector<neighbor_type>;

  void add_edge(const Vertex src, const Vertex dst, const Weight weight,
                const bool bidirectional = true) {
    adjacency_list_[src].push_back(std::make_pair(dst, weight));
    if (bidirectional) adjacency_list_[dst].push_back(std::make_pair(src, weight));
  }

  size_t vertex_count() const { return adjacency_list_.size(); }

  std::vector<Vertex> vertices() const {
    std::vector<Vertex> keys;
    for (const auto& kv : adjacency_list_) {
      keys.push_back(kv.first);
    }
    return keys;
  }

  const neighbor_list_type& neighbors(const Vertex& v) const {
    auto pos = adjacency_list_.find(v);
    if (pos == adjacency_list_.end()) throw std::runtime_error("Vertex not found");
    return pos->second;
  }

  constexpr static Weight Infinity = std::numeric_limits<Weight>::infinity();

 private:
  std::map<vertex_type, neighbor_list_type> adjacency_list_;
};

template <typename Vertex, typename Weight>
void shortest_path(const Graph<Vertex, Weight>& graph, const Vertex source,
                   std::map<Vertex, Weight>& min_distance, std::map<Vertex, Vertex>& previous) {
  const auto vertices = graph.vertices();

  min_distance.clear();
  for (const auto& v : vertices) {
    min_distance[v] = Graph<Vertex, Weight>::Infinity;
  }
  min_distance[source] = 0;

  previous.clear();
  std::set<std::pair<Weight, Vertex>> vertex_queue;
  vertex_queue.insert(std::make_pair(min_distance[source], source));
  while (!vertex_queue.empty()) {
    auto dist = vertex_queue.begin()->first;
    auto u = vertex_queue.begin()->second;

    vertex_queue.erase(std::begin(vertex_queue));

    const auto& neighbors = graph.neighbors(u);
    for (const auto& [v, w] : neighbors) {
      auto dist_via_u = dist + w;
      if (dist_via_u < min_distance[v]) {
        vertex_queue.erase(std::make_pair(min_distance[v], v));

        min_distance[v] = dist_via_u;
        previous[v] = u;
        vertex_queue.insert(std::make_pair(min_distance[v], v));
      }
    }
  }
}

template <typename Vertex>
void build_path(const std::map<Vertex, Vertex>& prev, const Vertex v, std::vector<Vertex>& result) {
  result.push_back(v);

  auto pos = prev.find(v);
  if (pos == std::end(prev)) return;

  build_path(prev, pos->second, result);
}

template <typename Vertex>
std::vector<Vertex> build_path(const std::map<Vertex, Vertex>& prev, const Vertex v) {
  std::vector<Vertex> result;
  build_path(prev, v, result);
  std::reverse(result.begin(), result.end());
  return result;
}

template <typename Vertex>
void print_path(const std::vector<Vertex>& path) {
  for (size_t i = 0; i < path.size(); i++) {
    std::cout << path[i];
    if (i < path.size() - 1) std::cout << " -> ";
  }
}

void test_shortest_path() {
  Graph<char, double> g;
  g.add_edge('A', 'B', 7);
  g.add_edge('A', 'C', 9);
  g.add_edge('A', 'F', 14);
  g.add_edge('B', 'C', 10);
  g.add_edge('B', 'D', 15);
  g.add_edge('C', 'D', 11);
  g.add_edge('C', 'F', 2);
  g.add_edge('D', 'E', 6);
  g.add_edge('E', 'F', 9);

  char source = 'A';
  std::map<char, double> min_distance;
  std::map<char, char> previous;
  shortest_path(g, source, min_distance, previous);

  for (const auto& [v, w] : min_distance) {
    std::cout << source << "-> " << v << " : " << w << '\t';
    print_path(build_path(previous, v));
    std::cout << '\n';
  }
}

class Weasel {
  std::string target;
  std::uniform_int_distribution<> chardist;
  std::uniform_real_distribution<> ratedist;
  std::mt19937 mt;
  const std::string allowed_chars = "ABCDEFGHIJKLMNOPQRSTUVWZ ";

 public:
  explicit Weasel(std::string_view t) : target(t), chardist(0, 26), ratedist(0, 100) {
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size>{};
    std::generate(std::begin(seed_data), std::end(seed_data), std::ref(rd));
    std::seed_seq seq(std::cbegin(seed_data), std::cend(seed_data));
    mt.seed(seq);
  }

  void run(const int copies) {
    auto parent = make_random();
    int step = 1;
    std::cout << std::left << std::setw(5) << std::setfill(' ') << step << parent << std::endl;

    do {
      std::vector<std::string> children;
      std::generate_n(std::back_inserter(children), copies,
                      [parent, this]() { return mutate(parent, 5); });

      parent = *std::max_element(
          children.cbegin(), children.cend(),
          [this](std::string_view c1, std::string_view c2) { return fitness(c1) < fitness(c2); });

      std::cout << std::setw(5) << std::setfill(' ') << step << parent << std::endl;
      step++;
    } while (parent != target);
  }

  Weasel() = delete;

  int fitness(std::string_view candidate) {
    int score = 0;
    for (size_t i = 0; i < candidate.size(); i++) {
      if (candidate[i] == target[i]) score++;
    }
    return score;
  }

  std::string mutate(std::string_view parent, const double rate) {
    std::stringstream sstr;
    for (const auto c : parent) {
      auto nc = ratedist(mt) > rate ? c : allowed_chars[chardist(mt)];
      sstr << nc;
    }
    return sstr.str();
  }

  std::string make_random() {
    std::stringstream ss;
    for (size_t i = 0; i < target.size(); i++) {
      ss << allowed_chars[chardist(mt)];
    }
    return ss.str();
  }
};

void test_weasel() {
  Weasel w("METHINKS IT IS LIKE A WEASEL");
  w.run(100);
}

class Universe {
 private:
  Universe() = delete;

 public:
  enum class Seed { Random, TenCellRow };

  Universe(const size_t width, const size_t height)
      : rows_(height), columns_(width), grid_(width * height), dist_(0, 4) {
    std::random_device rd;
    auto seed_data = std::array<int, std::mt19937::state_size>{};
    std::generate(seed_data.begin(), seed_data.end(), std::ref(rd));
    std::seed_seq seq(seed_data.cbegin(), seed_data.cend());
    mt_.seed(seq);
  }

  void run(const Seed s, const int generations,
           const std::chrono::milliseconds ms = std::chrono::milliseconds(100)) {
    reset();
    initialize(s);
    display();

    int i = 0;
    do {
      next_generation();
      display();

      using namespace std::chrono_literals;
      std::this_thread::sleep_for(ms);
    } while (i++ < generations || generations == 0);
  }

 private:
  void next_generation() {
    std::vector<unsigned char> newgrid(grid_.size());

    for (size_t r = 0; r < rows_; r++) {
      for (size_t c = 0; c < columns_; c++) {
        auto count = count_neighbors(r, c);

        if (cell(c, r) == alive_)
          newgrid[r * columns_ + c] = (count == 2 || count == 3) ? alive_ : dead_;
        else
          newgrid[r * columns_ + c] = (count == 3) ? alive_ : dead_;
      }
    }

    grid_.swap(newgrid);
  }

  void reset_display() const {
#ifdef __APPLE__
    system("clear");
#endif
  }

  void display() {
    reset_display();
    for (size_t r = 0; r < rows_; r++) {
      for (size_t c = 0; c < columns_; c++) {
        std::cout << (cell(c, r) ? '*' : ' ');
      }
      std::cout << '\n';
    }
    std::flush(std::cout);
  }

  void initialize(const Seed s) {
    if (s == Seed::TenCellRow) {
      for (size_t c = columns_ / 2 - 5; c < columns_ / 2 + 5; c++) cell(c, rows_ / 2) = alive_;
    } else {
      for (size_t r = 0; r < rows_; r++) {
        for (size_t c = 0; c < columns_; c++) {
          cell(c, r) = dist_(mt_) == 0 ? alive_ : dead_;
        }
      }
    }
  }

  void reset() {
    for (size_t r = 0; r < rows_; r++) {
      for (size_t c = 0; c < columns_; c++) {
        cell(c, r) = dead_;
      }
    }
  }

  int count_alive() const { return 0; }

  template <typename T1, typename... T>
  auto count_alive(T1 s, T... ts) const {
    return s + count_alive(ts...);
  }

  int count_neighbors(const size_t col, const size_t row) {
    if (row == 0 && col == 0) return count_alive(cell(1, 0), cell(1, 1), cell(0, 1));
    if (row == 0 && col == columns_ - 1)
      return count_alive(cell(columns_ - 2, 0), cell(columns_ - 2, 1), cell(columns_ - 1, 1));
    if (row == rows_ - 1 && col == 0)
      return count_alive(cell(0, rows_ - 2), cell(1, rows_ - 2), cell(1, rows_ - 1));
    if (row == rows_ - 1 && col == columns_ - 1)
      return count_alive(cell(columns_ - 1, rows_ - 2), cell(columns_ - 2, rows_ - 2),
                         cell(columns_ - 2, rows_ - 1));
    if (row == 0 && col > 0 && col < columns_ - 1)
      return count_alive(cell(col - 1, 0), cell(col - 1, 1), cell(col, 1), cell(col + 1, 1),
                         cell(col + 1, 0));
    if (row == rows_ - 1 && col > 0 && col < columns_ - 1)
      return count_alive(cell(col - 1, row), cell(col - 1, row - 1), cell(col, row - 1),
                         cell(col + 1, row - 1), cell(col + 1, row));
    if (col == 0 && row > 0 && row < rows_ - 1)
      return count_alive(cell(0, row - 1), cell(1, row - 1), cell(1, row), cell(1, row + 1),
                         cell(0, row + 1));
    if (col == columns_ - 1 && row > 0 && row < rows_ - 1)
      return count_alive(cell(col, row - 1), cell(col - 1, row - 1), cell(col - 1, row),
                         cell(col - 1, row + 1), cell(col, row + 1));

    return count_alive(cell(col - 1, row - 1), cell(col, row - 1), cell(col + 1, row - 1),
                       cell(col + 1, row), cell(col + 1, row + 1), cell(col, row + 1),
                       cell(col - 1, row + 1), cell(col - 1, row));
  }
  unsigned char& cell(const size_t col, const size_t row) { return grid_[row * columns_ + col]; }

 private:
  const size_t rows_;
  const size_t columns_;

  std::vector<unsigned char> grid_;
  const unsigned char alive_ = 1;
  const unsigned char dead_ = 0;

  std::uniform_int_distribution<> dist_;
  std::mt19937 mt_;
};

void test_universe() {
  using namespace std::chrono_literals;
  Universe u(50, 20);
  u.run(Universe::Seed::Random, 100, 100ms);
}

int main() { test_universe(); }
