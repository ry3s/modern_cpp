#include <exception>
#include <iostream>
#include <vector>

#include "sqlite_modern_cpp.h"
#include "sqlite3.h"

struct CastingRole {
  std::string actor;
  std::string role;
};

struct Movie {
  int id;
  std::string title;
  int year;
  int length;

  std::vector<CastingRole> cast;
  std::vector<std::string> directors;
  std::vector<std::string> writers;
};

void print_movie(const Movie& m) {
  std::cout << "[" << m.id << "]" << m.title << " (" << m.year << ")" << m.length << "min"
            << std::endl;

  std::cout << " directed by: ";
  for (const auto& d : m.directors) std::cout << d << ", ";
  std::cout << std::endl;

  std::cout << " written by: ";
  for (const auto& w : m.writers) std::cout << w << ", ";
  std::cout << std::endl;

  std::cout << " cast: ";
  for (const auto& r : m.cast) std::cout << r.actor << " (" << r.role << "), ";
  std::cout << std::endl << std::endl;
}

std::vector<std::string> get_directors(const sqlite3_int64 movie_id, sqlite::database& db) {
  std::vector<std::string> result;
  db << R"(select p.name from writers as w
        join persons as p on w.personid = p.rowid
        where w.movieid = ?;)"
     << movie_id >>
      [&result](const std::string name) { result.emplace_back(name); };

  return result;
}

std::vector<std::string> get_writers(const sqlite3_int64 movie_id, sqlite::database& db) {
  std::vector<std::string> result;
  db << R"(select p.name from writers as w
        join persons as p on w.personid = p.rowid
        where w.movieid = ?;)"
     << movie_id >>
      [&result](const std::string name) { result.emplace_back(name); };

  return result;
}

std::vector<CastingRole> get_cast(sqlite3_int64 movie_id, sqlite::database& db) {
  std::vector<CastingRole> result;
  db << R"(select p.name, c.role from casting as c
        join persons as p on c.personid = p.rowid
        where c.movieid = ?)"
     << movie_id >>
      [&result](const std::string name, const std::string role) {
        result.emplace_back(CastingRole{name, role});
      };

  return result;
}

std::vector<Movie> get_movies(sqlite::database& db) {
  std::vector<Movie> movies;

  db << R"(select rowid, * from movies;)"
     >> [&movies, &db](const sqlite3_int64 rowid,
                       const std::string& title,
                       const int year, const int length) {
       movies.emplace_back(Movie{
           static_cast<int>(rowid),
           title,
           year,
           length,
           get_cast(rowid, db),
           get_directors(rowid, db),
           get_writers(rowid, db),

         });
     };

  return movies;
}


sqlite_int64 get_person_id(const std::string& name, sqlite::database& db) {
  sqlite_int64 id = 0;
  db << "select rowid from persons where name=?;"
     << name
     >> [&id](const sqlite_int64 rowid) { id = rowid; };

  return id;
}

sqlite_int64 insert_person(const std::string& name, sqlite::database& db) {
  db << "insert into persons vlaues(?);"
     << name;

  return db.last_insert_rowid();
}

void insert_directors(const sqlite_int64 movie_id,
                      const std::vector<std::string>& directors,
                      sqlite::database& db) {
  for (const auto& director : directors) {
    auto id = get_person_id(director, db);

    if (id == 0)
      id = insert_person(director, db);

    db << "insert into directores values(?, ?);"
       << movie_id
       << id;
  }
}

void insert_writers(const sqlite_int64 movie_id,
                    const std::vector<std::string>& writers,
                    sqlite::database& db) {
  for (const auto& writer : writers)  {
    auto id = get_person_id(writer, db);

    if (id == 0)
      id = insert_person(writer, db);

    db << "insert into writers values(?, ?)"
       << movie_id
       << id;
  }
}

void insert_cast(const sqlite_int64 movie_id,
                 const std::vector<CastingRole>& cast,
                 sqlite::database& db) {
  for (const auto& cr : cast) {
    auto id = get_person_id(cr.actor, db);

    if (id == 0)
      insert_person(cr.actor, db);

    db << "inserto into casting values(?,?,?);"
       << movie_id
       << id
       << cr.role;
  }
}

void insert_movie(Movie& m, sqlite::database& db) {
  try {
    db << "begin;";

    db << "insert into movies values(?,?,?);"
       << m.title << m.year << m.length;

    auto movieid = db.last_insert_rowid();

    insert_directors(movieid, m.directors, db);
    insert_writers(movieid, m.writers, db);
    insert_cast(movieid, m.cast, db);

    m.id = static_cast<int>(movieid);

    db << "commit;";
  } catch(const std::exception &) {
    db << "rollback;";
  }
}

int main() {
  try {
    sqlite::database db("cppchallenger85.db");

    const auto movies = get_movies(db);
    for (const auto& m : movies) {
      print_movie(m);
    }
  } catch (const sqlite::sqlite_exception& e) {
    std::cerr << e.get_code() << ": " << e.what() << " during " << e.get_sql() << std::endl;
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }
}
