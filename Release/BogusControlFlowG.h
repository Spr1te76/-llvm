#ifndef _BogusControlFlow_H
#define _BogusControlFlow_H
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
namespace llvm{
FunctionPass* createBogusControlFlowG();
}
#endif