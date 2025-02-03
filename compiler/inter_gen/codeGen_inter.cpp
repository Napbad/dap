// //
// // Created by napbad on 10/24/24.
// //

#include <fstream>
#include <iostream>

#include <llvm/Bitstream/BitstreamReader.h>
#include <llvm/ExecutionEngine/SectionMemoryManager.h>
#include <llvm/IR/BasicBlock.h>
#include <llvm/IR/DerivedTypes.h>
#include <llvm/IR/IRBuilder.h>
#include <llvm/IR/IRPrintingPasses.h>
#include <llvm/IR/Instructions.h>
#include <llvm/IR/LLVMContext.h>
#include <llvm/IR/LegacyPassManager.h>
#include <llvm/IR/Module.h>
#include <llvm/IR/Type.h>
#include <llvm/IR/Verifier.h>
#include <llvm/IRReader/IRReader.h>
#include <llvm/MC/MCContext.h>
#include <llvm/MC/TargetRegistry.h>
#include <llvm/Pass.h>
#include <llvm/Support/FileSystem.h>
#include <llvm/Support/TargetSelect.h>
#include <llvm/Target/TargetMachine.h>
#include <llvm/Target/TargetOptions.h>
#include <llvm/TargetParser/Host.h>
#include <optional>

#include "codeGen_inter.h"

#include "check/variableCheck.h"
#include "common/config.h"
#include "metadata.h"
#include "parser/ASTNode.h"
#include "parser/parser.hpp"
#include "preprocessing/includeAnaylize.h"
#include "utilities/file_util.h"
#include "utilities/llvm_util.h"
#include "utilities/log_util.h"
#include "utilities/name_format_util.h"
#include "utilities/string_util.h"

#include <parser/parserMain.h>

namespace dap
{

using llvm::BasicBlock;
using llvm::ConstantInt;
using llvm::Function;
using llvm::Module;
using llvm::Type;
using llvm::Value;

namespace parser
{
using std::map;
using std::string;

Value *getValueFromOneName(inter_gen::InterGenContext *ctx, const std::string &name)
{
    Value *val = ctx->getVal(name);

    if (isa<AllocaInst>(val)) {
        auto allocaInst = dyn_cast<AllocaInst>(val);
        if (ctx->isNeedPointer()) {
            return allocaInst;
        }

        return BUILDER.CreateLoad(allocaInst->getAllocatedType(), allocaInst);
    }

    return val;
}

Value *getValFromStructValue(inter_gen::InterGenContext *ctx, Value *structValue, const std::string &structFieldName)
{
    if (!structValue) {
#ifdef D_DEBUG
        util::logErr("getValFromStructValue: structValue is nullptr", ctx, __FILE__, __LINE__);
#else
        util::logErr("getValFromStructValue: structValue is nullptr", ctx);
#endif
        return nullptr;
    }

    Type *structType;
    if (isa<AllocaInst>(structValue)) {
        structType = dyn_cast<AllocaInst>(structValue)->getAllocatedType();
    } else {
        structType = dyn_cast<StructType>(structValue->getType());
    }

    if (!structType) {
#ifdef D_DEBUG
        util::logErr("getValFromStructValue: structType is nullptr", ctx, __FILE__, __LINE__);
#else
        util::logErr("getValFromStructValue: structType is nullptr", ctx);
#endif
        return nullptr;
    }

    const auto structField = structType->getStructName();
    const inter_gen::StructMetaData *structMetaData = ctx->metaData->getStruct(structField.str());

    // std::vector<Value*>
    if (ctx->isNeedPointer()) {
        return BUILDER.CreateInBoundsGEP(
            structType, structValue,
            {ConstantInt::get(LLVMCTX, APInt(32, 0)),
             ConstantInt::get(LLVMCTX, APInt(32, structMetaData->getFieldIndex(structFieldName)))});
    }
    // return BUILDER.CreateLoad(structMetaData->getFieldType(structFieldName), BUILDER.CreateInBoundsGEP(structType,
    //         structValue,
    //         {ConstantInt::get(LLVMCTX, APInt(32, 0)),
    //             ConstantInt::get(LLVMCTX, APInt(32, structMetaData->getFieldIndex(structFieldName)))}));
    // structValue = BUILDER.CreateGEP(structType, structValue, {ConstantInt::get(LLVMCTX, APInt(32, 0))});
    return BUILDER.CreateInBoundsGEP(
        structType, structValue,
        {ConstantInt::get(LLVMCTX, APInt(32, 0)),
         ConstantInt::get(LLVMCTX, APInt(32, structMetaData->getFieldIndex(structFieldName)))});
    // return BUILDER.CreateExtractValue(structValue, {structMetaData->getFieldIndex(structFieldName)} /* TODO: fill
    // name here*/);
}

// QualifiedNameNode is considered as an initialValue which represent the variable
Value *QualifiedNameNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    // sync the context
    ctx->currLine = this->lineNum;

    // get the value from exist variables
    Value *value;

    // only one name
    if (name_parts->size() == 1) {
        value = getValueFromOneName(ctx, name_parts->front());
        return value;
    }

    // struct name
    bool isNeedPointer = ctx->isNeedPointer();
    ctx->setNeedPointer(true);
    Value *structValue = getValueFromOneName(ctx, name_parts->front());
    if (!structValue) {
        return nullptr;
    }

    string firstName = *name_parts->begin();
    name_parts->erase(name_parts->begin());

    for (int i = 0; i < name_parts->size(); i++) {
        if (i == name_parts->size() - 1) {
            ctx->setNeedPointer(isNeedPointer);
        }

        // update the structValue, wait for the next time's update
        structValue = getValFromStructValue(ctx, structValue, name_parts->at(i));
    }

    name_parts->insert(name_parts->begin(), firstName);
    value = structValue;

    // return it
    return value;
}

Value *StringNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    // sync the context
    ctx->currLine = this->lineNum;

    // generate the value
    Value *value = ConstantDataArray::getString(LLVMCTX, this->stringValue, true);

    // return it
    return value;
}

