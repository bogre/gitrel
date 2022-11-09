#include <chrono>
#include <cstdint>
#include <ctime>
#include <filesystem>
#include <fstream>
#include <iomanip>
#include <iostream>
#include <ratio>
#include <set>
#include <sstream>
#include <string>

#include "ReleaseMatcher.hpp"
constexpr std::string_view vertex = "[dag]";
constexpr std::string_view rel    = "[releases]";
constexpr std::string_view tas    = "[tasks]";
std::vector<ParentChild>   dag;
std::vector<Release>       releases;
using release_commits = std::pair<Release, std::set<std::string>>;
using releases_pair   = std::pair<Release, Release>;
using release_diff    = std::pair<releases_pair, std::set<std::string>>;
std::vector<release_commits> tasks1;
std::vector<release_diff>    tasks2;

auto main(int argc, char** argv) -> int
{
  if (argc != 2 or !(std::string(argv[1]) == "t1" or std::string(argv[1]) == "t2"))
  {
    std::cout << "provide argument like t1 or t2 for respective task\n";
    return 1;
  }

  auto taskno = (std::string(argv[1]) == "t1") ? 1 : 2;
  auto path   = std::string();
  if (taskno == 1)
    path = "./commits_of_tests";
  else
    path = "./diff_tests";
  auto now = []()
  {
    return std::chrono::steady_clock::now();
  };
  auto duration = [](const auto& start, const auto& end)
  {
    return std::chrono::duration<double, std::micro>(end - start).count();
  };
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
  auto add_task1 = [&]()
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
            start = end + 1;
            end   = line.find(",", start);
          }
          belongings.insert(line.substr(start, end - start));
        }
        tasks1.emplace_back(release, belongings);
      }
    }
  };

  auto add_task2 = [&]()
  {
    auto start       = 0U;
    auto end         = line.find("=");
    auto rel_name1   = std::string();
    auto rel_name2   = std::string();
    auto vertex1     = std::string();
    auto vertex2     = std::string();
    auto t1          = std::tm();
    auto t2          = std::tm();
    auto rel1        = Release{};
    auto rel2        = Release{};
    auto parse_error = false;
    auto diff        = std::set<std::string>{};

    if (end != std::string::npos)
    {
      auto release_names = line.substr(start, end - start);
      if (auto comma = release_names.find(","); comma != std::string::npos)
      {
        rel_name1 = release_names.substr(0, comma);
        rel_name2 = release_names.substr(comma + 1);
      }
      else
        parse_error = true;

      if (!parse_error)
      {
        start = end + 1;
        end   = line.find(",", start);

        auto vertex1 = line.substr(start, end - start);
        start        = end + 1;
        end          = line.find(",", start);

        std::istringstream timestampstr{line.substr(start, end - start)};
        timestampstr >> std::get_time(&t1, "%Y-%m-%dT%H:%M:%S");
        if (timestampstr.fail())
        {
          std::cout << "Date Parse failed!!!!!!\n";
          parse_error = true;
        }
        else
          rel1 = Release{rel_name1, vertex1, std::mktime(&t1)};
      }

      if (!parse_error)
      {
        start = end + 1;
        end   = line.find(",", start);

        auto vertex2 = line.substr(start, end - start);
        start        = end + 1;
        end          = line.find(",", start);

        std::istringstream timestampstr{line.substr(start, end - start)};
        timestampstr >> std::get_time(&t1, "%Y-%m-%dT%H:%M:%S");
        if (timestampstr.fail())
        {
          std::cout << "Date Parse failed!!!!!!\n";
          parse_error = true;
        }
        else
          rel2 = Release{rel_name2, vertex2, std::mktime(&t2)};
      }
      if (!parse_error)
      {
       while (end != std::string::npos)
        {
        start = end + 1;
        end   = line.find(",", start);
           diff.insert(line.substr(start, end - start));
        }
      }
      tasks2.emplace_back(std::pair(rel1, rel2), diff);
    }
  };
  auto add_task = [&]()
  {
    if (taskno == 1)
      add_task1();
    else if (taskno == 2)
      add_task2();
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
  std::cout << "Test folder:" << path << '\n';
  for (const auto& entry : std::filesystem::directory_iterator(path))
  {
    if (entry.path().string().ends_with(".ini"))
    {
      std::ifstream test(entry.path());

      if (test)
      {
        auto exec_t1 = [&]()
        {
          for (const auto& task : tasks1)
          {
            auto matcher     = ReleaseMatcher(dag, releases);
            auto start       = now();
            auto res         = matcher.commits_of(task.first);
            auto end         = now();
            bool pass        = task.second == res;
            auto time_report = std::string();
            if (pass)
            {
              time_report = (std::stringstream() << " [" << duration(start, end) << " micros]").str();
            }
            std::cout << "    Test case: release " << std::setw(15) << std::left << task.first.name << ' '
                      << (pass ? std::string("PASS") : std::string("FAILED")) << std::setw(20) << std::right << time_report << '\n';
            if (!pass)
            {
              std::cout << "    owning commits by release: " << task.first.name << "\n    result: (" << res.size() << ")\n";
              for (const auto& commit : res)
                std::cout << commit << ',';
              std::cout << "\n";
              std::cout << "    given: (" << task.second.size() << ")\n";
              for (const auto& commit : task.second)
                std::cout << commit << ',';
              std::cout << "\n";
            }
          }
        };
        auto exec_t2 = [&]()
        {
          for (const auto& task : tasks2)
          {
            auto matcher         = ReleaseMatcher(dag, releases);
            auto start           = now();
            const auto& [t1, t2] = task.first;
            auto res             = matcher.diff(t1, t2);
            auto end             = now();
            bool pass            = task.second == res;
            auto time_report     = std::string();
            if (pass)
            {
              time_report = (std::stringstream() << " [" << duration(start, end) << " micros]").str();
            }
            std::cout << "    Test case: release pair: " << std::setw(20)  << std::left<<std::string().append(1,'[').append(t1.name+","+t2.name).append(1,']')
                      << (pass ? std::string("PASS") : std::string("FAILED")) << std::setw(20) << std::right << time_report << '\n';
            if (!pass)
            {
              std::cout << "\n    result: (" << res.size() << ")\n";
              for (const auto& commit : res)
                std::cout << commit << ',';
              std::cout << "\n";
              std::cout << "    given: (" << task.second.size() << ")\n";
              for (const auto& commit : task.second)
                std::cout << commit << ',';
              std::cout << "\n";
            }
          }
        };
        std::cout << "  Test file:" << entry.path().filename() << '\n';
        read_test(test);
        if (taskno == 1)
          exec_t1();
        else if (taskno == 2)
          exec_t2();
      }
      else
        std::cout << "!!!failed reading test " << entry.path() << "\n\n";
      dag.clear();
      releases.clear();
      tasks1.clear();
      tasks2.clear();
    }
  }
  return 0;
}
