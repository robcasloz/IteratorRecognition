//
//
//

#include "IteratorRecognition/Config.hpp"

#include "private/PassCommandLineOptions.hpp"

#include "llvm/Support/CommandLine.h"
// using llvm::cl::opt
// using llvm::cl::list
// using llvm::cl::desc
// using llvm::cl::location
// using llvm::cl::cat
// using llvm::cl::OptionCategory

#include <string>
// using std::string

llvm::cl::OptionCategory
    IteratorRecognitionCLCategory("Iterator Recognition Pass",
                                  "Options for Iterator Recognition pass");

llvm::cl::list<std::string>
    FunctionWhitelist("itr-function-wl", llvm::cl::Hidden,
                      llvm::cl::desc("process only the specified functions"));

