//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/Utils/Extras.hpp"

#include "IteratorRecognition/Support/CondensationGraph.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include "llvm/Support/JSON.h"
// using json::Value
// using json::Object
// using json::Array

#include "llvm/Support/FileSystem.h"
// using llvm::sys::fs::F_Text

#include "llvm/ADT/DenseSet.h"
// using llvm::DenseSet

#include "llvm/ADT/GraphTraits.h"
// using llvm::GraphTraits

#include "llvm/Support/Path.h"
// using llvm::sys::path::filename

#include "llvm/Support/ToolOutputFile.h"
// using llvm::ToolOutputFile

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_ostream
// using llvm::raw_string_ostream

#include "boost/range/adaptors.hpp"
// using boost::adaptors::filtered

#include "boost/range/algorithm.hpp"
// using boost::range::transform

#include <string>
// using std::string

#include <iterator>
// using std::back_inserter

#include <algorithm>
// using std::transform

#include <utility>
// using std::move

#include <system_error>
// using std::error_code

#ifndef ITR_JSONTRANSFER_HPP
#define ITR_JSONTRANSFER_HPP

// namespace aliases

namespace itr = iteratorrecognition;

namespace ba = boost::adaptors;
namespace br = boost::range;

//

namespace llvm {

template <typename GraphT>
llvm::json::Value toJSON(const itr::CondensationGraph<GraphT *> &G) {
  llvm::json::Object root;
  llvm::json::Array condensations;

  for (const auto &cn : G) {
    llvm::json::Object mapping;
    std::string outs;
    llvm::raw_string_ostream ss(outs);

    llvm::json::Array condensationsArray;
    br::transform(*cn | ba::filtered(itr::is_not_null_unit),
                  std::back_inserter(condensationsArray), [&](const auto &e) {
                    ss << *e->unit();
                    auto s = ss.str();
                    outs.clear();
                    return s;
                  });
    mapping["condensation"] = std::move(condensationsArray);
    outs.clear();

    condensations.push_back(std::move(mapping));
  }

  root["condensations"] = std::move(condensations);

  return std::move(root);
}

} // namespace llvm

namespace iteratorrecognition {

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT>>
void ExportCondensations(const GraphT &G, const llvm::Twine &FilenamePart,
                         const llvm::Twine &Dir = ".") {
  std::string absFilename{Dir.str() + "/itr.scc." + FilenamePart.str() +
                          ".json"};
  llvm::StringRef filename{llvm::sys::path::filename(absFilename)};
  llvm::errs() << "Writing file '" << filename << "'... ";

  std::error_code ec;
  llvm::ToolOutputFile of(absFilename, ec, llvm::sys::fs::F_Text);

  if (ec) {
    llvm::errs() << "error opening file '" << filename << "' for writing!\n";
    of.os().clear_error();
  }

  of.os() << llvm::formatv("{0:2}", toJSON(G));
  of.os().close();

  if (!of.os().has_error()) {
    of.keep();
  }

  llvm::errs() << " done. \n";
}

template <typename NodeRef>
void ExportCondensationToLoopMapping(
    const llvm::DenseMap<NodeRef, llvm::DenseSet<llvm::Loop *>> &Map,
    const llvm::Twine &FilenamePart, const llvm::Twine &Dir = ".") {
  llvm::json::Object root;
  llvm::json::Array condensations;

  for (const auto &e : Map) {
    const auto &cn = *e.getFirst();
    const auto &loops = e.getSecond();

    llvm::json::Object mapping;
    std::string outs;
    llvm::raw_string_ostream ss(outs);

    const auto &firstUnit = (*cn.begin())->unit();

    if (firstUnit) {
      ss << *firstUnit;
    }
    mapping["condensation"] = ss.str();
    outs.clear();

    llvm::json::Array loopsArray;
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

  of.os() << llvm::formatv("{0:2}", llvm::json::Value(std::move(root)));
  of.os().close();

  if (!of.os().has_error()) {
    of.keep();
  }

  llvm::errs() << " done. \n";
}

} // namespace iteratorrecognition

#endif // header
