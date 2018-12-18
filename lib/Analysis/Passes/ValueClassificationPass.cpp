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

#define DEBUG_TYPE "iterator-recognition-classify"

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

  LLVM_DEBUG({
    llvm::dbgs() << "iterator var classification for function: "
                 << CurFunc.getName() << "\n";
  });

  llvm::SmallPtrSet<llvm::Instruction *, 8> itVars, pdVars;

  for (auto &e : info.getIteratorsInfo()) {
    LLVM_DEBUG(llvm::dbgs() << "loop: " << *e.getLoop()->getHeader() << "\n";);

    FindIteratorVars(e, itVars);
    FindPayloadVars(e, pdVars);

    LLVM_DEBUG({
      llvm::dbgs() << "iterator vars: \n";
      for (const auto &e : itVars) {
        llvm::dbgs() << *e << '\n';
      }
    });

    LLVM_DEBUG({
      llvm::dbgs() << "paylod vars: \n";
      for (const auto &e : pdVars) {
        llvm::dbgs() << *e << '\n';
      }
    });
  }

  return false;
}

} // namespace iteratorrecognition
