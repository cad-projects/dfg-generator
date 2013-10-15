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
 * Description: This header file pulls in all transformation and analysis passes for tools
 *              like opt and bugpoint that need this functionality.
 */
#ifndef CADLIB_LINKALLPASSES_H
#define CADLIB_LINKALLPASSES_H

#include <cstdlib>

namespace llvm {
  class Pass;
  class PassInfo;
  class PassRegistry;
}

llvm::Pass *createDfgGenerationPass();
llvm::Pass *createDfgPrintingPass();
llvm::Pass *createDetermineBitWidthPass();

namespace {
   struct CodesignForcePassLinking {
      CodesignForcePassLinking() {
         createDfgGenerationPass();
         createDfgPrintingPass();
         createDetermineBitWidthPass();
      }
   } CodesignForcePassLinking; // Force link by creating a global definition.
}

namespace llvm {
  class PassRegistry;
  ///Kernel analysis
  void initializeDfgGenerationPass(llvm::PassRegistry&);
  void initializeDfgPrintingPass(llvm::PassRegistry&);
  void initializeDetermineBitWidthPass(llvm::PassRegistry&);
}

#endif
