//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionWrapperPass.hpp"

#include "llvm/IR/PassManager.h"
// using llvm::FunctionAnalysisManager

#include "llvm/Passes/PassBuilder.h"
// using llvm::PassBuilder

#include "llvm/Passes/PassPlugin.h"
// using llvmGetPassPluginInfo

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs

#define DEBUG_TYPE "itr-plugin-registration"

// plugin registration for opt new passmanager

namespace {

namespace itr = iteratorrecognition;

void registerITRRecognizeCallbacks(llvm::PassBuilder &PB) {
  PB.registerAnalysisRegistrationCallback(
      [](llvm::FunctionAnalysisManager &FAM) {
        LLVM_DEBUG(llvm::dbgs() << "registering analysis "
                                << ITR_RECOGNITION_PASS_NAME << "\n";);
        FAM.registerPass([]() { return itr::IteratorRecognitionAnalysis(); });
      });
  PB.registerPipelineParsingCallback(
      [](llvm::StringRef Name, llvm::FunctionPassManager &FPM,
         llvm::ArrayRef<llvm::PassBuilder::PipelineElement>) {
        if (Name == "require<" ITR_RECOGNITION_PASS_NAME ">") {
          LLVM_DEBUG(llvm::dbgs() << "registering require analysis parser for "
                                  << ITR_RECOGNITION_PASS_NAME << "\n";);

          FPM.addPass(
              llvm::RequireAnalysisPass<itr::IteratorRecognitionAnalysis,
                                        llvm::Function>());
          return true;
        }
        if (Name == "invalidate<" ITR_RECOGNITION_PASS_NAME ">") {
          LLVM_DEBUG(llvm::dbgs()
                         << "registering invalidate analysis parser for "
                         << ITR_RECOGNITION_PASS_NAME << "\n";);

          FPM.addPass(
              llvm::InvalidateAnalysisPass<itr::IteratorRecognitionAnalysis>());
          return true;
        }
        return false;
      });
}

void registerCallbacks(llvm::PassBuilder &PB) {
  registerITRRecognizeCallbacks(PB);
}

} // namespace

extern "C" ::llvm::PassPluginLibraryInfo LLVM_ATTRIBUTE_WEAK
llvmGetPassPluginInfo() {
  return {LLVM_PLUGIN_API_VERSION, "IteratorRecognitionPlugin",
          STRINGIFY(VERSION_STRING), registerCallbacks};
}

