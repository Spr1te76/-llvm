#ifndef _ConstantSubstitution_H
#define _ConstantSubstitution_H
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
namespace llvm{
FunctionPass* createConstantSubstitution();
}
#endif