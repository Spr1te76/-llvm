#ifndef _DeSymbolization_H
#define _DeSymbolization_H
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
namespace llvm{
ModulePass* createDeSymbolization();
}
#endif