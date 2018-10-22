//
//
//

#include "TestCommon.hpp"

#include "Support/GenericDependenceNode.hpp"

#include "Support/GenericDependenceGraph.hpp"

#include "CondensationGraph.hpp"

#include "llvm/ADT/SCCIterator.h"
// using llvm::scc_begin
// using llvm::scc_end

#include "gtest/gtest.h"
// using testing::Test

#include <array>
// using std::array

namespace itr {
namespace testing {
namespace {

using TestNodeTy = pedigree::GenericDependenceNode<int>;
using TestGraphTy = pedigree::GenericDependenceGraph<TestNodeTy>;

} // unnamed namespace
} // namespace testing
} // namespace itr

namespace llvm {

template <>
struct GraphTraits<itr::testing::TestGraphTy *>
    : public pedigree::LLVMDependenceGraphTraitsHelperBase<
          itr::testing::TestGraphTy *> {};

template <>
struct llvm::GraphTraits<itr::CondensationGraph<itr::testing::TestGraphTy *>>
    : public itr::LLVMCondensationGraphTraitsHelperBase<
          itr::CondensationGraph<itr::testing::TestGraphTy *>> {};

} // namespace llvm

namespace itr {
namespace testing {
namespace {

//

struct CondensationGraphTest : public ::testing::Test {
  std::array<int, 6> TestNodes{{0, 1, 2, 3, 4, 5}};

  std::vector<TestNodeTy *> DepNodes1;
  TestGraphTy G1;

  void SetUp() override {
    for (auto &e : TestNodes) {
      DepNodes1.emplace_back(G1.getOrInsertNode(&e));
    }

    // SCC 1
    DepNodes1[0]->addDependentNode(DepNodes1[1]);
    DepNodes1[1]->addDependentNode(DepNodes1[2]);
    DepNodes1[2]->addDependentNode(DepNodes1[0]);

    // SCC 2
    DepNodes1[3]->addDependentNode(DepNodes1[4]);
    DepNodes1[4]->addDependentNode(DepNodes1[3]);

    // SCC 3
    // just node[5]

    // inter-SCC edges
    // SCC 0 to 2
    DepNodes1[0]->addDependentNode(DepNodes1[5]);
    DepNodes1[1]->addDependentNode(DepNodes1[5]);

    // SCC 0 to 1
    DepNodes1[2]->addDependentNode(DepNodes1[3]);

    // SCC 1 to 2
    DepNodes1[3]->addDependentNode(DepNodes1[5]);
    DepNodes1[4]->addDependentNode(DepNodes1[5]);
  }
};

//

TEST_F(CondensationGraphTest, CondensationsCount) {
  G1.connectRootNode();
  CondensationGraph<TestGraphTy *> CG{llvm::scc_begin(&G1), llvm::scc_end(&G1)};

  EXPECT_EQ(3 + 1, CG.size());
}

} // unnamed namespace
} // namespace testing
} // namespace itr