Value *IntegerNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    // sync the context
    ctx->currLine = this->lineNum;

    // generate the value
    Value *value;
    //
    // if (Type* expectType = ctx->getExpectType()) {
    //     value = ConstantInt::get(expectType, APInt(expectType->getIntegerBitWidth(),
    //     static_cast<uint64_t>(intValue.unsignedVal), false)); return value;
    // }

    // Determine the LLVM type and create the constant integer
    switch (intType) {
    case CHAR:
    case BYTE:
        if (isSigned) {
            value = ConstantInt::get(LLVMCTX, APInt(8, static_cast<int8_t>(intValue.charVal), true));
        } else {
            value = ConstantInt::get(LLVMCTX, APInt(8, intValue.unsignedCharVal, false));
        }
        break;
    case SHORT:
        if (isSigned) {
            value = ConstantInt::get(LLVMCTX, APInt(16, intValue.shortVal, true));
        } else {
            value = ConstantInt::get(LLVMCTX, APInt(16, intValue.unsignedShortVal, false));
        }
        break;
    case INT:
        if (isSigned) {
            value = ConstantInt::get(LLVMCTX, APInt(32, intValue.signedVal, true));
        } else {
            value = ConstantInt::get(LLVMCTX, APInt(32, static_cast<uint64_t>(intValue.unsignedVal), false));
        }
        break;
    case LONG:
        if (isSigned) {
            value = ConstantInt::get(LLVMCTX, APInt(64, intValue.longVal, true));
        } else {
            value = ConstantInt::get(LLVMCTX, APInt(64, intValue.unsignedLongVal, false));
        }
        break;
    case LONG_LONG:
        if (isSigned) {
            value = ConstantInt::get(LLVMCTX, APInt(64, intValue.longLongVal, true));
        } else {
            value = ConstantInt::get(LLVMCTX, APInt(64, intValue.unsignedLongLongVal, false));
        }
        break;
    default:
        value = ConstantInt::get(LLVMCTX, APInt(32, intValue.longLongVal, true));
        // throw std::runtime_error("Unsupported integer type for code generation");
    }

    // return it
    return value;
}

Value *FloatNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    // sync the context
    ctx->currLine = this->lineNum;

    // generate the value

    // Convert string value to double
    const double floatValue = std::stod(this->getValue());

    // Create APFloat from double
    const APFloat apFloatValue(floatValue);

    // generate the value
    Value *value = ConstantFP::get(LLVMCTX, apFloatValue);

    // return it
    return value;
}

void importFile(inter_gen::InterGenContext *ctx, const ProgramNode *program)
{
    ctx->currLine = program->lineNum;

    for (const auto path : *program->importedPackages) {
        const std::string target = util::getStrFromVec(*path->packageName->name_parts, ".");
        if (!inter_gen::moduleMap->contains(target)) {
            util::logErr("error at: " + ctx->sourcePath + ":" + std::to_string(ctx->currLine) +
                             " \nInclude statement failed: " + target,
                         ctx, __FILE__, __LINE__);
            return;
        }

        for (const auto moduleMetaData = inter_gen::moduleMap->at(target);
             const auto &[functionName, function] : moduleMetaData->getFunctions()) {
            if (!ctx->metaData->getFunction(function->getName())) {
                Function::Create(function->getType(), Function::ExternalLinkage, function->getName(), MODULE);
                ctx->metaData->addFunction(function);
            }
        }
    }
}

Value *ProgramNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    // get the package and set the package directory
    ctx->currLine = this->lineNum;

    util::create_package_dir(util::getStrFromVec(*packageName->name_parts, "."));

    importFile(ctx, this);

    for (const Statement *stmt : *statements) {
        stmt->codeGen(ctx);
    }
    // Generate code for program statements
    return nullptr;
}

std::pair<bool, Type *> detectType(const TypeNode *typeNode, const VariableDeclarationNode *node,
                                   inter_gen::InterGenContext *ctx)
{
    Type *varType;
    if (typeNode == nullptr) {
        // auto detect type
        if (node->initialValue == nullptr) {
            varType = nullptr;
            return {false, varType};
        }
        // TODO: precise type detect, pointer to some type, array of some type
        const Value *expressionValue = node->initialValue->codeGen(ctx);
        varType = expressionValue->getType();
        return {true, varType};
    }

    varType = util::typeOf(node->type, ctx);
    if (!varType) {
        varType = nullptr;
#ifdef D_DEBUG
        util::logErr("unknown type: " + node->type->getName(), ctx, __FILE__, __LINE__);
#else
        util::logErr("unknown type: " + node->type->getName(), ctx);
#endif
        return {false, varType};
    }
    return {true, varType};
}

Value *ArrayExpressionNode::codeGen(inter_gen::InterGenContext *ctx) const
{

    // sync the inter gen context
    ctx->currLine = this->lineNum;

    Value *arrayBase = expression->codeGen(ctx);

    bool needPointer = ctx->isNeedPointer();
    ctx->setNeedPointer(false);
    Value *indexValue = index->codeGen(ctx);
    ctx->setNeedPointer(needPointer);

    const std::vector<Value *> indices = {
        // ConstantInt::get(Type::getInt32Ty(LLVMCTX), 0),
        indexValue // Index
    };

    if (arrayBase && indexValue) {
        if (!arrayBase->getType()->isArrayTy() && !arrayBase->getType()->isPointerTy()) {
#ifdef D_DEBUG
            util::logErr("not support type to get value on offset", ctx, __FILE__, __LINE__);
#else
            util::logErr("not support type to get value on offset", ctx);
#endif
        }

        if (arrayBase->getType()->isArrayTy()) {
            auto arrayType = dyn_cast<ArrayType>(arrayBase->getType());
            Type *elementType = arrayType->getElementType();
            Value *elementPointer = BUILDER.CreateGEP(elementType, arrayBase, indices, "array_access");
            if (needPointer) {
                return elementPointer;
            }
            return BUILDER.CreateLoad(elementType, elementPointer, "array_access_value");
        }

        if (arrayBase->getType()->isPointerTy()) {
            auto pointerType = dyn_cast<PointerType>(arrayBase->getType());
            if (inter_gen::pointerMap.contains(pointerType)) {
                Value *elementPointer = BUILDER.CreateGEP(inter_gen::pointerMap[pointerType], arrayBase, indexValue,
                                                          "pointer_offset_access");
                if (needPointer) {
                    return elementPointer;
                }
                return BUILDER.CreateLoad(inter_gen::pointerMap[pointerType], elementPointer,
                                          "pointer_offset_access_value");
            }
            // Type * elementType = pointerType->getScalarType();
            return BUILDER.CreateGEP(IntegerType::get(LLVMCTX, 32), arrayBase, indices, "array_access");
        }
    }

    return nullptr;
}

