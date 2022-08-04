#ifndef _SplitBB_H
#define _SplitBB_H
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
namespace llvm{
FunctionPass* createSplitBasicBlockPass();
}
#endif