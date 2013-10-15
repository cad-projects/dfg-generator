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
 * Description: Implementation of methods to compute the DFG representation.
 */
#include "cad/Config.h"

#include "DfgGeneration.h"

#define DEBUG_TYPE "dfg-generation"
#include "DetermineBitWidth.h"

#include "Dfg.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/Instructions.h"

STATISTIC(DfgCounter, "[CAD] Counts number of functions analyzed");

using namespace llvm;
using namespace cadlib;

char DfgGeneration::ID = 0;
static const char dfg_generation_name[] = "[CAD] DFG Generation";
INITIALIZE_PASS_BEGIN(DfgGeneration, DEBUG_TYPE, dfg_generation_name, false, false)
INITIALIZE_PASS_DEPENDENCY(DetermineBitWidth)
INITIALIZE_PASS_END(DfgGeneration, DEBUG_TYPE, dfg_generation_name, false, false)

void DfgGeneration::getAnalysisUsage(AnalysisUsage &AU) const
{
   AU.addRequired<DetermineBitWidth>();
   AU.setPreservesAll();
}

Pass* createDfgGenerationPass() {
  return new DfgGeneration;
}

bool DfgGeneration::runOnFunction(Function &F)
{
   if (std::find(functionNames.begin(), functionNames.end(), F.getName()) == functionNames.end())
      return false;

   ++DfgCounter;
   errs() << "DFG Generation: #" << F.getName() << "#\n";
   graph = new DfgGraph(F.getName());

   unsigned int bbIdx = 1;
   std::map<BasicBlock*, std::tr1::tuple<const Instruction*, unsigned int> > ControlEdge;
   for (Function::iterator b = F.begin(), be = F.end(); b != be; b++)
   {
      unsigned int index = bbIdx++;
      BasicBlock& BB = *b;
      graph->bbMap[&BB] = index;
      graph->bbReverseMap[index] = &BB;
   }
   for (Function::iterator b = F.begin(), be = F.end(); b != be; b++)
   {
      BasicBlock& BB = *b;
      const TerminatorInst* LastInst = BB.getTerminator();
      if (LastInst)
      {
         if (LastInst->getOpcode() == Instruction::Br)
         {
            const BranchInst* br = dyn_cast<BranchInst>(LastInst);
            if (br->isConditional())
            {
               const Instruction* val = dyn_cast<Instruction>(br->getCondition());
               ControlEdge[br->getSuccessor(0)] = std::tr1::tuple<const Instruction*, unsigned int>(val, 1);
               ControlEdge[br->getSuccessor(1)] = std::tr1::tuple<const Instruction*, unsigned int>(val, 0);
            }
         }
         else if (LastInst->getOpcode() == Instruction::Switch)
         {
            assert(0 && "Switch not yet supported");
         }
         else if (!LastInst->getOpcode() == Instruction::Ret)
         {
            errs() << "LAST INSTRUCTION NOT SUPPORTED! " << LastInst->getOpcodeName() << "\n";
            assert(0);
         }
      }

      for (BasicBlock::iterator i = b->begin(), ie = b->end(); i != ie; i++)
      {
         Instruction& I = *i;
         std::set<const Value*> alreadyAnalyzed;
         bool isLoad = false, isStore = false;
         if (!isMemoryRelated(&I, alreadyAnalyzed, isLoad, isStore))
         {
            if (dyn_cast<CastInst>(&I) || dyn_cast<BranchInst>(&I)) continue;
            DfgNode* dfg = processInstruction(graph, &I, graph->bbMap[&BB]);
            if (dfg && ControlEdge.find(&BB) != ControlEdge.end())
            {
               DfgNode* src = graph->getNode(std::tr1::get<0>(ControlEdge[&BB]));
               DfgNode::Control_t type = DfgNode::T_EDGE;
               if (std::tr1::get<1>(ControlEdge[&BB]) == 0)
               {
                  type = DfgNode::F_EDGE;
               }
               dfg->ControlNodes.push_back(DfgNode::Condition_t(src, type, 0));
            }
         }
      }
   }

   errs() << "##\n\n";
   return false;
}

