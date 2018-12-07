//
//
//

#include "IteratorRecognition/Config.hpp"

#include <algorithm>
// using std::sort
// using std::unique
// using std::find

#include <iterator>
// using std::begin
// using std::end

#ifndef ITR_EXTRAS_HPP
#define ITR_EXTRAS_HPP

namespace iteratorrecognition {

// TODO do we need to restrict the arg types here?

auto is_null_unit = [](const auto &e) { return e->unit() == nullptr; };

auto is_not_null_unit = [](const auto &e) { return !is_null_unit(e); };

auto unique_inplace = [](auto &Vec) {
  using std::begin;
  using std::end;

  std::sort(begin(Vec), end(Vec));
  Vec.erase(std::unique(begin(Vec), end(Vec)), end(Vec));
};

auto is_subset_of = [](const auto &Set1, const auto &Set2) {
  using std::begin;
  using std::end;

  for (const auto &e1 : Set1) {
    auto found = std::find(begin(Set2), end(Set2), e1);
    if (found == end(Set2)) {
      return false;
    }

    return true;
  }
};

} // namespace iteratorrecognition

#endif // header
