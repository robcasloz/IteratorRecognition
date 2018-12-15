//
//
//

#include "IteratorRecognition/Config.hpp"

#include "Pedigree/Support/Utils/InstIterator.hpp"

#include "llvm/Analysis/LoopInfo.h"
// using llvm::LoopBase

#ifndef ITR_INSTTRAVERSAL_HPP
#define ITR_INSTTRAVERSAL_HPP

namespace iteratorrecognition {

template <typename BlockT, typename LoopT>
decltype(auto) make_loop_inst_range(llvm::LoopBase<BlockT, LoopT> &L) {
  return pedigree::make_inst_range(L.block_begin(), L.block_end());
}

template <typename BlockT, typename LoopT>
decltype(auto) make_loop_inst_range(const llvm::LoopBase<BlockT, LoopT> &L) {
  return pedigree::make_inst_range(L.block_begin(), L.block_end());
}

template <typename BlockT, typename LoopT>
decltype(auto) make_loop_inst_range(llvm::LoopBase<BlockT, LoopT> *L) {
  return make_loop_inst_range(*L);
}

template <typename BlockT, typename LoopT>
decltype(auto) make_loop_inst_range(const llvm::LoopBase<BlockT, LoopT> *L) {
  return make_loop_inst_range(*L);
}

} // namespace iteratorrecognition

#endif // header
