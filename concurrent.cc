#include <algorithm>
#include <iostream>
#include <iterator>
#include <utility>
#include <vector>
#include <thread>

template <typename RandomAccessIterator, typename F>
void ptransform(RandomAccessIterator begin, RandomAccessIterator end, F&& f) {
  auto size = std::distance(begin, end);
  if (size <= 10000) {
    std::transform(begin, end, std::forward<F>(f));
    return;
  }

  std::vector<std::thread> threads;
  const int num_thread = 8;
  auto first = begin;
  auto last = first;
  size /= num_thread;
  for (int i = 0; i < num_thread; i++) {
    first = last;
    if (i == num_thread - 1) last = end;
    else std::advance(last, size);

    threads.emplace_back([first, last, &f]() {
      std::transform(first, last, first, std::forward<F>(f));
    });

    for(auto& t : threads) t.join();
  }
}
int main() {}
