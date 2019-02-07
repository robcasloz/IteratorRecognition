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
    FunctionWhiteList("itr-func-wl", llvm::cl::Hidden,
                      llvm::cl::desc("process only the specified functions"),
                      llvm::cl::cat(IteratorRecognitionCLCategory));

llvm::cl::opt<unsigned>
    LoopDepthMin("itr-loop-depth-min", llvm::cl::Hidden, llvm::cl::init(1),
                 llvm::cl::desc("process loops at this depth or higher"),
                 llvm::cl::cat(IteratorRecognitionCLCategory));

llvm::cl::opt<unsigned>
    LoopDepthMax("itr-loop-depth-max", llvm::cl::Hidden, llvm::cl::init(10),
                 llvm::cl::desc("process loops at this depth or lower"),
                 llvm::cl::cat(IteratorRecognitionCLCategory));

