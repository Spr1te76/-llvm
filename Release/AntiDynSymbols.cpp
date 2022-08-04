#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "SplitBasicBlock.h"
#include "Utils.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include"llvm/Support/Alignment.h"
#include"AntiDynSymbols.h"
using namespace llvm;
using std::vector;
static cl::opt<int> Coverage("Coverage", cl::init(50), cl::desc("The confusion ratio is <Coverage>%."));
static cl::opt<int> CoverTimes("CoverTimes", cl::init(1), cl::desc("Obfuscate a function <CoverTimes> time(s)."));


namespace{
    class AntiDynSymbols : public FunctionPass {
        public:
            static char ID;
            AntiDynSymbols() : FunctionPass(ID) {srand(time(0));srand(rand());}
            bool runOnFunction(Function &F);
            BasicBlock* createLoop(BasicBlock *insertAfter,Function&F);
            void LoopObf1(Function&F);
    };
}
bool AntiDynSymbols::runOnFunction(Function&F)
{
    if(Coverage>100)
    {
        errs()<<"Coverage must be less than 100%.\n";
        return false;
    }
    if(CoverTimes>10)
    {
       errs()<<"CoverTimes must be less than 10.\n";
       return false;
    }
    for(int i=0;i<CoverTimes;i++)
    {
        int isCover=rand()%100;
        if(isCover>0&&isCover<=Coverage)
        {
            LoopObf1(F);
        }
    }
    return true;
}
void AntiDynSymbols::LoopObf1(Function&F)
{
    errs()<<"Anti-symbol execution\n";
    BasicBlock& entryBB=F.getEntryBlock();
    BasicBlock*entryBBTWO=entryBB.splitBasicBlock(entryBB.getTerminator());
    entryBB.getTerminator()->eraseFromParent();
 
    BasicBlock* LoopBody=BasicBlock::Create(F.getContext(),"LoopBody",&F,entryBBTWO);
    BasicBlock*LoopCond1=BasicBlock::Create(F.getContext(),"LoopBlock",&F,entryBBTWO);
    BasicBlock*LoopCal1=BasicBlock::Create(F.getContext(),"LoopCal1",&F,entryBBTWO);
    BasicBlock*LoopCal2=BasicBlock::Create(F.getContext(),"LoopCal2",&F,entryBBTWO);
    BasicBlock* LoopEnd1=BasicBlock::Create(F.getContext(),"LoopEnd1",&F,entryBBTWO);
    BranchInst* br=BranchInst::Create(LoopBody,&entryBB);

    AllocaInst* xptr=new AllocaInst(Type::getInt32Ty(F.getContext()),0,"allocInst",br);
    StoreInst* xstore=new StoreInst(CONST_I32(rand()%0xfffc),xptr,br);

    LoadInst* x=new LoadInst(TYPE_I32,xptr,"LoadRand1",false,LoopBody);
    ICmpInst* cond=new ICmpInst(*LoopBody,CmpInst::ICMP_SGT,x,CONST_I32(1));
    BranchInst::Create(LoopCond1,&F.back(),cond,LoopBody);

    LoadInst* y=new LoadInst(TYPE_I32,xptr,"LoadRand2",false,LoopCond1);
    BinaryOperator *Sremt=BinaryOperator::CreateSRem(y,CONST_I32(2),"",LoopCond1);
    ICmpInst* cond1=new ICmpInst(*LoopCond1,CmpInst::ICMP_NE,Sremt,CONST_I32(0));

    BranchInst::Create(LoopCal1,LoopCal2,cond1,LoopCond1);

    LoadInst* z=new LoadInst(TYPE_I32,xptr,"LoadRand3",false,LoopCal1);
    BinaryOperator *op2=BinaryOperator::CreateMul(z,CONST_I32(3),"",LoopCal1);
    BinaryOperator*op3=BinaryOperator::CreateAdd(op2,CONST_I32(1),"",LoopCal1);
    new StoreInst(op3,xptr,LoopCal1);
    BranchInst::Create(LoopEnd1,LoopCal1);

    LoadInst* p=new LoadInst(TYPE_I32,xptr,"LoadRand4",false,LoopCal2);
    BinaryOperator *op4=BinaryOperator::CreateSDiv(p,CONST_I32(2),"",LoopCal2);
    new StoreInst(op4,xptr,LoopCal2);
    BranchInst::Create(LoopEnd1,LoopCal2);

    LoadInst* ot=new LoadInst(TYPE_I32,xptr,"LoadRand5",false,LoopEnd1);
    ICmpInst* cond2=new ICmpInst(*LoopEnd1,CmpInst::ICMP_EQ,ot,CONST_I32(1));
    BranchInst::Create(entryBBTWO,LoopBody,cond2,LoopEnd1);

    fixStack(F);
}

FunctionPass* llvm::createAntiDynSymbols(){
    return new AntiDynSymbols();
}


char AntiDynSymbols::ID = 0;
static RegisterPass<AntiDynSymbols> X("AntiSym", "Anti-symbol execution.");