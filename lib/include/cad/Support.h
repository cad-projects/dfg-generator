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
 * Description: This file contains support methods.
 */
#ifndef CADLIB_SUPPORT_H
#define CADLIB_SUPPORT_H

#include "llvm/ADT/OwningPtr.h"
#include "llvm/Instructions.h"
#include "llvm/Support/FormattedStream.h"
#include "llvm/Support/ToolOutputFile.h"

#include <list>
#include <set>

using namespace llvm;

namespace cadlib
{

std::string getOperator(const Instruction* I);

bool isMemoryRelated(const Value* I, std::set<const Value*>& alreadyAnalyzed, bool& isLoad, bool& isStore);

std::string getMemoryString(const Value* I);

void getMemoryUses(const Value* I, std::set<const Value*>& Uses);

const Value* getMemoryVar(const Value* I);

void getMemoryOps(const Value* I, std::list<const Value*>& operations);

const Value* getRealValue(const Value *I);

struct FormattedOutput
{
      unsigned int IndentNum;

      char OpeningChar;

      char ClosingChar;

      bool StartLine;

      formatted_raw_ostream& oss;

      FormattedOutput(formatted_raw_ostream &oss);

      ~FormattedOutput();

      void Print(const std::string& str);

      void Indent();

      void DeIndent();

      void PrintSpaces();
};

}

#endif
