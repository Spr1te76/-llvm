#include "llvm/IR/Function.h"
#include "llvm/Pass.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/IR/Module.h"
#include "llvm/IR/Instructions.h"
#include "llvm/Transforms/Utils/Local.h"
#include "llvm/IR/Intrinsics.h"
#include "llvm/IR/IntrinsicsX86.h"
#include "SplitBasicBlock.h"
#include "Utils.h"
#include <vector>
#include <cstdlib>
#include <ctime>
#include "RandomControlFlow.h"
using std::vector;
using namespace llvm;

// 混淆次数，混淆次数越多混淆结果越复杂
static cl::opt<int> obfuTimes("rcf_loop", cl::init(1), cl::desc("Obfuscate a function <bcf_loop> time(s)."));

namespace{
    struct RandomControlFlow : public FunctionPass{
        static char ID;

        RandomControlFlow() : FunctionPass(ID){
            srand(time(NULL));
        }

        bool runOnFunction(Function &F);

        // 创建一组等效于 origVar 的指令
        Value* alterVal(Value *origVar,BasicBlock *insertAfter,Function&F);

        void insertRandomBranch(Value *randVar, BasicBlock *ifTrue, BasicBlock *ifFalse, BasicBlock *insertAfter,Function&F);

        // 以基本块为单位进行随机控制流混淆
        bool randcf(BasicBlock *BB,Function&F);

    };
}


bool RandomControlFlow::runOnFunction(Function &F){
    FunctionPass *pass = createSplitBasicBlockPass();
    pass->runOnFunction(F);
    for(int i = 0;i < obfuTimes;i ++)
    {
        vector<BasicBlock*> origBB;
        for(BasicBlock &BB : F)
        {
            origBB.push_back(&BB);
        }
        for(BasicBlock *BB : origBB)
        {
            randcf(BB,F);
        }
    }
    return true;
}

void RandomControlFlow::insertRandomBranch(Value *randVar, BasicBlock *ifTrue, BasicBlock *ifFalse, BasicBlock *insertAfter,Function&F){
    // 对随机数进行等价变换
    Value *alteredRandVar = alterVal(randVar, insertAfter,F);
    Value *randMod2 = BinaryOperator::Create(Instruction::And, alteredRandVar, CONST_I32(1), "", insertAfter);
    ICmpInst *condition = new ICmpInst(*insertAfter, ICmpInst::ICMP_EQ, randMod2, CONST_I32(1));
    BranchInst::Create(ifTrue, ifFalse, condition, insertAfter);
}

bool RandomControlFlow::randcf(BasicBlock *BB,Function&F){
    // 拆分得到 entryBB, bodyBB, endBB
    // 其中所有的 PHI 指令都在 entryBB(如果有的话)
    // endBB 只包含一条终结指令
    BasicBlock *entryBB = BB;
    BasicBlock *bodyBB = entryBB->splitBasicBlock(BB->getFirstNonPHIOrDbgOrLifetime(), "bodyBB");
    BasicBlock *endBB = bodyBB->splitBasicBlock(bodyBB->getTerminator(), "endBB");
    BasicBlock *cloneBB = createCloneBasicBlock(bodyBB);
    
    // 在 entryBB 后插入随机跳转，使其能够随机跳转到第 bodyBB 或其克隆基本块 cloneBB
    entryBB->getTerminator()->eraseFromParent();
    Function *rdrand = Intrinsic::getDeclaration(entryBB->getModule(), Intrinsic::x86_rdrand_32);
    CallInst *randVarStruct = CallInst::Create(rdrand->getFunctionType(), rdrand, "", entryBB);
    // 通过 rdrand 内置函数获取随机数
    Value *randVar = ExtractValueInst::Create(randVarStruct, 0, "", entryBB);
    insertRandomBranch(randVar, bodyBB, cloneBB, entryBB,F);

    // 添加 bodyBB 到 bodyBB.clone 的虚假随机跳转
    bodyBB->getTerminator()->eraseFromParent();
    insertRandomBranch(randVar, endBB, cloneBB, bodyBB,F);
    // 添加 bodyBB.clone 到 bodyBB 的虚假随机跳转
    cloneBB->getTerminator()->eraseFromParent();
    insertRandomBranch(randVar, bodyBB, endBB, cloneBB,F);

    return true;
}

