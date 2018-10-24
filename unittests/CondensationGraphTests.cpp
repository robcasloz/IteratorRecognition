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

#include <iterator>
// using std::distance

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

    G1.connectRootNode();
  }
};

//

TEST_F(CondensationGraphTest, CondensationsCount) {
  CondensationGraph<TestGraphTy *> CG{llvm::scc_begin(&G1), llvm::scc_end(&G1)};

  // add 1 for virtual root
  EXPECT_EQ(3 + 1, CG.size());
}

TEST_F(CondensationGraphTest, CondensationNodesCount) {
  CondensationGraph<TestGraphTy *> CG{llvm::scc_begin(&G1), llvm::scc_end(&G1)};
  auto SCC0NodesCount =
      std::distance(CG.scc_members_begin(DepNodes1[0]), CG.scc_members_end());

  auto SCC1NodesCount =
      std::distance(CG.scc_members_begin(DepNodes1[3]), CG.scc_members_end());

  auto SCC2NodesCount =
      std::distance(CG.scc_members_begin(DepNodes1[5]), CG.scc_members_end());

  EXPECT_EQ(3, SCC0NodesCount);
  EXPECT_EQ(2, SCC1NodesCount);
  EXPECT_EQ(1, SCC2NodesCount);
}

TEST_F(CondensationGraphTest, CondensationOutEdgesCount) {
  CondensationGraph<TestGraphTy *> CG{llvm::scc_begin(&G1), llvm::scc_end(&G1)};
  auto SCC0EdgesCount = std::distance(CG.child_edge_begin(DepNodes1[1]),
                                      CG.child_edge_end(DepNodes1[1]));

  auto SCC1EdgesCount = std::distance(CG.child_edge_begin(DepNodes1[3]),
                                      CG.child_edge_end(DepNodes1[3]));

  auto SCC2EdgesCount = std::distance(CG.child_edge_begin(DepNodes1[5]),
                                      CG.child_edge_end(DepNodes1[5]));

  EXPECT_EQ(2, SCC0EdgesCount);
  EXPECT_EQ(1, SCC1EdgesCount);
  EXPECT_EQ(0, SCC2EdgesCount);
}

TEST_F(CondensationGraphTest, CondensationInEdgesCount) {
  CondensationGraph<TestGraphTy *> CG{llvm::scc_begin(&G1), llvm::scc_end(&G1)};
  auto SCC0EdgesCount = std::distance(CG.inverse_child_edge_begin(DepNodes1[1]),
                                      CG.inverse_child_edge_end(DepNodes1[1]));

  auto SCC1EdgesCount = std::distance(CG.inverse_child_edge_begin(DepNodes1[3]),
                                      CG.inverse_child_edge_end(DepNodes1[3]));

  auto SCC2EdgesCount = std::distance(CG.inverse_child_edge_begin(DepNodes1[5]),
                                      CG.inverse_child_edge_end(DepNodes1[5]));

  // add 1 for virtual root
  EXPECT_EQ(0 + 1, SCC0EdgesCount);
  EXPECT_EQ(1 + 1, SCC1EdgesCount);
  EXPECT_EQ(2 + 1, SCC2EdgesCount);
}

} // unnamed namespace
} // namespace testing
} // namespace itr
