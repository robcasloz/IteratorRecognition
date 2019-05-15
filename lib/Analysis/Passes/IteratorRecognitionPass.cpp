//
//
//

#include "llvm/Transforms/Utils.h"

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionPass.hpp"

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
// using llvm::cl::ResetAllOptionOccurrences
// using llvm::cl::ParseEnvironmentOptions

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include <iterator>
// using std::distance

#include <system_error>
// using std::error_code

#define DEBUG_TYPE ITR_RECOGNIZE_PASS_NAME
#define PASS_CMDLINE_OPTIONS_ENVVAR "ITR_RECOGNIZE_ANALYSIS_CMDLINE_OPTIONS"

// namespace aliases

namespace itr = iteratorrecognition;

// plugin registration for opt

char itr::IteratorRecognitionWrapperPass::ID = 0;
char &llvm::IteratorRecognitionID = itr::IteratorRecognitionWrapperPass::ID;

using namespace llvm;
using namespace iteratorrecognition;
using namespace pedigree;
INITIALIZE_PASS_BEGIN(IteratorRecognitionWrapperPass, ITR_RECOGNIZE_PASS_NAME, PRJ_CMDLINE_DESC("iterator recognition pass"), false, true)
INITIALIZE_PASS_DEPENDENCY(LoopInfoWrapperPass)
INITIALIZE_PASS_DEPENDENCY(PDGraphWrapperPass)
INITIALIZE_PASS_END(IteratorRecognitionWrapperPass, ITR_RECOGNIZE_PASS_NAME, PRJ_CMDLINE_DESC("iterator recognition pass"), false, true)

namespace llvm {
  FunctionPass *llvm::createIteratorRecognitionWrapperPass() {
    return new iteratorrecognition::IteratorRecognitionWrapperPass();
  }
}

//

llvm::AnalysisKey itr::IteratorRecognitionAnalysis::Key;

namespace iteratorrecognition {

// new passmanager pass

IteratorRecognitionAnalysis::IteratorRecognitionAnalysis() {
  llvm::cl::ResetAllOptionOccurrences();
  llvm::cl::ParseEnvironmentOptions(DEBUG_TYPE, PASS_CMDLINE_OPTIONS_ENVVAR);
}

IteratorRecognitionAnalysis::Result
IteratorRecognitionAnalysis::run(llvm::Function &F,
                                 llvm::FunctionAnalysisManager &FAM) {
  const auto &LI{FAM.getResult<llvm::LoopAnalysis>(F)};
  pedigree::PDGraph &Graph{*FAM.getResult<pedigree::PDGraphAnalysis>(F)};
  Graph.connectRootNode();

  return {LI, Graph};
}

// legacy passmanager pass

IteratorRecognitionWrapperPass::IteratorRecognitionWrapperPass() : llvm::FunctionPass(ID) {
  initializeIteratorRecognitionWrapperPassPass(*PassRegistry::getPassRegistry());
}

void IteratorRecognitionWrapperPass::getAnalysisUsage(
    llvm::AnalysisUsage &AU) const {
  AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();
  AU.addRequired<pedigree::PDGraphWrapperPass>();

  AU.setPreservesAll();
}

bool IteratorRecognitionWrapperPass::runOnFunction(llvm::Function &CurFunc) {
  const auto &LI = getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
  pedigree::PDGraph &Graph{
      getAnalysis<pedigree::PDGraphWrapperPass>().getGraph()};
  Graph.connectRootNode();

  Info = std::make_unique<IteratorRecognitionInfo>(LI, Graph);

  return false;
}

} // namespace iteratorrecognition
