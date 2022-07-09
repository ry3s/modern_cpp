#include <pwd.h>
#include <stdlib.h>
#include <sys/sysctl.h>
#include <uuid/uuid.h>

#include <algorithm>
#include <chrono>
#include <cmath>
#include <exception>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <iterator>
#include <numeric>
#include <optional>
#include <regex>
#include <stdexcept>
#include <string>
#include <vector>

unsigned number_of_digits(const unsigned i) { return i > 0 ? (int)log10((double)i) + 1 : i; }

void print_pascal_triangle(int n) {
  for (int i = 0; i < n; i++) {
    int x = 1;
    std::cout << std::string((n - i - 1) * (n / 2), ' ');
    for (int j = 0; j <= i; j++) {
      int y = x;
      x = x * (i - j) / (j + 1);
      int maxlen = number_of_digits(x) - 1;
      std::cout << y << std::string(n - 1 - maxlen - n % 2, ' ');
    }
    std::cout << std::endl;
  }
}

void test_print_pascal_triangle() { print_pascal_triangle(5); }

enum class procstatus { idle, running, sleep, stop, zombie };

struct procinfo {
  int pid;
  std::string name;
  procstatus status;
  std::string account;
  uint64_t memory;
};

procstatus stat2procstatus(int stat) {
  switch (stat) {
    case SIDL:
      return procstatus::idle;
    case SRUN:
      return procstatus::running;
    case SSLEEP:
      return procstatus::sleep;
    case SSTOP:
      return procstatus::stop;
    case SZOMB:
      return procstatus::zombie;
    default:
      return procstatus::zombie;
  }
}

std::vector<procinfo> get_process() {
  int mib[4] = {CTL_KERN, KERN_PROC, KERN_PROC_ALL, 0};
  struct kinfo_proc *info;
  size_t length;

  if (sysctl(mib, 4, NULL, &length, NULL, 0) < 0) throw std::invalid_argument("TODO");
  if (!(info = (kinfo_proc *)malloc(length))) throw std::invalid_argument("TODO");
  if (sysctl(mib, 4, info, &length, NULL, 0) < 0) {
    free(info);
    throw std::invalid_argument("TODO");
  }

  std::vector<procinfo> procs;
  int count = length / sizeof(struct kinfo_proc);
  for (int i = 0; i < count; i++) {
    pid_t pid = info[i].kp_proc.p_pid;
    uid_t uid = info[i].kp_eproc.e_pcred.p_ruid;
    char *username = user_from_uid(uid, 0);
    char *command = info[i].kp_proc.p_comm;
    int stat = info[i].kp_proc.p_stat;

    procinfo pinfo;
    pinfo.pid = pid;
    pinfo.name = std::string(command);
    pinfo.status = stat2procstatus(stat);
    pinfo.account = std::string(username);
    pinfo.memory = 0;
    procs.push_back(pinfo);
  }

  free(info);
  return procs;
}

std::string status_to_string(const procstatus status) {
  switch (status) {
    case procstatus::idle:
      return "idle";
    case procstatus::running:
      return "running";
    case procstatus::sleep:
      return "sleep";
    case procstatus::stop:
      return "stop";
    case procstatus::zombie:
      return "zombie";
  }
}

void print_processes() {
  auto processes = get_process();
  std::sort(processes.begin(), processes.end(),
            [](const procinfo &p1, const procinfo &p2) { return p1.name < p2.name; });

  for (const auto &p : processes) {
    std::cout << std::left << std::setw(25) << std::setfill(' ') << p.name;
    std::cout << std::left << std::setw(8) << std::setfill(' ') << p.pid;
    std::cout << std::left << std::setw(12) << std::setfill(' ') << status_to_string(p.status);
    std::cout << std::left << std::setw(15) << std::setfill(' ') << p.account;
    std::cout << std::left << std::setw(10) << std::setfill(' ')
              << static_cast<int>(p.memory / 1024);
    std::cout << std::endl;
  }
}

