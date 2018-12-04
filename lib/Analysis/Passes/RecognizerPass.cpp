//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/Passes/RecognizerPass.hpp"

#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

#include "Pedigree/Analysis/Passes/PDGraphPass.hpp"

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

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/ADT/DenseSet.h"
// using llvm::DenseSet

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::location
// using llvm::cl::cat
// using llvm::cl::OptionCategory

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include <system_error>
// using std::error_code

#define DEBUG_TYPE "iterator-recognition"

// namespace aliases

namespace itr = iteratorrecognition;

// plugin registration for opt

char itr::RecognizerPass::ID = 0;
static llvm::RegisterPass<itr::RecognizerPass>
    X("itr-recognize", PRJ_CMDLINE_DESC("iterator recognition pass"), false,
      false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void registerRecognizerPass(const llvm::PassManagerBuilder &Builder,
                                   llvm::legacy::PassManagerBase &PM) {
  PM.add(new itr::RecognizerPass());

  return;
}

static llvm::RegisterStandardPasses
    RegisterRecognizerPass(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                           registerRecognizerPass);

//

namespace iteratorrecognition {

void RecognizerPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<llvm::LoopInfoWrapperPass>();
  AU.addRequired<pedigree::PDGraphPass>();

  AU.setPreservesAll();
}

bool RecognizerPass::runOnFunction(llvm::Function &CurFunc) {
  bool hasChanged = false;

  const auto *LI = &getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
  if (LI->empty()) {
    return hasChanged;
  }

  pedigree::PDGraph &Graph{getAnalysis<pedigree::PDGraphPass>().getGraph()};
  Graph.connectRootNode();
  Info = std::make_unique<IteratorRecognitionInfo>(*LI, Graph);

  return hasChanged;
}

} // namespace iteratorrecognition
