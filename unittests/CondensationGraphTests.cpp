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

//

struct CondensationGraphTest : public ::testing::Test {
  using TestNodeTy = pedigree::GenericDependenceNode<int>;
  std::array<int, 6> TestNodes{{0, 1, 2, 3, 4, 5}};

  std::vector<TestNodeTy *> DepNodes1;
  pedigree::GenericDependenceGraph<TestNodeTy> G1;

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

TEST_F(CondensationGraphTest, NodeComparison) {
  // CondensationGraph<int> CG{llvm::scc_begin(&G1), llvm::scc_end(&G1)};

  // EXPECT_EQ(true, DepNodes1[0]->compare(*DepNodes2[0]));
  // EXPECT_EQ(false, *DepNodes1[0] == *DepNodes2[0]);
  // EXPECT_EQ(false, DepNodes1[1]->compare(*DepNodes2[1]));
  // EXPECT_EQ(true, *DepNodes1[1] == *DepNodes2[1]);
}

} // unnamed namespace
} // namespace testing
} // namespace itr
