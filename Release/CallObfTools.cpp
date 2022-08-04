#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/Utils/ValueMapper.h"
#include "llvm/Transforms/Utils/Cloning.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include"llvm/Support/Alignment.h"
#include "AllHeadTools.h"

using namespace llvm;
static cl::opt<int> ObfMode("ObfMode", cl::init(0), cl::desc("ObfMode-<1,2,3,4,5>"));


namespace{
    class CallObfTools:public FunctionPass{
        public:
            static char ID;
            CallObfTools():FunctionPass(ID){}
            bool runOnFunction(Function&F);

    };
}
bool CallObfTools::runOnFunction(Function&F)
{
    int choosemod=ObfMode;
    if(ObfMode>0&&ObfMode<=5)
    {
        FunctionPass* FlatteBK=createFlattening();
        FunctionPass* CodeBK=createCodeSubstitution();
        FunctionPass* ConstBK=createConstantSubstitution();
        FunctionPass* BogusBK=createBogusControlFlowG();
        ModulePass* DeSymBK=createDeSymbolization();
        FunctionPass* AntiDynBK=createAntiDynSymbols();


        switch (choosemod)
        {
            case 1:
            {
                FlatteBK->runOnFunction(F);
                CodeBK->runOnFunction(F);
                ConstBK->runOnFunction(F);
                break;
            }
            case 2:
            {
                BogusBK->runOnFunction(F);
                CodeBK->runOnFunction(F);
                ConstBK->runOnFunction(F);
                break;
            }
            case 3:
            {
                Module* temp=F.getParent();
                DeSymBK->runOnModule(*temp);
                AntiDynBK->runOnFunction(F);
                CodeBK->runOnFunction(F);
                ConstBK->runOnFunction(F);
                break;
            }
            case 4:
            {
                FlatteBK->runOnFunction(F);
                BogusBK->runOnFunction(F);
                CodeBK->runOnFunction(F);
                ConstBK->runOnFunction(F);
                break;
            }
            case 5:
            {
                AntiDynBK->runOnFunction(F);
                BogusBK->runOnFunction(F);
                CodeBK->runOnFunction(F);
                ConstBK->runOnFunction(F); 
                break;
            }
        }
    }
    else
    {
        errs()<<"Please choose a mode.\n";
        return true;
    }
    return true;
}

char CallObfTools::ID = 0;
static RegisterPass<CallObfTools> X("Gllvm", "Start Obf.");