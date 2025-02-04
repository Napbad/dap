//
// Created by napbad on 11/11/24.
//

#include "metadata.h"

#include <utility>

#include "codeGen_inter.h"

namespace dap::inter_gen
{
std::unordered_map<std::string, ModuleMetaData *> *moduleMap = new std::unordered_map<std::string, ModuleMetaData *>;

StructMetaData::StructMetaData(InterGenContext *context, std::string name) : ctx(context), name_(std::move(name))
{
}
void StructMetaData::addField(const std::string &fieldName, VariableMetaData *varMetaData)
{
    fields_[fieldName] = varMetaData->getType();
    fieldMetaData_[fieldName] = varMetaData;
    fieldIndexMap_[fieldName] = fields_.size() - 1;
}

StructType *StructMetaData::getStructType() const
{
    if (!structType_) {
        std::vector<Type *> fieldTypes;
        fieldTypes.reserve(fields_.size());
        for (const auto &[fst, snd] : fields_) {
            fieldTypes.push_back(snd);
        }
        structType_ = StructType::create(LLVMCTX, fieldTypes, name_);
    }
    return structType_;
}

unsigned StructMetaData::getFieldIndex(const std::string &fieldName) const
{
    auto it = fieldIndexMap_.find(fieldName);
    if (it == fieldIndexMap_.end()) {
        throw std::runtime_error("Field '" + fieldName + "' not found");
    }
    return it->second;
}

void StructMetaData::setStructType(StructType *type) const
{
    structType_ = type;
}

Type *StructMetaData::getFieldType(const std::string &string) const
{
    return fields_.at(string);
}

std::string StructMetaData::getName() const
{
    return name_;
}

void FunctionMetaData::genFun(const InterGenContext *ctx) const
{
    Function *fun = Function::Create(funType, GlobalValue::ExternalLinkage, name_, MODULE);

    size_t idx = 0;
    for (auto &arg : fun->args())
        arg.setName(get<0>(args[idx++]));
}

VariableMetaData *FunctionMetaData::getReturnMetaData() const
{
    return get<1>(returnVal);
}

VariableMetaData *FunctionMetaData::getArgMetaData(const std::string &name)
{
    for (auto val : args) {
        if (get<0>(val) == name) {
            return get<3>(val);
        }
    }

    return nullptr;
}

void FunctionMetaData::setReturnMetaData(Value *value, VariableMetaData *variableMetaData)
{
    returnVal = {value, variableMetaData};
}

FunctionMetaData::FunctionMetaData(std::string name, FunctionType *type, InterGenContext *ctx)
    : name_(std::move(name)), funType(type), ctx(ctx)
{
}

ModuleMetaData::ModuleMetaData(Module *module) : module_(module)
{
    functionMap = std::unordered_map<std::string, FunctionMetaData *>();
    globalMetaDataMap = std::unordered_map<std::string, VariableMetaData *>();
    globalVals = std::vector<Value *>();
    structMap = std::unordered_map<std::string, StructMetaData *>();
    globalVar = std::vector<GlobalVariable *>();

    moduleName = module_->getName().str();
    modulePath = module_->getName();
}

void ModuleMetaData::addGlobalValMetaData(VariableMetaData *metaData)
{
    globalMetaDataMap.insert({metaData->getName(), metaData});
}

VariableMetaData *ModuleMetaData::getGlobalValMetaData(const std::string &name)
{
    if (!globalMetaDataMap.contains(name)) {
        return nullptr;
    }
    return globalMetaDataMap[name];
}

std::unordered_map<std::string, FunctionMetaData *> ModuleMetaData::getFunctions()
{
    return functionMap;
}

bool VariableMetaData::isMutable() const
{
    return mutableStack_.top();
}

bool VariableMetaData::isNullable() const
{
    return nullableStack_.top();
}

std::string VariableMetaData::getName() const
{
    return name_;
}

Type *VariableMetaData::getType() const
{
    return type_;
}

void VariableMetaData::enterNewScope()
{
    nullableStack_.push(nullableStack_.top());
    mutableStack_.push(mutableStack_.top());
}

void VariableMetaData::enterNewScope(bool newMutable)
{
    nullableStack_.push(nullableStack_.top());
    mutableStack_.push(newMutable);
}

void VariableMetaData::enterNewScope(void *voidArg, bool newNullable)
{
    nullableStack_.push(newNullable);
    mutableStack_.push(mutableStack_.top());
}

void VariableMetaData::enterNewScope(bool newMutable, bool newNullable)
{
    nullableStack_.push(newNullable);
}

VariableMetaData::VariableMetaData(const std::string &name, Type *varType, bool isMutable, bool isNullable,
                                   InterGenContext *ctx)
{
    name_ = name;
    type_ = varType;
    mutableStack_.push(isMutable);
    nullableStack_.push(isNullable);
    this->ctx = ctx;
}

} // namespace dap::inter_gen