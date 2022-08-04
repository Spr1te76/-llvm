#ifndef _CodeSubstitution_H
#define _CodeSubstitution_H
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
namespace llvm{
FunctionPass* createCodeSubstitution();
}
#endif