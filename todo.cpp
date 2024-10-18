#pragma leco tool

import args;
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
  args args { argc, argv };

  tora::db db { "out/.tora" };

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
