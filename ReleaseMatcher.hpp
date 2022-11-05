#pragma once
#include <algorithm>
#include <ctime>
#include <iterator>
#include <map>
#include <queue>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <iostream>
#include <bits/ranges_algo.h>
enum class op
{
  noop,
  dag,
  rel,
  task
};
struct ParentChild
{
  std::string parent;
  std::string child;
};
struct Release
{
  std::string name;
  std::string commit;
  std::time_t timestamp{};
  bool        operator<(const Release& rs) const
  {
    return timestamp < rs.timestamp;
  }
};
class ReleaseMatcher
{
public:
  ReleaseMatcher(std::vector<ParentChild> dag, std::vector<Release> releases)
  {
    for (const auto& pc : dag)
      dag_[pc.parent].push_back(pc.child);
    std::ranges::transform(
      releases,
      std::inserter(releases_, releases_.begin()),
      [](const auto& rel) {
        return std::pair<Release, std::vector<std::string>>{rel, {}};
      });
    for (const auto& el : dag_)
    {
      for (auto& rel_el : releases_)
        if (path_exists(el.first, rel_el.first.commit))
        {
          rel_el.second.push_back(el.first);
          break;
        };
    }
  }
  std::vector<std::string> commits_of(Release r)
  {
    return releases_[r];
  }
  std::vector<std::string> diff(Release r)
  {
    auto ownings = std::vector<std::string>{};
    auto current = std::string();
    auto holder  = std::vector<std::string>{{r.commit}};
    while (!holder.empty())
    {
      current = holder.back();
      holder.pop_back();
      ownings.push_back(current);
      for (const auto& commit : dag_[current])
      {
        /*if (auto res = std::ranges::find(releases_, commit, &Release::commit); res != releases_.end())
        {
          if (res->timestamp < r.timestamp)
            break;
        }*/
        holder.push_back(commit);
      }
    }
    return ownings;
    /*auto& ancestors = dag_[r.commit];
    std::ranges::copy_if(
      ancestors,
      std::back_inserter(ownings),
      [this](const std::string& c) { return releases_.end() != std::ranges::find(releases_, c, &Release::commit); });
    return {};*/
  }

private:
  auto path_exists1(const std::string& c1, const std::string& c2) const -> bool
  {
    if (c1 == c2)
      return true;
    for (const auto& commit : dag_[c1])
      if (path_exists1(commit, c2))
        return true;
    return false;
  }
  auto path_exists(const std::string& c1, const std::string& c2) const -> bool
  {
            std::cout<<"path exists("<<c1<<"-"<<c2<<")\n";
    auto que = std::queue<std::string>{};
    que.push(c1);
    auto current = std::string();
    while (!que.empty())
    {
      current = que.front();
      que.pop();
      if (current == c2)
        return true;

      for (const auto& commit : dag_[current])
            que.push(commit);
    }
    return false;
  }
  //  std::vector<std::string>  get_ownings()
  mutable std::map<std::string, std::vector<std::string>> dag_;
  std::map<Release, std::vector<std::string>>             releases_;
};
