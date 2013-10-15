include_directories(${CADLIB_SOURCE_DIR}/include)

add_llvm_library(KernelAnalysis
  Dfg.cpp
  DfgGeneration.cpp
  DfgPrinting.cpp
  DetermineBitWidth.cpp
  )
