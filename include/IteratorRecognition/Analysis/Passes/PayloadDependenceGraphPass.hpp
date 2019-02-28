//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "llvm/Pass.h"
// using llvm::FunctionPass

#include "llvm/IR/PassManager.h"
// using llvm::FunctionAnalysisManager
// using llvm::AnalysisInfoMixin

#include <vector>
// using std::vector

#include <string>
// using std::string

#define ITR_PAYLOAD_ANALYSIS_PASS_NAME "itr-payload-graph"

namespace llvm {
class Function;
class DominatorTree;
class LoopInfo;
class Loop;
class AAResults;
} // namespace llvm

namespace iteratorrecognition {

class IteratorRecognitionInfo;
class PayloadDependenceGraphAnalysis;

struct StaticCommutativityProperty {
  const llvm::Loop *CurLoop = nullptr;
  bool HasProperty = false;
  std::string Remark = "";
};

class StaticCommutativityResult {
  friend PayloadDependenceGraphAnalysis;

  using ContainerT = std::vector<StaticCommutativityProperty>;
  ContainerT Properties;

public:
  using iterator = ContainerT::iterator;
  using const_iterator = ContainerT::const_iterator;

  decltype(auto) size() const { return Properties.size(); }

  decltype(auto) begin() { return Properties.begin(); }
  decltype(auto) end() { return Properties.end(); }

  decltype(auto) begin() const { return Properties.begin(); }
  decltype(auto) end() const { return Properties.end(); }
};

// new passmanager pass
class PayloadDependenceGraphAnalysis
    : public llvm::AnalysisInfoMixin<PayloadDependenceGraphAnalysis> {
  friend AnalysisInfoMixin<PayloadDependenceGraphAnalysis>;

  static llvm::AnalysisKey Key;

public:
  using Result = StaticCommutativityResult;

  PayloadDependenceGraphAnalysis();

  Result run(llvm::Function &F, llvm::FunctionAnalysisManager &FAM);
  Result run(llvm::Function &F, llvm::DominatorTree &DT, llvm::LoopInfo &LI,
             llvm::AAResults &AA, IteratorRecognitionInfo &Info);
};

// legacy passmanager pass
class PayloadDependenceGraphLegacyPass : public llvm::FunctionPass {
  using ResultT = StaticCommutativityResult;
  ResultT Result;

public:
  static char ID;

  void getAnalysisUsage(llvm::AnalysisUsage &AU) const override;
  PayloadDependenceGraphLegacyPass() : llvm::FunctionPass(ID) {}

  bool runOnFunction(llvm::Function &F) override;

  ResultT &getResult() { return Result; }
  const ResultT &getResult() const { return Result; }
};

} // namespace iteratorrecognition

