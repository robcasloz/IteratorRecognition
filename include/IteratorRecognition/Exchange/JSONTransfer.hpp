//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/Utils/Extras.hpp"

#include "IteratorRecognition/Support/CondensationGraph.hpp"

#include "IteratorRecognition/Analysis/IteratorRecognition.hpp"

#include "llvm/Support/JSON.h"
// using json::Value
// using json::Object
// using json::Array

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

#include <utility>
// using std::move

#ifndef ITR_JSONTRANSFER_HPP
#define ITR_JSONTRANSFER_HPP

// namespace aliases

namespace itr = iteratorrecognition;

namespace ba = boost::adaptors;
namespace br = boost::range;

//

namespace llvm {

// TODO consider restricting to graph that have a unit with the usage
// requirements described here
template <typename GraphT>
json::Value toJSON(const itr::CondensationGraph<GraphT *> &G) {
  json::Object root;
  json::Array condensations;

  for (const auto &cn : G) {
    json::Object mapping;
    std::string outs;
    raw_string_ostream ss(outs);

    json::Array condensationsArray;
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

json::Value
toJSON(const itr::IteratorRecognitionInfo::CondensationToLoopsMapT &Map);

} // namespace llvm

namespace iteratorrecognition {

void WriteJSONToFile(const llvm::json::Value &V,
                     const llvm::Twine &FilenamePrefix, const llvm::Twine &Dir);

} // namespace iteratorrecognition

#endif // header
