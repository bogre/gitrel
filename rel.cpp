#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <set>
#include <sstream>
#include <string>

#include "ReleaseMatcher.hpp"
constexpr std::string_view                             vertex = "[dag]";
constexpr std::string_view                             rel    = "[releases]";
constexpr std::string_view                             tas    = "[tasks]";
std::vector<ParentChild>                               dag;
std::vector<Release>                                   releases;
std::vector<std::pair<Release, std::set<std::string>>> tasks;

auto main() -> int
{
  auto line = std::string();

  auto add_rel = [&]()
  {
    auto start = 0U;
    auto end   = line.find("=");
    if (end != std::string::npos)
    {
      auto rel = line.substr(start, end - start);
      start    = end + 1;
      end      = line.find(",");
      if (end != std::string::npos)
      {
        auto vertex = line.substr(start, end - start);
        start       = end + 1;
        std::tm            t{};
        std::istringstream timestampstr{line.substr(start)};
        timestampstr >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
        if (timestampstr.fail())
        {
          std::cout << "Date Parse failed\n";
        }
        else
          releases.push_back({rel, vertex, std::mktime(&t)});
      }
    }
  };
  auto add_vertex = [&]()
  {
    auto start = 0U;
    auto end   = line.find("=");
    if (end != std::string::npos)
    {
      auto parent = line.substr(start, end - start);
      start       = end + 1;
      end         = line.find(",");
      while (end != std::string::npos)
      {
        auto child = line.substr(start, end - start);
        dag.push_back({parent, child});
        start = end + 1;
        end   = line.find(",", start);
      }
      dag.push_back({parent, line.substr(start)});
    }
  };
  auto add_task = [&]()
  {
    auto start = 0U;
    auto end   = line.find("=");
    if (end != std::string::npos)
    {
      auto release_name = line.substr(start, end - start);
      start             = end + 1;
      end               = line.find(",", start);

      auto vertex = line.substr(start, end - start);
      start       = end + 1;
      end         = line.find(",", start);
      std::tm            t{};
      std::istringstream timestampstr{line.substr(start, end - start)};
      timestampstr >> std::get_time(&t, "%Y-%m-%dT%H:%M:%S");
      if (timestampstr.fail())
      {
        std::cout << "Date Parse failed!!!!!!\n";
      }
      else
      {
        auto release    = Release{release_name, vertex, std::mktime(&t)};
        auto belongings = std::set<std::string>{};
        if (end != std::string::npos)
        {
          start = end + 1;
          end   = line.find(",", start);
          while (end != std::string::npos)
          {
            belongings.insert(line.substr(start, end - start));
            // tasks[release].push_back(line.substr(start, end - start));
            start = end + 1;
            end   = line.find(",", start);
          }
          belongings.insert(line.substr(start, end - start));
        }
        tasks.emplace_back(release, belongings);
        /*if (std::ranges::find(releases, rel, &Release::name) == releases.end())
                  {  releases.push_back(release); }*/
        // tasks[release].push_back(line.substr(start, end - start));
        /*for (auto el :tasks[release]){
            std::cout<<rel<<".."<<el<<"..\n";
        }
        tasks[release]=belongings;*/
      }
    }
  };
  auto read_test = [&](std::ifstream& input)
  {
    auto oper = op::noop;
    while (std::getline(input, line))
    {
      if (line == rel)
        oper = op::rel;
      else if (line == vertex)
        oper = op::dag;
      else if (line == tas)
        oper = op::task;

      if (oper == op::rel)
        add_rel();
      else if (oper == op::dag)
        add_vertex();
      else if (oper == op::task)
        add_task();
    }
  };
  std::string path = "./commits_of_tests";
  std::cout << "Test folder:" << path << '\n';
  for (const auto& entry : std::filesystem::directory_iterator(path))
  {
    if (entry.path().string().ends_with(".ini"))
    {
      std::ifstream test(entry.path());

      if (test)
      {
        std::cout << "  Test file:" << entry.path().filename() << '\n';
        read_test(test);
        //auto matcher = ReleaseMatcher(dag, releases);
        for (const auto& task : tasks)
        {
          auto matcher = ReleaseMatcher(dag, releases);
          auto res  = matcher.commits_of(task.first);
          bool pass = task.second == res;
          std::cout << "    Test case: release " <<std::setw(15)<<std::left <<task.first.name << ' ' << (pass ? std::string("PASS") : std::string("FAILED")) <<" [100 ms]"<< '\n';
          if (!pass)
          {
            std::cout << "    owning commits by release: " << task.first.name << "\n    result: ";
            for (const auto& commit : res)
              std::cout << commit << ',';
            std::cout << "\n";
            std::cout << "    given: ";
            for (const auto& commit : task.second)
              std::cout << commit << ',';
            std::cout << "\n";
          }
        }
      }
      else
        std::cout << "!!!faled reading test " << entry.path() << "\n\n";
      //      for (auto el : dag)
      //        std::cout << el.parent << " " << el.child << '\n';
      dag.clear();
      //      for (auto el : releases)
      //        std::cout << el.name << " " << el.commit << " " << el.timestamp << '\n';
      releases.clear();
      /*      for (auto el : tasks)
            {
              std::cout << el.first.name << " --->>> ";
              for(auto vel : el.second) std::cout<< " " << vel << '\n';
            }*/
      tasks.clear();
    }
  }
  return 0;
}
