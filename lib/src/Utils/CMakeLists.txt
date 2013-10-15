include_directories(${CADLIB_SOURCE_DIR}/include)

add_llvm_library(Utils
   Config.cpp
   Support.cpp
  )

