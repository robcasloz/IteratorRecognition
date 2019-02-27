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

class Instruction;
class Loop;

namespace json {

template <typename GraphT, typename GT = llvm::GraphTraits<GraphT *>>
std::enable_if_t<itr::has_unit_t<typename GT::NodeRef>::value, Value>
toJSON(const itr::CondensationGraphNode<GraphT *> &CGN) {
  Object root;

  Object mapping;
  std::string outs;
  raw_string_ostream ss(outs);

  Array condensationsArray;
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
std::enable_if_t<!itr::has_unit_t<typename GT::NodeRef>::value, Value>
toJSON(const itr::CondensationGraphNode<GraphT *> &CGN) {
  Object root;

  Object mapping;
  std::string outs;
  raw_string_ostream ss(outs);

  Array condensationsArray;
  br::transform(CGN, std::back_inserter(condensationsArray),
                [&](const auto &e) { return toJSON(*e); });
  mapping["condensation"] = std::move(condensationsArray);
  root = std::move(mapping);

  return std::move(root);
}

template <typename GraphT>
Value toJSON(const itr::CondensationGraph<GraphT *> &G) {
  Object root;
  Array condensations;

  for (const auto &cn : G) {
    condensations.push_back(toJSON(*cn));
  }

  root["condensations"] = std::move(condensations);

  return std::move(root);
}

Value toJSON(const Instruction &Instruction);

Value toJSON(const Loop &CurLoop);

Value toJSON(const itr::dbg::LoopDebugInfoT &Info);

Value toJSON(const itr::IteratorRecognitionInfo::CondensationToLoopsMapT &Map);

Value toJSON(const itr::UpdateAction &UA);

Value toJSON(const itr::StaticCommutativityProperty &SCP);

Value toJSON(const itr::StaticCommutativityResult &SCR);

} // namespace json
} // namespace llvm

namespace iteratorrecognition {

template <typename PreambleT, typename IteratorT>
llvm::json::Value
ConvertToJSON(llvm::StringRef PreambleText, llvm::StringRef SequenceText,
              const PreambleT &Preamble, IteratorT Begin, IteratorT End) {
  llvm::json::Object mapping;
  llvm::json::Array updates;

  mapping[PreambleText] = llvm::json::toJSON(Preamble);
  // TODO maybe detect if the pointee is a pointer itself with SFINAE in order
  // to decide on the use of indirect_iterator or not
  std::transform(boost::make_indirect_iterator(Begin),
                 boost::make_indirect_iterator(End),
                 std::back_inserter(updates),
                 [](const auto &e) { return llvm::json::toJSON(e); });
  mapping[SequenceText] = std::move(updates);

  return std::move(mapping);
}

} // namespace iteratorrecognition

