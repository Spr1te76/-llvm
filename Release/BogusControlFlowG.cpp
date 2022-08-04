//===----------------------------------------------------------------------===//
//===- BogusControlFlowG.cpp - BogusControlFlowG Obfuscation
// pass-------------------------===//
//
// This file implements BogusControlFlowG's pass, inserting bogus control flow.
// It adds bogus flow to a given basic block this way:
//
// Before :
// 	         		     entry
//      			       |
//  	    	  	 ______v______
//   	    		|   Original  |
//   	    		|_____________|
//             		       |
// 		        	       v
//		        	     return
//
// After :
//           		     entry
//             		       |
//            		   ____v_____
//      			  |condition*| (false)
//           		  |__________|----+
//           		 (true)|          |
//             		       |          |
//           		 ______v______    |
// 		        +-->|   Original* |   |
// 		        |   |_____________| (true)
// 		        |   (false)|    !-----------> return
// 		        |    ______v______    |
// 		        |   |   Altered   |<--!
// 		        |   |_____________|
// 		        |__________|
//
//  * The results of these terminator's branch's conditions are always true, but
//  these predicates are
//    opacificated. For this, we declare two global values: x and y, and replace
//    the FCMP_TRUE predicate with (y < 10 || x * (x + 1) % 2 == 0) (this could
//    be improved, as the global values give a hint on where are the opaque
//    predicates)
//
//  The altered bloc is a copy of the original's one with junk instructions
//  added accordingly to the type of instructions we found in the bloc
//
//  The pass can also be loop many times on a function, including on the basic
//  blocks added in a previous loop. Be careful if you use a big probability
//  number and choose to run the loop many times wich may cause the pass to run
//  for a very long time. The default value is one loop, but you can change it
//  with -bcf_loop=[value]. Value must be an integer greater than 1,
//  otherwise the default value is taken. Exemple: -bcf_loop 2
//
//===----------------------------------------------------------------------------------===//


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
#include"BogusControlFlowG.h"
#include <ctime>
using std::vector;
using namespace llvm;
static cl::opt<int> obfuTimes("Gbcf_loop", cl::init(1), cl::desc("Obfuscate a function <bcf_loop> time(s)."));
#define MAX_BOGUS_CMP_CHOICE 3
namespace{
    class BogusControlFlowG : public FunctionPass {
        public:
            static char ID;
            BogusControlFlowG() : FunctionPass(ID) {}
            bool runOnFunction(Function &F);
            void bogus(BasicBlock *BB,Function&F);
            Value* createBogusCmp(BasicBlock *insertAfter,Function&F);
            Value* BogusCmpone(BasicBlock*inserAfter,Function&F); //y<0||(x+1)x%2==0
            Value* BogusCmpone1(BasicBlock*inserAfter,Function&F);//`1
            Value* BogusCmpone2(BasicBlock*inserAfter,Function&F);//`2

           


    };
}

bool BogusControlFlowG::runOnFunction(Function &F)
{
    FunctionPass* split=createSplitBasicBlockPass();
    split->runOnFunction(F);

    for(int i=0;i<obfuTimes;i++)
    {
        vector<BasicBlock*>originBlock;
        for(BasicBlock&BB:F)
        {
            originBlock.push_back(&BB);
        }
        for(BasicBlock*BB:originBlock)
        {
            bogus(BB,F);
        }
    }
    return true;
}

