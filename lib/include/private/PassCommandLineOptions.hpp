//
//
//

#include "llvm/Support/CommandLine.h"
// using llvm::cl::OptionCategory

#ifndef ITR_PASSCOMMANDLINEOPTIONS_HPP
#define ITR_PASSCOMMANDLINEOPTIONS_HPP

extern llvm::cl::OptionCategory IteratorRecognitionCLCategory;

extern llvm::cl::list<std::string> FunctionWhitelist;

#endif // header
