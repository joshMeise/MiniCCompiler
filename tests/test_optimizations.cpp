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
    int ret;

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

    if ((ret = optimizer.optimize()) == -1) {
        std::cerr << "Optimization failed.\n";
        return 1;
    }

    if (ret != 0) std::cout << ret << " unassigned variable(s).\n";

    optimizer.write_to_file(ofile);

    return 0;
}
