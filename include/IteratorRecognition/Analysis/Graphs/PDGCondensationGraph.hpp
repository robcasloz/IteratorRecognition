//
//
//

#include "IteratorRecognition/Config.hpp"

#include "IteratorRecognition/Support/CondensationGraph.hpp"

#include "Pedigree/Analysis/Graphs/DependenceGraphs.hpp"

#ifndef ITR_PDGCONDENSATIONGRAPH_HPP
#define ITR_PDGCONDENSATIONGRAPH_HPP

namespace iteratorrecognition {

extern template class CondensationGraph<pedigree::PDGraph *>;

} // namespace iteratorrecognition

//

namespace llvm {

template <>
struct GraphTraits<iteratorrecognition::CondensationGraph<pedigree::PDGraph *>>
    : public iteratorrecognition::LLVMCondensationGraphTraitsHelperBase<
          iteratorrecognition::CondensationGraph<pedigree::PDGraph *>> {};

template <>
struct GraphTraits<
    const iteratorrecognition::CondensationGraph<pedigree::PDGraph *>>
    : public iteratorrecognition::LLVMCondensationGraphTraitsHelperBase<
          const iteratorrecognition::CondensationGraph<pedigree::PDGraph *>> {};

template <>
struct GraphTraits<
    const iteratorrecognition::CondensationGraph<const pedigree::PDGraph *>>
    : public iteratorrecognition::LLVMCondensationGraphTraitsHelperBase<
          const iteratorrecognition::CondensationGraph<
              const pedigree::PDGraph *>> {};

template <>
struct GraphTraits<
    Inverse<iteratorrecognition::CondensationGraph<pedigree::PDGraph *>>>
    : public iteratorrecognition::LLVMCondensationInverseGraphTraitsHelperBase<
          iteratorrecognition::CondensationGraph<pedigree::PDGraph *>> {};

template <>
struct GraphTraits<
    Inverse<const iteratorrecognition::CondensationGraph<pedigree::PDGraph *>>>
    : public iteratorrecognition::LLVMCondensationInverseGraphTraitsHelperBase<
          const iteratorrecognition::CondensationGraph<pedigree::PDGraph *>> {};

template <>
struct GraphTraits<Inverse<
    const iteratorrecognition::CondensationGraph<const pedigree::PDGraph *>>>
    : public iteratorrecognition::LLVMCondensationInverseGraphTraitsHelperBase<
          const iteratorrecognition::CondensationGraph<
              const pedigree::PDGraph *>> {};

} // namespace llvm

#endif // header
