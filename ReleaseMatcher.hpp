#pragma once
#include <algorithm>
#include <ctime>
#include <iostream>
#include <iterator>
#include <map>
#include <memory_resource>
#include <queue>
#include <ranges>
#include <set>
#include <string>
#include <string_view>
#include <vector>

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
      [this](const auto& rel)
      {
        // for(auto& el: releases_)
        return std::pair<Release, std::set<std::string>>{rel, {}};
      });
  }

  std::set<std::string> commits_of(const Release& release) const
  {
    if (!prepare_release(release))
      return {};
    auto r_it             = std::find_if(releases_.begin(), releases_.end(), [&release](const auto& R) { return R.first.commit == release.commit; });
    auto ownings = std::set<std::string>{};
    auto current = std::string();
    auto holder  = std::queue<std::string>{{release.commit}};
    while (!holder.empty())
    {
      current = holder.front();
      holder.pop();
      if (!ownings.insert(current).second)
        continue;
      for (const auto& commit : dag_[current])
      {
        if (ownings.contains(commit))
          continue;
        holder.push(commit);
      }
    }

    std::set<std::string> tmp;
    for (const auto& commit : ownings)
      for (const auto& older_release : std::ranges::subrange(releases_.begin(), r_it))
      {
        if (older_release.second.contains(commit))
        {
          tmp.insert(commit);
          break;
        }
      }
    for (const auto& commit : tmp)
    {
      ownings.erase(commit);
    }
    return ownings;
  }
  std::vector<std::string> diff(const Release& r1, const Release& r2)
  {
    Release r;
    if (r1 < r2)
      r = r2;
    else
      r = r1;

    return {};
  }

private:
  auto prepare_release(const Release& release) const -> bool
  {
    // handle unknown commit
    if (false == dag_.contains(release.commit))
      return false;

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
    // handle case when the new release tag has been added through the request
    if (add_release)
    {
      r_it = releases_.insert(r_it, {release, {}});
    }
    for (auto& [rel, ancestors] : releases_)
      ancestors = get_release_ancestors(rel.commit);
    // handle case when release commit is also tagged with older release
    for (auto rel_it = releases_.begin(); rel_it != r_it; ++rel_it)
      if (rel_it->first.commit == release.commit)
        return false;

    // handle case when release commit is DAG root with non-existing younger non-root
    // release
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
        return false;
    }
    return true;
  }
  auto get_release_ancestors(const std::string& commit) const -> std::set<std::string>
  {
    auto r_it             = std::find_if(releases_.begin(), releases_.end(), [&commit](const auto& R) { return R.first.commit == commit; });
    auto procceed         = r_it != releases_.end();
    auto already_released = [&, this](const auto& ct)
    {
      if (procceed)
        for (const auto& older_release_it : std::ranges::subrange(releases_.begin(), r_it) | std::views::reverse)
        {
          if (older_release_it.second.contains(ct))
            return true;
        }
      return false;
    };
    auto que = std::queue<std::string>{};
    que.push(commit);
    auto current = std::string();
    auto visited = std::set<std::string>{};
    while (!que.empty())
    {
      current = que.front();
      que.pop();
      auto it = visited.insert(current);
      if (!it.second)
        continue;
      for (const auto& ct : dag_[current])
      {
        if (!(visited.contains(ct) or already_released(ct)))
          que.push(ct);
      }
    }
    return visited;
  }

  mutable std::map<std::string, std::vector<std::string>> dag_;
  mutable std::map<Release, std::set<std::string>>        releases_;
};
