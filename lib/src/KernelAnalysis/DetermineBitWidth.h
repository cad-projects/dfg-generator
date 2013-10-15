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
 * Description: This class defines the pass for determine operations' bitwidth
 */
#ifndef DETERMINEBITWIDTH_H
#define DETERMINEBITWIDTH_H

#include "llvm/Pass.h"
#include "llvm/Function.h"

#include <set>
#include <map>

namespace llvm {

// DetermineBitWidth
struct DetermineBitWidth : public FunctionPass {
      static char ID; // Pass identification, replacement for typeid
      DetermineBitWidth() : FunctionPass(ID) {}

      virtual bool runOnFunction(Function &F);

      void getAnalysisUsage(AnalysisUsage &AU) const;

      bool processInstruction(const Value* I);

      std::map<std::string, unsigned int> parameterSize;
      void parseConfig(const std::string& funName, const std::string &configFile);

      unsigned int getDataSize(const Value *I, const Type *Ty, bool checkBitwidth);
      Type* changeDataSize(Value* I, Type *Ty, unsigned int Size);

      unsigned int getBitWidth(const Value *I) const;

      bool hasBitWidth(const Value *I) const;

   private:

      std::map<const Value*, unsigned int> bitWidth;

};
}

#endif
