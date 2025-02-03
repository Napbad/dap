//
// Created by napbad on 2/3/25.
//

#ifndef GLOBALINTERINFO_H
#define GLOBALINTERINFO_H
#include <llvm/IR/DerivedTypes.h>
#include <unordered_map>

namespace dap::inter_gen
{
extern std::unordered_map<llvm::PointerType *, llvm::Type *> pointerMap;
}

#endif // GLOBALINTERINFO_H
