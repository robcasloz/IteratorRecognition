//
//
//

#pragma once

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/Utils/Extras.hpp"

#include "IteratorRecognition/Support/Utils/DebugInfo.hpp"

#include "IteratorRecognition/Support/CondensationGraph.hpp"

#include "IteratorRecognition/Analysis/GraphUpdater.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "IteratorRecognition/Analysis/Passes/PayloadDependenceGraphPass.hpp"

#include "llvm/Support/JSON.h"
// using json::Value
// using json::Object
// using json::Array

#include "llvm/ADT/StringRef.h"
// using llvm::StringRef

#include <llvm/ADT/GraphTraits.h>
// using llvm::GraphTraits

#include "llvm/Support/raw_ostream.h"
// using llvm::raw_string_ostream

#include "boost/iterator/indirect_iterator.hpp"
// using boost::make_indirect_iterator

#include "boost/range/adaptors.hpp"
// using boost::adaptors::filtered

#include "boost/range/algorithm.hpp"
// using boost::range::transform

#include <string>
// using std::string

#include <algorithm>
// using std::transform

#include <iterator>
// using std::back_inserter
// using std::make_move_iterator

#include <utility>
// using std::move

#include <type_traits>
// using std::enable_if

// namespace aliases

namespace ba = boost::adaptors;
namespace br = boost::range;

//

namespace llvm {

class Instruction;
class Loop;

} // namespace llvm

namespace iteratorrecognition {
namespace json {

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT *>>
std::enable_if_t<has_unit_t<typename GT::NodeRef>::value, llvm::json::Value>
toJSON(const CondensationGraphNode<GraphT *> &CGN) {
  llvm::json::Object root;

  llvm::json::Object mapping;
  std::string outs;
  llvm::raw_string_ostream ss(outs);

  llvm::json::Array condensationsArray;
  br::transform(CGN | ba::filtered(is_not_null_unit),
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
std::enable_if_t<!has_unit_t<typename GT::NodeRef>::value, llvm::json::Value>
toJSON(const CondensationGraphNode<GraphT *> &CGN) {
  llvm::json::Object root;

  llvm::json::Object mapping;
  std::string outs;
  llvm::raw_string_ostream ss(outs);

  llvm::json::Array condensationsArray;
  br::transform(CGN, std::back_inserter(condensationsArray),
                [&](const auto &e) { return toJSON(*e); });
  mapping["condensation"] = std::move(condensationsArray);
  root = std::move(mapping);

  return std::move(root);
}

template <typename GraphT>
llvm::json::Value toJSON(const CondensationGraph<GraphT *> &G) {
  llvm::json::Object root;
  llvm::json::Array condensations;

  for (const auto &cn : G) {
    condensations.push_back(toJSON(*cn));
  }

  root["condensations"] = std::move(condensations);

  return std::move(root);
}

llvm::json::Value toJSON(const llvm::Instruction &Instruction);

llvm::json::Value toJSON(const llvm::Loop &CurLoop);

llvm::json::Value toJSON(const dbg::LoopDebugInfoT &Info);

llvm::json::Value
toJSON(const IteratorRecognitionInfo::CondensationToLoopsMapT &Map);

llvm::json::Value toJSON(const UpdateAction &UA);

llvm::json::Value toJSON(const StaticCommutativityProperty &SCP);

llvm::json::Value toJSON(const StaticCommutativityResult &SCR);

} // namespace json
} // namespace iteratorrecognition

namespace iteratorrecognition {

template <typename PreambleT, typename IteratorT>
llvm::json::Value
ConvertToJSON(llvm::StringRef PreambleText, llvm::StringRef SequenceText,
              const PreambleT &Preamble, IteratorT Begin, IteratorT End) {
  llvm::json::Object mapping;
  llvm::json::Array updates;

  mapping[PreambleText] = json::toJSON(Preamble);
  // TODO maybe detect if the pointee is a pointer itself with SFINAE in order
  // to decide on the use of indirect_iterator or not
  std::transform(boost::make_indirect_iterator(Begin),
                 boost::make_indirect_iterator(End),
                 std::back_inserter(updates),
                 [](const auto &e) { return json::toJSON(e); });
  mapping[SequenceText] = std::move(updates);

  return std::move(mapping);
}

} // namespace iteratorrecognition

