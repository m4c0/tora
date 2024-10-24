//
// This is a tool I use for keeping a "brag list" at work, which I use for
// input for promotion docs, 1:1s, etc
//
#pragma leco tool
import args;
import hai;
import jojo;
import jute;
import mtime;
import silog;
import tora;

#include <stdio.h>
#include <stdlib.h>

static void init(tora::db & db) {
  db.exec(R"(
    CREATE TABLE size (
      id           TEXT NOT NULL PRIMARY KEY
    ) STRICT;
    INSERT INTO size VALUES ('XS');
    INSERT INTO size VALUES ('S');
    INSERT INTO size VALUES ('M');
    INSERT INTO size VALUES ('L');
    INSERT INTO size VALUES ('XL');

    CREATE TABLE link_type (
      id           TEXT NOT NULL PRIMARY KEY
    ) STRICT;
    INSERT INTO link_type VALUES ('DEMO');
    INSERT INTO link_type VALUES ('CODE');
    INSERT INTO link_type VALUES ('TASK');

    CREATE TABLE brag (
      id           INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
      name         TEXT NOT NULL,
      created_at   TEXT NOT NULL DEFAULT CURRENT_DATE,
      demoable     INT NOT NULL DEFAULT FALSE,
      code         INT NOT NULL DEFAULT FALSE,
      size         TEXT REFERENCES size(id)
    ) STRICT;

    CREATE TABLE link (
      id           INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
      brag         INTEGER NOT NULL REFERENCES brag(id),
      type         TEXT REFERENCES link_type(id),
      href         TEXT NOT NULL,
      notes        TEXT NULL
    ) STRICT;

    CREATE TABLE sprint (
      name         TEXT NOT NULL PRIMARY KEY
    ) STRICT;
    CREATE TABLE sprint_brag (
      brag         INTEGER NOT NULL REFERENCES brag(id),
      sprint       TEXT NOT NULL REFERENCES sprint(name),
      UNIQUE (brag, sprint)
    ) STRICT;

    CREATE TABLE comment (
      id           INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
      brag         INTEGER NOT NULL REFERENCES brag(id),
      notes        TEXT NOT NULL
    ) STRICT;
  )");
}

static void print_id(tora::stmt & stmt, unsigned index) {
  printf("\e[38;5;237m%4d\e[39m ", stmt.column_int(index));
}

static void print_brag(tora::stmt & stmt) {
  constexpr const unsigned char empty_size[3] { "--" };

  int size_colour = 8;
  auto size = stmt.column_text(3);
  if (size != nullptr) {
    auto sz = jute::view::unsafe(reinterpret_cast<const char *>(size));
    if (sz == "XS") size_colour = 5;
    else if (sz == "S") size_colour = 99;
    else if (sz == "M") size_colour = 11;
    else if (sz == "L") size_colour = 21;
    else if (sz == "XL") size_colour = 39;
  }

  print_id(stmt, 5);
  printf("[%s]    %s %s \e[38;5;%dm%2s\e[39m    %s\e[0K\n",
      stmt.column_text(0),
      stmt.column_int(1) == 0 ? "---" : "\e[31m[D]\e[39m",
      stmt.column_int(2) == 0 ? "---" : "\e[32m[C]\e[39m",
      size_colour,
      size == nullptr ? empty_size : size,
      stmt.column_text(4));
}

