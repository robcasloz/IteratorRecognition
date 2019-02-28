//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Analysis/Passes/AnnotatorPass.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionPass.hpp"

#include "IteratorRecognition/Exchange/MetadataAnnotation.hpp"

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

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

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::init
// using llvm::cl::ParseEnvironmentOptions
// using llvm::cl::ResetAllOptionOccurrences

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#include <algorithm>
// using std::find

#include <iterator>
// using std::begin
// using std::end

#define DEBUG_TYPE ITR_ANNOTATE_PASS_NAME
#define PASS_CMDLINE_OPTIONS_ENVVAR "ITR_ANNOTATE_CMDLINE_OPTIONS"

// namespace aliases

namespace itr = iteratorrecognition;

// plugin registration for opt

char itr::AnnotatorLegacyPass::ID = 0;

static llvm::RegisterPass<itr::AnnotatorLegacyPass>
    X(ITR_ANNOTATE_PASS_NAME,
      PRJ_CMDLINE_DESC("iterator recognition annotator pass"), false, false);

static llvm::cl::opt<bool> AnnotatePayload(
    "itr-annotate-payload", llvm::cl::init(false), llvm::cl::Hidden,
    llvm::cl::desc("annotate the payload instructions of loops"));

static llvm::cl::opt<unsigned> AnnotateLoopLevel(
    "itr-annotate-loop-level", llvm::cl::init(1), llvm::cl::Hidden,
    llvm::cl::desc("annotate loops of this depth or greater"));

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void registerAnnotatorLegacyPass(const llvm::PassManagerBuilder &Builder,
                                        llvm::legacy::PassManagerBase &PM) {
  PM.add(new itr::AnnotatorLegacyPass());

  return;
}

static llvm::RegisterStandardPasses
    RegisterAnnotatorLegacyPass(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                                registerAnnotatorLegacyPass);

//

namespace iteratorrecognition {

// new passmanager pass

bool AnnotatorPass::run(llvm::Function &F, IteratorRecognitionInfo &Info) {
  bool hasChanged = false;
  MetadataAnnotationWriter annotator;

  for (auto &e : Info.getIteratorsInfo()) {
    llvm::Loop &curLoop = const_cast<llvm::Loop &>(*e.getLoop());

    if (curLoop.getLoopDepth() >= AnnotateLoopLevel) {
      LLVM_DEBUG(llvm::dbgs() << "annotating with iterator loop: "
                              << strconv::to_string(curLoop) << '\n';);

      hasChanged |= annotator.append(e, DefaultIteratorInstructionKey, curLoop,
                                     DefaultLoopKey);
    }

    auto is_not_in = [](const auto &Elem, const auto &ForwardRange) -> bool {
      using std::begin;
      using std::end;
      return end(ForwardRange) ==
             std::find(begin(ForwardRange), end(ForwardRange), Elem);
    };

    if (AnnotatePayload) {
      LLVM_DEBUG(llvm::dbgs() << "annotating with payload loop: "
                              << strconv::to_string(curLoop) << '\n';);

      for (auto &block : curLoop.blocks()) {
        for (auto &inst : *block) {
          if (is_not_in(&inst, e)) {
            hasChanged |= annotator.append(inst, DefaultPayloadInstructionKey,
                                           curLoop, DefaultLoopKey);
          }
        }
      }
    }
  }

  return hasChanged;
}

llvm::PreservedAnalyses AnnotatorPass::run(llvm::Function &F,
                                           llvm::FunctionAnalysisManager &FAM) {
  llvm::cl::ResetAllOptionOccurrences();
  llvm::cl::ParseEnvironmentOptions(ITR_ANNOTATE_PASS_NAME,
                                    PASS_CMDLINE_OPTIONS_ENVVAR);

  run(F, FAM.getResult<IteratorRecognitionAnalysis>(F));

  return llvm::PreservedAnalyses::all();
}

// legacy passmanager pass

void AnnotatorLegacyPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequiredTransitive<IteratorRecognitionWrapperPass>();

  AU.setPreservesAll();
}

bool AnnotatorLegacyPass::runOnFunction(llvm::Function &F) {
  auto &info = getAnalysis<IteratorRecognitionWrapperPass>()
                   .getIteratorRecognitionInfo();
  AnnotatorPass ap;

  return ap.run(F, info);
}

} // namespace iteratorrecognition
