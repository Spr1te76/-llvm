#include"llvm/Pass.h"
#include"llvm/IR/Function.h"
#include"llvm/Support/raw_ostream.h"
#include<vector>
#include"llvm/IR/Instructions.h"
#include"llvm/Support/CommandLine.h"
#include"./SplitBasicBlock.h"
#include "llvm/IR/Dominators.h"
using std::vector;
using namespace llvm;


static cl::opt<int> splitNum("split_num", cl::init(2), cl::desc("Split<split_num> time(s) each BB"));

namespace{
    class SplitBasicBlock:public FunctionPass{
        public:
            static char ID;
            SplitBasicBlock():FunctionPass(ID){}//splitBasicBlock将该指令以上的切到外面，不包括该指令
            bool runOnFunction(Function&F);
            bool containPHI(BasicBlock *BB);
            void split(BasicBlock*BB);
    };
}

bool SplitBasicBlock::runOnFunction(Function &F){
    vector<BasicBlock*> origBB;
    for(BasicBlock &BB : F){
        origBB.push_back(&BB);
    }
    for(BasicBlock *BB : origBB){
    if(!containPHI(BB)){
        split(BB);
    }
    }
    return true;
}
bool SplitBasicBlock::containPHI(BasicBlock *BB){
    for(Instruction &I : *BB)
    {
        if(isa<PHINode>(&I))
        {
        return true;
        }
    }
    return false;
}
void SplitBasicBlock::split(BasicBlock *BB){
    int splitSize = (BB->size() + splitNum - 1) / splitNum;
    BasicBlock *curBB = BB;
    for(int i = 1;i < splitNum;i ++){
        int cnt = 0;
        for(Instruction &I : *curBB){
            if(cnt++ == splitSize){
                curBB = curBB->splitBasicBlock(&I);
                break;
            }
        }
    }
}
char SplitBasicBlock::ID = 0;

FunctionPass* llvm::createSplitBasicBlockPass(){
    return new SplitBasicBlock();
}

static RegisterPass<SplitBasicBlock> X("split", "Split a basic block intomultiple basic blocks.");