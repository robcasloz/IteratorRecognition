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

// namespace aliases

namespace itr = iteratorrecognition;

//

namespace llvm {
namespace json {

Value toJSON(const Instruction &I) {
  std::string outs;
  raw_string_ostream ss(outs);

  ss << I;
  Value info{ss.str()};

  return std::move(info);
}

Value toJSON(const Loop &CurLoop) {
  Object infoMapping;

  std::string outs;
  raw_string_ostream ss(outs);

  ss << *CurLoop.getLoopLatch()->getTerminator();
  infoMapping["latch"] = ss.str();

  const auto &info = itr::dbg::extract(CurLoop);
  infoMapping["di"] = toJSON(info);

  return std::move(infoMapping);
}

Value toJSON(const itr::dbg::LoopDebugInfoT &Info) {
  Object infoMapping;

  infoMapping["line"] = std::get<0>(Info);
  infoMapping["column"] = std::get<1>(Info);
  infoMapping["function"] = std::get<2>(Info);
  infoMapping["filename"] = std::get<3>(Info);

  return std::move(infoMapping);
}

Value toJSON(const itr::IteratorRecognitionInfo::CondensationToLoopsMapT &Map) {
  Object root;
  Array condensations;

  for (const auto &e : Map) {
    const auto &cn = *e.getFirst();
    const auto &loops = e.getSecond();

    auto mapping = std::move(*toJSON(cn).getAsObject());

    Array loopsArray;
    std::transform(loops.begin(), loops.end(), std::back_inserter(loopsArray),
                   [&](const auto &e) { return std::move(toJSON(*e)); });
    mapping["loops"] = std::move(loopsArray);

    condensations.push_back(std::move(mapping));
  }

  root["condensations"] = std::move(condensations);

  return std::move(root);
}

Value toJSON(const itr::UpdateAction &UA) { return UA.toJSON(); }

Value toJSON(const itr::StaticCommutativityProperty &SCP) {
  Object infoMapping;

  infoMapping["loop"] = toJSON(*SCP.CurLoop);
  infoMapping["commutative"] = SCP.HasProperty;
  infoMapping["remark"] = SCP.Remark;

  return std::move(infoMapping);
}

Value toJSON(const itr::StaticCommutativityResult &SCR) {
  Object root;
  Array loopsArray;

  std::transform(SCR.begin(), SCR.end(), std::back_inserter(loopsArray),
                 [&](const auto &e) { return std::move(toJSON(e)); });

  root["static commutativity"] = std::move(loopsArray);

  return std::move(root);
}

} // namespace json
} // namespace llvm