Value *VariableDeclarationNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    // sync the context position
    ctx->currLine = this->lineNum;
    ctx->setDefiningVariable(true);

    // find the type of variable
    Type *varType = nullptr;

    const auto [typeDetectedSuccess, detectedType] = detectType(type, this, ctx);
    if (!typeDetectedSuccess) {
#ifdef D_DEBUG
        util::logErr("can not detect type [no initial initialValue]: " + variableName->getName(), ctx, __FILE__,
                     __LINE__);
#else
        util::logErr("can not detect type [no initial initialValue]: " + variableName->getName(), ctx);
#endif
        return nullptr;
    }
    varType = detectedType;

    ctx->setExpectType(varType);

    // generate the variable
    AllocaInst *allocaVar = util::createAllocaInst(varType, BUILDER, this->variableName->getName(), &LLVMCTX);

    auto varMetaData = new inter_gen::VariableMetaData(variableName->getName(), varType, mutable_, nullable_, ctx);
    ctx->locals().emplace(this->variableName->getName(),
                          std::pair<Value *, inter_gen::VariableMetaData *>(allocaVar, varMetaData));

#ifdef D_DEBUG
    util::logInfo("Variable Declaration: " + this->variableName->getName(), ctx, __FILE__, __LINE__);
#endif

    if (this->variableGenerated) {
        BUILDER.CreateStore(this->variableGenerateValue, allocaVar, false);
    }

    if (this->initialValue) {
        Value *initialValueGen = initialValue->codeGen(ctx);

        if (!initialValueGen) {
            return nullptr;
        }

        BUILDER.CreateStore(initialValueGen, allocaVar, false);
    }

    ctx->setDefiningVariable(false);
    ctx->setExpectType(nullptr);
    // return the value
    return allocaVar;
}

Value *FunctionDeclarationNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    // sync the context of the node
    ctx->currLine = this->lineNum;

    // get the function declaration info
    auto params = std::vector<Type *>();
    if (this->parameterList) {
        for (const auto param : *this->parameterList) {
            const auto node = dynamic_cast<VariableDeclarationNode *>(param);
            params.push_back(util::typeOf(node->type, ctx));
        }
    }

    // generate the function info
    const auto type = FunctionType::get(util::typeOf(this->returnType, ctx), params, false);
    Function *function = Function::Create(type, Function::ExternalLinkage, 0, this->name, MODULE);
    const auto funMetaData = new inter_gen::FunctionMetaData(this->name, type, ctx);

    int argIndex = 0;
    for (auto &arg : function->args()) {
        arg.setName(this->parameterList->at(argIndex)->variableName->getName());
        ++argIndex;
    }
    ctx->metaData->addFunction(funMetaData);

    // generate the basic block
    if (!this->block) {
        return function;
    }

    // generate the block statements
    ctx->setCurrFun(function);
    ctx->setCurrFunMetaData(funMetaData);

    const auto basicBlock = BasicBlock::Create(LLVMCTX, util::getFunBasicBlockName(&this->name), function);
    ctx->pushBlock(basicBlock);
    BUILDER.SetInsertPoint(basicBlock);

    for (const auto stmt : *this->block) {
        stmt->codeGen(ctx);
    }
#ifdef D_DEBUG
    util::logInfo("Function Declaration: " + this->name, ctx, __FILE__, __LINE__);
#endif // D_DEBUG

    // register the function
    ctx->addFunctionToMetaData(funMetaData);

    // pop the block
    ctx->popBlock();

    if (ctx->package == "main" && function->getName() == "main") {
        ctx->setMainFun(function);
    }

    // delete the function generate contexts
    ctx->setCurrFun(nullptr);
    ctx->setCurrFunMetaData(nullptr);

    return function;
}

// Value *FunctionCallExpressionNode::codeGen(inter_gen::InterGenContext *ctx) const
// {
//     ctx->currLine = this->lineNum;
//     return nullptr;
// }

Value *StructDeclarationNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    ctx->currLine = this->lineNum;
    ctx->setDefStruct(true);

    auto *structMetaData = new inter_gen::StructMetaData(ctx, name->getName());
    ctx->registerStructMetadata(structMetaData);

    // define the type of the struct
    std::vector<Type *> fieldTypes;
    fieldTypes.reserve(structMemberList->size());

    // generate the types to fieldTypes and metaData
    for (const auto field : *structMemberList) {
        fieldTypes.push_back(util::typeOf(field->type, ctx));
        structMetaData->addField(field->variableName->getName(),
                                 new inter_gen::VariableMetaData(field->variableName->getName(), fieldTypes.back(),
                                                                 field->mutable_, field->nullable_));
    }

    StructType *structType = StructType::create(LLVMCTX, fieldTypes, name->getName());

    structMetaData->setStructType(structType);

    return nullptr;
}

