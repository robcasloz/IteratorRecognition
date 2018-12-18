//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/ADT/DenseSet.h"
// using llvm::DenseSet

#include "boost/type_traits/is_detected.hpp"
// using boost::is_detected

#include <algorithm>
// using std::sort
// using std::unique
// using std::find

#include <iterator>
// using std::begin
// using std::end

#include <utility>
// std::declval

#include <type_traits>
// using std::remove_pointer_t
// using std::integral_constant

namespace iteratorrecognition {

// TODO do we need to restrict the arg types here?

//

auto is_null_unit = [](const auto &e) { return e->unit() == nullptr; };

auto is_not_null_unit = [](const auto &e) { return !is_null_unit(e); };

template <typename T> using unit_t = decltype(std::declval<T &>().unit());

template <typename T>
using has_unit_t = std::integral_constant<
    bool, boost::is_detected_v<unit_t, std::remove_pointer_t<T>>>;

//

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

template <typename ValueT, typename ValueInfoT>
bool operator==(const llvm::DenseSet<ValueT, ValueInfoT> &LHS,
                const llvm::DenseSet<ValueT, ValueInfoT> &RHS) {
  if (LHS.size() != RHS.size())
    return false;

  for (auto &E : LHS)
    if (!RHS.count(E))
      return false;

  return true;
}

} // namespace iteratorrecognition

