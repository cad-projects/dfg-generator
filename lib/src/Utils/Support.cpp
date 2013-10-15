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
 * Description: Implementation of auxiliary methods for the CAD library.
 */
#include "cad/Support.h"

#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"

using namespace llvm;

namespace cadlib
{

std::string getOperator(const Instruction* I)
{
   switch(I->getOpcode())
   {
      case Instruction::SDiv:
      case Instruction::UDiv:
      {
         return "/";
      }
      case Instruction::Sub:
      {
         return "-";
      }
      case Instruction::Add:
      {
         return "+";
      }
      case Instruction::Mul:
      {
         return "*";
      }
      case Instruction::AShr:
      {
         return ">>";
      }
      case Instruction::ICmp:
      {
         CmpInst::Predicate pred = dyn_cast<ICmpInst>(I)->getSignedPredicate();
         switch(pred)
         {
            case CmpInst::ICMP_SGT:
            {
               return ">";
               break;
            }
            case CmpInst::ICMP_SLT:
            {
               return "<";
               break;
            }
            default:
            {
               assert(0 && "UNSUPPORTED PREDICATE!\n");
            }
         }
      }
      default:
      {
         errs() << "ERROR: " << I->getOpcodeName() << "\n";
         assert(0);
      }
   }
   return "";
}

bool isMemoryRelated(const Value* I, std::set<const Value*>& alreadyAnalyzed, bool& isLoad, bool& isStore)
{
   alreadyAnalyzed.insert(I);
   if (dyn_cast<GetElementPtrInst>(I))
   {
      for(Value::const_use_iterator It = I->use_begin(); It != I->use_end(); It++)
      {
         if (dyn_cast<StoreInst>(*It)) isStore = true;
         if (dyn_cast<LoadInst>(*It)) isLoad = true;
      }
      return true;
   }
   if (dyn_cast<LoadInst>(I) || dyn_cast<StoreInst>(I))
   {
      if (dyn_cast<StoreInst>(I)) isStore = true;
      if (dyn_cast<LoadInst>(I)) isLoad = true;
      return false;
   }
   bool memory = false;
   for(Value::const_use_iterator It = I->use_begin(); It != I->use_end(); It++)
   {
      if (alreadyAnalyzed.find(*It) == alreadyAnalyzed.end())
         return memory |= isMemoryRelated(*It, alreadyAnalyzed, isLoad, isStore);
   }
   return memory;
}

std::string addBracket(const Value* op)
{
   std::string opStr = getMemoryString(op);
   if (dyn_cast<Instruction>(op) &&
          (
          dyn_cast<Instruction>(op)->getOpcode() == Instruction::Add ||
          dyn_cast<Instruction>(op)->getOpcode() == Instruction::Sub
          )
       )
      opStr = "(" + opStr + ")";
   return opStr;
}

void getMemoryOps(const Value* I, std::list<const Value*>& operations)
{
   if (dyn_cast<Argument>(I) || dyn_cast<GlobalVariable>(I) || dyn_cast<Constant>(I))
   {
      return;
   }

   if(dyn_cast<BinaryOperator>(I))
   {
      getMemoryOps(getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(0)), operations);
      getMemoryOps(getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(1)), operations);
      operations.push_back(I);
      return;
   }
   errs() << "Not supported operation = " << dyn_cast<Instruction>(I)->getOpcodeName() << "\n";
   assert(0);
}

const Value* getMemoryVar(const Value* I)
{
   if (dyn_cast<Argument>(I) || dyn_cast<GlobalVariable>(I))
   {
      if (I->getType()->isPointerTy())
         return I;
      return 0;
   }

   if (dyn_cast<LoadInst>(I) || dyn_cast<Constant>(I))
   {
      return 0;
   }

   if (dyn_cast<GetElementPtrInst>(I))
   {
      const GetElementPtrInst* ptr = dyn_cast<GetElementPtrInst>(I);
      const Value* val0 = getMemoryVar(ptr->getPointerOperand());
      if (val0) return val0;
      assert(0 && "GetElementPtrInst without variable");
   }

   switch(dyn_cast<Instruction>(I)->getOpcode())
   {
      case Instruction::Mul:
      case Instruction::Add:
      case Instruction::Sub:
      {
         const Value* val0 = getMemoryVar(getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(0)));
         if (val0) return val0;
         const Value* val1 = getMemoryVar(getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(1)));
         if (val1) return val1;
         return 0;
      }
      default:
      {
         errs() << "not supported instruction = " << dyn_cast<Instruction>(I)->getOpcodeName() << "\n";
      }
   }
   assert(0 && "Malformed instruction");
}