void remove_empty_lines(std::filesystem::path filepath) {
  std::ifstream filein(filepath.native(), std::ios::in);

  if (!filein.is_open()) throw std::runtime_error("Cannot open input file");
  auto temppath = std::filesystem::temp_directory_path() / "tmep.txt";
  std::ofstream fileout(temppath.native(), std::ios::out | std::ios::trunc);
  if (!fileout.is_open()) throw std::runtime_error("Cannot create temporary file");

  std::string line;
  while (std::getline(filein, line)) {
    if (line.length() > 0 && line.find_first_not_of(' ') != line.npos) fileout << line << '\n';
  }

  filein.close();
  fileout.close();
  std::filesystem::remove(filepath);
  std::filesystem::rename(temppath, filepath);
}

void test_remove_empty_lines() {
  auto filepath = std::filesystem::path("./sample.txt");
  remove_empty_lines(filepath);
}

unsigned long long get_directory_size(const std::filesystem::path &dir,
                                      const bool follow_symlinks = false) {
  auto iter = std::filesystem::recursive_directory_iterator(
      dir, follow_symlinks ? std::filesystem::directory_options::follow_directory_symlink
                           : std::filesystem::directory_options::none);
  return std::accumulate(
      std::filesystem::begin(iter), std::filesystem::end(iter), 0ULL,
      [](const unsigned long long total, const std::filesystem::directory_entry &entry) {
        return total + (std::filesystem::is_regular_file(entry)
                            ? std::filesystem::file_size(entry.path())
                            : 0ULL);
      });
}

void test_get_directory_size() {
  auto path = std::filesystem::path(".");
  std::cout << get_directory_size(path) << std::endl;
}

namespace fs = std::filesystem;
namespace ch = std::chrono;

template <typename Duration>
bool is_older_than(const fs::path &path, const Duration duration) {
  auto file_time = fs::last_write_time(path).time_since_epoch();
  auto now = (ch::system_clock::now() - duration).time_since_epoch();
  return ch::duration_cast<Duration>(now - file_time).count() > 0;
}

template <typename Duration>
void remove_files_older_than(const std::filesystem::path &path, const Duration duration) {
  try {
    if (!fs::exists(path)) return;
    if (is_older_than(path, duration)) {
      fs::remove(path);
    } else if (fs::is_directory(path)) {
      for (const auto &entry : fs::directory_iterator(path)) {
        remove_files_older_than(entry.path(), duration);
      }
    }
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
  }
}

std::vector<fs::directory_entry> find_files(const fs::path &path, const std::string &regex) {
  std::vector<fs::directory_entry> result;
  std::regex re(regex.c_str());

  std::copy_if(fs::recursive_directory_iterator(path), fs::recursive_directory_iterator(),
               std::back_insert_iterator(result), [&re](const fs::directory_entry &entry) {
                 return fs::is_regular_file(entry.path()) &&
                        std::regex_match(entry.path().filename().string(), re);
               });
  return result;
}

class Logger {
 private:
  fs::path logpath;
  std::ofstream logfile;

 public:
  Logger() {
    uuid_t uuid;
    uuid_generate_random(uuid);
    uuid_string_t name;
    uuid_unparse(uuid, name);
    logpath = fs::temp_directory_path() / (std::string(name) + ".tmp");
    logfile.open(logpath.c_str(), std::ios::out | std::ios::trunc);
  }

  ~Logger() noexcept {
    try {
      if (logfile.is_open()) logfile.close();
      if (!logpath.empty()) fs::remove(logpath);
    } catch (...) {
    }
  }

  void presist(const fs::path &path) {
    logfile.close();
    fs::rename(logpath, path);
    logpath.clear();
  }

  Logger &operator<<(const std::string &message) {
    logfile << message.c_str() << '\n';
    return *this;
  }
};

void test_logger() {
  Logger logger;
  try {
    logger << "This is a line."
           << "This is another one.";
    throw std::runtime_error("error");
  } catch (...) {
    logger.presist(R"(lastlog.txt)");
  }
}

int main() { test_logger(); }
