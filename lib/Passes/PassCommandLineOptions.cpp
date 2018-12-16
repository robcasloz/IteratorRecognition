//
//
//

#include "IteratorRecognition/Config.hpp"

#include "private/PassCommandLineOptions.hpp"

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::desc
// using llvm::cl::location
// using llvm::cl::cat
// using llvm::cl::OptionCategory

llvm::cl::OptionCategory
    IteratorRecognitionCLCategory("Iterator Recognition Pass",
                                  "Options for Iterator Recognition pass");