DfgNode* DfgGeneration::processInstruction(DfgGraph* g, Instruction* I, unsigned int bbIdx)
{
   DetermineBitWidth& BW = getAnalysis<DetermineBitWidth>();

   switch(I->getOpcode())
   {
      case Instruction::Ret:
      {
         if (!dyn_cast<ReturnInst>(I)->getReturnValue()) return 0;
         break;
      }
      case Instruction::GetElementPtr:
      {
         const GetElementPtrInst* ptr = dyn_cast<GetElementPtrInst>(I);
         std::list<const Value*> Operations;
         for(GetElementPtrInst::const_op_iterator It = ptr->idx_begin(); It != ptr->idx_end(); It++)
         {
            getMemoryOps(*It, Operations);
         }
         for(std::list<const Value*>::iterator op = Operations.begin(); op != Operations.end(); op++)
         {
            graph->bitWidth[*op] = BW.getBitWidth(*op);
         }
         return 0;
      }
   }

   DfgNode* src = 0;
   DfgNode* tgt = 0;

   src = graph->getNode(I, BW.getBitWidth(I), bbIdx);

   switch(I->getOpcode())
   {
      case Instruction::Store:
      {
         std::set<const Value*> Uses;
         Value* ptr = dyn_cast<StoreInst>(I)->getPointerOperand();
         getMemoryUses(ptr, Uses);
         for(std::set<const Value*>::iterator It = Uses.begin(); It != Uses.end(); It++)
         {
            tgt = g->getNode(*It, BW.getBitWidth(*It), bbIdx);
            src->UseNodes.push_back(tgt);
         }
         const Value* val1 = getRealValue(dyn_cast<StoreInst>(I)->getValueOperand());
         tgt = g->getNode(val1, BW.getBitWidth(val1), bbIdx);
         src->UseNodes.push_back(tgt);
         if (dyn_cast<GetElementPtrInst>(ptr))
             processInstruction(g, dyn_cast<Instruction>(ptr), bbIdx);
         break;
      }
      case Instruction::Load:
      {
         std::set<const Value*> Uses;
         Value* ptr = dyn_cast<LoadInst>(I)->getPointerOperand();
         getMemoryUses(ptr, Uses);
         for(std::set<const Value*>::iterator It = Uses.begin(); It != Uses.end(); It++)
         {
            tgt = g->getNode(*It, BW.getBitWidth(*It), bbIdx);
            src->UseNodes.push_back(tgt);
         }
         if (dyn_cast<GetElementPtrInst>(ptr))
             processInstruction(g, dyn_cast<Instruction>(ptr), bbIdx);
         break;
      }
      case Instruction::AShr:
      case Instruction::Add:
      case Instruction::Mul:
      case Instruction::SDiv:
      case Instruction::Sub:
      case Instruction::UDiv:
      {
         const Value* op0 = getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(0));
         tgt = g->getNode(op0, BW.getBitWidth(op0), bbIdx);
         src->UseNodes.push_back(tgt);
         const Value* op1 = getRealValue(dyn_cast<BinaryOperator>(I)->getOperand(1));
         tgt = g->getNode(op1, BW.getBitWidth(op1), bbIdx);
         src->UseNodes.push_back(tgt);
         break;
      }
      case Instruction::ICmp:
      {
         const Value* op0 = getRealValue(dyn_cast<CmpInst>(I)->getOperand(0));
         tgt = g->getNode(op0, BW.getBitWidth(op0), bbIdx);
         src->UseNodes.push_back(tgt);
         const Value* op1 = getRealValue(dyn_cast<CmpInst>(I)->getOperand(1));
         tgt = g->getNode(op1, BW.getBitWidth(op1), bbIdx);
         src->UseNodes.push_back(tgt);
         break;
      }
      case Instruction::Br:
      case Instruction::Trunc:
      case Instruction::ZExt:
      {
         break;
      }
      default:
      {
         errs() << "ERROR: " << I->getOpcodeName() << "\n";
         assert(0);
      }
   }
   return src;
}
