//
//
//

#include "IteratorRecognition/Config.hpp"

#include <algorithm>
// using std::sort
// using std::unique
// using std::find

#ifndef ITR_EXTRAS_HPP
#define ITR_EXTRAS_HPP

namespace iteratorrecognition {

// TODO do we need to restrict the arg types here?

auto is_null_unit = [](const auto &e) { return e->unit() == nullptr; };

auto is_not_null_unit = [](const auto &e) { return !is_null_unit(e); };

auto unique_inplace = [](auto &Vec) {
  std::sort(Vec.begin(), Vec.end());
  Vec.erase(std::unique(Vec.begin(), Vec.end()), Vec.end());
};

auto is_subset_of = [](const auto &Set1, const auto &Set2) {
  for (const auto &e1 : Set1) {
    auto found = std::find(Set2.begin(), Set2.end(), e1);
    if (found == Set2.end()) {
      return false;
    }

    return true;
  }
};

} // namespace iteratorrecognition

#endif // header
