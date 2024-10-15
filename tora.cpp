#pragma leco tool
#pragma leco add_impl sqlite3
#include "sqlite3.h"

import hai;
import silog;

struct deleter {
  void operator()(sqlite3 * db) { sqlite3_close(db); }
};

class db {
  hai::value_holder<sqlite3 *, deleter> m_db {};

public:
  explicit db(const char * file) {
    if (SQLITE_OK != sqlite3_open(file, &*m_db)) silog::die("failed to open DB: %s", sqlite3_errmsg(*m_db));
  };
};

int main() try {
  db db { ":memory:" };
} catch (...) {
  return 1;
}
