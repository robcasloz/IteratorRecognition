//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Util.hpp"

#include "IteratorRecognition/Debug.hpp"

#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

#include "IteratorRecognition/Support/ShadowDependenceGraph.hpp"

#include "IteratorRecognition/Support/Utils/StringConversion.hpp"

#include "IteratorRecognition/Support/LoopRPO.hpp"

#include "IteratorRecognition/Support/FileSystem.hpp"

#include "IteratorRecognition/Analysis/DependenceCache.hpp"

#include "IteratorRecognition/Analysis/DetectOperations.hpp"

#include "IteratorRecognition/Analysis/StaticCommutativityAnalyzer.hpp"

#include "IteratorRecognition/Analysis/IteratorValueTracking.hpp"

#include "IteratorRecognition/Analysis/GraphUpdater.hpp"

#include "IteratorRecognition/Analysis/Passes/PayloadDependenceGraphPass.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/Passes/IteratorRecognitionPass.hpp"

#include "IteratorRecognition/Analysis/ValueClassification.hpp"

#include "Pedigree/Analysis/Passes/PDGraphPass.hpp"

#include "private/PassCommandLineOptions.hpp"

#include "llvm/Pass.h"
// using llvm::RegisterPass

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop
// using llvm::LoopInfo
// using llvm::LoopInfoWrapperPass

#include "llvm/Analysis/AliasAnalysis.h"
// using llvm::AAResultsWrapperPass
// using llvm::AAResults

#include "llvm/IR/Dominators.h"
// using llvm::DominatorTreeWrapperPass
// using llvm::DominatorTree

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/IR/Function.h"
// using llvm::Function

#include "llvm/IR/Instructions.h"
// using llvm::StoreInst

#include "llvm/IR/LegacyPassManager.h"
// using llvm::PassManagerBase

#include "llvm/Transforms/IPO/PassManagerBuilder.h"
// using llvm::PassManagerBuilder
// using llvm::RegisterStandardPasses

#include "llvm/ADT/SmallPtrSet.h"
// using llvm::SmallPtrSetImpl

#include "llvm/ADT/DenseMap.h"
// using llvm::DenseMap

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::ParseEnvironmentOptions
// using llvm::cl::ResetAllOptionOccurrences

#include "llvm/Support/ErrorHandling.h"
// using llvm::report_fatal_error

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include <string>
// using std::string

#include <algorithm>
// using std::for_each
// using std::reverse

#include <system_error>
// using std::error_code

#include <cassert>
// using assert

#define DEBUG_TYPE ITR_PAYLOAD_ANALYSIS_PASS_NAME
#define PASS_CMDLINE_OPTIONS_ENVVAR "ITR_PAYLOAD_ANALYSIS_CMDLINE_OPTIONS"

// namespace aliases

namespace itr = iteratorrecognition;

// plugin registration for opt

char itr::PayloadDependenceGraphLegacyPass::ID = 0;
static llvm::RegisterPass<itr::PayloadDependenceGraphLegacyPass>
    X(ITR_PAYLOAD_ANALYSIS_PASS_NAME,
      PRJ_CMDLINE_DESC("payload analysis (based on iterator recognition) pass"),
      false, false);

// plugin registration for clang

// the solution was at the bottom of the header file
// 'llvm/Transforms/IPO/PassManagerBuilder.h'
// create a static free-floating callback that uses the legacy pass manager to
// add an instance of this pass and a static instance of the
// RegisterStandardPasses class

static void registerPayloadDependenceGraphLegacyPass(
    const llvm::PassManagerBuilder &Builder,
    llvm::legacy::PassManagerBase &PM) {
  PM.add(new itr::PayloadDependenceGraphLegacyPass());

  return;
}

static llvm::RegisterStandardPasses RegisterPayloadDependenceGraphLegacyPass(
    llvm::PassManagerBuilder::EP_EarlyAsPossible,
    registerPayloadDependenceGraphLegacyPass);

static llvm::cl::opt<bool>
    ExportGraphUpdates("itr-export-updates",
                       llvm::cl::desc("export graph updates"));

static llvm::cl::opt<bool> ExportResults("itr-export-results",
                                         llvm::cl::desc("export results"));

//

