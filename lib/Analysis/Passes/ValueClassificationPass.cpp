//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Analysis/Passes/ValueClassificationPass.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionWrapperPass.hpp"

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

namespace iteratorrecognition {

void ValueClassificationPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequiredTransitive<IteratorRecognitionWrapperPass>();

  AU.setPreservesAll();
}

bool ValueClassificationPass::runOnFunction(llvm::Function &CurFunc) {
  auto &info = getAnalysis<IteratorRecognitionWrapperPass>()
                   .getIteratorRecognitionInfo();

  if (FunctionWhitelist.size()) {
    auto found = std::find(FunctionWhitelist.begin(), FunctionWhitelist.end(),
                           std::string{CurFunc.getName()});
    if (found == FunctionWhitelist.end()) {
      return false;
    }
  }

  LLVM_DEBUG({
    llvm::dbgs() << "iterator var classification for function: "
                 << CurFunc.getName() << "\n";
  });

  for (auto &e : info.getIteratorsInfo()) {
    llvm::SmallPtrSet<llvm::Instruction *, 8> itVars, pdVars, pdTempVars,
        pdLiveVars, directItUsesInPayload, pdMemVars;

    LLVM_DEBUG(llvm::dbgs()
                   << "loop: " << e.getLoop()->getHeader()->getName() << "\n";);

    FindIteratorVars(e, itVars);
    FindPayloadVars(e, pdVars);
    FindPayloadTempAndLiveVars(e, pdVars, pdTempVars, pdLiveVars);
    FindDirectUsesOfIn(itVars, pdVars, directItUsesInPayload);
    FindMemPayloadVars(pdVars, pdMemVars);

    LLVM_DEBUG({
      llvm::dbgs() << "iterator: \n";
      for (const auto &e : itVars) {
        llvm::dbgs() << *e << '\n';
      }

      llvm::dbgs() << "payload: \n";
      for (const auto &e : pdVars) {
        llvm::dbgs() << *e << '\n';
      }

      llvm::dbgs() << "payload mem: \n";
      for (auto *e : pdMemVars) {
        llvm::dbgs() << *e << '\n';
      }

      llvm::dbgs() << "payload temp: \n";
      for (const auto &e : pdTempVars) {
        llvm::dbgs() << *e << '\n';
      }

      llvm::dbgs() << "payload live: \n";
      for (const auto &e : pdLiveVars) {
        llvm::dbgs() << *e << '\n';
      }

      llvm::dbgs() << "direct uses of iterator in payload: \n";
      for (const auto &e : directItUsesInPayload) {
        llvm::dbgs() << *e << '\n';
      }
    });
  }

  return false;
}

} // namespace iteratorrecognition
