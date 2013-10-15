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
 * Description: This file defines the pass for printing of the minimal DFG (i.e., 
 *              representing only dependences between arithmetic and memory nodes)
 */
#ifndef DFGPRINTING_H
#define DFGPRINTING_H

#include "TinyXML/tinyxml.h"

#include "llvm/Pass.h"
#include "llvm/Function.h"

namespace llvm {

class DfgGraph;

  // DfgPrinting
  struct DfgPrinting : public FunctionPass {
    static char ID; // Pass identification, replacement for typeid
    DfgPrinting() : FunctionPass(ID) {}

    void printDot(Function &F);

    void printXML(Function &F);

    virtual bool runOnFunction(Function &F);

    void getAnalysisUsage(AnalysisUsage &AU) const;

    void printXmlBB(DfgGraph* graph, const std::vector<const Value*>& bbInstruction, TiXmlElement& bbNode);

    void printXmlOp(DfgGraph* graph, const Value* Op, TiXmlElement& opNode, bool depth);
  };
}

#endif
