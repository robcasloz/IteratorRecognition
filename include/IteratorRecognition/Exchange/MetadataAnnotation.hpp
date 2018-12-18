//
//
//

#pragma once

#include "llvm/IR/Instruction.h"
// using llvm::Instruction

#include "llvm/Analysis/LoopInfo.h"
// using llvm::Loop

#include "llvm/IR/MDBuilder.h"
// using llvm::MDBuilder

#include "llvm/IR/Metadata.h"
// using llvm::Metadata
// using llvm::MDNode
// using llvm::MDString

#include "llvm/ADT/SmallVector.h"
// using llvm::SmallVector

#include "boost/range/algorithm.hpp"
// using boost::make_iterator_range
// using boost::find_if
// using boost::copy

#include "boost/range/iterator.hpp"
// using boost::end

#include <tuple>
// using std::make_tuple
// using std::get
// using std::tie

#include <iterator>
// using std::back_inserter

namespace iteratorrecognition {

static constexpr char *DefaultIteratorInstructionKey = "icsa.itr.iterator";
static constexpr char *DefaultPayloadInstructionKey = "icsa.itr.payload";
static constexpr char *DefaultLoopKey = "llvm.loop.icsa.itr";

class MetadataAnnotationWriter {
  decltype(auto) annotateLoop(llvm::StringRef Key, llvm::Loop &CurLoop) {
    bool hasChanged = false;

    llvm::SmallVector<llvm::Metadata *, 4> newMetadata{};

    auto out = std::back_inserter(newMetadata);
    *out = nullptr; // preserve first place for new loop ID

    // preserve existing metadata, if any
    auto *curID = CurLoop.getLoopID();
    if (curID) {
      auto rng = boost::make_iterator_range(curID->op_begin(), curID->op_end());

      auto pred = [&Key](auto &e) -> bool {
        auto *s = llvm::dyn_cast<llvm::MDString>(e);
        return s && s->getString().equals(Key);
      };

      if (boost::find_if(rng, pred) != boost::end(rng)) {
        return std::make_tuple(hasChanged, curID);
      }

      boost::copy(
          boost::make_iterator_range(curID->op_begin(), curID->op_end()), out);
    }

    auto &ctx = CurLoop.getHeader()->getParent()->getContext();
    llvm::MDBuilder builder(ctx);
    *out = builder.createString(Key);

    // update loop metadata id
    auto *newID = llvm::MDNode::get(ctx, newMetadata);
    newID->replaceOperandWith(0, newID);

    CurLoop.setLoopID(newID);

    return std::make_tuple(hasChanged, llvm::dyn_cast<llvm::MDNode>(newID));
  }

public:
  MetadataAnnotationWriter() = default;

  bool append(llvm::Instruction &Inst, llvm::StringRef Key,
              llvm::MDNode *AdditionalData) {
    auto *data = Inst.getMetadata(Key);
    data =
        data ? llvm::MDNode::concatenate(data, AdditionalData) : AdditionalData;
    Inst.setMetadata(Key, data);

    return true;
  }

  template <typename ForwardRange>
  bool append(ForwardRange &Rng, llvm::StringRef InstructionKey,
              llvm::MDNode *AdditionalData) {
    bool hasChanged = false;

    for (auto &e : Rng) {
      hasChanged |= append(*e, InstructionKey, AdditionalData);
    }

    return hasChanged;
  }

  template <typename ForwardRange>
  bool append(ForwardRange &Rng, llvm::StringRef InstructionKey,
              llvm::Loop &CurLoop, llvm::StringRef LoopKey) {
    bool hasChanged = false;
    llvm::MDNode *loopID = nullptr;

    std::tie(hasChanged, loopID) = annotateLoop(LoopKey, CurLoop);
    hasChanged |= append(Rng, InstructionKey, loopID);

    return hasChanged;
  }
};

} // namespace iteratorrecognition

