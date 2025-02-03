//
// Created by napbad on 10/28/24.
//

#ifndef LLVM_UTIL_H
#define LLVM_UTIL_H

#include "parser/ASTNode.h"

#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/Type.h>

namespace dap::inter_gen
{
extern std::unordered_map<llvm::PointerType *, llvm::Type *> pointerMap;
}

namespace dap::util
{
// Returns an LLVM type based on the TypeNode
llvm::Type *typeOf(const parser::TypeNode *type, const inter_gen::InterGenContext *ctx,
                   parser::Expression *size = nullptr);

void initTypeMap(llvm::LLVMContext &llvmCtx);

void initTargets();

llvm::AllocaInst *createAllocaInst(llvm::Type *type, llvm::IRBuilder<> &builder, const std::string &name,
                                   llvm::LLVMContext *llvmContext);

llvm::Value *toBool(llvm::Value *src, inter_gen::InterGenContext *ctx);

bool basicBlockEndWithBranchStatement(const llvm::BasicBlock *bb, const inter_gen::InterGenContext *ctx);

llvm::Value *getLastValue(llvm::BasicBlock *src, inter_gen::InterGenContext *ctx);

llvm::Constant *createNullValue(llvm::LLVMContext &context, llvm::Type *type);

// /* Returns an LLVM type based on the identifier, which returns type for metadata (specially for pointer cause that
//  * llvm's pointer do not have type  info) */
// llvm::Type *typeOf_d(const parser::QualifiedNameNode &type, const inter_gen::InterGenContext *ctx, parser::Expr
// *size);

// llvm::Type *getPointerOf(llvm::Type *type, const inter_gen::InterGenContext *ctx = nullptr);

// llvm::Value *getVal(Value *src, inter_gen::InterGenContext *ctx);
} // namespace dap::util

#endif // LLVM_UTIL_H