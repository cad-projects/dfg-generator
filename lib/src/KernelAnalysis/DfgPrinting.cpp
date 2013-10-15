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
 * Description: Implementation of the methods for DFG printing.
 */
#include "DfgPrinting.h"

#include "cad/Config.h"

#include "Dfg.h"
#include "DfgGeneration.h"

#define DEBUG_TYPE "dfg-printing"
#include "llvm/Constants.h"
#include "llvm/Instructions.h"
#include "llvm/Support/raw_ostream.h"
#include "llvm/ADT/Statistic.h"
#include "llvm/ADT/StringExtras.h"

#include <vector>
#include <fstream>
#include <sstream>

using namespace llvm;
using namespace cadlib;

#define CLUSTERING 0

static cl::list<std::string> formats("format",
  cl::desc("[CAD] Specify the output format for the DFG printing"),
  cl::value_desc("\"xml\",\"dot\""));


void DfgPrinting::getAnalysisUsage(AnalysisUsage &AU) const
{
   AU.addRequired<DfgGeneration>();
   AU.setPreservesAll();
}

char DfgPrinting::ID = 0;
static const char dfg_printing_name[] = "[CAD] DFG Printing";
INITIALIZE_PASS_BEGIN(DfgPrinting, DEBUG_TYPE, dfg_printing_name, false, false)
INITIALIZE_PASS_DEPENDENCY(DfgGeneration)
INITIALIZE_PASS_END(DfgPrinting, DEBUG_TYPE, dfg_printing_name, false, false)

Pass* createDfgPrintingPass() {
   return new DfgPrinting;
}

bool DfgPrinting::runOnFunction(Function &F)
{
   if (std::find(functionNames.begin(), functionNames.end(), F.getName()) == functionNames.end())
      return false;

   errs() << "DFG Printing: #" << F.getName() << "#\n";
   if (formats.size() == 0 or std::find(formats.begin(), formats.end(), "dot") != formats.end())
      printDot(F);
   if (std::find(formats.begin(), formats.end(), "xml") != formats.end())
      printXML(F);
   errs() << "##\n\n";
   return false;
}

void DfgPrinting::printXmlOp(DfgGraph* graph, const Value* Op, TiXmlElement& opNode, bool depth)
{
   if (dyn_cast<ConstantInt>(Op))
   {
      opNode.SetValue("constant");
      opNode.SetAttribute("value", dyn_cast<ConstantInt>(Op)->getValue().toString(10, true).c_str());
      return;
   }

   if (!depth)
   {
      opNode.SetAttribute("name", Op->getName().data());
      return;
   }

   if (dyn_cast<BinaryOperator>(Op))
   {
      TiXmlElement op0("op");
      TiXmlElement op1("op");
      printXmlOp(graph, getRealValue(dyn_cast<BinaryOperator>(Op)->getOperand(0)), op0, false);
      printXmlOp(graph, getRealValue(dyn_cast<BinaryOperator>(Op)->getOperand(1)), op1, false);
      opNode.InsertEndChild(op0);
      opNode.InsertEndChild(op1);
   }
   else if (dyn_cast<LoadInst>(Op))
   {
      TiXmlElement address("address");
      const Value* op = getMemoryVar(dyn_cast<LoadInst>(Op)->getPointerOperand());
      printXmlOp(graph, op, address, false);
      printXmlOp(graph, dyn_cast<LoadInst>(Op)->getPointerOperand(), address, true);
      opNode.InsertEndChild(address);
   }
   else if (dyn_cast<StoreInst>(Op))
   {
      TiXmlElement address("address");
      const Value* op = getMemoryVar(dyn_cast<StoreInst>(Op)->getPointerOperand());
      printXmlOp(graph, op, address, false);
      printXmlOp(graph, dyn_cast<StoreInst>(Op)->getPointerOperand(), address, true);
      opNode.InsertEndChild(address);
      TiXmlElement value("value");
      const Value* val = getRealValue(dyn_cast<StoreInst>(Op)->getValueOperand());
      printXmlOp(graph, val, value, false);
      opNode.InsertEndChild(value);
   }
   else if (dyn_cast<GetElementPtrInst>(Op))
   {
      std::list<const Value*> Operations;
      const GetElementPtrInst* ptr = dyn_cast<GetElementPtrInst>(Op);
      for(GetElementPtrInst::const_op_iterator It = ptr->idx_begin(); It != ptr->idx_end(); It++)
      {
         getMemoryOps(*It, Operations);
      }
      for(std::list<const Value*>::iterator Op = Operations.begin(); Op != Operations.end(); Op++)
      {
         TiXmlElement node("op");
         if (*Op != Operations.back())
            node.SetAttribute("name", dyn_cast<Instruction>(*Op)->getName().data());
         else
            node.SetAttribute("name", "offset");
         node.SetAttribute("type", dyn_cast<Instruction>(*Op)->getOpcodeName());
         ///information about operands
         printXmlOp(graph, *Op, node, true);
         ///information about precision
         node.SetAttribute("precision", graph->getWidth(*Op));
         opNode.InsertEndChild(node);
      }
   }
   else if (dyn_cast<ICmpInst>(Op))
   {
      CmpInst::Predicate pred = dyn_cast<ICmpInst>(Op)->getSignedPredicate();
      switch(pred)
      {
         case CmpInst::ICMP_SGT:
         {
            opNode.SetAttribute("type", "cmp_sgt");
            break;
         }
         case CmpInst::ICMP_SLT:
         {
            opNode.SetAttribute("type", "cmp_slt");
            break;
         }
         default:
         {
            assert(0 && "UNSUPPORTED PREDICATE!\n");
         }
      }
      TiXmlElement op0("op");
      TiXmlElement op1("op");
      printXmlOp(graph, getRealValue(dyn_cast<ICmpInst>(Op)->getOperand(0)), op0, false);
      printXmlOp(graph, getRealValue(dyn_cast<ICmpInst>(Op)->getOperand(1)), op1, false);
      opNode.InsertEndChild(op0);
      opNode.InsertEndChild(op1);
      for(Value::const_use_iterator It = Op->use_begin(); It != Op->use_end(); It++)
      {
         if (!dyn_cast<BranchInst>(*It)) continue;
         const BranchInst* br = dyn_cast<BranchInst>(*It);
         assert(br->isConditional() && "Malformed comparison");
         opNode.SetAttribute("true_edge", graph->getBbIdx(br->getSuccessor(0)));
         opNode.SetAttribute("false_edge", graph->getBbIdx(br->getSuccessor(1)));
      }
   }
   else
   {
      errs() << "Type not supported: " << dyn_cast<Instruction>(Op)->getOpcodeName() << "\n";
      assert(0);
   }
}

