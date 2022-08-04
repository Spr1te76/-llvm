#ifndef _RandomControlFlow_H
#define _RandomControlFlow_H
#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
namespace llvm{
FunctionPass* createRandomControlFlow();
}
#endif