//===- IteratorRecognition.h - IteratorRecognition --------------*- C++ -*-===//
//
//                     The LLVM Compiler Infrastructure
//
// This file is distributed under the University of Illinois Open Source
// License. See LICENSE.TXT for details.
//
//===----------------------------------------------------------------------===//
//
// This header file defines prototypes for accessor functions that expose passes
// in the IteratorRecognition library.
//
//===----------------------------------------------------------------------===//

#ifndef LLVM_ITERATOR_RECOGNITION_H
#define LLVM_ITERATOR_RECOGNITION_H

namespace llvm {

class FunctionPass;

FunctionPass *createIteratorRecognitionWrapperPass();

} // End llvm namespace

#endif
