# IteratorRecognition
add_llvm_library(LLVMIteratorRecognition
  Analysis/Graphs/PDGCondensationGraph.cpp
  Analysis/IteratorRecognition.cpp
  Analysis/Passes/ValueClassificationPass.cpp
  Analysis/Passes/PayloadDependenceGraphPass.cpp
  Analysis/Passes/IteratorRecognitionPass.cpp
  Analysis/ValueClassification.cpp
  Support/Utils/DebugInfo.cpp
  Support/Utils/StringConversion.cpp
  Support/FileSystem.cpp
  Passes/RegisterPasses.cpp
  Passes/PassCommandLineOptions.cpp
  Debug.cpp
  Exchange/JSONTransfer.cpp
  Exchange/Passes/JSONExporterPass.cpp
  Transforms/Utils/DemotePayloadPhis.cpp
  Transforms/Passes/AnnotatorPass.cpp

  ADDITIONAL_HEADER_DIRS
  ${LLVM_MAIN_INCLUDE_DIR}/llvm/Analysis

  DEPENDS
  intrinsics_gen
  )

include_directories(${LLVM_MAIN_SRC_DIR}/lib/IteratorRecognition/include)
