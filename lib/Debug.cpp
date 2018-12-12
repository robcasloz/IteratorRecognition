//
//
//

#include "IteratorRecognition/Debug.hpp"

#ifdef BOOST_NO_EXCEPTIONS

namespace boost {

[[noreturn]] inline void throw_exception(std::exception const &e) {
  std::cerr << e.what() << '\n';

  std::terminate();
}

} // namespace boost

#endif // BOOST_NO_EXCEPTIONS

#if ITERATORRECOGNITION_DEBUG

namespace iteratorrecognition {
namespace debug {

bool passDebugFlag = false;
LogLevel passLogLevel = LogLevel::Info;

} // namespace debug
} // namespace iteratorrecognition

#endif // ITERATORRECOGNITION_DEBUG
