#pragma leco tool
#pragma leco add_impl sqlite3
#include "sqlite3.h"

import silog;

int main() try {
  sqlite3 * db;

  if (SQLITE_OK != sqlite3_open(":memory:", &db)) silog::die("failed to open DB: %s", sqlite3_errmsg(db));

  sqlite3_close(db);
} catch (...) {
  return 1;
}
