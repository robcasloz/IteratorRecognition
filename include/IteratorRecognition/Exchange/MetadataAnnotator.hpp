//
//
//

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

#include <iterator>
// using std::back_inserter

#ifndef ITR_METADATAANNOTATOR_HPP
#define ITR_METADATAANNOTATOR_HPP

namespace iteratorrecognition {

constexpr const char *DefaultLoopKey = "llvm.loop.icsa.itr";
constexpr const char *DefaultInstructionKey = "icsa.itr.iterator";

class MetadataAnnotator {
  std::string LoopKey{DefaultLoopKey};
  std::string InstructionKey{DefaultInstructionKey};

  void annotate(llvm::Instruction &CurInstruction, llvm::MDNode *LoopMetadata) {
    CurInstruction.setMetadata(InstructionKey, LoopMetadata);
  }

public:
  MetadataAnnotator() = default;

  bool annotate(llvm::Loop &CurLoop) {
    bool hasChanged = false;

    llvm::SmallVector<llvm::Metadata *, 4> newMetadata{};

    auto out = std::back_inserter(newMetadata);
    *out = nullptr; // preserve first place for new loop ID

    // preserve existing metadata, if any
    auto *curMetadata = CurLoop.getLoopID();
    if (curMetadata) {
      auto rng = boost::make_iterator_range(curMetadata->op_begin(),
                                            curMetadata->op_end());

      auto pred = [this](auto &e) -> bool {
        auto *s = llvm::dyn_cast<llvm::MDString>(e);
        return s && s->getString().equals(LoopKey);
      };

      if (boost::find_if(rng, pred) != boost::end(rng)) {
        return hasChanged;
      }

      boost::copy(boost::make_iterator_range(curMetadata->op_begin(),
                                             curMetadata->op_end()),
                  out);
    }

    auto &ctx = CurLoop.getHeader()->getParent()->getContext();
    llvm::MDBuilder builder(ctx);
    *out = builder.createString(LoopKey);

    // update loop metadata id
    auto *newID = llvm::MDNode::get(ctx, newMetadata);
    newID->replaceOperandWith(0, newID);

    CurLoop.setLoopID(newID);

    return hasChanged;
  }

  template <typename ForwardRange>
  void annotate(llvm::Loop &CurLoop, ForwardRange Rg) {
    annotate(CurLoop);
  }
};

} // namespace iteratorrecognition

#endif // header