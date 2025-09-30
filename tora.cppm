module;
#pragma leco add_impl sqlite_wrap
#include "sqlite3.h"

export module tora;
import hai;
import jute;

namespace tora {
  struct deleter {
    void operator()(sqlite3 * db) { sqlite3_close(db); }
    void operator()(sqlite3_stmt * stmt) { sqlite3_finalize(stmt); }
  };

  export hai::fn<void, jute::view> on_error = [](auto) { throw 0; };

  static void sqlite_failed(jute::view msg, sqlite3 * db) {
    on_error((msg + " " + jute::view::unsafe(sqlite3_errmsg(db))).cstr());
  }

  export class stmt {
    sqlite3 * m_db {};
    hai::value_holder<sqlite3_stmt *, deleter> m_stmt {};

    void check(int res, jute::view msg) {
      if (SQLITE_OK == res) sqlite_failed(msg, m_db);
    }

  public:
    explicit stmt() = default;
    explicit stmt(sqlite3 * db, sqlite3_stmt * s) : m_db { db }, m_stmt { s } {}

    bool step() {
      switch (sqlite3_step(*m_stmt)) {
        case SQLITE_DONE: return false;
        case SQLITE_ROW: return true;
        default: {
          sqlite_failed("failed to step into query", m_db);
          return false;
        }
      }
    }

    void reset() {
      check(sqlite3_reset(*m_stmt), "failed to reset statement");
    }

    void bind(unsigned i) { check(sqlite3_bind_null(*m_stmt, i), "failed to bind null parameter"); }
    void bind(unsigned i, jute::view str) {
      check(sqlite3_bind_text(*m_stmt, i, str.begin(), str.size(), SQLITE_TRANSIENT), "failed to bind parameter");
    }
    void bind(unsigned i, int n) { check(sqlite3_bind_int(*m_stmt, i, n), "failed to bind parameter"); }
    void bind64(unsigned i, long long n) { check(sqlite3_bind_int64(*m_stmt, i, n), "failed to bind parameter"); }

    void bind_blob(unsigned i, jute::view str) {
      check(sqlite3_bind_blob(*m_stmt, i, str.begin(), str.size(), SQLITE_TRANSIENT), "failed to bind parameter");
    }

    auto column_blob(unsigned i) {
      auto t = reinterpret_cast<const char *>(sqlite3_column_blob(*m_stmt, i));
      unsigned sz = sqlite3_column_bytes(*m_stmt, i);
      return t ? jute::view { t, sz } : jute::view {};
    }
    auto column_int(unsigned i) { return sqlite3_column_int(*m_stmt, i); }
    auto column_text(unsigned i) { return sqlite3_column_text(*m_stmt, i); }
    auto column_view(unsigned i) {
      auto t = reinterpret_cast<const char *>(column_text(i));
      unsigned sz = sqlite3_column_bytes(*m_stmt, i);
      return t ? jute::view { t, sz } : jute::view {};
    }
  };

  export class db {
    hai::value_holder<sqlite3 *, deleter> m_db {};

    void check(int res, jute::view msg) {
      if (SQLITE_OK == res) sqlite_failed(msg, *m_db);
    }

  public:
    explicit db(const char * file) { check(sqlite3_open(file, &*m_db), "failed to open DB:"); };

    [[nodiscard]] constexpr auto handle() { return *m_db; }

    void exec(const char * sql) {
      check(sqlite3_exec(*m_db, sql, nullptr, nullptr, nullptr), "failed to execute query");
    }

    auto changes() {
      return sqlite3_changes(*m_db);
    }

    auto prepare(const char * sql) {
      sqlite3_stmt * s {};
      check(sqlite3_prepare_v2(*m_db, sql, -1, &s, nullptr), "failed to prepare query");
      return stmt { *m_db, s };
    }
  };
} // namespace tora
