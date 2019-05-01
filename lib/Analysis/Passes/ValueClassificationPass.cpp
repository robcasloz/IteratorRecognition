//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Analysis/Passes/ValueClassificationPass.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionPass.hpp"

#include "IteratorRecognition/Analysis/IteratorValueTracking.hpp"

#include "IteratorRecognition/Analysis/ValueClassification.hpp"

#include "private/PassCommandLineOptions.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo
// using llvm::LoopInfoWrapperPass

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/IR/Dominators.h"
// using llvm::DominatorTreeWrapperPass
// using llvm::DominatorTree

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::location
// using llvm::cl::cat
// using llvm::cl::OptionCategory

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

#define DEBUG_TYPE "itrclassify"

// namespace aliases

namespace itr = iteratorrecognition;

// plugin registration for opt

char itr::ValueClassificationPass::ID = 0;
static llvm::RegisterPass<itr::ValueClassificationPass> X(
    "itr-classify",
    PRJ_CMDLINE_DESC("classify loop vars (based on iterator recognition) pass"),
    false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void
registerValueClassificationPass(const llvm::PassManagerBuilder &Builder,
                                llvm::legacy::PassManagerBase &PM) {
  PM.add(new itr::ValueClassificationPass());

  return;
}

static llvm::RegisterStandardPasses RegisterValueClassificationPass(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    registerValueClassificationPass);

//

enum class ViewSelection {
  Basic,
  Standard,
  More,
  All,
};

static llvm::cl::bits<ViewSelection> ViewSelectionOption(
    "itr-view", llvm::cl::desc(""),
    llvm::cl::values(clEnumValN(ViewSelection::Basic, "basic", "basic"),
                     clEnumValN(ViewSelection::Standard, "standard",
                                "standard"),
                     clEnumValN(ViewSelection::More, "more", "more"),
                     clEnumValN(ViewSelection::All, "all", "all")),
    llvm::cl::CommaSeparated, llvm::cl::cat(IteratorRecognitionCLCategory));

static void checkAndSetCmdLineOptions() {
  if (!ViewSelectionOption.getBits()) {
    ViewSelectionOption.addValue(ViewSelection::Basic);
  }

  if (ViewSelectionOption.isSet(ViewSelection::Standard)) {
    ViewSelectionOption.addValue(ViewSelection::Basic);
  } else if (ViewSelectionOption.isSet(ViewSelection::More)) {
    ViewSelectionOption.addValue(ViewSelection::Basic);
    ViewSelectionOption.addValue(ViewSelection::Standard);
  } else {
    ViewSelectionOption.addValue(ViewSelection::Basic);
    ViewSelectionOption.addValue(ViewSelection::Standard);
    ViewSelectionOption.addValue(ViewSelection::More);
  }
}

//

namespace iteratorrecognition {

ValueClassificationPass::ValueClassificationPass() : llvm::FunctionPass(ID) {
  checkAndSetCmdLineOptions();
}

void ValueClassificationPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<llvm::DominatorTreeWrapperPass>();
  AU.addRequiredTransitive<IteratorRecognitionWrapperPass>();

  AU.setPreservesAll();
}

bool ValueClassificationPass::runOnFunction(llvm::Function &CurFunc) {
  if (FunctionWhiteList.size()) {
    auto found = std::find(FunctionWhiteList.begin(), FunctionWhiteList.end(),
                           std::string{CurFunc.getName()});
    if (found == FunctionWhiteList.end()) {
      return false;
    }
  }

  LLVM_DEBUG({
    llvm::dbgs() << "iterator var classification for function: "
                 << CurFunc.getName() << '\n';
  });

  auto *DT = &getAnalysis<llvm::DominatorTreeWrapperPass>().getDomTree();
  auto &info = getAnalysis<IteratorRecognitionWrapperPass>()
                   .getIteratorRecognitionInfo();

  for (auto &e : info.getIteratorsInfo()) {
    auto *curLoop = e.getLoop();

    if (curLoop->getLoopDepth() > LoopDepthMax ||
        curLoop->getLoopDepth() < LoopDepthMin) {
      continue;
    }

    using SetTy = llvm::SmallPtrSet<llvm::Instruction *, 8>;
    SetTy itVals, pdVals;
    SetTy directItUsesInPayloadVals, pdMemLiveInThruVals, pdMemLiveOutVals;
    SetTy pdVirtRegLiveVals, pdVirtRegLiveInVals, pdVirtRegLiveThruVals,
        pdVirtRegLiveOutVals;

    LLVM_DEBUG(llvm::dbgs()
                   << "loop: " << curLoop->getHeader()->getName() << '\n';);

    if (ViewSelectionOption.isSet(ViewSelection::Basic)) {
      FindIteratorValues(e, itVals);
      FindPayloadValues(e, pdVals);

      LLVM_DEBUG({
        llvm::dbgs() << "iterator: \n";
        for (const auto &e : itVals) {
          llvm::dbgs() << *e << '\n';
        }

        llvm::dbgs() << "payload: \n";
        for (const auto &e : pdVals) {
          llvm::dbgs() << *e << '\n';
        }
      });
    }

    if (ViewSelectionOption.isSet(ViewSelection::Standard)) {
      LLVM_DEBUG(llvm::dbgs() << "iterator disposition:\n";);

      IteratorDispositionAnalyzer ida{e};

      LLVM_DEBUG({
        for (const auto *p : pdVals) {
          llvm::dbgs() << *p << " iterator varies as: "
                       << static_cast<int>(ida.getDisposition(p)) << '\n';
        }
      });
    }

    if (ViewSelectionOption.isSet(ViewSelection::More)) {
      FindVirtRegPayloadLiveValues(e, pdVals, pdVirtRegLiveVals);
      SplitVirtRegPayloadLiveValues(e, pdVals, pdVirtRegLiveVals, *DT,
                                    pdVirtRegLiveInVals, pdVirtRegLiveThruVals,
                                    pdVirtRegLiveOutVals);
      FindDirectUsesOfIn(itVals, pdVals, directItUsesInPayloadVals);
      FindMemPayloadLiveValues(pdVals, pdMemLiveInThruVals, pdMemLiveOutVals);

      LLVM_DEBUG({
        llvm::dbgs() << "payload mem live in or thru: \n";
        for (auto *e : pdMemLiveInThruVals) {
          llvm::dbgs() << *e << '\n';
        }

        llvm::dbgs() << "payload mem live out: \n";
        for (auto *e : pdMemLiveOutVals) {
          llvm::dbgs() << *e << '\n';
        }

        llvm::dbgs() << "payload virt reg live in: \n";
        for (const auto &e : pdVirtRegLiveInVals) {
          llvm::dbgs() << *e << '\n';
        }

        llvm::dbgs() << "payload virt reg live thru: \n";
        for (const auto &e : pdVirtRegLiveThruVals) {
          llvm::dbgs() << *e << '\n';
        }

        llvm::dbgs() << "payload virt reg live out: \n";
        for (const auto &e : pdVirtRegLiveOutVals) {
          llvm::dbgs() << *e << '\n';
        }

        llvm::dbgs() << "direct uses of iterator in payload: \n";
        for (const auto &e : directItUsesInPayloadVals) {
          llvm::dbgs() << *e << '\n';
        }
      });
    }
  }

  return false;
}

} // namespace iteratorrecognition

