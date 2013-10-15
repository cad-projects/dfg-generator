set(MODULE TRUE)
set(LLVM_NO_RTTI 1)

add_subdirectory(KernelAnalysis)
add_subdirectory(TinyXML)
add_subdirectory(Utils)

add_llvm_loadable_module(cad-lib
   RegisterPasses.cpp
)

target_link_libraries(cad-lib
KernelAnalysis
Utils
TinyXML
)

set_target_properties(cad-lib
  PROPERTIES
  LINKER_LANGUAGE CXX
  PREFIX ""
)