void DfgPrinting::printXmlBB(DfgGraph* graph, const std::vector<const Value*>& bbInstruction, TiXmlElement& bbNode)
{
   for(unsigned int i = 0; i < bbInstruction.size(); i++)
   {
      if (!graph->isNode(bbInstruction[i]) || !dyn_cast<Instruction>(bbInstruction[i])) continue;
      ///information about operation
      TiXmlElement node("op");
      if (!dyn_cast<StoreInst>(bbInstruction[i]))
         node.SetAttribute("name", bbInstruction[i]->getName().data());
      node.SetAttribute("type", dyn_cast<Instruction>(bbInstruction[i])->getOpcodeName());
      ///information about operands
      printXmlOp(graph, bbInstruction[i], node, true);
      ///information about precision
      node.SetAttribute("precision", graph->getWidth(bbInstruction[i]));
      bbNode.InsertEndChild(node);
   }
}

void DfgPrinting::printXML(Function &F)
{
   errs() << " - xml format\n";
   DfgGeneration& DG = getAnalysis<DfgGeneration>();
   DfgGraph* graph =  DG.graph;
   if (!graph) return;

   std::string fileName = graph->getFunctionName()+".xml";
   TiXmlDocument doc(fileName.c_str());
   TiXmlElement root("FASTER_XML");

   TiXmlElement application("application");

   TiXmlElement function("function");
   function.SetAttribute("name", graph->getFunctionName().c_str());

   const std::map<unsigned int, std::vector<const Value*> >& bbNodes = graph->getBbNodes();

   TiXmlElement Interface("interface");
   TiXmlNode* firstParameter = 0;
   ///first, I write down the in/out streams (BB=0)
   for(unsigned int p = 0; p < bbNodes.find(0)->second.size(); p++)
   {
      const Value* par = bbNodes.find(0)->second[p];
      std::string type = par->getType()->isPointerTy() ? "stream" : "parameter";
      TiXmlElement Parameter(type.c_str());
      Parameter.SetAttribute("name", par->getName().data());
      Parameter.SetAttribute("width", graph->getWidth(par));
      if (par->getType()->isPointerTy())
      {
         if (graph->getNode(par)->Type == DfgNode::IN_PARAM)
            Parameter.SetAttribute("direction", "IN");
         else if (graph->getNode(par)->Type == DfgNode::OUT_PARAM)
            Parameter.SetAttribute("direction", "OUT");
         if (firstParameter)
            Interface.InsertBeforeChild(firstParameter, Parameter);
         else
            Interface.InsertEndChild(Parameter);
      }
      else
      {
         if (firstParameter)
            Interface.InsertAfterChild(firstParameter, Parameter);
         else
            firstParameter = Interface.InsertEndChild(Parameter);
      }
   }
   function.InsertEndChild(Interface);

   TiXmlElement dfg("dfg");
   for(std::map<unsigned int, std::vector<const Value*> >::const_iterator bb = bbNodes.begin(); bb != bbNodes.end(); bb++)
   {
      if (bb->first == 0) continue;
      TiXmlElement bbNode("basic_block");
      bbNode.SetAttribute("id", bb->first);
      printXmlBB(graph, bb->second, bbNode);
      dfg.InsertEndChild(bbNode);
   }

   function.InsertEndChild(dfg);

   application.InsertEndChild(function);
   root.InsertEndChild(application);
   doc.InsertEndChild(root);
   doc.SaveFile();
}

