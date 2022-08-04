project(GloryLLVM)
include(AddLLVM)
find_package(LLVM REQUIRED CONFIG)
list(APPEND CMAKE_MODULE_PATH "${LLVM_CMAKE_DIR}")

include_directories("./")

separate_arguments(LLVM_DEFINITIONS_LIST NATIVE_COMMAND ${LLVM_DEFINITIONS})
add_definitions(${LLVM_DEFINITIONS_LIST})
include_directories(${LLVM_INCLUDE_DIRS})


add_llvm_library(GloryLLVMTest MODULE
    Flattening.cpp
    SplitBasicBlock.cpp
    Utils.cpp
    Utils2.cpp
    BogusControlFlowG.cpp
    BogusControlFlow2.cpp
    CodeSubstitution.cpp
    RandomControlFlow.cpp
    ConstantSubstitution.cpp
    AntiDynSymbols.cpp
    DeSymbolization.cpp
    CallObfTools.cpp
    CryptoUtils.cpp
    DEPENDS
    intrinsics_gen
    LINK_COMPONENTS
    Core
    Support
)


