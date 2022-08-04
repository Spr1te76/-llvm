#ifndef _AntiDynSymbols_H
#define _AntiDynSymbols_H
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
namespace llvm{
FunctionPass* createAntiDynSymbols();
}
#endif