static void brag_list(tora::db & db, args & args, bool full = false) {
  if (args.take() != "") silog::die("'list' doesn't have sub-commands");

  auto stmt = db.prepare(R"(
    SELECT created_at, demoable, code, size, name, id
    FROM brag
    ORDER BY created_at ASC
  )");
  bool odd = true;
  while (stmt.step()) {
    if (odd) printf("\e[48;5;232m");
    odd = !odd;

    print_brag(stmt);

    printf("\e[0m\e[2K");
  }
}
void brag_view(tora::db & db, args & args) {
  auto id = args.take_int();
  if (args.take() != "") silog::die("excess of arguments");

  auto stmt = db.prepare(R"(
    SELECT created_at, demoable, code, size, name, id
    FROM brag
    WHERE id = ?
  )");
  stmt.bind(1, id);
  if (!stmt.step()) silog::die("brag with id %d not found", id);
  print_brag(stmt);
  printf("\n");

  auto s2 = db.prepare("SELECT sprint FROM sprint_brag WHERE brag = ? ORDER BY sprint");
  s2.bind(1, id);
  printf("     Sprints: ");
  if (s2.step()) printf("%s", s2.column_text(0));
  while (s2.step()) printf(", %s", s2.column_text(0));
  printf("\n");

  s2 = db.prepare("SELECT type, href, notes, id FROM link WHERE brag = ?");
  s2.bind(1, id);
  printf("     Links:\n");
  while (s2.step()) {
    int colour = 39;
    auto type = jute::view::unsafe(reinterpret_cast<const char *>(s2.column_text(0)));
    if (type == "CODE") colour = 31;
    else if (type == "DEMO") colour = 32;
    else if (type == "TASK") colour = 33;

    print_id(s2, 3);
    printf("- \e[%dm%s\e[39m %s", colour, s2.column_text(0), s2.column_text(1));
    if (s2.column_text(2)) printf(" \e[38;5;238m(%s)\e[39m", s2.column_text(2));
    printf("\n");
  }

  s2 = db.prepare("SELECT notes, id FROM comment WHERE brag = ?");
  s2.bind(1, id);
  while (s2.step()) {
    auto notes = jute::view::unsafe(reinterpret_cast<const char *>(s2.column_text(0)));
    bool first = true;
    while (notes != "") {
      auto [l, r] = notes.split('\n');
      if (first) {
        printf("\n");
        print_id(s2, 1);
        printf("%.*s", static_cast<int>(l.size()), l.data());
        first = false;
      } else {
        printf("\n%4s %.*s", "", static_cast<int>(l.size()), l.data());
      }
      notes = r;
    }
  }
  printf("\n");
}

static void brag_prompt(tora::db & db, args & args) {
  if (args.take() != "") silog::die("'prompt' doesn't have sub-commands");

  printf("Given the following notes, write a promotion document.\n\n");

  auto stmt = db.prepare("SELECT id, name FROM brag ORDER BY created_at DESC");
  while (stmt.step()) {
    printf("%s\n", stmt.column_text(1));

    auto s2 = db.prepare("SELECT type, href, notes FROM link WHERE brag = ?");
    s2.bind(1, stmt.column_int(0));
    printf("    Links:\n");
    while (s2.step()) {
      printf("    - %s %s",
          s2.column_text(0),
          s2.column_text(1));
      if (s2.column_text(2)) printf(" (%s)", s2.column_text(2));
      printf("\n");
    }

    s2 = db.prepare("SELECT notes FROM comment WHERE brag = ?");
    s2.bind(1, stmt.column_int(0));
    while (s2.step()) {
      auto notes = jute::view::unsafe(reinterpret_cast<const char *>(s2.column_text(0)));
      while (notes != "") {
        auto [l, r] = notes.split('\n');
        printf("\n    %.*s\n", static_cast<int>(l.size()), l.data());
        notes = r;
      }
    }

    printf("\n");
  }
}

void brag_add(tora::db & db, args & args) {
  auto stmt = db.prepare("INSERT INTO brag (name) VALUES (?)");
  stmt.bind(1, args.take());
  if (args.take() != "") silog::die("excess of parameters");
  stmt.step();
}
void brag_rename(tora::db & db, args & args) {
  auto stmt = db.prepare("UPDATE brag SET name = ? WHERE id = ?");
  stmt.bind(2, args.take_int());
  stmt.bind(1, args.take());
  if (args.take() != "") silog::die("excess of parameters");
  stmt.step();
}
void brag_size(tora::db & db, args & args) {
  auto stmt = db.prepare("UPDATE brag SET size = ? WHERE id = ?");
  stmt.bind(2, args.take_int());
  stmt.bind(1, args.take());
  if (args.take() != "") silog::die("excess of parameters");
  stmt.step();
}
void brag_demo(tora::db & db, args & args) {
  auto stmt = db.prepare("UPDATE brag SET demoable = 1 - demoable WHERE id = ?");
  stmt.bind(1, args.take());
  if (args.take() != "") silog::die("excess of parameters");
  stmt.step();
}
void brag_code(tora::db & db, args & args) {
  auto stmt = db.prepare("UPDATE brag SET code = 1 - code WHERE id = ?");
  stmt.bind(1, args.take());
  if (args.take() != "") silog::die("excess of parameters");
  stmt.step();
}

