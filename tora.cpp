#pragma leco tool
#pragma leco add_impl sqlite3
#include "sqlite3.h"

import hai;
import silog;

struct deleter {
  void operator()(sqlite3 * db) { sqlite3_close(db); }
  void operator()(sqlite3_stmt * stmt) { sqlite3_finalize(stmt); }
};

class stmt {
  sqlite3 * m_db {};
  hai::value_holder<sqlite3_stmt *, deleter> m_stmt {};

public:
  explicit stmt(sqlite3 * db, sqlite3_stmt * s) : m_db { db }, m_stmt { s } {}

  [[nodiscard]] bool step() {
    switch (sqlite3_step(*m_stmt)) {
      case SQLITE_DONE: return false;
      case SQLITE_ROW: return true;
      default: silog::die("failed to step into query: %s", sqlite3_errmsg(m_db));
    }
  }
};

class db {
  hai::value_holder<sqlite3 *, deleter> m_db {};

  void check(int res, const char * msg) {
    if (SQLITE_OK != res) silog::die("%s %s", msg, sqlite3_errmsg(*m_db));
  }

public:
  explicit db(const char * file) {
    check(sqlite3_open(file, &*m_db), "failed to open DB:");
  };

  void exec(const char * sql) {
    check(sqlite3_exec(*m_db, sql, nullptr, nullptr, nullptr), "failed to execute query");
  }

  auto prepare(const char * sql) {
    sqlite3_stmt * s {};
    check(sqlite3_prepare_v2(*m_db, sql, -1, &s, nullptr), "failed to prepare query");
    return stmt { *m_db, s };
  }
};

int main() try {
  db db { ":memory:" };

  auto stmt = db.prepare("select 1 union select 2");
  while (stmt.step()) {
    silog::trace("row", 0);
  }
} catch (...) {
  return 1;
}