Value* BogusControlFlowG::createBogusCmp(BasicBlock *insertAfter,Function&F)
{
    Value* resul=NULL;
    switch (rand()%MAX_BOGUS_CMP_CHOICE)
    {
    case 0:
        resul=BogusCmpone(insertAfter,F);
        break;
    case 1:
        resul=BogusCmpone1(insertAfter,F);
        break;
    case 2:
        resul=BogusCmpone2(insertAfter,F);
        break;
    default:
        break;
    }

    return resul;

}
Value* BogusControlFlowG::BogusCmpone(BasicBlock*insertAfter,Function&F)
{
    Module *M=insertAfter->getModule();
    GlobalVariable *xptr=new GlobalVariable(*M,TYPE_I32,false,GlobalValue::CommonLinkage,CONST_I32(0),"x");
    GlobalVariable *yptr=new GlobalVariable(*M,TYPE_I32,false,GlobalValue::CommonLinkage,CONST_I32(0),"y");//has been alloc is ok
    LoadInst *x=new LoadInst(TYPE_I32,xptr,"",insertAfter);
    LoadInst *y=new LoadInst(TYPE_I32,yptr,"",insertAfter);
    ICmpInst *cond1=new ICmpInst(*insertAfter,CmpInst::ICMP_SLT,y,CONST_I32(10));
    BinaryOperator *op1=BinaryOperator::CreateAdd(x,CONST_I32(1),"",insertAfter);
    BinaryOperator *op2=BinaryOperator::CreateMul(op1,x,"",insertAfter);
    BinaryOperator *op3=BinaryOperator::CreateURem(op2,CONST_I32(2),"",insertAfter);
    ICmpInst *cond2=new ICmpInst(*insertAfter,CmpInst::ICMP_EQ,op3,CONST_I32(0));
    BinaryOperator* resul=BinaryOperator::CreateOr(cond1,cond2,"",insertAfter);
    return resul;
}
//(x|y)+(x^y)-x-y==0
Value* BogusControlFlowG::BogusCmpone1(BasicBlock*insertAfter,Function&F)
{
    Module *M=insertAfter->getModule();
    GlobalVariable *xptr=new GlobalVariable(*M,TYPE_I32,false,GlobalValue::CommonLinkage,CONST_I32(0),"x");
    GlobalVariable *yptr=new GlobalVariable(*M,TYPE_I32,false,GlobalValue::CommonLinkage,CONST_I32(0),"y");
    LoadInst *x=new LoadInst(TYPE_I32,xptr,"",insertAfter);
    LoadInst *y=new LoadInst(TYPE_I32,yptr,"",insertAfter);
    BinaryOperator *op1=BinaryOperator::CreateXor(x,y,"",insertAfter);
    BinaryOperator *op2=BinaryOperator::CreateAnd(x,y,"",insertAfter);
    BinaryOperator *op3=BinaryOperator::CreateAdd(x,y,"",insertAfter);
    BinaryOperator *op4=BinaryOperator::CreateAdd(op1,op2,"",insertAfter);
    BinaryOperator *op5=BinaryOperator::CreateSub(op4,op3,"",insertAfter);
    BinaryOperator*resul=op5;
    return resul;




}
//(x^y)^(y^z)^(x^z)+x(x+y+z)+y(x+y+z)+z(x+y+z)-(x+y+z)*(x+y+z)+x(y+z)+y(x+z)+z(x+y)-2(xy+yz+xz)==0
Value* BogusControlFlowG::BogusCmpone2(BasicBlock*insertAfter,Function&F)
{
    Module *M=insertAfter->getModule();
    GlobalVariable *xptr=new GlobalVariable(*M,TYPE_I32,false,GlobalValue::CommonLinkage,CONST_I32(0),"x");
    GlobalVariable *yptr=new GlobalVariable(*M,TYPE_I32,false,GlobalValue::CommonLinkage,CONST_I32(0),"y");
    GlobalVariable *zptr=new GlobalVariable(*M,TYPE_I32,false,GlobalValue::CommonLinkage,CONST_I32(0),"z");
    LoadInst *x=new LoadInst(TYPE_I32,xptr,"",insertAfter);
    LoadInst *y=new LoadInst(TYPE_I32,yptr,"",insertAfter);
    LoadInst *z=new LoadInst(TYPE_I32,zptr,"",insertAfter); 
    BinaryOperator *op1=BinaryOperator::CreateXor(x,y,"",insertAfter);
    BinaryOperator *op2=BinaryOperator::CreateXor(y,z,"",insertAfter);
    BinaryOperator *op3=BinaryOperator::CreateXor(x,z,"",insertAfter);
    BinaryOperator *op4=BinaryOperator::CreateXor(op1,op2,"",insertAfter);
    BinaryOperator *op5=BinaryOperator::CreateXor(op4,op3,"",insertAfter);
    BinaryOperator *op6=BinaryOperator::CreateAdd(x,y,"",insertAfter);
    BinaryOperator *op7=BinaryOperator::CreateAdd(op6,z,"",insertAfter);
    BinaryOperator *op8=BinaryOperator::CreateMul(x,op7,"",insertAfter);
    BinaryOperator *op9=BinaryOperator::CreateMul(y,op7,"",insertAfter);
    BinaryOperator *op10=BinaryOperator::CreateMul(z,op7,"",insertAfter);
    BinaryOperator *op11=BinaryOperator::CreateMul(op7,op7,"",insertAfter);
    BinaryOperator *op12=BinaryOperator::CreateAdd(op8,op9,"",insertAfter);
    BinaryOperator *op13=BinaryOperator::CreateAdd(op12,op10,"",insertAfter);
    BinaryOperator *op14=BinaryOperator::CreateSub(op13,op11,"",insertAfter);
    BinaryOperator *op15=NULL;
    BinaryOperator *op16=BinaryOperator::CreateAdd(y,z,"",insertAfter);
    BinaryOperator *op17=BinaryOperator::CreateAdd(x,z,"",insertAfter);
    BinaryOperator *op18=BinaryOperator::CreateAdd(x,y,"",insertAfter);
    BinaryOperator *op19=BinaryOperator::CreateMul(x,op16,"",insertAfter);
    BinaryOperator *op20=BinaryOperator::CreateMul(y,op17,"",insertAfter);
    BinaryOperator *op21=BinaryOperator::CreateMul(z,op18,"",insertAfter);
    BinaryOperator *op22=BinaryOperator::CreateAdd(op19,op20,"",insertAfter);
    BinaryOperator *op23=BinaryOperator::CreateAdd(op22,op21,"",insertAfter);
    BinaryOperator *op24=BinaryOperator::CreateMul(x,y,"",insertAfter);
    BinaryOperator *op25=BinaryOperator::CreateMul(y,z,"",insertAfter);
    BinaryOperator *op26=BinaryOperator::CreateMul(x,z,"",insertAfter);
    BinaryOperator *op27=BinaryOperator::CreateAdd(op24,op25,"",insertAfter);
    BinaryOperator *op28=BinaryOperator::CreateAdd(op27,op26,"",insertAfter);
    BinaryOperator *op29=BinaryOperator::CreateAdd(op14,op23,"",insertAfter);
    BinaryOperator *op30=BinaryOperator::CreateAdd(op28,op28,"",insertAfter);
    BinaryOperator *op31=BinaryOperator::CreateSub(op29,op30,"",insertAfter);
    BinaryOperator*resul=op31;
    return resul;




}
//
void BogusControlFlowG::bogus(BasicBlock*entryBB,Function&F)
{

    BasicBlock* bodyBB=entryBB->splitBasicBlock(entryBB->getFirstNonPHIOrDbgOrLifetime(),"bodyBB");
    BasicBlock* endBB=bodyBB->splitBasicBlock(bodyBB->getTerminator(),"endBB");
    BasicBlock*cloneBB=createCloneBasicBlock(bodyBB);
    entryBB->getTerminator()->eraseFromParent();
    bodyBB->getTerminator()->eraseFromParent();
    cloneBB->getTerminator()->eraseFromParent();

    Value*cond1=createBogusCmp(entryBB,F);
    Value*cond2=createBogusCmp(bodyBB,F);
    BranchInst::Create(bodyBB,cloneBB,cond1,entryBB);
    BranchInst::Create(endBB,cloneBB,cond2,bodyBB);
    BranchInst::Create(bodyBB,cloneBB);

}

FunctionPass* llvm::createBogusControlFlowG(){
    return new BogusControlFlowG();
}

char BogusControlFlowG::ID = 0;
static RegisterPass<BogusControlFlowG> X("bcf", "Add bogus control flow to each function.");

