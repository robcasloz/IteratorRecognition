//
//
//

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Support/Utils/InstTraversal.hpp"

#include "llvm/Config/llvm-config.h"
// using LLVM_ENABLE_DUMP

#include "llvm/IR/BasicBlock.h"
// using llvm::BasicBlock

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSet

#include "llvm/Support/Compiler.h"
// using LLVM_DUMP_METHOD

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include "boost/range/adaptors.hpp"
// using boost::adaptors::filtered

#include "boost/range/algorithm.hpp"
// using boost::range::transform

#include <cassert>
// using assert

#define DEBUG_TYPE "itr"

STATISTIC(NumTopLevelProcessed, "Number of top-level loops processed");
STATISTIC(NumProcessed, "Number of loops processed");
STATISTIC(NumInseparable, "Number of inseparable top-level loops found");

// namespace aliases

namespace ba = boost::adaptors;
namespace br = boost::range;

//

namespace iteratorrecognition {

// TODO remove code duplication in these functions

bool HasPayloadOnlyBlocks(const IteratorInfo &Info, const llvm::Loop &L) {
  assert((Info.getLoop() == &L || Info.getLoop()->contains(&L)) &&
         "Queried loop is not a subloop of this iterator!");

  llvm::SmallPtrSet<const llvm::BasicBlock *, 16> itBlocks;

  for (auto *e : Info) {
    itBlocks.insert(e->getParent());
  }

  for (auto *e : L.getBlocks()) {
    if ((e == L.getHeader()) || L.isLoopLatch(e)) {
      continue;
    }

    if (!itBlocks.count(e)) {
      return true;
    }
  }

  return false;
}

void GetPayloadOnlyBlocks(const IteratorInfo &Info, const llvm::Loop &L,
                          llvm::SmallVectorImpl<llvm::BasicBlock *> &Blocks) {
  assert((Info.getLoop() == &L || Info.getLoop()->contains(&L)) &&
         "Queried loop is not a subloop of this iterator!");

  llvm::SmallPtrSet<const llvm::BasicBlock *, 16> itBlocks;

  for (auto *e : Info) {
    itBlocks.insert(e->getParent());
  }

  for (auto *e : L.getBlocks()) {
    if ((e == L.getHeader()) || L.isLoopLatch(e)) {
      continue;
    }

    if (!itBlocks.count(e)) {
      Blocks.push_back(e);
    }
  }
}

bool HasMixedBlocks(const IteratorInfo &Info, const llvm::Loop &L) {
  assert((Info.getLoop() == &L || Info.getLoop()->contains(&L)) &&
         "Queried loop is not a subloop of this iterator!");

  llvm::SmallVector<llvm::BasicBlock *, 32> blocks;

  GetPayloadOnlyBlocks(Info, L, blocks);

  for (auto *e : L.getBlocks()) {
    auto found = std::find(blocks.begin(), blocks.end(), e);
    if (found == blocks.end()) {
      continue;
    }

    for (auto &i : *e) {
      if (!Info.isIterator(&i)) {
        return true;
      }
    }
  }

  return false;
}

void GetMixedBlocks(const IteratorInfo &Info, const llvm::Loop &L,
                    llvm::SmallVectorImpl<llvm::BasicBlock *> &Mixed) {
  assert((Info.getLoop() == &L || Info.getLoop()->contains(&L)) &&
         "Queried loop is not a subloop of this iterator!");

  llvm::SmallVector<llvm::BasicBlock *, 32> blocks;

  GetPayloadOnlyBlocks(Info, L, blocks);

  for (auto *e : L.getBlocks()) {
    auto found = std::find(blocks.begin(), blocks.end(), e);
    if (found == blocks.end()) {
      continue;
    }

    for (auto &i : *e) {
      if (!Info.isIterator(&i)) {
        Mixed.push_back(i.getParent());
        break;
      }
    }
  }
}

bool HasPayloadOnlySubloops(const IteratorInfo &Info, const llvm::Loop &L) {
  assert((Info.getLoop() == &L || Info.getLoop()->contains(&L)) &&
         "Queried loop is not a subloop of this iterator!");

  llvm::SmallVector<llvm::BasicBlock *, 32> blocks;

  GetPayloadOnlyBlocks(Info, L, blocks);

  for (auto *e : Info.getLoop()->getSubLoops()) {
    unsigned blockCount = 0;

    for (auto *b : e->getBlocks()) {
      auto found = std::find(blocks.begin(), blocks.end(), b);
      if (found == blocks.end()) {
        break;
      }

      ++blockCount;
    }

    if (blockCount == e->getNumBlocks()) {
      return true;
    }
  }

  return false;
}

void GetPayloadOnlySubloops(const IteratorInfo &Info, const llvm::Loop &L,
                            llvm::SmallVectorImpl<llvm::Loop *> &SubLoops) {
  assert((Info.getLoop() == &L || Info.getLoop()->contains(&L)) &&
         "Queried loop is not a subloop of this iterator!");

  llvm::SmallVector<llvm::BasicBlock *, 32> blocks;

  GetPayloadOnlyBlocks(Info, L, blocks);

  for (auto *e : Info.getLoop()->getSubLoops()) {
    unsigned blockCount = 0;

    for (auto *b : e->getBlocks()) {
      auto found = std::find(blocks.begin(), blocks.end(), b);
      if (found == blocks.end()) {
        break;
      }

      ++blockCount;
    }

    if (blockCount == e->getNumBlocks()) {
      SubLoops.push_back(e);
    }
  }
}

//

IteratorRecognitionInfo::IteratorRecognitionInfo(const llvm::LoopInfo &CurLI,
                                                 BaseGraphT &CurPDG)
    : LI(CurLI), PDG(CurPDG), CG{llvm::scc_begin(&PDG), llvm::scc_end(&PDG)} {
  MapCondensationToLoops();
  RecognizeIterator();

  (void)NumTopLevelProcessed;
  (void)NumProcessed;
  (void)NumInseparable;

#ifdef LLVM_ENABLE_STATS || !defined(NDEBUG)
  // TODO const_cast is required due to LLVM API inconsistency with constness
  const auto &preorderLoopForest =
      const_cast<llvm::LoopInfo &>(LI).getLoopsInPreorder();
  NumProcessed += preorderLoopForest.size();
  NumTopLevelProcessed += std::distance(LI.begin(), LI.end());

  for (const auto &ii : getIteratorsInfo()) {
    auto numItInst = ii.getNumInstructions();
    auto loopInsts = make_loop_inst_range(ii.getLoop());
    auto numLoopInst = std::distance(loopInsts.begin(), loopInsts.end());

    if (numLoopInst == numItInst) {
      NumInseparable++;
    }
  }
#endif // LLVM_ENABLE_STATS || !defined(NDEBUG)

  LLVM_DEBUG(dump(););
}

void IteratorRecognitionInfo::MapCondensationToLoops() {
  for (const auto &cn : CGT::nodes(CG)) {
    CondensationToLoopsMapT::mapped_type loops;

    LLVM_DEBUG(llvm::dbgs() << "condensation: " << cn
                            << " maps to loops with headers: \n";);

    for (const auto &n : *cn | ba::filtered(is_not_null_unit)) {
      auto *loop = LI.getLoopFor(n->unit()->getParent());

      while (loop) {
        loops.insert(loop);
        loop = loop->getParentLoop();
      }
    }

    loops.erase(nullptr);

    LLVM_DEBUG({
      for (const auto *loop : loops) {
        auto *hdr = loop->getHeader();
        llvm::dbgs() << '\t' << hdr << ' ' << hdr->getName() << '\n';
      }
    });

    Map.try_emplace(cn, loops);
  }
}

void IteratorRecognitionInfo::RecognizeIterator() {
  const auto &loops = const_cast<llvm::LoopInfo &>(LI).getLoopsInPreorder();

  for (const auto *loop : loops) {
    llvm::SmallPtrSet<CGT::NodeRef, 8> loopCondensations;

    for (auto &e : Map) {
      if (e.second.count(loop)) {
        loopCondensations.insert(e.first);
      }
    }

    llvm::SmallVector<llvm::Instruction *, 8> inst;

    for (auto *cn : loopCondensations) {
      bool workFound = false;

      for (auto &ccn : ICGT::children(cn)) {
        if (loopCondensations.count(ccn)) {
          workFound = true;
          break;
        }
      }

      if (!workFound) {
        br::transform(*cn | ba::filtered(is_not_null_unit),
                      std::back_inserter(inst),
                      [&](const auto &e) { return e->unit(); });
      }
    }

    unique_inplace(inst);
    IteratorsInfo.emplace_back(loop, inst.begin(), inst.end());
  }
}

void IteratorRecognitionInfo::print(llvm::raw_ostream &OS) const {
  for (const auto &ii : getIteratorsInfo()) {
    auto *hdr = ii.getLoop()->getHeader();
    OS << "loop with header: " << hdr << ' ' << hdr->getName() << '\n';
    OS << "\titerator instructions:\n";
    for (auto *i : ii) {
      OS << '\t' << *i << '\n';
    }
  }
}

#if !defined(NDEBUG) || defined(LLVM_ENABLE_DUMP)
LLVM_DUMP_METHOD void IteratorRecognitionInfo::dump() const {
  print(llvm::dbgs());
  llvm::dbgs() << '\n';
}
#endif

} // namespace iteratorrecognition

