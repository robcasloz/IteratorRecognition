//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

#include "IteratorRecognition/Support/Utils/DebugInfo.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/Support/FileSystem.h"
// using llvm::sys::fs::F_Text

#include "llvm/Support/Path.h"
// using llvm::sys::path::filename

#include "llvm/Support/ToolOutputFile.h"
// using llvm::ToolOutputFile

#include "llvm/Support/Debug.h"
// using LLVM_DEBUG macro
// using llvm::dbgs
// using llvm::errs

#include <utility>
// using std::move

#include <algorithm>
// using std::transform

//

namespace iteratorrecognition {
namespace json {

llvm::json::Value toJSON(const llvm::Instruction &I) {
  std::string outs;
  llvm::raw_string_ostream ss(outs);

  ss << I;
  llvm::json::Value info{ss.str()};

  return std::move(info);
}

llvm::json::Value toJSON(const llvm::Loop &CurLoop) {
  llvm::json::Object infoMapping;

  std::string outs;
  llvm::raw_string_ostream ss(outs);

  ss << *CurLoop.getLoopLatch()->getTerminator();
  infoMapping["latch"] = ss.str();

  const auto &info = dbg::extract(CurLoop);
  infoMapping["di"] = toJSON(info);

  return std::move(infoMapping);
}

llvm::json::Value toJSON(const dbg::LoopDebugInfoT &Info) {
  llvm::json::Object infoMapping;

  infoMapping["line"] = std::get<0>(Info);
  infoMapping["column"] = std::get<1>(Info);
  infoMapping["function"] = std::get<2>(Info);
  infoMapping["filename"] = std::get<3>(Info);

  return std::move(infoMapping);
}

llvm::json::Value
toJSON(const IteratorRecognitionInfo::CondensationToLoopsMapT &Map) {
  llvm::json::Object root;
  llvm::json::Array condensations;

  for (const auto &e : Map) {
    const auto &cn = *e.getFirst();
    const auto &loops = e.getSecond();

    auto mapping = std::move(*toJSON(cn).getAsObject());

    llvm::json::Array loopsArray;
    std::transform(loops.begin(), loops.end(), std::back_inserter(loopsArray),
                   [&](const auto &e) { return std::move(toJSON(*e)); });
    mapping["loops"] = std::move(loopsArray);

    condensations.push_back(std::move(mapping));
  }

  root["condensations"] = std::move(condensations);

  return std::move(root);
}

llvm::json::Value toJSON(const UpdateAction &UA) { return UA.toJSON(); }

llvm::json::Value toJSON(const StaticCommutativityProperty &SCP) {
  llvm::json::Object infoMapping;

  infoMapping["loop"] = toJSON(*SCP.CurLoop);
  infoMapping["commutative"] = SCP.HasProperty;
  infoMapping["remark"] = SCP.Remark;

  return std::move(infoMapping);
}

llvm::json::Value toJSON(const StaticCommutativityResult &SCR) {
  llvm::json::Object root;
  llvm::json::Array loopsArray;

  std::transform(SCR.begin(), SCR.end(), std::back_inserter(loopsArray),
                 [&](const auto &e) { return std::move(toJSON(e)); });

  root["static commutativity"] = std::move(loopsArray);

  return std::move(root);
}

} // namespace json
} // namespace iteratorrecognition

