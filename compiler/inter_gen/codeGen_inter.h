//
// Created by napbad on 10/24/24.
//

#ifndef CODEGEN_H
#define CODEGEN_H

#include <queue>
#include <stack>
#include <utility>

#include <llvm/ExecutionEngine/ExecutionEngine.h>
#include <llvm/IR/Function.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/LLVMContext.h>

#define LLVMCTX ctx->module->getContext()
#define MODULE ctx->module
#define BUILDER ctx->builder
namespace dap::parser
{
class ProgramNode;
class QualifiedNameNode;
} // namespace dap::parser

namespace dap::inter_gen
{

extern llvm::LLVMContext *llvmContext;

class FunctionMetaData;
class StructMetaData;
class VariableMetaData;

class ModuleMetaData;
class IncludeGraphNode;

/**
 * @brief Represents an intermediate generation block, containing a basic
 * block, return value, and local variables.
 */
class InterGenBlock
{
  public:
    llvm::BasicBlock *block{};  ///< The current basic block
    llvm::Value *returnValue{}; ///< The return value of the current block
    std::map<std::string, std::pair<llvm::Value *, VariableMetaData *>> locals{};
    std::stack<llvm::BasicBlock *> loopExitBlocks{};
    std::unordered_map<llvm::Value *, llvm::Type *> ptrValBaseTypeMapping{};
    ///< Local variables of the current block

    /**
     * @brief Constructor to initialize the basic block.
     * @param block Pointer to the basic block
     */
    explicit InterGenBlock(llvm::BasicBlock *block) : block(block)
    {
    }
};

/**
 * @brief Context class for generating intermediate representation (IR).
 */
class InterGenContext
{
    std::stack<InterGenBlock *> blocks; ///< Stack of blocks
    llvm::Function *mainFunction{};     ///< Main function
    llvm::Function *currFun = nullptr;  ///< Current function
    FunctionMetaData *currentFunMetaData = nullptr;
    bool definingStruct = false;   ///< if now defining struct
    bool definingVariable = false; ///< if now defining variable
    std::stack<bool> definingNestedIfStatement;
    bool assigning;
    llvm::Type *expectType = nullptr;

    std::stack<llvm::BasicBlock *> elseIfBlock_merge;
    std::stack<llvm::Value *> elseIfBlockValue;
    std::vector<llvm::BasicBlock *> targetMergeBlocks;
    std::vector<std::pair<llvm::BasicBlock *, llvm::Value *>> valueToMerge;
    std::string name;

  public:
    llvm::Module *module;      ///< LLVM module
    llvm::IRBuilder<> builder; ///< IR builder
    ModuleMetaData *metaData = nullptr;
    int currLine = -1;
    std::string sourcePath;
    std::string package;
    std::string fileName;

    FunctionMetaData *getFunMetaData(const std::string &name, const inter_gen::InterGenContext *ctx) const;
    std::pair<llvm::Value *, VariableMetaData *> getValWithMetadata(const std::string &name);
    void addFunctionToMetaData(FunctionMetaData *function_meta_data) const;
    std::pair<llvm::Value *, VariableMetaData *> getValWithMetadata(const parser::QualifiedNameNode *name);
    FunctionMetaData *getCurrFunMetaData() const;
    void setCurrFunMetaData(inter_gen::FunctionMetaData *funMetaData);

    void setNeedPointer(const bool cond)
    {
        assigning = cond;
    }

    bool isNeedPointer() const
    {
        return assigning;
    }

    void setName(const std::string &nameString);

    std::string getName() const;

    /**
     * @brief Constructor to initialize the module and IR builder.
     */
    explicit InterGenContext(std::string sourcePathInput);

    llvm::Value *getVal(const std::string &name);

    /**
     * @brief Get the basic block of the current block.
     * @return Pointer to the current block's basic block
     */
    llvm::BasicBlock *currBlock()
    {
        return blocks.top()->block;
    }

    /**
     * @brief Get the map of local variables of the current block.
     * @return Map of local variables of the current block
     */
    std::map<std::string, std::pair<llvm::Value *, VariableMetaData *>> &locals()
    {
        return blocks.top()->locals;
    }

    /**
     * @brief Push a new basic block onto the block stack.
     * @param block Pointer to the new basic block
     */
    void pushBlock(llvm::BasicBlock *block)
    {
        blocks.push(new InterGenBlock{block});
    }

    /**
     * @brief Check if there is a current block.
     * @return True if there is a current block, otherwise false
     */
    [[nodiscard]] bool hasBlock() const
    {
        return !blocks.empty();
    }

