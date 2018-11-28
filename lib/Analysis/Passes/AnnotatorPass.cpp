//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Analysis/Passes/AnnotatorPass.hpp"

#include "IteratorRecognition/Analysis/Passes/RecognizerPass.hpp"

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

static llvm::cl::OptionCategory
    AnnotatorPassCategory("Iterator Recognition Metadata Annotator Pass",
                          "Options for Iterator Recognition pass");

#if ITERATORRECOGNITION_DEBUG
static llvm::cl::opt<bool, true>
    Debug("itr-annotate-debug",
          llvm::cl::desc("debug iterator recognition pass"),
          llvm::cl::location(itr::debug::passDebugFlag),
          llvm::cl::cat(AnnotatorPassCategory));

static llvm::cl::opt<LogLevel, true> DebugLevel(
    "itr-annotate-debug-level",
    llvm::cl::desc("debug level for Iterator Recognition pass"),
    llvm::cl::location(itr::debug::passLogLevel),
    llvm::cl::values(
        clEnumValN(LogLevel::Info, "Info", "informational messages"),
        clEnumValN(LogLevel::Notice, "Notice", "significant conditions"),
        clEnumValN(LogLevel::Warning, "Warning", "warning conditions"),
        clEnumValN(LogLevel::Error, "Error", "error conditions"),
        clEnumValN(LogLevel::Debug, "Debug", "debug messages")
// clang-format off
#if (LLVM_VERSION_MAJOR <= 3 && LLVM_VERSION_MINOR < 9)
        , clEnumValEnd
#endif
        // clang-format on
        ),
    llvm::cl::cat(AnnotatorPassCategory));
#endif // ITERATORRECOGNITION_DEBUG

namespace iteratorrecognition {

void AnnotatorPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<RecognizerPass>();

  AU.setPreservesAll();
}

bool AnnotatorPass::runOnFunction(llvm::Function &CurFunc) {
  bool hasChanged = false;

  return hasChanged;
}

} // namespace iteratorrecognition