void brag_comment_add(tora::db & db, args & args) {
  auto stmt = db.prepare("INSERT INTO comment (brag, notes) VALUES (?, ?)");
  stmt.bind(1, args.take_int());
  stmt.bind(2, args.take());
  if (args.take() != "") silog::die("excess of parameters");
  stmt.step();
}

void brag_comment(tora::db & db, args & args) {
  auto cmd = args.take();
  if (cmd == "add") brag_comment_add(db, args);
  else silog::die("invalid comment sub-command");
}

void brag_link_add(tora::db & db, args & args) {
  auto stmt = db.prepare("INSERT INTO link (brag, type, href, notes) VALUES (?, ?, ?, ?)");
  stmt.bind(1, args.take_int());
  stmt.bind(2, args.take());
  stmt.bind(3, args.take());
  stmt.bind(4, args.take());
  if (args.take() != "") silog::die("excess of parameters");
  stmt.step();
}
void brag_link_type(tora::db & db, args & args) {
  auto stmt = db.prepare("UPDATE link SET type = ? WHERE id = ?");
  stmt.bind(2, args.take_int());
  stmt.bind(1, args.take());
  if (args.take() != "") silog::die("excess of parameters");
  stmt.step();
}
void brag_link_notes(tora::db & db, args & args) {
  auto stmt = db.prepare("UPDATE link SET notes = ? WHERE id = ?");
  stmt.bind(2, args.take_int());
  stmt.bind(1, args.take());
  if (args.take() != "") silog::die("excess of parameters");
  stmt.step();
}
void brag_link_delete(tora::db & db, args & args) {
  auto stmt = db.prepare("DELETE FROM link WHERE id = ?");
  stmt.bind(1, args.take_int());
  if (args.take() != "") silog::die("excess of parameters");
  stmt.step();
}

void brag_link(tora::db & db, args & args) {
  auto cmd = args.take();
  if (cmd == "add") brag_link_add(db, args);
  else if (cmd == "type") brag_link_type(db, args);
  else if (cmd == "notes") brag_link_notes(db, args);
  else if (cmd == "delete") brag_link_delete(db, args);
  else silog::die("invalid link sub-command");
}

int main(int argc, char ** argv) try {
  args args { argc, argv };

  hai::cstr file = jute::view { ":memory:" }.cstr();
  bool create = true;
  if (mtime::of("test.sql") == 0) {
    file = (jute::view::unsafe(getenv("HOME")) + "/.brag").cstr();
    create = mtime::of(file.begin()) == 0;
  }

  tora::db db { file.begin() };
  if (create) init(db);

  db.exec("PRAGMA foreign_key");

  if (mtime::of("test.sql")) db.exec(jojo::read_cstr("test.sql").begin());

  auto cmd = args.take();
  if (cmd == "") cmd = "list";

  if (cmd == "list") brag_list(db, args); 
  else if (cmd == "view") brag_view(db, args);
  else if (cmd == "prompt") brag_prompt(db, args);
  else if (cmd == "add") brag_add(db, args);
  else if (cmd == "size") brag_size(db, args);
  else if (cmd == "demo") brag_demo(db, args);
  else if (cmd == "code") brag_code(db, args);
  else if (cmd == "rename") brag_rename(db, args);
  else if (cmd == "comment") brag_comment(db, args);
  else if (cmd == "link") brag_link(db, args);
  else silog::die("unknown command [%s]", cmd.begin());
} catch (...) {
  return 1;
}

