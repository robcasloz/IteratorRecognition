//
//
//

#include "IteratorRecognition/Config.hpp"

#ifndef ITR_NODEPREDICATES_HPP
#define ITR_NODEPREDICATES_HPP

namespace iteratorrecognition {

// TODO do we need to restrict the arg type here?
auto is_not_null_unit = [](const auto &e) { return e->unit() != nullptr; };

} // namespace iteratorrecognition

#endif // header
