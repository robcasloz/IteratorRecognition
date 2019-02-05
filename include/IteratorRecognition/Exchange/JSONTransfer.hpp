//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/Utils/Extras.hpp"

#include "IteratorRecognition/Support/Utils/DebugInfo.hpp"

#include "IteratorRecognition/Support/CondensationGraph.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "llvm/Support/JSON.h"
// using json::Value
// using json::Object
// using json::Array

#include <llvm/ADT/GraphTraits.h>
// using llvm::GraphTraits

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_string_ostream

#include "boost/range/adaptors.hpp"
// using boost::adaptors::filtered

#include "boost/range/algorithm.hpp"
// using boost::range::transform

#include <string>
// using std::string

#include <iterator>
// using std::back_inserter

#include <utility>
// using std::move

#include <type_traits>
// using std::enable_if

// namespace aliases

namespace itr = iteratorrecognition;

namespace ba = boost::adaptors;
namespace br = boost::range;

//

namespace iteratorrecognition {

void WriteJSONToFile(const llvm::json::Value &V,
                     const llvm::Twine &FilenamePrefix, const llvm::Twine &Dir);

} // namespace iteratorrecognition

//

namespace llvm {

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT *>>
std::enable_if_t<itr::has_unit_t<typename GT::NodeRef>::value, json::Value>
toJSON(const itr::CondensationGraphNode<GraphT *> &CGN) {
  json::Object root;

  json::Object mapping;
  std::string outs;
  raw_string_ostream ss(outs);

  json::Array condensationsArray;
  br::transform(CGN | ba::filtered(itr::is_not_null_unit),
                std::back_inserter(condensationsArray), [&](const auto &e) {
                  outs.clear();
                  ss << *e->unit();
                  return ss.str();
                });
  mapping["condensation"] = std::move(condensationsArray);
  root = std::move(mapping);

  return std::move(root);
}

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT *>>
std::enable_if_t<!itr::has_unit_t<typename GT::NodeRef>::value, json::Value>
toJSON(const itr::CondensationGraphNode<GraphT *> &CGN) {
  json::Object root;

  json::Object mapping;
  std::string outs;
  raw_string_ostream ss(outs);

  json::Array condensationsArray;
  br::transform(CGN, std::back_inserter(condensationsArray),
                [&](const auto &e) { return toJSON(*e); });
  mapping["condensation"] = std::move(condensationsArray);
  root = std::move(mapping);

  return std::move(root);
}

template <typename GraphT>
json::Value toJSON(const itr::CondensationGraph<GraphT *> &G) {
  json::Object root;
  json::Array condensations;

  for (const auto &cn : G) {
    condensations.push_back(toJSON(*cn));
  }

  root["condensations"] = std::move(condensations);

  return std::move(root);
}

class Instruction;

json::Value toJSON(const Instruction &Instruction);

class Loop;

json::Value toJSON(const Loop &CurLoop);

json::Value toJSON(const itr::dbg::LoopDebugInfoT &Info);

json::Value
toJSON(const itr::IteratorRecognitionInfo::CondensationToLoopsMapT &Map);

} // namespace llvm