Value *BinaryExpressionNode::codeGen(inter_gen::InterGenContext *ctx) const
{

    ctx->currLine = this->lineNum;
    // Generate code for left-hand side and right-hand side Expressions
    const bool needPointer = ctx->isNeedPointer();

    if (operatorType == ASSIGN) {
        ctx->setNeedPointer(true);
    }
    Value *lhsVal = left->codeGen(ctx);
    ctx->setNeedPointer(needPointer);
    Value *rhsVal = right->codeGen(ctx);

    // Check for null results from the code generation
    if (!lhsVal || !rhsVal) {
        return nullptr;
    }

    auto lhsType = lhsVal->getType();
    if (isa<AllocaInst>(lhsVal)) {
        lhsType = (dyn_cast<AllocaInst>(lhsVal))->getAllocatedType();
    }

    auto rhsType = rhsVal->getType();
    if (isa<AllocaInst>(rhsVal)) {
        rhsType = (dyn_cast<AllocaInst>(rhsVal))->getAllocatedType();
    }
    // Check if both expressions are of the same type
    if (lhsType != rhsType && operatorType != ASSIGN) {
        if (llvm::isa<IntegerType>(lhsType) && llvm::isa<IntegerType>(rhsType)) {
            if (lhsType->getIntegerBitWidth() > rhsType->getIntegerBitWidth()) {
                rhsVal = BUILDER.CreateSExtOrTrunc(rhsVal, lhsType, "sext_or_trunc");
            } else if (lhsType->getIntegerBitWidth() < rhsType->getIntegerBitWidth()) {
                lhsVal = BUILDER.CreateSExtOrTrunc(lhsVal, rhsType, "sext_or_trunc");
            } else {
                lhsVal = BUILDER.CreateSExtOrTrunc(lhsVal, rhsType, "sext_or_trunc");
            }
        } else if (lhsType->isPointerTy() && rhsType->isIntegerTy()) {
            rhsVal = BUILDER.CreateIntToPtr(rhsVal, lhsType, "int_to_ptr");
            rhsType = lhsType;
        } else if (lhsType->isIntegerTy() && rhsType->isPointerTy()) {
            lhsVal = BUILDER.CreatePtrToInt(lhsVal, rhsType, "ptr_to_int");
            lhsType = rhsType;
        } else {
#ifdef D_DEBUG
            util::logErr("Type mismatch in binary Expression", ctx, __FILE__, __LINE__);
#else
            util::logErr("Type mismatch in binary Expression", ctx);
#endif

            return nullptr;
        }
    }

    const bool isFloatingPoint = lhsType->isFloatingPointTy();

    switch (operatorType) {
        // Arithmetic operators
    case PLUS:
        return isFloatingPoint ? BUILDER.CreateFAdd(lhsVal, rhsVal, "addtmp")
                               : BUILDER.CreateAdd(lhsVal, rhsVal, "addtmp");
    case MINUS:
        return isFloatingPoint ? BUILDER.CreateFSub(lhsVal, rhsVal, "subtmp")
                               : BUILDER.CreateSub(lhsVal, rhsVal, "subtmp");
    case MUL:
        return isFloatingPoint ? BUILDER.CreateFMul(lhsVal, rhsVal, "multmp")
                               : BUILDER.CreateMul(lhsVal, rhsVal, "multmp");
    case DIV:
        return isFloatingPoint ? BUILDER.CreateFDiv(lhsVal, rhsVal, "divtmp")
                               : BUILDER.CreateSDiv(lhsVal, rhsVal, "divtmp");
    case MOD:
        return isFloatingPoint ? BUILDER.CreateFRem(lhsVal, rhsVal, "modtmp")
                               : BUILDER.CreateSRem(lhsVal, rhsVal, "modtmp");

    // Comparison operators
    case LESS_THAN:
        return isFloatingPoint ? BUILDER.CreateFCmpOLT(lhsVal, rhsVal, "ltcmp")
                               : BUILDER.CreateICmpSLT(lhsVal, rhsVal, "ltcmp");
    case GREATER_THAN:
        return isFloatingPoint ? BUILDER.CreateFCmpOGT(lhsVal, rhsVal, "gtcmp")
                               : BUILDER.CreateICmpSGT(lhsVal, rhsVal, "gtcmp");
    case LESS_THAN_EQUAL:
        return isFloatingPoint ? BUILDER.CreateFCmpOLE(lhsVal, rhsVal, "lecmp")
                               : BUILDER.CreateICmpSLE(lhsVal, rhsVal, "lecmp");
    case GREATER_THAN_EQUAL:
        return isFloatingPoint ? BUILDER.CreateFCmpOGE(lhsVal, rhsVal, "gecmp")
                               : BUILDER.CreateICmpSGE(lhsVal, rhsVal, "gecmp");
    case EQUAL:
        return isFloatingPoint ? BUILDER.CreateFCmpOEQ(lhsVal, rhsVal, "eqcmp")
                               : BUILDER.CreateICmpEQ(lhsVal, rhsVal, "eqcmp");
    case NOT_EQUAL:
        return isFloatingPoint ? BUILDER.CreateFCmpONE(lhsVal, rhsVal, "necmp")
                               : BUILDER.CreateICmpNE(lhsVal, rhsVal, "necmp");

    // Logical operators
    case AND:
        return BUILDER.CreateAnd(lhsVal, rhsVal, "andtmp");
    case OR:
        return BUILDER.CreateOr(lhsVal, rhsVal, "ortmp");
    case XOR:
        return BUILDER.CreateXor(lhsVal, rhsVal, "xortmp");

    // Bitwise operators
    case SHIFT_LEFT:
        return BUILDER.CreateShl(lhsVal, rhsVal, "lshifttmp");
    case SHIFT_RIGHT:
        return BUILDER.CreateAShr(lhsVal, rhsVal, "urshifttmp");

    // Assignment operators
    case ASSIGN:
        return BUILDER.CreateStore(rhsVal, lhsVal, false);
    case ADD_ASSIGN: {
        Value *result = isFloatingPoint ? BUILDER.CreateFAdd(lhsVal, rhsVal, "addassigntmp")
                                        : BUILDER.CreateAdd(lhsVal, rhsVal, "addassigntmp");
        return BUILDER.CreateStore(result, lhsVal);
    }
    case MINUS_ASSIGN: {
        Value *result = isFloatingPoint ? BUILDER.CreateFSub(lhsVal, rhsVal, "subassigntmp")
                                        : BUILDER.CreateSub(lhsVal, rhsVal, "subassigntmp");
        return BUILDER.CreateStore(result, lhsVal);
    }
    case MUL_ASSIGN: {
        Value *result = isFloatingPoint ? BUILDER.CreateFMul(lhsVal, rhsVal, "mulassigntmp")
                                        : BUILDER.CreateMul(lhsVal, rhsVal, "mulassigntmp");
        return BUILDER.CreateStore(result, lhsVal);
    }
    case DIV_ASSIGN: {
        Value *result = isFloatingPoint ? BUILDER.CreateFDiv(lhsVal, rhsVal, "divassigntmp")
                                        : BUILDER.CreateSDiv(lhsVal, rhsVal, "divassigntmp");
        return BUILDER.CreateStore(result, lhsVal);
    }
    case MOD_ASSIGN: {
        Value *result = isFloatingPoint ? BUILDER.CreateFRem(lhsVal, rhsVal, "modassigntmp")
                                        : BUILDER.CreateSRem(lhsVal, rhsVal, "modassigntmp");
        return BUILDER.CreateStore(result, lhsVal);
    }

    default:
#ifdef D_DEBUG
        util::logErr("Unknown binary operator", ctx, __FILE__, __LINE__);
#else
        util::logErr("Unknown binary operator", ctx);
#endif
        return nullptr;
    }
}