void DfgPrinting::printDot(Function &F)
{
   errs() << " - dot format\n";
   DfgGeneration& DG = getAnalysis<DfgGeneration>();
   DfgGraph* graph =  DG.graph;
   if (!graph) return;

   std::string functionName = graph->getFunctionName();

   std::ostringstream oss;
   oss << "digraph G {\n";
   const std::vector<DfgNode*>& Nodes = graph->getNodes();
   std::map<DfgNode*, unsigned int> reverseMap;
   const std::map<unsigned int, std::vector<const Value*> >& bbNodes = graph->getBbNodes();
   unsigned int cnt = 0;
   for(std::map<unsigned int, std::vector<const Value*> >::const_iterator b = bbNodes.begin(); b != bbNodes.end(); b++)
   {
#if CLUSTERING
      if (b->first > 0)
      {
         oss << "subgraph cluster_" << b->first << " {\n";
         oss << " style=filled;\n";
         oss << " color=\"#eeeeee\";\n";
      }
#endif
      for(unsigned int i = 0; i < b->second.size(); i++, cnt++)
      {
         DfgNode* dNode = graph->getNode(b->second[i]);
         oss << cnt << "[style=filled,color=white,label=\"" << dNode->getName();
         if (!dyn_cast<CmpInst>(dNode->Op)) oss << "\\nwidth = " + utostr(graph->getWidth(dNode->Op));
         oss << "\"";
         if (dNode->Type == DfgNode::IN_PARAM)
            oss << ", shape=\"invhouse\", color=\"gray\"";
         if (dNode->Type == DfgNode::OUT_PARAM)
            oss << ", shape=\"house\", color=\"gray\"";
         if (dNode->Type == DfgNode::LOAD)
            oss << ", shape=\"box\", color=\"green\"";
         if (dNode->Type == DfgNode::STORE)
            oss << ", shape=\"box\", color=\"yellow\"";
         oss << "];\n";
         reverseMap[dNode] = cnt;
      }
#if CLUSTERING
      if (b->first > 0)
      {
         oss << "}\n";
      }
      else
      {
         oss << "{rank=same;";
         for(unsigned int i = 0; i < b->second.size(); i++)
         {
            DfgNode* dNode = graph->getNode(b->second[i]);
            if (dNode->Type == DfgNode::IN_PARAM) oss << " " << reverseMap[dNode];
         }
         oss << "}\n";
      }
#endif
   }

   for(unsigned int i = 0; i < Nodes.size(); i++)
   {
      const std::vector<DfgNode*>& Uses = Nodes[i]->UseNodes;
      for(unsigned int u = 0; u < Uses.size(); u++)
      {
         if (Uses[u]->Type == DfgNode::OUT_PARAM && Nodes[i]->Type == DfgNode::STORE)
            oss << reverseMap[Nodes[i]] << "->" << reverseMap[Uses[u]];
         else
            oss << reverseMap[Uses[u]] << "->" << reverseMap[Nodes[i]];
         oss << "[label=\"" << Uses[u]->Op->getName().str() << "\"]";
         oss << ";\n";
      }

      const std::vector<DfgNode::Condition_t>& ControlNodes = Nodes[i]->ControlNodes;
      for(unsigned int u = 0; u < ControlNodes.size(); u++)
      {
         DfgNode* src = std::tr1::get<0>(ControlNodes[u]);
         oss << reverseMap[src] << "->" << reverseMap[Nodes[i]];
         oss << "[";
         if(std::tr1::get<1>(ControlNodes[u]) == DfgNode::T_EDGE)
            oss << "label=\"T\", color=\"blue\"";
         else if(std::tr1::get<1>(ControlNodes[u]) == DfgNode::F_EDGE)
            oss << "label=\"F\", color=\"red\"";
         else
            assert(0 && "Condition not yet supported");
         oss << "];\n";
      }
   }
   oss << "}\n";

   std::ofstream file((functionName + ".dot").c_str());
   file << oss.str();
   file.close();
}
