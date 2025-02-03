//
// Created by napbad on 11/26/24.
//

#include <filesystem>
#include <fstream>
#include <string>
#include <unordered_map>

#include "codeGen_mechine.h"

#include "common/config.h"

#include "inter_gen/codeGen_inter.h"
#include "utilities/string_util.h"

#include "parser/parserMain.h"

std::unordered_map<dap::inter_gen::IncludeGraphNode *, bool> dap::mech_gen::visited;

void dap::mech_gen::execGen(const std::set<inter_gen::IncludeGraphNode *> &map)
{

    for (const auto node : map) {
        genExecToOneFile(node);
    }

    std::string files = util::getStrFromVec(*filesToCompile, " ");
    files.append(" ").append(buildDir).append("dap/runtime/asm/_start.o ");

    std::string targetPath = buildDir + targetExecName;
    const std::string targetPathSource = targetPath;

    if (std::filesystem::exists(targetPath)) {
        while (std::filesystem::is_directory(targetPath)) {
            targetPath.append("_");
        }
        util::logInfo("targetPath exists: " + targetPathSource + ", change to " + targetPath, nullptr, __FILE__,
                      __LINE__);
    }
#ifdef D_DEBUG
    util::logInfo("Compiling with command: " + ("ld " + files + arg + " -o " + targetPath + " "), nullptr, __FILE__,
                  __LINE__);
#else
    util::logInfo("Compiling with command: " + ("ld " + files + arg + " -o " + targetExecName + " "), nullptr);
#endif

    system(("ld " + files + arg + " -o " + targetExecName + " ").c_str());
#ifdef D_DEBUG
    util::logInfo("executable file has been generate: " + buildDir + targetExecName, nullptr, __FILE__, __LINE__);
#else
    util::logInfo("executable file has been generate: " + buildDir + targetExecName, nullptr);

#endif
}

void dap::mech_gen::execGen_singleFile(inter_gen::InterGenContext *ctx, dap::parser::ProgramNode *program)
{
    ctx->genExec(program);
    std::string files = util::getStrFromVec(*filesToCompile, " ");
    files.append(" ").append(buildDir).append("dap/runtime/asm/_start.o ");

    std::string targetPath = buildDir + targetExecName;
    std::string targetPathSource = targetPath;

    if (std::filesystem::exists(targetPath)) {
        while (std::filesystem::is_directory(targetPath)) {
            targetPath.append("_");
        }
        util::logInfo("targetPath exists: " + targetPathSource + ", change to " + targetPath, ctx, __FILE__, __LINE__);
    }

    system(("ld " + files + arg + " -o " + targetPath + " ").c_str());
#ifdef D_DEBUG
    util::logInfo("executable file has been generate: " + buildDir + targetExecName, ctx, __FILE__, __LINE__);
#else
    util::logInfo("executable file has been generate: " + buildDir + targetExecName, ctx)
#endif
}

void dap::mech_gen::genExecToOneFile(inter_gen::IncludeGraphNode *node)
{
    if (!node->getIncludes().empty()) {
        for (const auto include : node->getIncludes()) {
            genExecToOneFile(include);
        }
    }

    std::string basic_string = node->getName();
    if (visited.contains(node)) {
        return;
    }
    programMap_d->at(node->getProgram())->genExec(node->getProgram());
    visited.insert({node, true});
}