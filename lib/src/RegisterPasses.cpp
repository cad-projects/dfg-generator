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
 * Description: Add the passes to the dynamic library
 */
#include "cad/LinkAllPasses.h"
#include "cad/RegisterPasses.h"

#include "llvm/Analysis/Passes.h"
#include "llvm/Analysis/CFGPrinter.h"
#include "llvm/InitializePasses.h"
#include "llvm/PassManager.h"
#include "llvm/PassRegistry.h"
#include "llvm/Support/CommandLine.h"
#include "llvm/Transforms/IPO/PassManagerBuilder.h"

#include <string>

using namespace llvm;

static void initializeCodesignPasses(PassRegistry &Registry) {
   ///Kernel analysis
   initializeDfgGenerationPass(Registry);
   initializeDfgPrintingPass(Registry);
   initializeDetermineBitWidthPass(Registry);
}

namespace {
// Statically register all passes such that they are available after
// loading the module.
class StaticInitializer {

public:
    StaticInitializer() {
      PassRegistry &Registry = *PassRegistry::getPassRegistry();
      initializeCodesignPasses(Registry);
    }
};
} // end of anonymous namespace.

static StaticInitializer InitializeEverything;

static void registerCodesignPreoptPasses(llvm::PassManagerBase &PM) {

}

static void registerCodesignPasses(llvm::PassManagerBase &PM) {
  registerCodesignPreoptPasses(PM);
}

static
void registerCodesignEarlyAsPossiblePasses(const llvm::PassManagerBuilder &Builder,
                                        llvm::PassManagerBase &PM) {

  registerCodesignPasses(PM);
}

static void registerCodesignOptLevel0Passes(const llvm::PassManagerBuilder &,
                                         llvm::PassManagerBase &PM) {
  registerCodesignPreoptPasses(PM);
}


static llvm::RegisterStandardPasses
PassRegister(llvm::PassManagerBuilder::EP_EarlyAsPossible,
             registerCodesignEarlyAsPossiblePasses);
static llvm::RegisterStandardPasses
PassRegisterPreopt(llvm::PassManagerBuilder::EP_EnabledOnOptLevel0,
                  registerCodesignOptLevel0Passes);