    /**
     * @brief Pop the top block from the block stack.
     */
    void popBlock()
    {
        const InterGenBlock *top = blocks.top();
        blocks.pop();
        delete top;
    }

    /**
     * @brief Set the return value of the current block.
     * @param value Return value
     */
    void setCurrRetVal(llvm::Value *value)
    {
        blocks.top()->returnValue = value;
    }

    /**
     * @brief Set the current function.
     * @param fun Pointer to the function
     */
    void setCurrFun(llvm::Function *fun)
    {
        currFun = fun;
    }

    /**
     * @brief Get the current function.
     * @return Pointer to the current function
     */
    [[nodiscard]] llvm::Function *getCurrFun() const
    {
        return currFun;
    }

    /**
     * @brief Get the return value of the current block.
     * @return Return value of the current block
     */
    llvm::Value *getCurrRetVal()
    {
        return blocks.top()->returnValue;
    }

    /**
     * @brief Generate IR code for the given program.
     * @param program Pointer to the program
     */
    void genIR(parser::ProgramNode *program);

    /**
     * @brief Generate executable code for the given program.
     * @param program Pointer to the program
     */
    void genExec(parser::ProgramNode *program);
    /**
     * @brief Set the main function.
     * @param fun Pointer to the main function
     */
    void setMainFun(llvm::Function *fun)
    {
        mainFunction = fun;
    }

    /**
     * @brief Get the defining struct flag.
     * @return whether context is defining struct
     */
    [[nodiscard]] bool isDefStruct() const
    {
        return definingStruct;
    }

    /**
     * @brief Set the defining struct flag.
     * @param cond New value of the defining struct flag
     */
    void setDefStruct(const bool cond)
    {
        definingStruct = cond;
    }

    InterGenBlock *topBlock()
    {
        return blocks.top();
    }

    bool retValSetFlag()
    {
        return blocks.top()->returnValue != nullptr;
    }

    void setDefiningVariable(bool cond)
    {
        definingVariable = cond;
    }

    bool isDefiningVariable() const
    {
        return definingVariable;
    }

    void addPtrValBaseTypeMapping(llvm::Value *val, llvm::Type *baseType)
    {
        if (blocks.top()) {
            blocks.top()->ptrValBaseTypeMapping.insert({val, baseType});
        }
    }

    llvm::Type *getPtrValBaseTy(llvm::Value *value)
    {
        if (blocks.top() && blocks.top()->ptrValBaseTypeMapping.contains(value)) {
            return blocks.top()->ptrValBaseTypeMapping.at(value);
        }
        // REPORT_ERROR("Value not found in ptrValBaseTypeMapping", __FILE__, __LINE__);
        return nullptr;
    }

    void registerStructMetadata(StructMetaData *metaData) const;

    std::string errMsg(const std::string &msg) const
    {
        return "Error at: " + sourcePath + std::to_string(currLine) + ": \n" + msg;
    }

    void setDefiningNestIfStatementFlag(const bool cond)
    {
        if (cond)
            definingNestedIfStatement.push(true);
        else
            definingNestedIfStatement.pop();
    }

    bool isDefiningNestIfStatement() const
    {
        return definingNestedIfStatement.top();
    }

    void addNestIfStatementResult(llvm::Value *val, llvm::BasicBlock *block)
    {
        elseIfBlock_merge.push(block);
        elseIfBlockValue.push(val);
    }

    std::pair<llvm::Value *, llvm::BasicBlock *> getNestIfStatementResult()
    {
        llvm::Value *val = elseIfBlockValue.top();
        llvm::BasicBlock *block = elseIfBlock_merge.top();
        elseIfBlockValue.pop();
        elseIfBlock_merge.pop();
        return {val, block};
    }

    void addTargetMergeBlock(llvm::BasicBlock *block)
    {
        targetMergeBlocks.push_back(block);
    }

    void addValueToBeMerged(llvm::Value *val, llvm::BasicBlock *block)
    {
        valueToMerge.emplace_back(block, val);
    }

    std::vector<std::pair<llvm::BasicBlock *, llvm::Value *>> getValueToBeMerged()
    {
        return valueToMerge;
    }

    llvm::BasicBlock *getTargetMergeBlock() const
    {
        llvm::BasicBlock *block = targetMergeBlocks.back();
        return block;
    }

    void setExpectType(llvm::Type *type)
    {
        expectType = type;
    }

    llvm::Type *getExpectType() const
    {
        return expectType;
    }
};

void interGen_oneFile(IncludeGraphNode *node);

void interGen(const std::set<IncludeGraphNode *> &map);

} // namespace dap::inter_gen

#endif // CODEGEN_H