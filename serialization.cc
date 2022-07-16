#include <stdio.h>

#include <cstdlib>
#include <exception>
#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <string>
#include <vector>

#include "json.hpp"
#include "rapidxml.hpp"
#include "rapidxml_print.hpp"
#include "rapidxml_utils.hpp"

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

const std::vector<Movie> movies{{
                                    11001,
                                    "The Matrix",
                                    1999,
                                    196,
                                    {{"Keanu Reeves", "Neo"},
                                     {"Laurence Fishburne", "Morpheus"},
                                     {"Carrie-Anne Moss", "Trinity"},
                                     {"Hugo Weaving", "Agent Smith"}},
                                    {"Lana Wachowski", "Lilly Wachowski"},
                                    {"Lana Wachowski", "Lilly Wachowski"},
                                },
                                {
                                    9871,
                                    "Forrest Gump",
                                    1994,
                                    202,
                                    {{"Tom Hanks", "Forrest Gump"},
                                     {"Sally Field", "Mrs. Gump"},
                                     {"Robin Wright", "Jenny Curran"},
                                     {"Mykelti Williamson", "Bubba Blue"}},
                                    {"Robert Zemeckis"},
                                    {"Winston Groom", "Eric Roth"},
                                }};

void serialize(const std::vector<Movie>& movies, const std::string& filepath) {
  rapidxml::xml_document<> doc;
  auto decl = doc.allocate_node(rapidxml::node_declaration);
  decl->append_attribute(doc.allocate_attribute("version", "1.0"));
  doc.append_node(decl);

  auto root = doc.allocate_node(rapidxml::node_element, "movies");
  doc.append_node(root);

  for (const auto& m : movies) {
    auto movie_node = doc.allocate_node(rapidxml::node_element, "movie");
    root->append_node(movie_node);

    auto id = doc.allocate_attribute("id", doc.allocate_string(std::to_string(m.id).c_str()));
    auto title = doc.allocate_attribute("title", m.title.c_str());
    auto year = doc.allocate_attribute("year", doc.allocate_string(std::to_string(m.year).c_str()));
    auto length =
        doc.allocate_attribute("length", doc.allocate_string(std::to_string(m.length).c_str()));
    movie_node->append_attribute(id);
    movie_node->append_attribute(title);
    movie_node->append_attribute(year);
    movie_node->append_attribute(length);

    auto cast_node = doc.allocate_node(rapidxml::node_element, "cast");
    movie_node->append_node(cast_node);
    for (const auto& c : m.cast) {
      auto node = doc.allocate_node(rapidxml::node_element, "role");
      cast_node->append_node(node);
      auto star = doc.allocate_attribute("star", c.actor.c_str());
      auto name = doc.allocate_attribute("name", c.role.c_str());
      node->append_attribute(star);
      node->append_attribute(name);
    }

    auto directors_node = doc.allocate_node(rapidxml::node_element, "directors");
    movie_node->append_node(directors_node);
    for (const auto& d : m.directors) {
      auto node = doc.allocate_node(rapidxml::node_element, "director");
      directors_node->append_node(node);
      auto name = doc.allocate_attribute("name", d.c_str());
      node->append_attribute(name);
    }

    auto writers_node = doc.allocate_node(rapidxml::node_element, "writers");
    movie_node->append_node(writers_node);
    for (const auto& w : m.writers) {
      auto node = doc.allocate_node(rapidxml::node_element, "writer");
      writers_node->append_node(node);
      auto name = doc.allocate_attribute("name", w.c_str());
      node->append_attribute(name);
    }
  }

  std::ofstream file(filepath);
  file << doc;
}

std::vector<Movie> deserialize(const std::string& filepath) {
  std::vector<Movie> movies;
  rapidxml::xml_document<> doc;
  rapidxml::file<> file(filepath.c_str());

  try {
    doc.parse<0>(file.data());
  } catch (rapidxml::parse_error& err) {
    std::cerr << err.what() << " " << err.where<char*>() << std::endl;
    std::cout << "HOGE" << std::endl;
    return movies;
  }

  auto root = doc.first_node("movies");
  for (auto movie_node = root->first_node("movie"); movie_node;
       movie_node = movie_node->next_sibling("movie")) {
    Movie m;
    m.id = std::stoi(movie_node->first_attribute("id")->value());
    m.title = movie_node->first_attribute("title")->value();
    m.year = std::stoi(movie_node->first_attribute("year")->value());
    m.length = std::stoi(movie_node->first_attribute("length")->value());

    for (auto role_node = movie_node->first_node("cast")->first_node("role"); role_node;
         role_node = role_node->next_sibling("role")) {
      m.cast.push_back(CastingRole{
          role_node->first_attribute("star")->value(),
          role_node->first_attribute("name")->value(),
      });
    }

    for (auto director_node = movie_node->first_node("directors")->first_node("director");
         director_node; director_node = director_node->next_sibling("director")) {
      m.directors.push_back(director_node->first_attribute("name")->value());
    }

    for (auto writer_node = movie_node->first_node("writers")->first_node("writer"); writer_node;
         writer_node = writer_node->next_sibling("writer")) {
      m.directors.push_back(writer_node->first_attribute("name")->value());
    }
    movies.push_back(m);
  }

  return movies;
}

void test_xml() {
  serialize(movies, "movies.xml");
  auto result = deserialize("movies.xml");
  assert(result.size() == 2);
  assert(result[0].title == "The Matrix");
  assert(result[1].title == "Forrest Gump");
}

using json = nlohmann::json;

void to_json(json& j, const CastingRole& c) { j = json{{"star", c.actor}, {"name", c.role}}; }

void to_json(json& j, const Movie& m) {
  j = json::object({
      {"id", m.id},
      {"title", m.title},
      {"year", m.year},
      {"length", m.length},
      {"cast", m.cast},
      {"directors", m.directors},
      {"writers", m.writers},
  });
}

void json_serialize(const std::vector<Movie>& movies, const std::string& filepath) {
  json data{{"movies", movies}};
  std::ofstream ofile(filepath.c_str());
  if (ofile.is_open()) {
    ofile << std::setw(2) << data << std::endl;
  }
}

std::vector<Movie> json_deserialize(const std::string& filepath) {
  std::vector<Movie> movies;

  std::ifstream ifile(filepath);
  if (!ifile.is_open()) return movies;

  json data;
  try {
    ifile >> data;
    if (data.is_object()) {
      for (const auto& element : data.at("movies")) {
        Movie m;
        m.id = element.at("id").get<int>();
        m.title = element.at("title").get<std::string>();
        m.year = element.at("year").get<int>();
        m.length = element.at("length").get<int>();

        for (const auto& role : element.at("cast")) {
          m.cast.push_back(
              CastingRole{role.at("star").get<std::string>(), role.at("name").get<std::string>()});
        }

        for (const auto& director : element.at("directors")) {
          m.directors.push_back(director);
        }

        for (const auto& writer : element.at("writers")) {
          m.writers.push_back(writer);
        }
        movies.push_back(std::move(m));
      }
    }
  } catch (const std::exception& e) {
    std::cerr << e.what() << std::endl;
  }

  return movies;
}

void test_json() {
  json_serialize(movies, "movies.json");
  auto result = json_deserialize("movies.json");
  assert(result.size() == 2);
  assert(result[0].title == "The Matrix");
  assert(result[1].title == "Forrest Gump");
}
int main() { test_json(); }
