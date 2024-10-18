//
// This is a tool I use for keeping a "brag list" at work, which I use for
// input for promotion docs, 1:1s, etc
//
#pragma leco tool
import args;
import jute;
import silog;
import tora;

extern "C" char * getenv(const char *name);

static void init(tora::db & db) {
  db.exec(R"(
    CREATE TABLE size (
      id           CHAR(2) NOT NULL PRIMARY KEY
    );
    INSERT INTO size VALUES ('XS');
    INSERT INTO size VALUES ('S');
    INSERT INTO size VALUES ('M');
    INSERT INTO size VALUES ('L');
    INSERT INTO size VALUES ('XL');

    CREATE TABLE link_type (
      id           CHAR(4) NOT NULL PRIMARY KEY
    );
    INSERT INTO link_type VALUES ('DEMO');
    INSERT INTO link_type VALUES ('CODE');

    CREATE TABLE brag (
      id           INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
      name         TEXT NOT NULL,
      created_at   DATETIME NOT NULL DEFAULT CURRENT_TIMESTAMP,
      demoable     BOOLEAN NOT NULL DEFAULT FALSE,
      code         BOOLEAN NOT NULL DEFAULT FALSE,
      size         CHAR(2) REFERENCES size(id)
    );

    CREATE TABLE link (
      id           INTEGER NOT NULL PRIMARY KEY AUTOINCREMENT,
      brag         INTEGER NOT NULL REFERENCES brag(id),
      type         CHAR(4) REFERENCES link_type(id),
      href         TEXT NOT NULL,
      notes        TEXT NULL
    );
    CREATE TABLE sprint (
      name         TEXT NOT NULL PRIMARY KEY
    );
    CREATE TABLE sprint_brag (
      brag         INTEGER NOT NULL REFERENCES brag(id),
      sprint       INTEGER NOT NULL REFERENCES sprint(name)
    );
  )");
}

static void brag(tora::db & db, args & args) {
  auto cmd = args.take();
  if (cmd == "") cmd = "list";
}

int main(int argc, char ** argv) try {
  args args { argc, argv };

  auto file = (jute::view::unsafe(getenv("HOME")) + "/.brag").cstr();
  tora::db db { file.begin() };

  auto cmd = args.take();
  if (cmd == "") cmd = "brag";

  if (cmd == "init") init(db);
  else if (cmd == "brag") brag(db, args);
  else silog::die("unknown command %s", cmd.begin());
} catch (...) {
  return 1;
}

