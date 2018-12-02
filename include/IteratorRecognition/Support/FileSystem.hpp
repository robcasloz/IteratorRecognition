//
//
//

#include "llvm/Support/ErrorOr.h"
// using llvm::ErrorOr

#include <string>
// using std::string

#ifndef ITR_FILESYSTEM_HPP
#define ITR_FILESYSTEM_HPP

namespace llvm {
class Twine;
} // namespace llvm

namespace iteratorrecognition {

llvm::ErrorOr<std::string> CreateDirectory(const llvm::Twine &Path);

} // namespace iteratorrecognition

#endif // header
