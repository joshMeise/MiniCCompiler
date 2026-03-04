/*
 * test_assembly_gen.cpp - 
 *
 * Josh Meise
 * 03-04-2026
 * Description: 
 *
 */

#include <assembly_generator.h>
#include <fstream>
#include <iostream>
#include <string>
#include <cstdlib>
#include <llvm-c/IRReader.h>
#include <llvm-c/Core.h>

int main(int argc, char** argv) {
    std::string iname, oname;
    LLVMModuleRef m;
    char* err;
    LLVMMemoryBufferRef lmb;
    std::optional<std::unordered_map<LLVMValueRef, int>> opt;

    // Check arguments.
    if (argc != 3) {
        std::cerr << "usage: ./test_assembly_gen <infile.ll> <outfile>\n";
        return EXIT_FAILURE;
    }
    
    iname = std::string(argv[1]);
    oname = std::string(argv[2]);

    // Create LLVM module with file contents.
    if (LLVMCreateMemoryBufferWithContentsOfFile(iname.c_str(), &lmb, &err) != 0) {
        std::cerr << err << std::endl;
        LLVMDisposeMessage(err);
        return EXIT_FAILURE;
    }

    // Parse the LLVM memory buffer into IR data structures.
    if (LLVMParseIRInContext(LLVMGetGlobalContext(), lmb, &m, &err) != 0) {
        std::cerr << err << std::endl;
        LLVMDisposeMessage(err);
        LLVMDisposeMemoryBuffer(lmb);
        return EXIT_FAILURE;
    }

    opt = allocate_registers(m);

    if (!opt.has_value()) {
        std::cerr << "Failed to allocate registers.\n";
        return EXIT_FAILURE;
    }






    LLVMDisposeModule(m);
    LLVMShutdown();

    return EXIT_SUCCESS;
}
