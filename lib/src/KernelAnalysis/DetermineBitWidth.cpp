/**
 * The MIT License (MIT)
 * 
 * Copyright (c) 2013 cad-projects
 * 
 * Permission is hereby granted, free of charge, to any person obtaining a copy of
 * this software and associated documentation files (the "Software"), to deal in
 * the Software without restriction, including without limitation the rights to
 * use, copy, modify, merge, publish, distribute, sublicense, and/or sell copies of
 * the Software, and to permit persons to whom the Software is furnished to do so,
 * subject to the following conditions:
 * 
 * The above copyright notice and this permission notice shall be included in all
 * copies or substantial portions of the Software.
 * 
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY, FITNESS
 * FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR
 * COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER
 * IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE SOFTWARE.
 */
/**
 * Author: Christian Pilato <christian.pilato@polimi.it>
 *
 * Description: Implementation of methods to compute minimal bitwidth of operations.
 */
#include "DetermineBitWidth.h"

#include "cad/Config.h"

#include "llvm/ADT/Statistic.h"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/Transforms/Utils/BasicBlockUtils.h"

#include <fstream>
#include <list>

using namespace llvm;
using namespace cadlib;

char DetermineBitWidth::ID = 0;
#define DEBUG_TYPE "determine-bitwidth"
static const char determine_bitwidth_name[] = "[CAD] Determine data bitwidth";
INITIALIZE_PASS(DetermineBitWidth, DEBUG_TYPE, determine_bitwidth_name, false, false)

void DetermineBitWidth::getAnalysisUsage(AnalysisUsage &AU) const
{

}

Pass* createDetermineBitWidthPass() {
  return new DetermineBitWidth;
}

void DetermineBitWidth::parseConfig(const std::string& funName, const std::string& configFile)
{
   std::string line;
   std::ifstream myfile(configFile.c_str());
   if (myfile.is_open())
   {
      while (myfile.good())
      {
         getline (myfile,line);
         if (line.find("DATASIZE") != std::string::npos)
         {
            std::string substring = line.substr(line.find_first_of(" ")+1,line.size());
            std::string Name = substring.substr(0, substring.find_first_of(" "));
            std::string FuncName = Name.substr(0, Name.find_first_of("."));
            std::string VarName = Name.substr(Name.find_first_of(".")+1,Name.size());
            if (FuncName != funName) continue;
            unsigned int size = atoi(substring.substr(substring.find_first_of(" ")+1,substring.size()).c_str());
            parameterSize[VarName] = size;
            errs() << "#" << VarName << "# -> size = " << size << "\n";
         }
      }
      myfile.close();
   }
}

unsigned int DetermineBitWidth::getBitWidth(const Value* I) const
{
   assert(bitWidth.find(I) != bitWidth.end() && "missing value");
   return bitWidth.find(I)->second;
}

bool DetermineBitWidth::hasBitWidth(const Value *I) const
{
   return bitWidth.find(I) != bitWidth.end();
}

bool DetermineBitWidth::runOnFunction(Function &F)
{
   if (std::find(functionNames.begin(), functionNames.end(), F.getName()) == functionNames.end())
      return false;

   errs() << "Determine bit width: #" << F.getName() << "#\n";

   if (configFile.size())
   {
      parseConfig(F.getName(), configFile);
   }

   bool Modified = false;
   std::list<Value*> WorkList;
   for(Function::ArgumentListType::iterator p = F.getArgumentList().begin(); p != F.getArgumentList().end(); p++)
   {
      Argument& A = *p;
      processInstruction(&A);
   }

   for (Function::iterator b = F.begin(), be = F.end(); b != be; b++)
   {
      for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; i++)
      {
         Instruction& I = *i;
         WorkList.push_back(&I);
      }
   }

   bool Continue = false;
   do
   {
      Continue = false;
      std::set<Instruction*> CastInsts;
      for(std::list<Value*>::iterator l = WorkList.begin(); l != WorkList.end(); l++)
      {
         if (processInstruction(*l))
         {
            Continue = true;
            Modified = true;
         }
      }
      for(std::set<Instruction*>::iterator c = CastInsts.begin(); c != CastInsts.end(); c++)
      {
         Modified = true;
         std::list<Value*>::iterator f = std::find(WorkList.begin(), WorkList.end(), *c);
         if (f != WorkList.end())
         {
            errs() << "removing cast operation\n";
            WorkList.erase(f);
         }
      }
   }
   while(Continue);

   for(Function::ArgumentListType::iterator p = F.getArgumentList().begin(); p != F.getArgumentList().end(); p++)
   {
      Argument& A = *p;
      errs() << A << " -> Size = " << bitWidth[&A] << "\n";
   }

   errs() << "##\n\n";
   return Modified;
}

