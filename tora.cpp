#pragma leco tool
#pragma leco add_impl sqlite3
#include "sqlite3.h"

import hai;
import jute;
import silog;

struct deleter {
  void operator()(sqlite3 * db) { sqlite3_close(db); }
  void operator()(sqlite3_stmt * stmt) { sqlite3_finalize(stmt); }
};

class stmt {
  sqlite3 * m_db {};
  hai::value_holder<sqlite3_stmt *, deleter> m_stmt {};

  void check(int res, const char * msg) {
    if (SQLITE_OK != res) silog::die("%s %s", msg, sqlite3_errmsg(m_db));
  }

public:
  explicit stmt(sqlite3 * db, sqlite3_stmt * s) : m_db { db }, m_stmt { s } {}

  bool step() {
    switch (sqlite3_step(*m_stmt)) {
      case SQLITE_DONE: return false;
      case SQLITE_ROW: return true;
      default: silog::die("failed to step into query: %s", sqlite3_errmsg(m_db));
    }
  }

  void bind(unsigned i, jute::view str) {
    check(sqlite3_bind_text(*m_stmt, i, str.begin(), str.size(), SQLITE_TRANSIENT), "failed to bind parameter");
  }
  void bind(unsigned i, int n) {
    check(sqlite3_bind_int(*m_stmt, i, n), "failed to bind parameter");
  }

  auto column_int(unsigned i) { return sqlite3_column_int(*m_stmt, i); }
  auto column_text(unsigned i) { return sqlite3_column_text(*m_stmt, i); }
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

class args {
  int m_c;
  char ** m_v;
public:
  args(int argc, char ** argv) : m_c { argc - 1 }, m_v { argv + 1 } {};

  jute::view take() {
    return m_c-- ? jute::view::unsafe(*m_v++) : jute::view {};
  }
};

int main(int argc, char ** argv) try {
  args args { argc, argv };

  db db { "out/.tora" };

  auto cmd = args.take();
  if (cmd == "") cmd = "list";

  if (cmd == "init") {
    db.exec(R"(
      CREATE TABLE notification (
        id           INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        created_at   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        dismissed_at DATETIME,
        text         TEXT NOT NULL
      );
    )");
  } else if (cmd == "add") {
    auto stmt = db.prepare("INSERT INTO notification (text) VALUES (?)");
    stmt.bind(1, args.take());
    stmt.step();
  } else if (cmd == "list") {
    auto stmt = db.prepare("SELECT * FROM notification WHERE dismissed_at IS NULL");
    while (stmt.step()) {
      silog::log(silog::info, "%d %s %s", stmt.column_int(0), stmt.column_text(1), stmt.column_text(3));
    }
  } else if (cmd == "dismiss") {
    auto stmt = db.prepare("UPDATE notification SET dismissed_at = CURRENT_TIMESTAMP WHERE id = ?");
    stmt.bind(1, args.take());
    stmt.step();
  } else {
    silog::die("unknown command %s", cmd.begin());
  }
} catch (...) {
  return 1;
}