llvm::AnalysisKey itr::PayloadDependenceGraphAnalysis::Key;

namespace iteratorrecognition {

// new passmanager pass

PayloadDependenceGraphAnalysis::Result
PayloadDependenceGraphAnalysis::run(llvm::Function &F,
                                    llvm::FunctionAnalysisManager &FAM) {
  llvm::cl::ResetAllOptionOccurrences();
  llvm::cl::ParseEnvironmentOptions(ITR_PAYLOAD_ANALYSIS_PASS_NAME,
                                    PASS_CMDLINE_OPTIONS_ENVVAR);

  return run(F, FAM.getResult<llvm::DominatorTreeAnalysis>(F),
             FAM.getResult<llvm::LoopAnalysis>(F),
             FAM.getResult<llvm::AAManager>(F),
             FAM.getResult<IteratorRecognitionAnalysis>(F));
}

PayloadDependenceGraphAnalysis::Result
PayloadDependenceGraphAnalysis::run(llvm::Function &F, llvm::DominatorTree &DT,
                                    llvm::LoopInfo &LI, llvm::AAResults &AA,
                                    IteratorRecognitionInfo &Info) {
  if (ExportGraphUpdates || ExportResults) {
    auto dirOrErr = CreateDirectory(ReportsDir);
    if (std::error_code ec = dirOrErr.getError()) {
      llvm::errs() << "Error: " << ec.message() << '\n';
      llvm::report_fatal_error("Failed to create reports directory" +
                               ReportsDir);
    }

    ReportsDir = dirOrErr.get();
  }

  Result result;

  if (FunctionWhiteList.size()) {
    auto found = std::find(FunctionWhiteList.begin(), FunctionWhiteList.end(),
                           std::string{F.getName()});
    if (found == FunctionWhiteList.end()) {
      return result;
    }
  }

  unsigned loopCount = 0;

  LLVM_DEBUG({
    llvm::dbgs() << "payload dependence graph for function: " << F.getName()
                 << "\n";
  });

  llvm::SmallVector<llvm::Loop *, 8> loopTraversal;
  for (auto *e : LI.getLoopsInPreorder()) {
    loopTraversal.push_back(e);
  }
  std::reverse(loopTraversal.begin(), loopTraversal.end());

  for (auto *curLoop : loopTraversal) {
    if (curLoop->getLoopDepth() > LoopDepthMax ||
        curLoop->getLoopDepth() < LoopDepthMin) {
      continue;
    }

    LLVM_DEBUG(llvm::dbgs() << "#######\nloop: " << strconv::to_string(*curLoop)
                            << "\n";);

    auto infoOrError = Info.getIteratorInfoFor(curLoop);
    if (!infoOrError) {
      continue;
    }
    auto &info = *infoOrError;

    llvm::SmallPtrSet<llvm::Instruction *, 8> itVals, pdVals, pdLiveVals;
    llvm::SmallPtrSet<llvm::Instruction *, 8> pdVirtRegLiveVals,
        pdVirtRegLiveInVals, pdVirtRegLiveThruVals, pdVirtRegLiveOutVals;

    FindIteratorValues(info, itVals);
    FindPayloadValues(info, pdVals);

    if (pdVals.empty()) {
      LLVM_DEBUG(llvm::dbgs() << "no payload\n";);
      continue;
    }

    FindVirtRegPayloadLiveValues(info, pdVals, pdVirtRegLiveVals);
    SplitVirtRegPayloadLiveValues(info, pdVals, pdVirtRegLiveVals, DT,
                                  pdVirtRegLiveInVals, pdVirtRegLiveThruVals,
                                  pdVirtRegLiveOutVals);

    auto &g = Info.getGraph();
    using DGType = std::remove_reference_t<decltype(g)>;
    using DGT = llvm::GraphTraits<DGType *>;

    auto is_payload = [&pdVals](const auto *e) {
      return pdVals.count(e->unit()) != 0;
    };

    // step 1 graph update

    auto pdModRefFilter = [&set = pdVals](const llvm::Instruction &I) {
      return I.mayReadOrWriteMemory() && set.count(&I);
    };

    LoopRPO pdModRefTraversal(*curLoop, Info.getLoopInfo(), pdModRefFilter);

    LLVM_DEBUG({
      llvm::dbgs() << "payload ModRef RPO traversal:\n";
      for (auto *e : pdModRefTraversal) {
        llvm::dbgs() << *e << '\n';
      }
    });

    DependenceCache dc(pdModRefTraversal.begin(), pdModRefTraversal.end(), AA);

    LLVM_DEBUG({
      llvm::dbgs() << "dependence cache:\n";
      dc.print(llvm::dbgs());
    });

    IteratorVarianceAnalyzer iva(info);
    IteratorVarianceGraphUpdateGenerator<DGType> ivgug{g, iva};
    ActionQueueT dos, undos;

    for (auto &dep : dc) {
      auto v = ivgug.create(dep.first.first, dep.first.second);
      if (v.first && v.second) {
        dos.push_back(std::move(v.first));
        undos.push_back(std::move(v.second));
      }
    }

    if (ExportGraphUpdates) {
      const auto &json =
          ConvertToJSON("loop", "updates", *curLoop, dos.begin(), dos.end());

      WriteJSONToFile(json,
                      "itr.graph_updates." + F.getName() + ".loop." +
                          std::to_string(loopCount),
                      ReportsDir);
    }

    std::for_each(dos.begin(), dos.end(), [](const auto &e) {
      assert(e && "Smart pointer is empty!");
      ExecuteAction(*e);
    });

    // step 2 create shadow graph

    SDependenceGraph<DGType> sg2(g);
    sg2.computeNodes(is_payload);
    sg2.computeEdges();

    // step 3 detect operations

    llvm::SmallVector<LoadModifyStore, 8> lms;
    llvm::SmallPtrSet<llvm::Instruction *, 8> uniqueTargets;

    for (auto *e : pdVirtRegLiveOutVals) {
      lms.emplace_back(LoadModifyStore{e, {}, {}});
    }

    for (auto &e : dc) {
      auto &dep = e.first;
      if (uniqueTargets.count(dep.second)) {
        continue;
      }

      uniqueTargets.insert(dep.second);
      lms.emplace_back(LoadModifyStore{dep.second, {}, {}});
    }

    MutationDetector<DGType> md{dc};
    for (auto &e : lms) {
      md.process(sg2, *curLoop, e);
    }

    // step 4 determine commutativity

    StaticCommutativityAnalyzer sca;
    bool isCommutative = sca.analyze(lms, iva);
    LLVM_DEBUG(llvm::dbgs() << "loop: " << strconv::to_string(*curLoop)
                            << " commutativity: " << isCommutative << '\n';);

    result.Properties.push_back({curLoop, isCommutative, ""});

    // step 5 reverse graph updates

    std::for_each(undos.rbegin(), undos.rend(), [](const auto &e) {
      assert(e && "Smart pointer is empty!");
      ExecuteAction(*e);
    });

    loopCount++;
  }

  if (ExportResults) {
    WriteJSONToFile(llvm::json::toJSON(result), "sca." + F.getName(),
                    ReportsDir);
  }

  return result;
}

// legacy passmanager pass

void PayloadDependenceGraphLegacyPass::getAnalysisUsage(
    llvm::AnalysisUsage &AU) const {
  AU.addRequired<llvm::DominatorTreeWrapperPass>();
  AU.addRequiredTransitive<llvm::LoopInfoWrapperPass>();
  AU.addRequiredTransitive<llvm::AAResultsWrapperPass>();
  AU.addRequiredTransitive<IteratorRecognitionWrapperPass>();

  AU.setPreservesAll();
}

bool PayloadDependenceGraphLegacyPass::runOnFunction(llvm::Function &F) {
  auto &DT = getAnalysis<llvm::DominatorTreeWrapperPass>().getDomTree();
  auto &LI = getAnalysis<llvm::LoopInfoWrapperPass>().getLoopInfo();
  auto &AA = getAnalysis<llvm::AAResultsWrapperPass>().getAAResults();
  auto &IRI = getAnalysis<IteratorRecognitionWrapperPass>()
                  .getIteratorRecognitionInfo();
  PayloadDependenceGraphAnalysis pdga{};

  Result = pdga.run(F, DT, LI, AA, IRI);

  return false;
}

} // namespace iteratorrecognition
