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
    // handle unknown commit
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
    // handle case when the new release tag has been added through the request
    if (add_release)
    {
      r_it = releases_.insert(r_it, {release, {}});
    }

    /// for (auto& [rel, ancestors] : releases_)
    //  ancestors.clear();
//      std::cout <<"start: "<< r_it->first.name << ": \n";
    for (auto& [rel, ancestors] : releases_)
    {
 //     std::cout <<"prepare: "<< rel.name << ": \n";
      ancestors = get_release_ancestors(rel.commit);
      for (auto& a : ancestors)
      {
  //      std::cout << a << " ";
      }
   //   std::cout << "\n ";
    }

    // handle case when release commit is also tagged with older release
    for (auto rel_it = releases_.begin(); rel_it != r_it; ++rel_it)
      if (rel_it->first.commit == release.commit)
      {
        return {};
      }
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
        return {};
    }
    /*auto new_commmit = std::string();
     for (const auto& older_release : std::ranges::subrange(releases_.begin(), r_it) | std::views::reverse)
     {//std::cout<<"iinnnnn   "<<older_release.first.commit<<"\n";
       if (path_exists2(older_release.first.commit, r_it->first.commit))
       {
         new_commmit = older_release.first.commit;
       }//std::cout<<"ooouuuuuttt"<<older_release.first.commit<<"\n";
     }
     if (new_commmit.size())
       for (r_it = releases_.begin(); r_it->first.commit != new_commmit; ++r_it)
       {
       }*/
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
        // auto add_to_holder = true;
        //  auto older_release_it_reverse = releases_.rbegin();
        // auto memoi = std::set<std::string>();
        //  for (const auto& older_release_it : std::ranges::subrange(releases_.begin(), r_it) | std::views::reverse)

        /*     for (auto older_release_it = releases_.begin(); older_release_it != r_it; ++older_release_it)
              {
                if (/*(rel_it->first.commit == commit) ||  path_exists1(older_release_it->first.commit, commit))
                {
                  add_to_holder = false;
                  break;
                }
              };*/
        // if (add_to_holder)
        holder.push(commit);
      }
    }
    /*    if (ownings.size() == 1 && *ownings.begin() == release.commit && releases_.size()>1 && (--releases_.end())->first.name == release.name)
        {
          ownings.clear();
        }*/
    std::set<std::string> tmp;
    for (const auto& commit : ownings)
      for (const auto& older_release : std::ranges::subrange(releases_.begin(), r_it))
      {
        // std::cout << older_release_it.first.name << '\n';
        if (older_release.second.contains(commit))
        {
          tmp.insert(commit);
          break;
        }
      }
 //   std::cout << release.name << ", for erasing:\n";
    for (const auto& commit : tmp)
    {
  //    std::cout << commit << " ";
      ownings.erase(commit);
    }
//    std::cout << "\n ";
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
  auto get_release_ancestors(const std::string& commit) const -> std::set<std::string>
  {
    auto r_it             = std::find_if(releases_.begin(), releases_.end(), [&commit](const auto& R) { return R.first.commit == commit; });
    auto procceed         = r_it != releases_.end();
    auto already_released = [&, this](const auto& ct)
    {
      // std::ranges::find(releases_, commit, &releases_::value_type::first::commit);
      //       auto r_it = std::ranges::find_if(releases_, [&commit](const auto& c){return c==commit;}, &Release::commit);
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
//      std::cout << "current " << current << "\n ";
      if (!it.second)
        continue;
      for (const auto& ct : dag_[current])
      {
 //       std::cout << "ct: " << ct << ", ";
        if (!(visited.contains(ct) or already_released(ct)))
        {
  //        std::cout << "pushed "
                 //   << "\n ";
          que.push(ct);
        }
      }
    }
    return visited;
  }
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
    auto                                current = std::pmr::string();
    std::byte                           buffer[40000];
    std::pmr::monotonic_buffer_resource rsrc(buffer, sizeof buffer);
    auto                                visited = std::pmr::set<std::pmr::string>{&rsrc};
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
      if (!it.second && current != c1)
        continue;
      // return false;
      if (current == c2)
        return true;

      // std::cout << current << "_";
      for (const auto& commit : dag_[current])
        if (!visited.contains(commit))
          que.push(commit);
    }
    return false;
  }
  //  std::vector<std::string>  get_ownings()
  mutable std::map<std::string, std::vector<std::string>> dag_;
  mutable std::map<Release, std::set<std::string>>        releases_;
};
