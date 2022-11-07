#pragma once
#include <algorithm>
#include <ctime>
#include <iostream>
#include <iterator>
#include <map>
#include <queue>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <vector>
#include <memory_resource>
#include <bits/ranges_algo.h>
#include <bits/ranges_util.h>
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

  bool operator<(const Release& rs) const
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
    {
      dag_[pc.child].push_back(pc.parent);
      dag_.try_emplace(pc.parent);
    }
    std::ranges::transform(
      releases,
      std::inserter(releases_, releases_.begin()),
      [](const auto& rel)
      {
        auto ancestors = std::set<std::string>{};
        // for(auto& el: releases_)
        return std::pair<Release, std::set<std::string>>{rel, ancestors};
      });
  }

  std::set<std::string> commits_of(const Release& release) const
  {
    if (false == dag_.contains(release.commit))
      return {};

    auto r_it = releases_.begin();
    for (; r_it != releases_.end(); ++r_it)
    {
      if (r_it->first.name == release.name)
        break;
    }
    auto add_release = false;
    if (r_it == releases_.end())
    {
      add_release = true;
    }
    else
    {
      if (r_it->first.timestamp != release.timestamp)
      {
        std::cout << "@@@@@@@@@@@@@@@@@@@@@@@@ update release time stamp\n";
        releases_.erase(r_it);
        add_release = true;
      }
      if (r_it->first.commit != release.commit)
      {
        std::cout << "############# update release commit\n";
        releases_.erase(r_it);
        add_release = true;
      }  // r.timestamp;
    }
    if (add_release)
      r_it = releases_.insert(r_it, {release, {}});

    for (auto rel_it = releases_.begin(); rel_it != r_it; ++rel_it)
      if (rel_it->first.commit == release.commit)
      {
        return {};
      }
    if (dag_.at(release.commit).empty())
    {
      auto depricated_root = releases_.size() > 1;
      auto rel_it          = r_it;
      ++rel_it;
      for (; rel_it != releases_.end(); ++rel_it)
      {
        if (!dag_.at(rel_it->first.commit).empty())
          depricated_root = false;
      }
      if (depricated_root)
        return {};
    }
    auto ownings = std::set<std::string>{};
    auto current = std::string();
    auto holder  = std::vector<std::string>{{release.commit}};
    while (!holder.empty())
    {
      current = holder.back();
      holder.pop_back();
      if(!ownings.insert(current).second)continue;
      for (const auto& commit : dag_[current])
      {
        auto add_to_holder = true;
        // auto older_release_it_reverse = releases_.rbegin();
        for (const auto& older_release_it : std::ranges::subrange(releases_.begin(), r_it) | std::views::reverse)
        {
          //std::cout << older_release_it.first.name << '\n';
          if (/*(rel_it->first.commit == commit) || */ path_exists2(older_release_it.first.commit, commit))
          {
            add_to_holder = false;
            break;
          }
        }

        /*     for (auto older_release_it = releases_.begin(); older_release_it != r_it; ++older_release_it)
              {
                if (/*(rel_it->first.commit == commit) ||  path_exists1(older_release_it->first.commit, commit))
                {
                  add_to_holder = false;
                  break;
                }
              };*/
        if (add_to_holder)
          holder.push_back(commit);
      }
    }
    /*    if (ownings.size() == 1 && *ownings.begin() == release.commit && releases_.size()>1 && (--releases_.end())->first.name == release.name)
        {
          ownings.clear();
        }*/

    return ownings;
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
      [this](const std::string& c) { return releases_.end() != std::ranges::fsecondsecondind(releases_, c, &Release::commit); });
    return {};*/
  }

private:
  auto path_exists(const std::string& c1, const std::string& c2) const -> bool
  {
    static std::set<std::string> visited;
    if (c1 == c2)
      return true;
    auto res = visited.insert(c1);
    if (!res.second)
    {
      visited.clear();
      return false;
    }
    for (const auto& commit : dag_[c1])
      if (path_exists(commit, c2))
        return true;
    return false;
  }
  auto path_exists1(const std::string& c1, const std::string& c2) const -> bool
  {
    // std::cout << "path exists(" << c1 << "-" << c2 << ")";
    auto que = std::queue<std::string>{};
    que.push(c1);
    auto current = std::pmr::string();
            std::byte buffer[40000];
            std::pmr::monotonic_buffer_resource rsrc(buffer, sizeof buffer);
    auto visited = std::pmr::set<std::pmr::string>{&rsrc};
    while (!que.empty())
    {
      current = std::pmr::string(que.front());
      que.pop();
      auto it = visited.insert(current);
      if (!it.second)
        return false;
      if (current == std::pmr::string(c2))
        return true;

      // std::cout << current << "_";
      for (const auto& commit : dag_[std::string(current.c_str())])
        que.push(commit);
    }
    return false;
  }

  auto path_exists2(const std::string& c1, const std::string& c2) const -> bool
  {
    // std::cout << "path exists(" << c1 << "-" << c2 << ")";
    auto que = std::queue<std::string>{};
    que.push(c1);
    auto current = std::string();
    auto visited = std::set<std::string>{};
    while (!que.empty())
    {
      current = que.front();
      que.pop();
      auto it = visited.insert(current);
      if (!it.second)
        return false;
      if (current == c2)
        return true;

      // std::cout << current << "_";
      for (const auto& commit : dag_[current])
        que.push(commit);
    }
    return false;
  }
  //  std::vector<std::string>  get_ownings()
  mutable std::map<std::string, std::vector<std::string>> dag_;
  mutable std::map<Release, std::set<std::string>>        releases_;
};