Value *UnaryExpressionNode::codeGen(inter_gen::InterGenContext *ctx) const
{

    ctx->currLine = this->lineNum;
    // Generate code for the initialValue of the unary operation
    Value *operVal = expression->codeGen(ctx);

    Value *res;
    Value *var;
    Value *varPtr;
    // Handle increment or decrement
    switch (operatorType) {
    case DECREMENT:
        operVal = BUILDER.CreateSub(operVal, ConstantInt::get(LLVMCTX, APInt(1, 1)), "decrease"); // Decrement
        return operVal;
    case INCREMENT:
        operVal = BUILDER.CreateAdd(operVal, ConstantInt::get(operVal->getType(), 1), "increase"); // Increment
        return operVal;
    case MUL:
        //
        // if (llvm::isa<PointerType>(operVal->getType())) {
        //     operVal = BUILDER.CreateLoad(expectDerefType_d->top(), operVal, "dereference");
        //     return operVal;
        // }

#ifdef D_DEBUG
        util::logErr("Invalid pointer dereference", ctx, __FILE__, __LINE__);
#else
        util::logErr("Invalid pointer dereference", ctx);
#endif

    case MINUS:
        return BUILDER.CreateNeg(operVal, "negate");
    case BIT_AND:

        if (const auto *loadInst = dyn_cast<LoadInst>(operVal)) {
            Value *ptr = loadInst->getOperand(0);
            return ptr;
        }

        if (dyn_cast<AllocaInst>(operVal)) {
            return operVal;
        }
        if (operVal->getType()->isArrayTy()) {
            if (llvm::isa<LoadInst>(operVal)) {
                auto *castReturn = dyn_cast<LoadInst>(operVal);
                return castReturn->getPointerOperand();
            }
            Value *value = BUILDER.CreateGEP(
                PointerType::get(operVal->getType()->getArrayElementType(), 0), operVal,
                {ConstantInt::get(LLVMCTX, APInt(32, 0)), ConstantInt::get(LLVMCTX, APInt(32, 0))}, "arrAddress");
            return value;
        }
        return BUILDER.CreateGEP(operVal->getType(), operVal, ConstantInt::get(LLVMCTX, APInt(1, 0)), "address");
    case DIV_ASSIGN:
        res = BUILDER.CreateSDiv(expression->codeGen(ctx), operVal);
        var = expression->codeGen(ctx);
        varPtr = BUILDER.CreateGEP(var->getType(), var, ConstantInt::get(LLVMCTX, APInt(1, 0)));
        return BUILDER.CreateStore(res, varPtr);
    case ADD_ASSIGN:
        res = BUILDER.CreateAdd(expression->codeGen(ctx), operVal);
        var = expression->codeGen(ctx);
        varPtr = BUILDER.CreateGEP(var->getType(), var, ConstantInt::get(LLVMCTX, APInt(1, 0)));
        return BUILDER.CreateStore(res, varPtr);

    case MINUS_ASSIGN:
        res = BUILDER.CreateSub(expression->codeGen(ctx), operVal);
        var = expression->codeGen(ctx);
        varPtr = BUILDER.CreateGEP(var->getType(), var, ConstantInt::get(LLVMCTX, APInt(1, 0)));
        return BUILDER.CreateStore(res, varPtr);

    case MUL_ASSIGN:
        res = BUILDER.CreateMul(expression->codeGen(ctx), operVal);
        var = expression->codeGen(ctx);
        varPtr = BUILDER.CreateGEP(var->getType(), var, ConstantInt::get(LLVMCTX, APInt(1, 0)));
        return BUILDER.CreateStore(res, varPtr);

    case MOD_ASSIGN:
        res = BUILDER.CreateSRem(expression->codeGen(ctx), operVal);
        var = expression->codeGen(ctx);
        varPtr = BUILDER.CreateGEP(var->getType(), var, ConstantInt::get(LLVMCTX, APInt(1, 0)));

        return BUILDER.CreateStore(res, varPtr);
    case BANG:
        return BUILDER.CreateNot(operVal, "not");
    default:
        return nullptr;
    }
}

llvm::Value *TruncExpressionNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    // sync context
    ctx->currLine = this->lineNum;

    // get target type
    Type *targetType = util::typeOf(this->targetType, ctx);

    Value *value = expression->codeGen(ctx);

    // trunc
    value = BUILDER.CreateSExtOrTrunc(value, targetType, "trunc");

    return value;
}

Value *FunctionCallExpressionNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    // context sync
    ctx->currLine = this->lineNum;

    // find the function, create the function
    Function *function = MODULE->getFunction(name->getName());
    if (!function) {
#ifdef D_DEBUG
        util::logErr("Function " + name->getName() + " not found", ctx, __FILE__, __LINE__);
        return nullptr;
#else
        util::logErr("Function " + name->getName() + " not found", ctx);
        return nullptr;
#endif
    }

    // build args
    std::vector<Value *> functionArgs;
    for (const auto arg : *args) {
        functionArgs.push_back(arg->codeGen(ctx));
    }

    if (function->getReturnType() == nullptr || function->getReturnType()->isVoidTy()) {
        BUILDER.CreateCall(function, functionArgs);
        return nullptr;
    }

    // call
    Value *callResult = BUILDER.CreateCall(function, functionArgs, "call");

    // return the value
    return callResult;
}

PHINode *createPHINode(Value *thenValue, BasicBlock *thenBlock, Value *elseValue, BasicBlock *elseBlock,
                       inter_gen::InterGenContext *ctx)
{
    Type *thenType = nullptr;
    const Type *elseType = nullptr;
    if (thenValue) {
        thenType = thenValue->getType();
    }
    if (elseValue) {
        elseType = elseValue->getType();
    }

    if (!thenType && !elseType) {
        return nullptr;
    }

    if (thenType != elseType) {
#ifdef D_DEBUG
        util::logErr("Invalid type for PHINode", ctx, __FILE__, __LINE__);
#else
        util::logErr("Invalid type for PHINode", ctx);
#endif
        return nullptr;
    }

    PHINode *phiNode = BUILDER.CreatePHI(thenType, 2, "PHI_node");

    phiNode->addIncoming(thenValue, thenBlock);
    phiNode->addIncoming(elseValue, elseBlock);
    return phiNode;
}

Value *ifStatementBlockGenerate_if(Value *conditionBoolVal, const std::vector<Statement *> *thenBlockStatement,
                                   inter_gen::InterGenContext *ctx)
{

    // if then block
    const std::string thenBlockName = ctx->getCurrFunMetaData()->getName();
    BasicBlock *thenBlock = BasicBlock::Create(LLVMCTX, util::getIfBasicBlockName(thenBlockName), ctx->getCurrFun());

    // merge block
    const std::string mergeBlockName = ctx->getCurrFunMetaData()->getName();
    BasicBlock *mergeBlock =
        BasicBlock::Create(LLVMCTX, util::getMergeBasicBlockName(mergeBlockName), ctx->getCurrFun());

    BUILDER.CreateCondBr(conditionBoolVal, thenBlock, mergeBlock);

    // then block
    BUILDER.SetInsertPoint(thenBlock);
    ctx->pushBlock(thenBlock);
    Value *thenBlockReturnValue = nullptr;

    for (const auto *statement : *thenBlockStatement) {
        statement->codeGen(ctx);
    }

    if (!util::basicBlockEndWithBranchStatement(thenBlock, ctx)) {
        thenBlockReturnValue = util::getLastValue(thenBlock, ctx);
        BUILDER.CreateBr(mergeBlock);
    }
    ctx->popBlock();

    BUILDER.SetInsertPoint(mergeBlock);

    if (ctx->isDefiningNestIfStatement()) {
        ctx->addValueToBeMerged(thenBlockReturnValue, thenBlock);
    }

    return thenBlockReturnValue;
}