Value* RandomControlFlow::alterVal(Value *startVar,BasicBlock *insertAfter,Function&F){
    uint32_t code = rand() % 10;
    Value *result;
    if(code == 0){
        //x = x * (x + 1) - x^2
        BinaryOperator *op1 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(1), "", insertAfter);
        BinaryOperator *op2 = BinaryOperator::Create(Instruction::Mul, startVar, op1, "", insertAfter);
        BinaryOperator *op3 = BinaryOperator::Create(Instruction::Mul, startVar, startVar, "", insertAfter);
        BinaryOperator *op4 = BinaryOperator::Create(Instruction::Sub, op2, op3, "", insertAfter);
        result = op4;
    }else if(code == 1){
        //x = 3 * x * (x - 2) - 3 * x^2 + 7 * x
        BinaryOperator *op1 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(3), "", insertAfter);
        BinaryOperator *op2 = BinaryOperator::Create(Instruction::Sub, startVar, CONST_I32(2), "", insertAfter);
        BinaryOperator *op3 = BinaryOperator::Create(Instruction::Mul, op1, op2, "", insertAfter);
        BinaryOperator *op4 = BinaryOperator::Create(Instruction::Mul, startVar, startVar, "", insertAfter);
        BinaryOperator *op5 = BinaryOperator::Create(Instruction::Mul, op4, CONST_I32(3), "", insertAfter);
        BinaryOperator *op6 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(7), "", insertAfter);
        BinaryOperator *op7 = BinaryOperator::Create(Instruction::Sub, op3, op5, "", insertAfter);
        BinaryOperator *op8 = BinaryOperator::Create(Instruction::Add, op6, op7, "", insertAfter);
        result = op8;
    }else if(code == 2){
        //x = (x - 1) * (x + 3) - (x + 4) * (x - 3) - 9
        BinaryOperator *op1 = BinaryOperator::Create(Instruction::Sub, startVar, CONST_I32(1), "", insertAfter);
        BinaryOperator *op2 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(3), "", insertAfter);
        BinaryOperator *op3 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(4), "", insertAfter);
        BinaryOperator *op4 = BinaryOperator::Create(Instruction::Sub, startVar, CONST_I32(3), "", insertAfter);
        BinaryOperator *op5 = BinaryOperator::Create(Instruction::Mul, op1, op2, "", insertAfter);
        BinaryOperator *op6 = BinaryOperator::Create(Instruction::Mul, op3, op4, "", insertAfter);
        BinaryOperator *op7 = BinaryOperator::Create(Instruction::Sub, op5, op6, "", insertAfter);
        BinaryOperator *op8 = BinaryOperator::Create(Instruction::Sub, op7, CONST_I32(9), "", insertAfter);
        result = op8;
    }else if(code==3){
      //x=x2+x(x+2)+2x(x+3)+(x-4)-4x2+4-8x
       BinaryOperator *op1 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(2), "", insertAfter);
       BinaryOperator *op2 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(3), "", insertAfter);
       BinaryOperator *op3 = BinaryOperator::Create(Instruction::Sub, startVar, CONST_I32(4), "", insertAfter);
       BinaryOperator *op4 = BinaryOperator::Create(Instruction::Mul, startVar, op1, "", insertAfter);
       BinaryOperator *op5 = BinaryOperator::Create(Instruction::Mul, startVar, startVar, "", insertAfter);
       BinaryOperator *op6 = BinaryOperator::Create(Instruction::Add, op5, op4, "", insertAfter);
       BinaryOperator *op7 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(2), "", insertAfter);
       BinaryOperator *op8 = BinaryOperator::Create(Instruction::Mul, op7, op2, "", insertAfter);
       BinaryOperator *op9 = BinaryOperator::Create(Instruction::Add, op6, op8, "", insertAfter);
       BinaryOperator *op10 = BinaryOperator::Create(Instruction::Add, op3, op9, "", insertAfter); 
       BinaryOperator *op11 = BinaryOperator::Create(Instruction::Mul, op5, CONST_I32(4), "", insertAfter);
       BinaryOperator *op12 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(8), "", insertAfter);
       BinaryOperator *op13 = BinaryOperator::Create(Instruction::Sub, op10, op11, "", insertAfter);
       BinaryOperator *op14 = BinaryOperator::Create(Instruction::Sub, op13, op12, "", insertAfter);
       BinaryOperator *op15 = BinaryOperator::Create(Instruction::Add, op14, CONST_I32(4), "", insertAfter);
       result=op15;
    }
    else if(code==4){
      //x=x*(x+4)+x*(x+16)-2*x^2-19 *x
       BinaryOperator *op1 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(4), "", insertAfter);
       BinaryOperator *op2 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(16), "", insertAfter);
       BinaryOperator *op3 = BinaryOperator::Create(Instruction::Mul, startVar, startVar, "", insertAfter);
       BinaryOperator *op4 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(19), "", insertAfter);
       BinaryOperator *op5=  BinaryOperator::Create(Instruction::Mul, startVar,op1 , "", insertAfter);
       BinaryOperator*op6=  BinaryOperator::Create(Instruction::Mul, startVar,op2 , "", insertAfter);
       BinaryOperator*op7=  BinaryOperator::Create(Instruction::Add, op5,op6 , "", insertAfter);
       BinaryOperator*op8=  BinaryOperator::Create(Instruction::Mul, CONST_I32(2),op3 , "", insertAfter);
       BinaryOperator*op9=  BinaryOperator::Create(Instruction::Sub, op7,op8, "", insertAfter);
       BinaryOperator*op10=  BinaryOperator::Create(Instruction::Sub, op9,op4, "", insertAfter);
       result=op10;



    }
    else if(code==5){
    //x=x*(x+3)+x(x-1)-x(2x+1)
       BinaryOperator *op1 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(3), "", insertAfter);
       BinaryOperator *op2 = BinaryOperator::Create(Instruction::Sub, startVar, CONST_I32(1), "", insertAfter);
       BinaryOperator *op3 = BinaryOperator::Create(Instruction::Mul, startVar, op2, "", insertAfter);
       BinaryOperator *op4 = BinaryOperator::Create(Instruction::Mul, startVar, op1, "", insertAfter);
       BinaryOperator *op5 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(2), "", insertAfter);
       BinaryOperator *op6 = BinaryOperator::Create(Instruction::Add, op5,  CONST_I32(1), "", insertAfter);
       BinaryOperator *op7 = BinaryOperator::Create(Instruction::Add, op3, op4, "", insertAfter);
       BinaryOperator *op8 = BinaryOperator::Create(Instruction::Sub, op7, op6, "", insertAfter);
       result=op8;
    

    }
    else if (code==6){
       //x = (x-1)(x+3)-(x+4)*(x-3)-9+x(x+3)-x(x+3)+x2+3x
        BinaryOperator *op1 = BinaryOperator::Create(Instruction::Sub, startVar, CONST_I32(1), "", insertAfter);
        BinaryOperator *op2 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(3), "", insertAfter);
        BinaryOperator *op3 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(4), "", insertAfter);
        BinaryOperator *op4 = BinaryOperator::Create(Instruction::Sub, startVar, CONST_I32(3), "", insertAfter);
        BinaryOperator *op5 = BinaryOperator::Create(Instruction::Mul, op1, op2, "", insertAfter);
        BinaryOperator *op6 = BinaryOperator::Create(Instruction::Mul, op3, op4, "", insertAfter);
        BinaryOperator *op7 = BinaryOperator::Create(Instruction::Sub, op5, op6, "", insertAfter);
        BinaryOperator *op8 = BinaryOperator::Create(Instruction::Sub, op7, CONST_I32(9), "", insertAfter);
        BinaryOperator *op9 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(3), "", insertAfter);
        BinaryOperator *op10 = BinaryOperator::Create(Instruction::Mul, startVar, startVar, "", insertAfter);
        BinaryOperator *op11 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(3), "", insertAfter);
        BinaryOperator *op12 = BinaryOperator::Create(Instruction::Mul, startVar, op9, "", insertAfter);
        BinaryOperator *op13 = BinaryOperator::Create(Instruction::Sub, op8, op12, "", insertAfter);
        BinaryOperator *op14 = BinaryOperator::Create(Instruction::Add, op13, op10, "", insertAfter);
        BinaryOperator *op15 = BinaryOperator::Create(Instruction::Add, op13, op11, "", insertAfter);
        result = op11;
   


    }
    else if(code==7){
     //x=x*(x+4)+x*(x+16)-2*x^2-19 *x
       BinaryOperator *op1 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(4), "", insertAfter);
       BinaryOperator *op2 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(16), "", insertAfter);
       BinaryOperator *op3 = BinaryOperator::Create(Instruction::Mul, startVar, startVar, "", insertAfter);
       BinaryOperator *op4 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(19), "", insertAfter);
       BinaryOperator *op5=  BinaryOperator::Create(Instruction::Mul, startVar,op1 , "", insertAfter);
       BinaryOperator*op6=  BinaryOperator::Create(Instruction::Mul, startVar,op2 , "", insertAfter);
       BinaryOperator*op7=  BinaryOperator::Create(Instruction::Add, op5,op6 , "", insertAfter);
       BinaryOperator*op8=  BinaryOperator::Create(Instruction::Mul, CONST_I32(2),op3 , "", insertAfter);
       BinaryOperator*op9=  BinaryOperator::Create(Instruction::Sub, op7,op8, "", insertAfter);
       BinaryOperator*op10=  BinaryOperator::Create(Instruction::Sub, op9,op4, "", insertAfter);
       result=op10;


    }
    else if(code==8){
       //x=(x+2)(x+3)-6(x+1)+x(x+2)-2x^2
       BinaryOperator *op1 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(2), "", insertAfter);
       BinaryOperator *op2 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(3), "", insertAfter);
       BinaryOperator *op3 = BinaryOperator::Create(Instruction::Mul, startVar, startVar, "", insertAfter);
       BinaryOperator *op4 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(1), "", insertAfter);
       BinaryOperator *op5 =  BinaryOperator::Create(Instruction::Mul, op1,op2 , "", insertAfter);
       BinaryOperator*op6  =  BinaryOperator::Create(Instruction::Mul, CONST_I32(6),op4 , "", insertAfter);
       BinaryOperator*op7  =  BinaryOperator::Create(Instruction::Sub, op5,op6 , "", insertAfter);
       BinaryOperator*op8  =  BinaryOperator::Create(Instruction::Mul, startVar,op1 , "", insertAfter);
       BinaryOperator*op9  =  BinaryOperator::Create(Instruction::Mul, CONST_I32(2),op3 , "", insertAfter);
       BinaryOperator*op10 =  BinaryOperator::Create(Instruction::Add, op7,op8, "", insertAfter);
       BinaryOperator*op11 =  BinaryOperator::Create(Instruction::Sub, op10,op9, "", insertAfter);
       result=op11;



    }
    else if(code==9){
         //x=x2+x(x+2)+2x(x+3)+(x-4)-4x2+4-8x+x(x+5)-x2-5x
       BinaryOperator *op1 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(2), "", insertAfter);
       BinaryOperator *op2 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(3), "", insertAfter);
       BinaryOperator *op3 = BinaryOperator::Create(Instruction::Sub, startVar, CONST_I32(4), "", insertAfter);
       BinaryOperator *op4 = BinaryOperator::Create(Instruction::Mul, startVar, op1, "", insertAfter);
       BinaryOperator *op5 = BinaryOperator::Create(Instruction::Mul, startVar, startVar, "", insertAfter);
       BinaryOperator *op6 = BinaryOperator::Create(Instruction::Add, op5, op4, "", insertAfter);
       BinaryOperator *op7 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(2), "", insertAfter);
       BinaryOperator *op8 = BinaryOperator::Create(Instruction::Mul, op7, op2, "", insertAfter);
       BinaryOperator *op9 = BinaryOperator::Create(Instruction::Add, op6, op8, "", insertAfter);
       BinaryOperator *op10 = BinaryOperator::Create(Instruction::Add, op3, op9, "", insertAfter); 
       BinaryOperator *op11 = BinaryOperator::Create(Instruction::Mul, op5, CONST_I32(4), "", insertAfter);
       BinaryOperator *op12 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(8), "", insertAfter);
       BinaryOperator *op13 = BinaryOperator::Create(Instruction::Sub, op10, op11, "", insertAfter);
       BinaryOperator *op14 = BinaryOperator::Create(Instruction::Sub, op13, op12, "", insertAfter);
       BinaryOperator *op15 = BinaryOperator::Create(Instruction::Add, op14, CONST_I32(4), "", insertAfter);
       BinaryOperator *op16 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(5), "", insertAfter);
       BinaryOperator *op17 = BinaryOperator::Create(Instruction::Mul, startVar, op16, "", insertAfter);
       BinaryOperator *op18 = BinaryOperator::Create(Instruction::Mul, startVar, CONST_I32(5), "", insertAfter);
       BinaryOperator *op19 = BinaryOperator::Create(Instruction::Add, op15, op17, "", insertAfter);
       BinaryOperator *op20 = BinaryOperator::Create(Instruction::Sub, op19, op5, "", insertAfter);
       BinaryOperator *op21 = BinaryOperator::Create(Instruction::Sub, op20, op18, "", insertAfter);
       result=op21;


       result=op15;


    }
    else if(code==10){
     //x=(x+2)(x+3)-6(x+1)+x(x+2)-2x^2
       BinaryOperator *op1 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(2), "", insertAfter);
       BinaryOperator *op2 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(3), "", insertAfter);
       BinaryOperator *op3 = BinaryOperator::Create(Instruction::Mul, startVar, startVar, "", insertAfter);
       BinaryOperator *op4 = BinaryOperator::Create(Instruction::Add, startVar, CONST_I32(1), "", insertAfter);
       BinaryOperator *op5=  BinaryOperator::Create(Instruction::Mul, op1,op2 , "", insertAfter);
       BinaryOperator*op6=  BinaryOperator::Create(Instruction::Mul, CONST_I32(6),op4 , "", insertAfter);
       BinaryOperator*op7=  BinaryOperator::Create(Instruction::Sub, op5,op6 , "", insertAfter);
       BinaryOperator*op8=  BinaryOperator::Create(Instruction::Mul, startVar,op1 , "", insertAfter);
       BinaryOperator*op9=  BinaryOperator::Create(Instruction::Mul, CONST_I32(2),op3 , "", insertAfter);
       BinaryOperator*op10=  BinaryOperator::Create(Instruction::Add, op7,op8, "", insertAfter);
       BinaryOperator*op11=  BinaryOperator::Create(Instruction::Sub, op10,op9, "", insertAfter);
       result=op11;



    }
    return result;
}

FunctionPass* llvm::createRandomControlFlow(){
    return new RandomControlFlow();
}

char RandomControlFlow::ID = 0;
static RegisterPass<RandomControlFlow> X("rcf", "Add random control flow to each function.");