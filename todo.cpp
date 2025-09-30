#pragma leco tool
#include <stdio.h>
#include <stdlib.h>

import args;
import jute;
import silog;
import tora;

// The quintessencial "TO-DO" application.
// Usage: todo <cmd> <cmd args>
// 
// Where cmd is one of:
//   init         - creates DB
//   add <text>   - creates an entry with "<text>" as content
//   list         - list existing items (default command)
//   dismiss <N>  - dismiss entry with ID <N>
//
// Currently data is stored inside a file named "out/.tora" for lazyness
// reasons
int main(int argc, char ** argv) try {
  tora::on_error = [](auto msg) { silog::die("%s", msg.cstr().begin()); };

  args args { argc, argv };

  tora::db db { (jute::view::unsafe(getenv("HOME")) + "/.todo").cstr().begin() };

  auto cmd = args.take();
  if (cmd == "") cmd = "list";

  if (cmd == "init") {
    db.exec(R"(
      CREATE TABLE notification (
        id           INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
        created_at   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
        dismissed_at DATETIME,
        flag         TEXT,
        text         TEXT NOT NULL
      );
    )");
  } else if (cmd == "add") {
    auto stmt = db.prepare("INSERT INTO notification (text) VALUES (?)");
    stmt.bind(1, args.take());
    stmt.step();
  } else if (cmd == "list") {
    auto stmt = db.prepare("SELECT * FROM notification WHERE dismissed_at IS NULL");
    bool odd = true;
    while (stmt.step()) {
      if (odd) printf("\e[48;5;232m");
      odd = !odd;

      auto flag = reinterpret_cast<const char *>(stmt.column_text(3));

      printf("\e[38;5;238m%s %4d \e[39m%s %s\e[0K\n",
          stmt.column_text(1),
          stmt.column_int(0),
          flag ? flag : "  ",
          stmt.column_text(4));
      printf("\e[0m\e[2K");
    }
  } else if (cmd == "edit") {
    auto stmt = db.prepare("UPDATE notification SET text = ? WHERE id = ?");
    stmt.bind(2, args.take_int());
    stmt.bind(1, args.take());
    stmt.step();
  } else if (cmd == "dismiss") {
    auto stmt = db.prepare("UPDATE notification SET dismissed_at = CURRENT_TIMESTAMP WHERE id = ?");
    stmt.bind(1, args.take());
    stmt.step();
  } else if (cmd == "important") {
    auto stmt = db.prepare("UPDATE notification SET flag = ? WHERE id = ?");
    stmt.bind(1, "🔴");
    stmt.bind(2, args.take());
    stmt.step();
  } else if (cmd == "urgent") {
    auto stmt = db.prepare("UPDATE notification SET flag = ? WHERE id = ?");
    stmt.bind(1, "💥");
    stmt.bind(2, args.take());
    stmt.step();
  } else {
    silog::die("unknown command %s", cmd.begin());
  }
} catch (...) {
  return 1;
}