void getMemoryUses(const Value* I, std::set<const Value*> &Uses)
{
   if (dyn_cast<ConstantInt>(I))
      return;

   if (dyn_cast<Argument>(I) || dyn_cast<GlobalVariable>(I))
   {
      Uses.insert(I);
      return;
   }

   if (dyn_cast<GetElementPtrInst>(I))
   {
      const GetElementPtrInst* ptr = dyn_cast<GetElementPtrInst>(I);
      getMemoryUses(ptr->getPointerOperand(), Uses);
      for(GetElementPtrInst::const_op_iterator It = ptr->idx_begin(); It != ptr->idx_end(); It++)
      {
         getMemoryUses(getRealValue(*It), Uses);
      }
      return;
   }

   switch(dyn_cast<Instruction>(I)->getOpcode())
   {
      case Instruction::Load:
      {
         getMemoryUses(getRealValue(dyn_cast<LoadInst>(I)->getPointerOperand()), Uses);
         return;
      }
      case Instruction::Mul:
      case Instruction::Add:
      case Instruction::Sub:
      {
         getMemoryUses(getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(0)), Uses);
         getMemoryUses(getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(1)), Uses);
         return;
      }
   }
}

std::string getMemoryString(const Value* I)
{
   if (dyn_cast<Argument>(I) || dyn_cast<GlobalVariable>(I))
      return I->getName();

   if (dyn_cast<ConstantInt>(I))
      return dyn_cast<ConstantInt>(I)->getValue().toString(10, true);

   if (dyn_cast<GetElementPtrInst>(I))
   {
      const GetElementPtrInst* ptr = dyn_cast<GetElementPtrInst>(I);
      std::string Name = getMemoryString(ptr->getPointerOperand());
      for(GetElementPtrInst::const_op_iterator It = ptr->idx_begin(); It != ptr->idx_end(); It++)
      {
         Name += "[" + getMemoryString(*It) + "]";
      }
      return Name;
   }

   switch(dyn_cast<Instruction>(I)->getOpcode())
   {
      case Instruction::Load:
      {
         return getMemoryString(dyn_cast<LoadInst>(I)->getPointerOperand());
      }
      case Instruction::Add:
      case Instruction::AShr:
      case Instruction::Sub:
      {
         std::string Operation;
         Operation += getMemoryString(getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(0)));
         Operation += " " + getOperator(dyn_cast<Instruction>(I)) + " ";
         Operation += getMemoryString(getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(1)));
         return Operation;
      }
      case Instruction::Mul:
      case Instruction::SDiv:
      {
         const Value* op0 = getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(0));
         const Value* op1 = getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(1));
         std::string Operation;
         Operation += addBracket(op0);
         Operation += " " + getOperator(dyn_cast<Instruction>(I)) + " ";
         Operation += addBracket(op1);
         return Operation;
      }
      default:
      {
         errs() << "not supported! " << dyn_cast<Instruction>(I)->getOpcodeName() << "\n";
         assert(0);
      }
   }
   return "[ERROR]";
}

const Value* getRealValue(const Value* I)
{
   if (dyn_cast<CastInst>(I))
   {
      const Value* val = dyn_cast<UnaryInstruction>(I)->getOperand(0);
      return val;
   }
   if (dyn_cast<BranchInst>(I))
   {
      const Value* val = dyn_cast<BranchInst>(I)->getCondition();
      return val;
   }
   return I;
}

FormattedOutput::FormattedOutput(formatted_raw_ostream& _oss) :
   IndentNum(0),
   OpeningChar('{'), ClosingChar('}'),
   StartLine(true),
   oss(_oss)
{

}

FormattedOutput::~FormattedOutput()
{

}

void FormattedOutput::Indent()
{
   IndentNum+=3;
}

void FormattedOutput::DeIndent()
{
   IndentNum-=3;
}

void FormattedOutput::PrintSpaces()
{
   for(unsigned int i = 0; i < IndentNum; i++)
      oss << " ";
}

void FormattedOutput::Print(const std::string &str)
{
   for(unsigned int l = 0; l < str.size(); l++)
   {
      if (StartLine && str[l] != ClosingChar) PrintSpaces();
      if (str[l] == OpeningChar)
      {
         oss << "\n";
         PrintSpaces();
         oss << OpeningChar << "\n";
         Indent();
         StartLine = true;
      }
      else if (str[l] == ClosingChar)
      {
         if (!StartLine) oss << "\n";
         DeIndent();
         PrintSpaces();
         oss << ClosingChar;
         if (l+1 != str.size() || str[l+1] == ';')
         {
            oss << ";\n";
            l++;
         }
         else
         {
            oss << "\n";
         }
         StartLine = true;
      }
      else
      {
         oss << str[l];
         StartLine = (str[l] == '\n');
      }
   }
}
}
