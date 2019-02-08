//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

#include "llvm/Analysis/AliasAnalysis.h"
// using llvm::AAResults
// using llvm::ModRefInfo

#include "llvm/Analysis/MemoryLocation.h"
// using llvm::MemoryLocation

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/ADT/MapVector.h"
// using llvm::MapVector

#include "llvm/ADT/Optional.h"
// using llvm::Optional
// using llvm::None

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream

#include <utility>
// using std::make_pair

#include <type_traits>
// using std::underlying_type_t

namespace iteratorrecognition {

class DependenceCache {
public:
  using DataT = llvm::ModRefInfo;
  using ContainerT =
      llvm::MapVector<std::pair<llvm::Instruction *, llvm::Instruction *>,
                      DataT>;

private:
  ContainerT Dependences;

public:
  // TODO allow to use another object of same type to populate the cache
  template <typename IteratorT>
  DependenceCache(IteratorT Begin, IteratorT End, llvm::AAResults &AA) {
    calculate(Begin, End, AA);
  }

  template <typename IteratorT>
  void calculate(IteratorT Begin, IteratorT End, llvm::AAResults &AA) {
    Dependences.clear();

    for (auto it1 = Begin, ie1 = End; it1 != ie1; ++it1) {
      auto &i1 = *it1;
      for (auto it2 = std::next(it1), ie2 = ie1; it2 != ie2; ++it2) {
        auto &i2 = *it2;

        auto mri = AA.getModRefInfo(i2, llvm::MemoryLocation::getOrNone(i1));

        if (llvm::isModSet(mri)) {
          Dependences.insert({{i1, i2}, mri});
        }
      }
    }
  }

  template <typename KeyT>
  llvm::Optional<ContainerT::value_type> getData(const KeyT &K) const {
    auto found = Dependences.find(K);

    if (found != Dependences.end()) {
      return *found;
    }

    return llvm::None;
  }

  llvm::Optional<ContainerT::value_type>
  getData(const llvm::Instruction *I1, const llvm::Instruction *I2) const {
    return getData(std::make_pair(const_cast<llvm::Instruction *>(I1),
                                  const_cast<llvm::Instruction *>(I2)));
  }

  using iterator = ContainerT::iterator;
  using const_iterator = ContainerT::const_iterator;

  decltype(auto) begin() { return Dependences.begin(); }
  decltype(auto) end() { return Dependences.end(); }

  decltype(auto) begin() const { return Dependences.begin(); }
  decltype(auto) end() const { return Dependences.end(); }

  void print(llvm::raw_ostream &OS) const {
    for (auto &e : Dependences) {
      auto &dep = e.first;
      auto &data = e.second;

      OS << "I1: " << strconv::to_string(*dep.first)
         << "\nI2: " << strconv::to_string(*dep.second)
         << "\ndata: " << static_cast<unsigned>(data) << '\n';
    }
  }
};

} // namespace iteratorrecognition
