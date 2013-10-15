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
 * Description: This file defines the representation of the minimal DFG (i.e.,
 *              representing only dependences between arithmetic and memory nodes)
 */
#ifndef DFG_H
#define DFG_H

#include <map>
#include <vector>
#include <string>
#include <set>
#include <tr1/tuple>

namespace llvm {

class Value;
class Instruction;
class DfgNode;
class BasicBlock;

struct DfgNode
{
   typedef enum
   {
      IN_PARAM,
      OUT_PARAM,
      INOUT_PARAM,
      LOAD,
      STORE,
      INSTRUCTION
   } Type_t;

   typedef enum
   {
      T_EDGE,
      F_EDGE,
      SWITCH_DEF_EDGE,
      SWITCH_CASE_EDGE
   } Control_t;

   const Value* Op;
   std::vector<DfgNode*> DefinitionNodes;
   std::vector<DfgNode*> UseNodes;
   typedef std::tr1::tuple<DfgNode*, Control_t, unsigned int> Condition_t;
   std::vector<Condition_t> ControlNodes;
   Type_t Type;

   DfgNode(const Value* NewOp, unsigned int width);

   std::string getName() const;

   unsigned getWidth() const;

   private:

      unsigned int Width;

};

class DfgGraph
{

   private:

      std::string FunctionName;

      std::vector<DfgNode*> Nodes;

      std::map<const Value*, DfgNode*> nodeMap;
      std::map<unsigned int, std::vector<const Value*> > bbNodes;

      std::map<const Value*, unsigned int> bitWidth;
      std::map<BasicBlock*, unsigned int> bbMap;
      std::map<unsigned int, BasicBlock*> bbReverseMap;

      friend class DfgGeneration;
   public:

      DfgGraph(const std::string& Name);

      std::string getFunctionName() const {
         return FunctionName;
      }

      DfgNode* getNode(const Value *Op, unsigned int Width, unsigned int bb);

      DfgNode* getNode(const Value* Op) const;

      bool isNode(const Value* Op) const;

      const std::vector<DfgNode*>& getNodes() const;

      const std::vector<const Value*>& getBbNode(unsigned int bb) const;

      const std::map<unsigned int, std::vector<const Value*> >& getBbNodes() const;

      unsigned int getBbIdx(BasicBlock* bb) const;

      BasicBlock* getBasicBlock(unsigned int idx);

      unsigned int getWidth(const Value*) const;

};

}

#endif
