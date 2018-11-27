//
//
//

#include "IteratorRecognition/Config.hpp"

#include <algorithm>
// using std::sort
// using std::unique

#ifndef ITR_EXTRAS_HPP
#define ITR_EXTRAS_HPP

namespace iteratorrecognition {

// TODO do we need to restrict the arg type here?
auto is_not_null_unit = [](const auto &e) { return e->unit() != nullptr; };

// TODO do we need to restrict the arg type here?
auto unique_inplace = [](auto &Vec) {
  std::sort(Vec.begin(), Vec.end());
  Vec.erase(std::unique(Vec.begin(), Vec.end()), Vec.end());
};

} // namespace iteratorrecognition

#endif // header
