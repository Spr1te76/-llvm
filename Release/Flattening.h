#ifndef _Flattening_H
#define _Flattening_H
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
namespace llvm{
FunctionPass* createFlattening();
}
#endif