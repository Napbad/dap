//
// Created by napbad on 1/11/25.
//

#include "config.h"

#include <vector>

namespace dap
{
std::string buildDir = DEFUALT_BUILD_DIR;

std::string targetExecName = DEFAULT_TARGET_NAME;

std::string D_VERSION = "0.0.1";

void readConfig(std::string configPath)
{
}

std::vector<std::string> *filesToCompile = new std::vector<std::string>();
} // namespace dap