//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionWrapperPass.hpp"

#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

#include "IteratorRecognition/Support/FileSystem.hpp"

#include "private/PassCommandLineOptions.hpp"

#include "IteratorRecognition/Analysis/Passes/Exchange/JSONExporterPass.hpp"

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

#include "llvm/Support/ErrorHandling.h"
// using llvm::report_fatal_error

#include "llvm/Support/Debug.h"
// using DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include <system_error>
// using std::error_code

#define DEBUG_TYPE "iterator-recognition-export-json"

// namespace aliases

namespace itr = iteratorrecognition;

// plugin registration for opt

char itr::JSONExporterPass::ID = 0;
static llvm::RegisterPass<itr::JSONExporterPass>
    X("itr-export-json", PRJ_CMDLINE_DESC("iterator recognition pass"), false,
      false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void registerJSONExporterPass(const llvm::PassManagerBuilder &Builder,
                                     llvm::legacy::PassManagerBase &PM) {
  PM.add(new itr::JSONExporterPass());

  return;
}

static llvm::RegisterStandardPasses
    RegisterJSONExporterPass(llvm::PassManagerBuilder::EP_EarlyAsPossible,
                             registerJSONExporterPass);

//

static llvm::cl::opt<std::string>
    ReportsDir("itr-reports-dir", llvm::cl::desc("output reports directory"),
               llvm::cl::cat(IteratorRecognitionCLCategory));

static llvm::cl::opt<bool>
    ExportSCC("itr-export-scc", llvm::cl::desc("export condensations"),
              llvm::cl::init(false),
              llvm::cl::cat(IteratorRecognitionCLCategory));

static llvm::cl::opt<bool> ExportMapping(
    "itr-export-mapping", llvm::cl::desc("export condensation to loop mapping"),
    llvm::cl::init(false), llvm::cl::cat(IteratorRecognitionCLCategory));

namespace iteratorrecognition {

void JSONExporterPass::getAnalysisUsage(llvm::AnalysisUsage &AU) const {
  AU.addRequired<IteratorRecognitionWrapperPass>();

  AU.setPreservesAll();
}

bool JSONExporterPass::runOnFunction(llvm::Function &CurFunc) {
  bool hasChanged = false;
  auto *info = getAnalysis<IteratorRecognitionWrapperPass>().getIteratorInfo();

  auto dirOrErr = CreateDirectory(ReportsDir);
  if (std::error_code ec = dirOrErr.getError()) {
    llvm::errs() << "Error: " << ec.message() << '\n';
    llvm::report_fatal_error("Failed to create reports directory" + ReportsDir);
  }

  ReportsDir = dirOrErr.get();

  ExportCondensations(info->getCondensationGraph(), CurFunc.getName(),
                      ReportsDir);
  ExportCondensationToLoopMapping(info->getMap(), CurFunc.getName(),
                                  ReportsDir);

  return hasChanged;
}

} // namespace iteratorrecognition

