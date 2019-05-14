//===-- IteratorRecognition.cpp - IteratorRecognition Infrastructure ------===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This file defines the common initialization infrastructure for the
// IteratorRecognition library.
//
//===----------------------------------------------------------------------===//

#include "llvm/InitializePasses.h"
#include "llvm/PassRegistry.h"

using namespace llvm;

/// initializeIteratorRecognition - Initialize all passes in the
/// IteratorRecognition library.
void llvm::initializeIteratorRecognition(PassRegistry &Registry) {
  initializeIteratorRecognitionWrapperPassPass(Registry);
}
