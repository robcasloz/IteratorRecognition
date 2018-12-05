//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Exchange/JSONTransfer.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include <algorithm>
// using std::transform

namespace llvm {

json::Value
toJSON(const itr::IteratorRecognitionInfo::CondensationToLoopsMapT &Map) {
  json::Object root;
  json::Array condensations;

  for (const auto &e : Map) {
    const auto &cn = *e.getFirst();
    const auto &loops = e.getSecond();

    json::Object mapping;
    std::string outs;
    raw_string_ostream ss(outs);

    const auto &firstUnit = (*cn.begin())->unit();

    if (firstUnit) {
      ss << *firstUnit;
    }
    mapping["condensation"] = ss.str();
    outs.clear();

    json::Array loopsArray;
    std::transform(loops.begin(), loops.end(), std::back_inserter(loopsArray),
                   [&](const auto &e) {
                     ss << *e->getLoopLatch()->getTerminator();
                     return ss.str();
                   });
    mapping["loops"] = std::move(loopsArray);
    outs.clear();

    condensations.push_back(std::move(mapping));
  }

  root["condensations"] = std::move(condensations);

  return std::move(root);
}

} // namespace llvm

namespace iteratorrecognition {

void ExportCondensationToLoopMapping(
    const itr::IteratorRecognitionInfo::CondensationToLoopsMapT &Map,
    const llvm::Twine &FilenamePart, const llvm::Twine &Dir) {

  std::string absFilename{Dir.str() + "/itr.scc_to_loop." + FilenamePart.str() +
                          ".json"};
  llvm::StringRef filename{llvm::sys::path::filename(absFilename)};
  llvm::errs() << "Writing file '" << filename << "'... ";

  std::error_code ec;
  llvm::ToolOutputFile of(absFilename, ec, llvm::sys::fs::F_Text);

  if (ec) {
    llvm::errs() << "error opening file '" << filename << "' for writing!\n";
    of.os().clear_error();
  }

  of.os() << llvm::formatv("{0:2}", toJSON(Map));
  of.os().close();

  if (!of.os().has_error()) {
    of.keep();
  }

  llvm::errs() << " done. \n";
}

} // namespace iteratorrecognition
