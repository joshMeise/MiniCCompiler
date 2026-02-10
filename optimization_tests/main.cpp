/*
 * main.cpp - 
 *
 * Josh Meise
 * 02-09-2026
 * Description: 
 *
 */

#include <optimizer.h>

int main(void) {
    std::string fname = std::string("test.ll");
    std::string new_fname = std::string("test.new.ll");
    Optimizer optimizer(fname);

    optimizer.global_optimizations();

    optimizer.print_gen();

    optimizer.write_to_file(new_fname);
    return 0;
}