Value *ifStatementBlockGenerate_ifelse(Value *conditionBoolVal, const std::vector<Statement *> *thenBlockStatement,
                                       const std::vector<Statement *> *elseBlockStatement,
                                       inter_gen::InterGenContext *ctx)
{

    // if then block
    const std::string thenBlockName = ctx->getCurrFunMetaData()->getName();
    BasicBlock *thenBlock = BasicBlock::Create(LLVMCTX, util::getIfBasicBlockName(thenBlockName), ctx->getCurrFun());

    // else block
    const std::string elseBlockName = ctx->getCurrFunMetaData()->getName();
    BasicBlock *elseBlock = BasicBlock::Create(LLVMCTX, util::getElseBasicBlockName(elseBlockName), ctx->getCurrFun());

    // merge block
    const std::string mergeBlockName = ctx->getCurrFunMetaData()->getName();
    BasicBlock *mergeBlock =
        BasicBlock::Create(LLVMCTX, util::getMergeBasicBlockName(mergeBlockName), ctx->getCurrFun());

    BUILDER.CreateCondBr(conditionBoolVal, thenBlock, elseBlock);

    // then block
    BUILDER.SetInsertPoint(thenBlock);
    ctx->pushBlock(thenBlock);
    Value *thenBlockReturnValue = nullptr;

    for (const auto *statement : *thenBlockStatement) {
        statement->codeGen(ctx);
    }

    if (!util::basicBlockEndWithBranchStatement(thenBlock, ctx)) {
        thenBlockReturnValue = util::getLastValue(thenBlock, ctx);
        BUILDER.CreateBr(mergeBlock);
    }
    ctx->popBlock();

    // else block
    BUILDER.SetInsertPoint(elseBlock);
    ctx->pushBlock(elseBlock);
    Value *elseBlockReturnValue = nullptr;

    for (const auto *statement : *elseBlockStatement) {
        statement->codeGen(ctx);
    }

    if (!util::basicBlockEndWithBranchStatement(elseBlock, ctx)) {
        elseBlockReturnValue = util::getLastValue(elseBlock, ctx);
        BUILDER.CreateBr(mergeBlock);
    }
    ctx->popBlock();

    BUILDER.SetInsertPoint(mergeBlock);

    if (ctx->isDefiningNestIfStatement()) {
        //  this is a nested block and add it to the merge list to wait for merge
        ctx->addValueToBeMerged(thenBlockReturnValue, thenBlock);
        ctx->addValueToBeMerged(elseBlockReturnValue, elseBlock);

        ctx->addTargetMergeBlock(mergeBlock);

        return nullptr;
    }

    PHINode *node = createPHINode(thenBlockReturnValue, thenBlock, elseBlockReturnValue, elseBlock, ctx);
    return node;
}

PHINode *ifStatementBlockGenerate_ifelseif(Value *conditionBoolVal, const std::vector<Statement *> *thenBlockStatement,
                                           const Statement *elseIfStatement, inter_gen::InterGenContext *ctx)
{
    // context relate
    bool isHighestLevelIf = false;
    BasicBlock *mergeBlock;
    if (!ctx->isDefiningNestIfStatement()) {
        isHighestLevelIf = true;
        ctx->setDefiningNestIfStatementFlag(true);
    } else {
        // merge block
        const std::string mergeBlockName = ctx->getCurrFunMetaData()->getName();
        mergeBlock = BasicBlock::Create(LLVMCTX, util::getElseBasicBlockName(mergeBlockName), ctx->getCurrFun());
        ctx->addTargetMergeBlock(mergeBlock);
    }

    // if then block
    std::string thenBlockName = ctx->getCurrFunMetaData()->getName();
    BasicBlock *thenBlock = BasicBlock::Create(LLVMCTX, util::getIfBasicBlockName(thenBlockName), ctx->getCurrFun());

    // else block
    const std::string elseBlockName = ctx->getCurrFunMetaData()->getName();
    BasicBlock *elseBlock = BasicBlock::Create(LLVMCTX, util::getElseBasicBlockName(elseBlockName), ctx->getCurrFun());

    BUILDER.CreateCondBr(conditionBoolVal, thenBlock, elseBlock);

    BUILDER.SetInsertPoint(thenBlock);
    ctx->pushBlock(thenBlock);
    Value *thenBlockReturnValue = nullptr;

    for (const auto *statement : *thenBlockStatement) {
        statement->codeGen(ctx);
    }

    ctx->popBlock();

    // merge the else if block to the main block
    BUILDER.SetInsertPoint(elseBlock);
    ctx->pushBlock(elseBlock);
    elseIfStatement->codeGen(ctx);

    // create PHI node
    PHINode *node;

    ctx->popBlock();

    BasicBlock *blockToMerge = ctx->getTargetMergeBlock();

    // add branch statement to the then block
    if (!util::basicBlockEndWithBranchStatement(thenBlock, ctx)) {
        thenBlockReturnValue = util::getLastValue(thenBlock, ctx);
        BUILDER.SetInsertPoint(thenBlock);
        BUILDER.CreateBr(blockToMerge);
    }

    BUILDER.SetInsertPoint(blockToMerge);

    // set default value if null
    if (!thenBlockReturnValue) {
        thenBlockReturnValue = Constant::getNullValue(IntegerType::get(LLVMCTX, 32));
    }

    // create PHI node
    if (isHighestLevelIf) {
        std::vector<std::pair<BasicBlock *, Value *>> valuesToBeMerged = ctx->getValueToBeMerged();
        node = BUILDER.CreatePHI(thenBlockReturnValue->getType(), valuesToBeMerged.size() + 1);

        node->addIncoming(thenBlockReturnValue, thenBlock);
        for (const auto &[block, value] : valuesToBeMerged) {
            node->addIncoming(value, block);
        }

        ctx->setDefiningNestIfStatementFlag(false);

    } else {
        // add then block to the merge list
        ctx->addValueToBeMerged(thenBlockReturnValue, thenBlock);
        return nullptr;
    }

    return node;
}
Value *IfStatementNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    // context sync
    ctx->currLine = this->lineNum;

    // if condition
    Value *condition = conditionExpression->codeGen(ctx);
    if (!condition) {
#ifdef D_DEBUG
        util::logErr("Invalid condition", ctx, __FILE__, __LINE__);
#else
        util::logErr("Invalid condition", ctx);
#endif
        return nullptr;
    }
    Value *conditionBoolVal = util::toBool(condition, ctx);

    // merge block
    Value *value = nullptr;

    if (!elseIf && !elseBlock) {
        value = ifStatementBlockGenerate_if(conditionBoolVal, thenBlock, ctx);
    }

    if (!elseIf && elseBlock) {
        value = ifStatementBlockGenerate_ifelse(conditionBoolVal, thenBlock, elseBlock, ctx);
    }

    if (elseIf) {
        value = ifStatementBlockGenerate_ifelseif(conditionBoolVal, thenBlock, elseIf, ctx);
    }
    // PHI node

    // return and clear flags
    return value;
}