unsigned int DetermineBitWidth::getDataSize(const Value* I, const Type* Ty, bool checkBitwidth)
{
   if (checkBitwidth && bitWidth.find(I) != bitWidth.end()) return bitWidth[I];
   if (Ty->isIntegerTy())
   {
      return dyn_cast<IntegerType>(Ty)->getBitWidth();
   }
   else if (Ty->isPointerTy())
   {
      Type* pointedTy = dyn_cast<SequentialType>(Ty)->getElementType();
      return getDataSize(I, pointedTy, checkBitwidth);
   }
   assert(0 && "Not supported type!");
   return 0;
}

Type* DetermineBitWidth::changeDataSize(Value* I, Type* Ty, unsigned int Size)
{
   if (Ty->isIntegerTy())
   {
      IntegerType* newT = IntegerType::get(I->getContext(), Size);
      return newT;
   }
   else if (Ty->isPointerTy())
   {
      Type* pointedTy = dyn_cast<SequentialType>(Ty)->getElementType();
      unsigned int AddressSpace = dyn_cast<PointerType>(Ty)->getAddressSpace();
      Type* newPointedTy = changeDataSize(I, pointedTy, Size);
      PointerType* newT = PointerType::get(newPointedTy, AddressSpace);
      return newT;
   }
   else
   {
      assert(0 && "Not supported type!");
   }
   return 0;
}


