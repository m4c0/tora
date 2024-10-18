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
  )");
}

static void brag_list(tora::db & db) {
  auto stmt = db.prepare(R"(
    SELECT created_at, demoable, code, size, name 
    FROM brag
    ORDER BY created_at DESC
  )");
  while (stmt.step()) {
    printf("[%s] %c %c %2s %s\n",
        stmt.column_text(0),
        stmt.column_int(1) == 0 ? ' ' : 'D',
        stmt.column_int(2) == 0 ? ' ' : 'C',
        stmt.column_text(3),
        stmt.column_text(4));
  }
}

static void brag(tora::db & db, args & args) {
  auto cmd = args.take();
  if (cmd == "") cmd = "list";

  if (cmd == "list") brag_list(db); 
  else silog::die("unknown command [brag %s]", cmd.begin());
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
  if (cmd == "") cmd = "brag";

  if (cmd == "brag") brag(db, args);
  else silog::die("unknown command %s", cmd.begin());
} catch (...) {
  return 1;
}

