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

    optimizer.local_optimizations();

//    optimizer.print_gen_fa();
//    optimizer.print_kill_fa();
//    optimizer.print_in_fa();
//    optimizer.print_out_fa();
//    optimizer.print_gen_ra();
//    optimizer.print_kill_ra();
//    optimizer.print_in_ra();
//    optimizer.print_out_ra();
//
    optimizer.write_to_file(new_fname);
    return 0;
}
