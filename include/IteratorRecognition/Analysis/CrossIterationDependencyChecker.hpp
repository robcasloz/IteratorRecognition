//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/Analysis/AliasAnalysis.h"
// using llvm::AAResults

#include "llvm/Analysis/MemoryLocation.h"
// using llvm::MemoryLocation

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include <utility>
// using std::pair

namespace iteratorrecognition {

class CrossIterationDependencyChecker {
  llvm::SmallVector<std::pair<llvm::Instruction *, llvm::Instruction *>, 16>
      Dependences;

public:
  template <typename IteratorT>
  CrossIterationDependencyChecker(IteratorT Begin, IteratorT End,
                                  llvm::AAResults &AA) {
    for (auto it1 = Begin, ie1 = End; it1 != ie1; ++it1) {
      auto &i1 = *it1;
      for (auto it2 = std::next(it1), ie2 = ie1; it2 != ie2; ++it2) {
        auto &i2 = *it2;

        auto mri = AA.getModRefInfo(i2, llvm::MemoryLocation::getOrNone(i1));

        if (llvm::isModSet(mri)) {
          Dependences.push_back({i1, i2});
        }
      }
    }
  }

  using iterator = decltype(Dependences)::iterator;
  using const_iterator = decltype(Dependences)::const_iterator;

  decltype(auto) begin() { return Dependences.begin(); }
  decltype(auto) end() { return Dependences.end(); }

  decltype(auto) begin() const { return Dependences.begin(); }
  decltype(auto) end() const { return Dependences.end(); }
};

} // namespace iteratorrecognition
