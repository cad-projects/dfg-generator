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
 * Description: This class defines the pass for computing the minimal DFG (i.e.,
 *              representing only dependences between arithmetic and memory nodes)
 */
#ifndef DFGGENERATION_H
#define DFGGENERATION_H

#include "llvm/Pass.h"
#include "llvm/Function.h"

namespace llvm {

class DfgGraph;
class DfgNode;
class Instruction;

  // DfgGeneration
  struct DfgGeneration : public FunctionPass {

    DfgGraph* graph;

    static char ID; // Pass identification, replacement for typeid
    DfgGeneration() : FunctionPass(ID), graph(NULL) { }

    virtual bool runOnFunction(Function &F);

    DfgNode* processInstruction(DfgGraph* g, Instruction *I, unsigned int bbIdx);

    // We don't modify the program, so we preserve all analyses
    virtual void getAnalysisUsage(AnalysisUsage &AU) const;

  };
}

#endif
