//
//
//

#include "llvm/Support/CommandLine.h"
// using llvm::cl::OptionCategory

#ifndef ITR_PASSCOMMANDLINEOPTIONS_HPP
#define ITR_PASSCOMMANDLINEOPTIONS_HPP

extern llvm::cl::OptionCategory IteratorRecognitionCLCategory;

extern llvm::cl::opt<std::string> ReportsDir;

extern llvm::cl::list<std::string> FunctionWhiteList;

extern llvm::cl::opt<unsigned> LoopDepthMin;

extern llvm::cl::opt<unsigned> LoopDepthMax;

#endif // header