bool DetermineBitWidth::processInstruction(const Value* I)
{
   if (dyn_cast<Argument>(I))
   {
      if (parameterSize.find(dyn_cast<Argument>(I)->getName()) != parameterSize.end())
      {
         bitWidth[I] = parameterSize[dyn_cast<Argument>(I)->getName()];
      }
      else if (bitWidth.find(I) == bitWidth.end())
      {
         bitWidth[I] = getDataSize(I, I->getType(), false);
      }
      return false;
   }

   if (dyn_cast<ConstantInt>(I))
   {
      std::string BinValue = dyn_cast<ConstantInt>(I)->getValue().toString(2, true);
      bitWidth[I] = BinValue.size()+1;
      return false;
   }

   assert(dyn_cast<Instruction>(I) && ("Unknown type"));
   const Instruction* In = dyn_cast<Instruction>(I);
   switch(In->getOpcode())
   {
      case Instruction::Add:
      case Instruction::AShr:
      case Instruction::Mul:
      case Instruction::SDiv:
      case Instruction::Sub:
      {
         bool Modified = false;
         unsigned int bit0 = 0;
         unsigned int bit1 = 0;
         if (bitWidth.find(dyn_cast<BinaryOperator>(I)->getOperand(0)) != bitWidth.end())
         {
            bit0 = bitWidth[dyn_cast<BinaryOperator>(I)->getOperand(0)];
         }
         if (bitWidth.find(dyn_cast<BinaryOperator>(I)->getOperand(1)) != bitWidth.end())
         {
            bit1 = bitWidth[dyn_cast<BinaryOperator>(I)->getOperand(1)];
         }
         Modified |= processInstruction(dyn_cast<BinaryOperator>(I)->getOperand(0));
         Modified |= processInstruction(dyn_cast<BinaryOperator>(I)->getOperand(1));
         if (bit0 < bitWidth[dyn_cast<BinaryOperator>(I)->getOperand(0)] || bit1 < bitWidth[dyn_cast<BinaryOperator>(I)->getOperand(1)])
            Modified = true;
         bit0 = bitWidth[dyn_cast<BinaryOperator>(I)->getOperand(0)];
         bit1 = bitWidth[dyn_cast<BinaryOperator>(I)->getOperand(1)];
         if ((bitWidth.find(I) != bitWidth.end()) && (bitWidth[I] < std::max(bit0, bit1)))
            Modified = true;
         bitWidth[I] = std::max(bit0, bit1);
         return Modified;
      }
      case Instruction::ICmp:
      {
         bool Modified = false;
         unsigned int bit0 = 0;
         unsigned int bit1 = 0;
         if (bitWidth.find(dyn_cast<CmpInst>(I)->getOperand(0)) != bitWidth.end())
         {
            bit0 = bitWidth[dyn_cast<CmpInst>(I)->getOperand(0)];
         }
         if (bitWidth.find(dyn_cast<CmpInst>(I)->getOperand(1)) != bitWidth.end())
         {
            bit1 = bitWidth[dyn_cast<CmpInst>(I)->getOperand(1)];
         }
         Modified |= processInstruction(dyn_cast<CmpInst>(I)->getOperand(0));
         Modified |= processInstruction(dyn_cast<CmpInst>(I)->getOperand(1));
         if (bit0 < bitWidth[dyn_cast<CmpInst>(I)->getOperand(0)] || bit1 < bitWidth[dyn_cast<CmpInst>(I)->getOperand(1)])
            Modified = true;
         bitWidth[I] = 1;
         return Modified;
      }
      case Instruction::GetElementPtr:
      {
         const GetElementPtrInst* ptr = dyn_cast<GetElementPtrInst>(I);
         processInstruction(ptr->getPointerOperand());
         if (bitWidth.find(I) != bitWidth.end())
         {
            unsigned int bit = bitWidth[ptr->getPointerOperand()];
            if (bitWidth[I] > bit)
            {
               bitWidth[ptr->getPointerOperand()] = bitWidth[I];
               return true;
            }
         }
         else
         {
            bitWidth[I] = bitWidth[ptr->getPointerOperand()];
         }
         std::list<const Value*> Operations;
         for(GetElementPtrInst::const_op_iterator It = ptr->idx_begin(); It != ptr->idx_end(); It++)
         {
            getMemoryOps(*It, Operations);
         }
         for(std::list<const Value*>::iterator op = Operations.begin(); op != Operations.end(); op++)
         {
            processInstruction(*op);
         }
         return false;
      }
      case Instruction::Load:
      {
         bool Modified = processInstruction(dyn_cast<LoadInst>(I)->getPointerOperand());
         bitWidth[I] = bitWidth[dyn_cast<LoadInst>(I)->getPointerOperand()];
         return Modified;
      }
      case Instruction::Store:
      {
         bool Modified = false;
         Modified |= processInstruction(dyn_cast<StoreInst>(I)->getPointerOperand());
         Modified |= processInstruction(dyn_cast<StoreInst>(I)->getValueOperand());
         unsigned int bit = bitWidth[dyn_cast<StoreInst>(I)->getValueOperand()];
         bitWidth[dyn_cast<StoreInst>(I)->getPointerOperand()] = bit;
         bitWidth[I] = bit;
         return Modified;
      }
      case Instruction::Trunc:
      case Instruction::ZExt:
      {
         bool Modified = processInstruction(dyn_cast<CastInst>(I)->getOperand(0));
         bitWidth[I] = bitWidth[dyn_cast<CastInst>(I)->getOperand(0)];
         return Modified;
      }
      case Instruction::Ret:
      {
         bool Modified = false;
         if (dyn_cast<ReturnInst>(I)->getReturnValue())
         {
            Modified = processInstruction(dyn_cast<ReturnInst>(I)->getReturnValue());
         }
         return Modified;
      }
      case Instruction::Br:
      {
         bool Modified = false;
         if (dyn_cast<BranchInst>(I)->isConditional())
         {
            Modified = processInstruction(dyn_cast<BranchInst>(I)->getCondition());
         }
         bitWidth[I] = 0;
         return Modified;
      }
      default:
      {
         errs() << "not supported = " << In->getOpcodeName() << "\n";
         assert(0);
      }
   }

   ///you should never reach this point! everything should be already processed
   return false;
}
