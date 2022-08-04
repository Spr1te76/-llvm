#ifndef __UTILS_H__
#define __UTILS_H__

#include "llvm/IR/Function.h"
#define INIT_CONTEXT(F) CONTEXT=&F.getContext()
#define TYPE_I32 Type::getInt32Ty(F.getContext())
#define TYPE_I32p Type::getInt32PtrTy(F.getContext())
#define TYPE_I8 Type::getInt8Ty(F.getContext())
#define TYPE_I8p Type::getInt8PtrTy(F.getContext())
#define TYPE_I64 Type::getInt64Ty(F.getContext())

#define CONST_I32(V) ConstantInt::get(TYPE_I32, V, false)
#define CONST(T, V) ConstantInt::get(T, V)


namespace llvm{
    void fixStack(Function &F);
    BasicBlock* createCloneBasicBlock(BasicBlock *BB);
}

#endif