Value *ForStatementNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    ctx->currLine = this->lineNum;
    // Generate code for initialization Expression
    Value *initGen = conditionDeclaration->codeGen(ctx);

    // Generate code for loop condition
    // Value *condVal = conditionDeclaration->codeGen(ctx);
    Value *brCond = condition->codeGen(ctx);

    // Handle different types for loop condition: integer, floating point, or
    // // pointer
    // switch (condVal->getType()->getTypeID()) {
    // case Type::IntegerTyID:
    //     brCond = BUILDER.CreateICmp(CmpInst::ICMP_NE, condVal, ConstantInt::get(LLVMCTX, APInt(1, 0)));
    //     break;
    // case Type::DoubleTyID:
    // case Type::FloatTyID:
    //     brCond = BUILDER.CreateFCmp(CmpInst::FCMP_ONE, condVal, ConstantFP::get(LLVMCTX, APFloat(0.0)));
    //     break;
    // case Type::PointerTyID:
    //     brCond = BUILDER.CreateICmp(CmpInst::ICMP_NE, condVal, ConstantPointerNull::get(PointerType::get(LLVMCTX,
    //     0))); break;
    // default:
    //     return nullptr;
    // }

    // Create the loop and exit basic blocks
    Function *fun = ctx->getCurrFun();
    BasicBlock *loopBB = BasicBlock::Create(LLVMCTX, "loop", fun);
    BasicBlock *exitBB = BasicBlock::Create(LLVMCTX, "exit");
    ctx->topBlock()->loopExitBlocks.push(exitBB);
    BUILDER.CreateCondBr(brCond, loopBB, exitBB);

    // Insert the loop body and conditional branch back to loop or exit
    BUILDER.SetInsertPoint(loopBB);
    for (auto statement : *block) {
        statement->codeGen(ctx);
    }
    BUILDER.CreateStore(variableChange->codeGen(ctx),
                        initGen);     // Update the loop step variable
    brCond = condition->codeGen(ctx); // Re-check the loop condition
    BUILDER.CreateCondBr(brCond, loopBB, exitBB);

    // Insert the exit block after the loop ends
    fun->insert(fun->end(), exitBB);
    BUILDER.SetInsertPoint(exitBB);

    ctx->topBlock()->loopExitBlocks.pop();

    return nullptr;
}

Value *ReturnStatementNode::codeGen(inter_gen::InterGenContext *ctx) const
{
    ctx->currLine = this->lineNum;
    if (expression == nullptr) {
        BUILDER.CreateRetVoid();
        ctx->setCurrRetVal(nullptr);
        return nullptr;
    }
    // Generate code for the return initialValue
    Value *retVal = expression->codeGen(ctx);

    const bool isNullable = inter_gen::isNullable(expression, ctx);
    ctx->getCurrFunMetaData()->setReturnMetaData(
        retVal, new inter_gen::VariableMetaData("", retVal->getType(), true, isNullable));

    BUILDER.CreateRet(retVal);
    ctx->setCurrRetVal(retVal);
    // blockMappingRet_d->insert({ctx->currBlock(), retVal});
    return retVal;
}

} // namespace parser

