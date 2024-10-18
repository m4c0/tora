#pragma leco tool

import hai;
import jute;
import silog;
import tora;

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
