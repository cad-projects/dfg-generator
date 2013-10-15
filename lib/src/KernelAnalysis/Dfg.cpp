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
 * Description: Implementation of some methods for representing and managing the 
 *              minimal DFG representation.
 */
#include "cad/Config.h"

#include "Dfg.h"

#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"

#include <sstream>

using namespace llvm;
using namespace cadlib;

DfgNode::DfgNode(const Value* NewOp, unsigned int width) : Op(NewOp), Type(INSTRUCTION), Width(width)
{
   if (dyn_cast<Argument>(Op))
   {
      Type = IN_PARAM;

      if (NewOp->getType()->isPointerTy())
      {
         std::set<const Value*> alreadyAnalyzed;
         bool isLoad = false;
         bool isStore = false;
         isMemoryRelated(NewOp, alreadyAnalyzed, isLoad, isStore);

         if (isStore)
         {
            if (isLoad) Type = INOUT_PARAM;
            else Type = OUT_PARAM;
         }
      }
   }

   if (dyn_cast<StoreInst>(Op)) Type = STORE;
   if (dyn_cast<LoadInst>(Op)) Type = LOAD;

}

unsigned DfgNode::getWidth() const
{
   return Width;
}

std::string DfgNode::getName() const
{
   if (dyn_cast<ConstantInt>(Op))
      return "[arit: " + dyn_cast<ConstantInt>(Op)->getValue().toString(10, true) + "]";

   if (dyn_cast<Argument>(Op))
   {
      bool input = false;
      bool output = false;
      for(Value::const_use_iterator It = Op->use_begin(); It != Op->use_end(); It++)
      {
         if (dyn_cast<StoreInst>(*It)) output = true;
         if (dyn_cast<LoadInst>(*It)) input = true;
      }
      std::string v;
      if (input) v = "IN";
      if (output) v += "OUT";
      return "[" + v + " param: " + std::string(Op->getName().str()) + "]";
   }

   if (dyn_cast<GlobalVariable>(Op))
      return "[arit: " + Op->getName().str() + "]";;

   switch(dyn_cast<Instruction>(Op)->getOpcode())
   {
      case Instruction::Trunc:
      case Instruction::ZExt:
      {
         const CastInst* cast = dyn_cast<CastInst>(Op);
         unsigned int srcBit = cast->getSrcTy()->getIntegerBitWidth();
         unsigned int tgtBit = cast->getDestTy()->getIntegerBitWidth();
         std::ostringstream oss;
         oss << "{cast: " << srcBit << " -> " << tgtBit << "}";
         return oss.str();
      }
      case Instruction::Store:
      {
         std::string value = "[ store: ";
         value += getMemoryString(dyn_cast<StoreInst>(Op)->getPointerOperand());
         /*value += " = {store: ";
         if (dyn_cast<Constant>(dyn_cast<StoreInst>(Op)->getValueOperand()))
            value += getMemoryString(getRealValue(dyn_cast<StoreInst>(Op)->getValueOperand()));
         else if (dyn_cast<Instruction>(dyn_cast<StoreInst>(Op)->getValueOperand()))
            value += getRealValue(dyn_cast<StoreInst>(Op)->getValueOperand())->getName().str();
         else
            assert(0 && "Not supported operand");*/
         value += "]";
         return value;
      }
      case Instruction::Load:
      {
         const Value* val = dyn_cast<LoadInst>(Op)->getPointerOperand();
         return /*Op->getName().str() + */" [ load: " + getMemoryString(val) + "]";
      }
      case Instruction::GetElementPtr:
      {
         const Value* val = dyn_cast<GetElementPtrInst>(Op)->getPointerOperand();
         return Op->getName().str() + " = {compute_addr: " + std::string(val->getName().str()) + "}";
      }
      case Instruction::Add:
      case Instruction::AShr:
      case Instruction::ICmp:
      case Instruction::Mul:
      case Instruction::SDiv:
      case Instruction::Sub:
      case Instruction::UDiv:
      {
         return "[ arit: " + getOperator(dyn_cast<Instruction>(Op)) + "]";
      }
      default:
      {
         errs() << "ERROR: DfgNode::getName(): " << dyn_cast<Instruction>(Op)->getOpcodeName() << "\n";
         assert(0);
      }
   }
   return dyn_cast<Instruction>(Op)->getOpcodeName();
}

DfgGraph::DfgGraph(const std::string& Name) : FunctionName(Name) {

}

const std::vector<DfgNode*>& DfgGraph::getNodes() const
{
   return Nodes;
}

DfgNode* DfgGraph::getNode(const Value* Op) const
{
   assert(nodeMap.find(Op) != nodeMap.end() && "missing node");
   return nodeMap.find(Op)->second;
}

unsigned int DfgGraph::getBbIdx(BasicBlock* bb) const
{
   assert(bbMap.find(bb) != bbMap.end() && "missing node");
   return bbMap.find(bb)->second;
}

BasicBlock* DfgGraph::getBasicBlock(unsigned int idx)
{
   assert(bbReverseMap.find(idx) != bbReverseMap.end() && "missing bb");
   return bbReverseMap.find(idx)->second;
}


bool DfgGraph::isNode(const Value* Op) const
{
   return nodeMap.find(Op) != nodeMap.end();
}

DfgNode* DfgGraph::getNode(const Value* Op, unsigned int Width, unsigned int bb)
{
   if (nodeMap.find(Op) != nodeMap.end()) return nodeMap.find(Op)->second;
   DfgNode* dn = new DfgNode(Op, Width);
   if (dn->Type == DfgNode::IN_PARAM || dn->Type == DfgNode::OUT_PARAM) bb = 0;
   bbNodes[bb].push_back(Op);
   Nodes.push_back(dn);
   nodeMap[Op] = dn;
   return dn;
}

const std::vector<const Value*>& DfgGraph::getBbNode(unsigned int bb) const
{
   return bbNodes.find(bb)->second;
}

const std::map<unsigned int, std::vector<const Value*> >& DfgGraph::getBbNodes() const
{
   return bbNodes;
}

unsigned int DfgGraph::getWidth(const Value* I) const
{
   if (isNode(I)) return getNode(I)->getWidth();
   assert(bitWidth.find(I) != bitWidth.end() && "Malformed bitwidth");
   return bitWidth.find(I)->second;
}
