//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Analysis/Passes/AnnotatorPass.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionWrapperPass.hpp"

#include "IteratorRecognition/Exchange/MetadataAnnotation.hpp"

#include "private/PassCommandLineOptions.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo
// using llvm::LoopInfoWrapperPass

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo
// using llvm::LoopInfoWrapperPass

#define DEBUG_TYPE "iterator-recognition"

// namespace aliases

namespace itr = iteratorrecognition;

// plugin registration for opt

char itr::AnnotatorPass::ID = 0;
static llvm::RegisterPass<itr::AnnotatorPass>
    X("itr-annotate", PRJ_CMDLINE_DESC("iterator recognition annotator pass"),
      false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void registerAnnotatorPass(const llvm::PassManagerBuilder &Builder,
                                  llvm::legacy::PassManagerBase &PM) {
  PM.add(new itr::AnnotatorPass());

  return;
}

static llvm::RegisterStandardPasses
    RegisterAnnotatorPass(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                          registerAnnotatorPass);

//

namespace iteratorrecognition {

void AnnotatorPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequiredTransitive<IteratorRecognitionWrapperPass>();

  AU.setPreservesAll();
}

bool AnnotatorPass::runOnFunction(llvm::Function &CurFunc) {
  bool hasChanged = false;
  auto &info = getAnalysis<IteratorRecognitionWrapperPass>()
                   .getIteratorRecognitionInfo();

  MetadataAnnotationWriter annotator;

  for (auto &e : info.getIteratorsInfo()) {
    hasChanged |= annotator.annotate(const_cast<llvm::Loop &>(*e.getLoop()), e);
  }

  return hasChanged;
}

} // namespace iteratorrecognition
