#include "parserMain.h"

#include "inter_gen/metadata.h"
#include "utilities/string_util.h"

#include <filesystem>
#include <fstream>

namespace dap
{
std::unordered_map<parser::ProgramNode *, inter_gen::InterGenContext *> *programMap_d =
    new std::unordered_map<parser::ProgramNode *, inter_gen::InterGenContext *>();

namespace parser
{

std::string currentParsingFile = "";

std::pair<ProgramNode *, inter_gen::InterGenContext *> parseFile(const std::string &file)
{
    // init
    yylineno = 1;
    program = new ProgramNode();

    currentParsingFile = file;

    FILE *openedFile = fopen(file.c_str(), "r");
    // freopen(file.c_str(), "r", stdin);
    // std::ifstream openedFile(file);

    yyrestart(openedFile);

    yyparse();

    // store program and context info
    auto ctx = new inter_gen::InterGenContext(file);
    auto pair = std::make_pair(program, ctx);
    programMap_d->insert(pair);

    std::string moduleName = util::getStrFromVec(*(program->packageName->name_parts), ".");
    moduleName.append(".");
    std::filesystem::__cxx11::path pathObj(file);
    std::string fileName = pathObj.filename().string();

    moduleName.append(fileName);

    moduleName.erase(moduleName.find(".dap"), 4);

    ctx->setName(moduleName);
    inter_gen::moduleMap->emplace(moduleName, ctx->metaData);

    fclose(openedFile);

    return pair;
}
} // namespace parser

} // namespace dap