namespace inter_gen
{

llvm::LLVMContext *llvmContext = new llvm::LLVMContext();

FunctionMetaData *InterGenContext::getFunMetaData(const std::string &name, const InterGenContext *ctx) const
{

    if (FunctionMetaData *funMetaData = metaData->getFunction(name)) {
        return funMetaData;
    }
#ifdef D_DEBUG
    util::logErr("Function " + name + " not found", ctx, __FILE__, __LINE__);
#else
    util::logErr("Function " + name + " not found", ctx);
#endif

    return nullptr;
}

std::pair<Value *, VariableMetaData *> InterGenContext::getValWithMetadata(const parser::QualifiedNameNode *name)
{
    int idx = 0;
    std::pair<Value *, VariableMetaData *> val_with_metadata = this->getValWithMetadata(name->getName());
    while (val_with_metadata.first && val_with_metadata.second) {
        if (name->name_parts->size() == idx + 1) {
            return val_with_metadata;
        }
        idx++;
    }
    {
        // REPORT_ERROR(errMsg("Value not found" + name->getName()), __FILE__, __LINE__);
    }
    return {nullptr, nullptr};
}

void InterGenContext::genIR(parser::ProgramNode *program)
{
    program->codeGen(this);
    this->package = program->packageName->getName();

    if (!mainFunction && (sourcePath.ends_with("/main.dap") || sourcePath == "main.dap")) {
        constexpr std::vector<Type *> argTypes;
        // FunctionType *ftype = FunctionType::get(Type::getInt32Ty(module->getContext()), llvm::ArrayRef(argTypes),
        // false); mainFunction = Function::Create(ftype, GlobalValue::ExternalLinkage, "main", module);
        BasicBlock *bblock = BasicBlock::Create(module->getContext(), "entry", mainFunction, nullptr);

        builder.SetInsertPoint(bblock);
        pushBlock(bblock);

        builder.CreateRet(llvm::ConstantInt::get(module->getContext(), llvm::APInt(32, 0)));

        popBlock();
    }

    std::error_code EC;
    util::replaceAll(package, ".", "/");
    std::string outputPath = buildDir + package + "/" + fileName + ".ll";
    std::fstream outputFile(outputPath, std::ios::out);

    if (outputFile.is_open()) {
        std::cout << "Writing LLVM IR to " << outputPath << std::endl;
    }
    outputFile.close();

    raw_fd_ostream outfile(outputPath, EC, sys::fs::OF_Text);

    if (EC) {
        errs() << "Error opening output file: " << EC.message() << "\n";
        return;
    }
    legacy::PassManager pm;
    pm.add(createPrintModulePass(outfile));
    pm.run(*module);

    outfile.flush();
    outfile.close();
}

void InterGenContext::genExec(parser::ProgramNode *program)
{
    // Initialize LLVM targets
    InitializeAllTargetInfos();
    InitializeAllTargets();
    InitializeAllTargetMCs();
    InitializeAllAsmParsers();
    InitializeAllAsmPrinters();

    this->package = program->packageName->getName();

    program->codeGen(this);

    if (!mainFunction && (sourcePath.ends_with("/main.dap") || sourcePath == "main.dap")) {
        // Define the main function (i32 @main())
        // FunctionType *ftype = FunctionType::get(Type::getInt32Ty(module->getContext()), false);
        // mainFunction = Function::Create(ftype, GlobalValue::ExternalLinkage, "main", module);

        // Create the entry block and set the insertion point
        BasicBlock *bblock = BasicBlock::Create(module->getContext(), "entry", mainFunction);
        builder.SetInsertPoint(bblock);
        pushBlock(bblock);

        // Return 0 from the main function
        builder.CreateRet(ConstantInt::get(Type::getInt32Ty(module->getContext()), 0));

        // Verify the function
        verifyFunction(*mainFunction);
        popBlock();
    }

    // Target machine setup
    const std::string targetTriple = sys::getDefaultTargetTriple();
    module->setTargetTriple(targetTriple);

    std::string error;
    const Target *target = TargetRegistry::lookupTarget(targetTriple, error);

    if (!target) {
        errs() << "Error: " << error;
        return;
    }

    // CPU and features
    const std::string cpu = "generic";
    const std::string features;

    const TargetOptions opt;
    std::optional<Reloc::Model>();
    TargetMachine *targetMachine = target->createTargetMachine(targetTriple, cpu, features, opt, Reloc::PIC_);

    module->setDataLayout(targetMachine->createDataLayout());

    // Create the MCJIT execution engine
    std::unique_ptr<Module> modulePtr(module);
    auto mem_mgr = std::make_unique<SectionMemoryManager>();

    // Save the generated code to a file
    std::error_code EC;
    util::replaceAll(package, ".", "/");

    std::string outputPath = buildDir + package + "/" + fileName + ".o";
    std::fstream outputFile(outputPath, std::ios::out);
    outputFile.close();
    filesToCompile->push_back(outputPath);
    raw_fd_ostream dest(outputPath, EC, sys::fs::OF_None);

    if (EC) {
        errs() << "Could not open file for writing: " << EC.message() << "\n";
        return;
    }

    legacy::PassManager pass;
    if (targetMachine->addPassesToEmitFile(pass, dest, nullptr, CodeGenFileType::ObjectFile)) {
        errs() << "TargetMachine can't emit a file of this type\n";
        return;
    }

    pass.run(*module);
    dest.flush();

    outs() << "Object file emitted to " << outputPath << "\n";
}

std::unordered_map<IncludeGraphNode *, bool> visited{};

void InterGenContext::registerStructMetadata(StructMetaData *metaData) const
{
    this->metaData->addStruct(metaData);
}

void interGen_oneFile(IncludeGraphNode *node)
{
    if (visited.contains(node)) {
        return;
    }
    visited.insert({node, false});
    if (!node->getIncludes().empty()) {
        for (const auto oneNode : node->getIncludes()) {
            interGen_oneFile(oneNode);
        }
    }
    programMap_d->at(node->getProgram())->genIR(node->getProgram());
    visited.insert({node, true});
}

void interGen(const std::set<IncludeGraphNode *> &map)
{
    for (const auto node : map) {
        interGen_oneFile(node);
    }
}

Value *InterGenContext::getVal(const std::string &name)
{
    std::stack<InterGenBlock *> tmp;
    while (!blocks.empty()) {
        if (blocks.top()->locals.contains(name)) {
            Value *res = blocks.top()->locals[name].first;

            while (!tmp.empty()) {
                blocks.push(tmp.top());
                tmp.pop();
            }
            return res;
        }
        if (Function *fun = blocks.top()->block->getParent()) {
            for (auto &arg : fun->args()) {
                if (arg.getName() == name) {
                    return &arg;
                }
            }
        }
        tmp.push(blocks.top());
        blocks.pop();
    }

    while (!tmp.empty()) {
        blocks.push(tmp.top());
        tmp.pop();
    }

    if (this->module->getGlobalVariable(name, true)) {
        return this->module->getGlobalVariable(name, true);
    }

    return nullptr;
}

std::pair<Value *, VariableMetaData *> InterGenContext::getValWithMetadata(const std::string &name)
{
    std::stack<InterGenBlock *> tmp;
    while (!blocks.empty()) {
        if (blocks.top()->locals.contains(name)) {
            std::map<std::string, std::pair<Value *, VariableMetaData *>>::mapped_type res = blocks.top()->locals[name];
            while (!tmp.empty()) {
                blocks.push(tmp.top());
                tmp.pop();
            }
            return res;
        }
        if (Function *fun = blocks.top()->block->getParent()) {
            const auto &funMetadata = metaData->getFunction(fun->getName().str());
            for (auto &arg : fun->args()) {
                if (arg.getName() == name) {
                    return {&arg, funMetadata->getArgMetaData(arg.getName().str())};
                }
            }
        }
        tmp.push(blocks.top());
        blocks.pop();
    }

    if (module->getGlobalVariable(name)) {
        // return {module->getGlobalVariable(name), getModuleMetaData(name)->getGlobalValMetaData(name)};
    }

    return {nullptr, nullptr};
}

void InterGenContext::addFunctionToMetaData(FunctionMetaData *function_meta_data) const
{
    metaData->addFunction(function_meta_data);
}

FunctionMetaData *InterGenContext::getCurrFunMetaData() const
{
    return currentFunMetaData;
}

void InterGenContext::setCurrFunMetaData(FunctionMetaData *funMetaData)
{
    currentFunMetaData = funMetaData;
}

void InterGenContext::setName(const std::string &nameString)
{
    name = nameString;
}

std::string InterGenContext::getName() const
{
    return name;
}

InterGenContext::InterGenContext(std::string sourcePathInput)
    : assigning(false), builder(llvm::IRBuilder(*llvmContext)), sourcePath(std::move(std::move(sourcePathInput)))
{
    fileName = sourcePath.substr(sourcePath.find_last_of('/') + 1);
    module = new Module(sourcePath, *llvmContext);
    metaData = new ModuleMetaData(module);
    definingNestedIfStatement.push(false);
}

} // namespace inter_gen
} // namespace dap
