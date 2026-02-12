/*
 * test_optimizations.cpp - 
 *
 * Josh Meise
 * 02-09-2026
 * Description: 
 *
 */

#include <optimizer.h>
#include <iostream>

int main(int argc, char** argv) {
    std::string ifile;
    std::string ofile;
    Optimizer optimizer;

    // Check arguments.
    if (argc != 3) {
        std::cerr << "usage: ./test_optimizations <in_file.ll> <out_file.ll>\n";
        return 1;
    }

    // Extract file names.
    ifile = std::string(argv[1]);
    ofile = std::string(argv[2]);

    // Create LLVM module.
    optimizer = Optimizer(ifile);

    optimizer.optimize();

    optimizer.write_to_file(ofile);

    return 0;
}
