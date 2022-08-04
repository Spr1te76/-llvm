#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/IR/LegacyPassManager.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/Local.h"
#include "./SplitBasicBlock.h"
#include "./Utils.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include"Flattening.h"
using std::vector;
using namespace llvm;

namespace
{
    class Flattening:public FunctionPass{
        public:
            static char ID;
            Flattening():FunctionPass(ID){srand(time(0));}
            bool runOnFunction(Function&F);
            void Flatten(Function&F);
    };
}

bool Flattening::runOnFunction(Function&F)
{
    FunctionPass *split=createSplitBasicBlockPass();
    split->runOnFunction(F);
    Flatten(F);
    return true;
}



void Flattening::Flatten(Function&F)
{
    if(F.size() <= 1){
        return;
    }
    vector<BasicBlock*> originBlock;
    for(BasicBlock &BB:F)
    {
        originBlock.push_back(&BB);
    }
    originBlock.erase(originBlock.begin());
    BasicBlock &entryBB = F.getEntryBlock();
    if(BranchInst *br = dyn_cast<BranchInst>(entryBB.getTerminator()))
    {
        if(br->isConditional())
        {
            BasicBlock *newBB=entryBB.splitBasicBlock(br,"newBB");
            originBlock.insert(originBlock.begin(),newBB);
        }
    }
    BasicBlock * DisPatchBlock=BasicBlock::Create(F.getContext(),"DisPatchBlock",&F,&entryBB);
    BasicBlock * returnBB=BasicBlock::Create(F.getContext(),"returnBB",&F,&entryBB);
    BranchInst::Create(DisPatchBlock,returnBB);
    entryBB.moveBefore(DisPatchBlock);
    entryBB.getTerminator()->eraseFromParent();
    BranchInst *brInstEntry=BranchInst::Create(DisPatchBlock,&entryBB);
    int randNumCase=rand();
    AllocaInst *allocInst=new AllocaInst(Type::getInt32Ty(F.getContext()),0,"allocInst",brInstEntry);
    new StoreInst(CONST_I32(randNumCase),allocInst,brInstEntry);
    LoadInst* loadrandcase=new LoadInst(TYPE_I32,allocInst,"LoadRand",false,DisPatchBlock);
    BasicBlock* switchdf=BasicBlock::Create(F.getContext(),"swtichdw",&F,returnBB);
    BranchInst::Create(returnBB,switchdf);
    SwitchInst * swInst=SwitchInst::Create(loadrandcase,switchdf,0,DisPatchBlock);
    for(BasicBlock *BB : originBlock){
        BB->moveBefore(returnBB);
        swInst->addCase(CONST_I32(randNumCase), BB);
        randNumCase = rand();
    }
    for(BasicBlock* BB:originBlock)
    {
        if(BB->getTerminator()->getNumSuccessors()==0)
            continue;
        else if(BB->getTerminator()->getNumSuccessors()==1)
        {
            BasicBlock *sucBlock=BB->getTerminator()->getSuccessor(0);
            BB->getTerminator()->eraseFromParent();
            ConstantInt*numCase=swInst->findCaseDest(sucBlock);
            new StoreInst(numCase,allocInst,BB);
            BranchInst::Create(returnBB,BB);
        }
        else if(BB->getTerminator()->getNumSuccessors()==2)
        {
            ConstantInt *CaseTRUE=swInst->findCaseDest(BB->getTerminator()->getSuccessor(0));
            ConstantInt *CaseFALSE=swInst->findCaseDest(BB->getTerminator()->getSuccessor(1));
            BranchInst *br= cast<BranchInst>(BB->getTerminator());
            SelectInst *sel=SelectInst::Create(br->getCondition(),CaseTRUE,CaseFALSE,"",BB->getTerminator());
            BB->getTerminator()->eraseFromParent();
            new StoreInst(sel,allocInst,BB);
            BranchInst::Create(returnBB,BB);
        }
    }
    fixStack(F);
}

FunctionPass* llvm::createFlattening(){
    return new Flattening();
}


char Flattening::ID = 0;
static RegisterPass<Flattening> X("flat", "The control stream flattening.");