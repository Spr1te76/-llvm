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
#include"llvm/IR/Type.h"
#include"DeSymbolization.h"
using namespace llvm;
using std::vector;
static cl::opt<int> density("exe_den", cl::init(100), cl::desc("Replacement density for performing obfuscation."));

namespace{
    class DeSymbolization : public ModulePass {
        public:
            static char ID;
            DeSymbolization() : ModulePass(ID) {srand(time(0));srand(rand());}
            bool runOnModule(Module &M);

    };
}

bool DeSymbolization::runOnModule(Module&M)
{
    if(density<=0||density>100)
    {
        errs()<<"The density of confusion replacement cannot be less than or equal to 0.";
        return false;
    }
    //Create Global Array
    ArrayType *ty=ArrayType::get(Type::getInt32Ty(M.getContext()),256);
    GlobalVariable *gv=new llvm::GlobalVariable(M,ty,true,llvm::GlobalValue::InternalLinkage ,0,"abcdefgtable");
    gv->setAlignment(MaybeAlign(16));
    vector<Constant*> table;
    for(int i=0;i<256;i++)
    {
        table.push_back(ConstantInt::get(llvm::Type::getInt32Ty(M.getContext()), i));
    }    
    Constant* const_array = ConstantArray::get(ty, table);
    gv->setInitializer(const_array);

    vector<StoreInst*>originStore8(0,0);
    vector<StoreInst*>originStore32(0,0);
    vector<StoreInst*>originStore64(0,0);

    int flag=0;
    for(Function&F:M)
    {
        for(BasicBlock&BB:F)
        {
            for(Instruction&I:BB)
            {
                if(isa<StoreInst>(I))
                {
                    if(rand()%100<density)
                    {
                        StoreInst *BI=cast<StoreInst>(&I);
                        if(BI->getOperand(0)->getType()->isIntegerTy()&&BI->getOperand(0)->getType()->getIntegerBitWidth()==32)
                        {
                            errs()<<"TYPE_INT_32\n";
                            originStore32.push_back(BI);
                        }
                        if(BI->getOperand(0)->getType()->isIntegerTy()&&BI->getOperand(0)->getType()->getIntegerBitWidth()==8)
                        {
                            errs()<<"TYPE_INT_8\n";
                            originStore8.push_back(BI);
                        }
                        if(BI->getOperand(0)->getType()->isIntegerTy()&&BI->getOperand(0)->getType()->getIntegerBitWidth()==64)
                        {
                            errs()<<"TYPE_INT_64\n";
                            originStore64.push_back(BI);
                        }
                    }

                }

            }
        }
        
        for(StoreInst*BI:originStore32)
        {
            BasicBlock* BB=BI->getParent();
            AllocaInst* Alpointer=new AllocaInst(TYPE_I8p,0,"pointer",BI);
            BI->moveBefore(Alpointer);
            BasicBlock* body=BB->splitBasicBlock(Alpointer);
                    
            BitCastInst* pBi=new BitCastInst(BI->getOperand(1),TYPE_I8p,"",Alpointer);
            pBi->moveAfter(Alpointer);
            BasicBlock*end=body->splitBasicBlock(pBi->getNextNode());
            new StoreInst(pBi,Alpointer,body->getTerminator());

            int firstflag=-3;
            for(int i=0;i<4;i++)
            {
                if(firstflag)
                {
                    LoadInst* data1=new LoadInst(TYPE_I8p,Alpointer,"data1",false,body->getTerminator());
                    LoadInst* context1=new LoadInst(TYPE_I8,data1,"context1",false,body->getTerminator());
                    ZExtInst* zext1=new ZExtInst(context1,TYPE_I32,"zext1",body->getTerminator());
                    vector<llvm::Value*> vect_1; 
                    vect_1.push_back(CONST_I32(0));
                    vect_1.push_back(zext1);
                    GetElementPtrInst* getcontext1=GetElementPtrInst::CreateInBounds(ArrayType::get(Type::getInt32Ty(M.getContext()),256),gv,ArrayRef<llvm::Value*>(vect_1),"getcontext1",body->getTerminator());
                    LoadInst* context2=new LoadInst(TYPE_I32,getcontext1,"context2",body->getTerminator());
                    TruncInst* Trunc1=new TruncInst(context2,TYPE_I8,"Trunc1",body->getTerminator());
                    LoadInst* context3=new LoadInst(TYPE_I8p,Alpointer,"context1",false,body->getTerminator());
                    new StoreInst(Trunc1,context3,body->getTerminator());
                    LoadInst* context4=new LoadInst(TYPE_I8p,Alpointer,"context3",false,body->getTerminator());
                    GetElementPtrInst* getcontext2=GetElementPtrInst::CreateInBounds(TYPE_I8,context4,CONST_I32(1),"getcontext2",body->getTerminator());
                    new StoreInst(getcontext2,Alpointer,body->getTerminator());
                    firstflag++;
                }
                else
                {
                    LoadInst* context1=new LoadInst(TYPE_I8p,Alpointer,"context3",false,body->getTerminator());
                    LoadInst* context2=new LoadInst(TYPE_I8,context1,"context1",false,body->getTerminator());
                    ZExtInst* zext1=new ZExtInst(context2,TYPE_I32,"zext1",body->getTerminator());
                    vector<llvm::Value*> vect_1; 
                    vect_1.push_back(CONST_I32(0));
                    vect_1.push_back(zext1);
                    GetElementPtrInst* getcontext1=GetElementPtrInst::CreateInBounds(ArrayType::get(Type::getInt32Ty(M.getContext()),256),gv,ArrayRef<llvm::Value*>(vect_1),"getcontext1",body->getTerminator());
                    LoadInst* context3=new LoadInst(TYPE_I32,getcontext1,"context2",body->getTerminator());
                    TruncInst* Trunc1=new TruncInst(context3,TYPE_I8,"Trunc1",body->getTerminator());
                    LoadInst* context4=new LoadInst(TYPE_I8p,Alpointer,"context1",false,body->getTerminator());
                    new StoreInst(Trunc1,context4,body->getTerminator());
                }
            }
        }
        for(StoreInst*BI:originStore64)
        {
            BasicBlock* BB=BI->getParent();
            AllocaInst* Alpointer=new AllocaInst(TYPE_I8p,0,"pointer",BI);
            BI->moveBefore(Alpointer);
            BasicBlock* body=BB->splitBasicBlock(Alpointer);
                    
            BitCastInst* pBi=new BitCastInst(BI->getOperand(1),TYPE_I8p,"",Alpointer);
            pBi->moveAfter(Alpointer);
            BasicBlock*end=body->splitBasicBlock(pBi->getNextNode());
            new StoreInst(pBi,Alpointer,body->getTerminator());

            int firstflag=-7;
            for(int i=0;i<8;i++)
            {
                if(firstflag)
                {
                    LoadInst* data1=new LoadInst(TYPE_I8p,Alpointer,"data1",false,body->getTerminator());
                    LoadInst* context1=new LoadInst(TYPE_I8,data1,"context1",false,body->getTerminator());
                    ZExtInst* zext1=new ZExtInst(context1,TYPE_I32,"zext1",body->getTerminator());
                    vector<llvm::Value*> vect_1; 
                    vect_1.push_back(CONST_I32(0));
                    vect_1.push_back(zext1);
                    GetElementPtrInst* getcontext1=GetElementPtrInst::CreateInBounds(ArrayType::get(Type::getInt32Ty(M.getContext()),256),gv,ArrayRef<llvm::Value*>(vect_1),"getcontext1",body->getTerminator());
                    LoadInst* context2=new LoadInst(TYPE_I32,getcontext1,"context2",body->getTerminator());
                    TruncInst* Trunc1=new TruncInst(context2,TYPE_I8,"Trunc1",body->getTerminator());
                    LoadInst* context3=new LoadInst(TYPE_I8p,Alpointer,"context1",false,body->getTerminator());
                    new StoreInst(Trunc1,context3,body->getTerminator());
                    LoadInst* context4=new LoadInst(TYPE_I8p,Alpointer,"context3",false,body->getTerminator());
                    GetElementPtrInst* getcontext2=GetElementPtrInst::CreateInBounds(TYPE_I8,context4,CONST_I32(1),"getcontext2",body->getTerminator());
                    new StoreInst(getcontext2,Alpointer,body->getTerminator());
                    firstflag++;
                }
                else
                {
                    LoadInst* context1=new LoadInst(TYPE_I8p,Alpointer,"context3",false,body->getTerminator());
                    LoadInst* context2=new LoadInst(TYPE_I8,context1,"context1",false,body->getTerminator());
                    ZExtInst* zext1=new ZExtInst(context2,TYPE_I32,"zext1",body->getTerminator());
                    vector<llvm::Value*> vect_1; 
                    vect_1.push_back(CONST_I32(0));
                    vect_1.push_back(zext1);
                    GetElementPtrInst* getcontext1=GetElementPtrInst::CreateInBounds(ArrayType::get(Type::getInt32Ty(M.getContext()),256),gv,ArrayRef<llvm::Value*>(vect_1),"getcontext1",body->getTerminator());
                    LoadInst* context3=new LoadInst(TYPE_I32,getcontext1,"context2",body->getTerminator());
                    TruncInst* Trunc1=new TruncInst(context3,TYPE_I8,"Trunc1",body->getTerminator());
                    LoadInst* context4=new LoadInst(TYPE_I8p,Alpointer,"context1",false,body->getTerminator());
                    new StoreInst(Trunc1,context4,body->getTerminator());
                }
            }
        }
        for(StoreInst*BI:originStore8)
        {
            BasicBlock* BB=BI->getParent();
            AllocaInst* Alpointer=new AllocaInst(TYPE_I8p,0,"pointer",BI);
            BI->moveBefore(Alpointer);
            BasicBlock* body=BB->splitBasicBlock(Alpointer);
                    
            BitCastInst* pBi=new BitCastInst(BI->getOperand(1),TYPE_I8p,"",Alpointer);
            pBi->moveAfter(Alpointer);
            BasicBlock*end=body->splitBasicBlock(pBi->getNextNode());
            new StoreInst(pBi,Alpointer,body->getTerminator());

            int firstflag=0;
            for(int i=0;i<1;i++)
            {
                if(firstflag)
                {
                    LoadInst* data1=new LoadInst(TYPE_I8p,Alpointer,"data1",false,body->getTerminator());
                    LoadInst* context1=new LoadInst(TYPE_I8,data1,"context1",false,body->getTerminator());
                    ZExtInst* zext1=new ZExtInst(context1,TYPE_I32,"zext1",body->getTerminator());
                    vector<llvm::Value*> vect_1; 
                    vect_1.push_back(CONST_I32(0));
                    vect_1.push_back(zext1);
                    GetElementPtrInst* getcontext1=GetElementPtrInst::CreateInBounds(ArrayType::get(Type::getInt32Ty(M.getContext()),256),gv,ArrayRef<llvm::Value*>(vect_1),"getcontext1",body->getTerminator());
                    LoadInst* context2=new LoadInst(TYPE_I32,getcontext1,"context2",body->getTerminator());
                    TruncInst* Trunc1=new TruncInst(context2,TYPE_I8,"Trunc1",body->getTerminator());
                    LoadInst* context3=new LoadInst(TYPE_I8p,Alpointer,"context1",false,body->getTerminator());
                    new StoreInst(Trunc1,context3,body->getTerminator());
                    LoadInst* context4=new LoadInst(TYPE_I8p,Alpointer,"context3",false,body->getTerminator());
                    GetElementPtrInst* getcontext2=GetElementPtrInst::CreateInBounds(TYPE_I8,context4,CONST_I32(1),"getcontext2",body->getTerminator());
                    new StoreInst(getcontext2,Alpointer,body->getTerminator());
                    firstflag++;
                }
                else
                {
                    LoadInst* context1=new LoadInst(TYPE_I8p,Alpointer,"context3",false,body->getTerminator());
                    LoadInst* context2=new LoadInst(TYPE_I8,context1,"context1",false,body->getTerminator());
                    ZExtInst* zext1=new ZExtInst(context2,TYPE_I32,"zext1",body->getTerminator());
                    vector<llvm::Value*> vect_1; 
                    vect_1.push_back(CONST_I32(0));
                    vect_1.push_back(zext1);
                    GetElementPtrInst* getcontext1=GetElementPtrInst::CreateInBounds(ArrayType::get(Type::getInt32Ty(M.getContext()),256),gv,ArrayRef<llvm::Value*>(vect_1),"getcontext1",body->getTerminator());
                    LoadInst* context3=new LoadInst(TYPE_I32,getcontext1,"context2",body->getTerminator());
                    TruncInst* Trunc1=new TruncInst(context3,TYPE_I8,"Trunc1",body->getTerminator());
                    LoadInst* context4=new LoadInst(TYPE_I8p,Alpointer,"context1",false,body->getTerminator());
                    new StoreInst(Trunc1,context4,body->getTerminator());
                }
            }
        }
        originStore32.clear();
        originStore64.clear();
        originStore8.clear();

    }
    
    return true;
}


ModulePass* llvm::createDeSymbolization(){
    return new DeSymbolization();
}

char DeSymbolization::ID = 0;
static RegisterPass<DeSymbolization> X("DeSym", "De-symbolize general variables.The parameter \"exe_den\" is used to indicate what percentage of the variables will be confused by this item, and its range is between 1 and 100");
