/*
 * assembly_generator.h - 
 *
 * Josh Meise
 * 03-03-2026
 * Description: 
 *
 */

#pragma once
#include <llvm-c/Core.h>
#include <unordered_map>
#include <optional>

std::optional<std::unordered_map<LLVMValueRef, int>> allocate_registers(LLVMModuleRef m);

