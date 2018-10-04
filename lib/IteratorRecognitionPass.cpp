//
//
//

#include "Config.hpp"

#include "Util.hpp"

#include "Debug.hpp"

#include "Analysis/Passes/PDGraphPass.hpp"

#include "IteratorRecognitionPass.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo
// using llvm::LoopInfoWrapperPass

#include "llvm/IR/Type.h"
// using llvm::Type

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/Support/Casting.h"
// using llvm::dyn_cast

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/ADT/SCCIterator.h"
// using llvm::scc_begin
// using llvm::scc_end

#include "llvm/ADT/iterator_range.h"
// using llvm::make_range

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::location
// using llvm::cl::cat
// using llvm::cl::OptionCategory

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs

#include <memory>
// using std::unique_ptr

#define DEBUG_TYPE "iterator-recognition"

// plugin registration for opt

char itr::IteratorRecognitionPass::ID = 0;
static llvm::RegisterPass<itr::IteratorRecognitionPass>
    X("itr", PRJ_CMDLINE_DESC("iterator recognition pass"), false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void
registerIteratorRecognitionPass(const llvm::PassManagerBuilder &Builder,
                                llvm::legacy::PassManagerBase &PM) {
  PM.add(new itr::IteratorRecognitionPass());

  return;
}

static llvm::RegisterStandardPasses RegisterIteratorRecognitionPass(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    registerIteratorRecognitionPass);

//

static llvm::cl::OptionCategory
    IteratorRecognitionPassCategory("Iterator Recognition Pass",
                                    "Options for Iterator Recognition pass");

#if ITERATORRECOGNITION_DEBUG
static llvm::cl::opt<bool, true>
    Debug("itr-debug", llvm::cl::desc("debug iterator recognition pass"),
          llvm::cl::location(itr::debug::passDebugFlag),
          llvm::cl::cat(IteratorRecognitionPassCategory));

static llvm::cl::opt<LogLevel, true> DebugLevel(
    "itr-debug-level",
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
    llvm::cl::cat(IteratorRecognitionPassCategory));
#endif // ITERATORRECOGNITION_DEBUG

//

namespace itr {

void IteratorRecognitionPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<llvm::LoopInfoWrapperPass>();
  AU.addRequired<pedigree::PDGraphPass>();

  AU.setPreservesAll();
}

bool IteratorRecognitionPass::runOnFunction(llvm::Function &CurFunc) {
  bool hasChanged = false;

  const auto *LI = &getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
  if (LI->empty()) {
    return hasChanged;
  }

  pedigree::PDGraph &Graph{getAnalysis<pedigree::PDGraphPass>().getGraph()};

  for (auto scc = llvm::scc_begin(&Graph); !scc.isAtEnd(); ++scc)
    ;

  auto ret_type = CurFunc.getReturnType();

  for (auto bi = CurFunc.begin(); CurFunc.end() != bi; ++bi)
    for (auto ii = bi->begin(); bi->end() != ii; ++ii) {
      for (auto user : ii->users())
        if (auto user_inst = llvm::dyn_cast<llvm::Instruction>(user))
          ;

      for (auto &use : ii->operands())
        auto v = use.get();
    }

  return hasChanged;
}

} // namespace itr
