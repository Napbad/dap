//
// Created by napbad on 11/25/24.
//

#ifndef INCLUDEANAYLIZE_H
#define INCLUDEANAYLIZE_H

#include <map>
#include <set>
#include <string>
#include <utility>
#include <vector>

namespace dap
{
namespace parser
{
class ProgramNode;
}

namespace inter_gen
{
class InterGenContext;

class IncludeGraphNode
{
  public:
    explicit IncludeGraphNode(std::string name, parser::ProgramNode *program) : name(std::move(name)), program(program)
    {
    }

    [[nodiscard]] std::string getName() const
    {
        return name;
    }

    void addInclude(IncludeGraphNode *node)
    {
        includes.push_back(node);
    }

    void addIncludedBy(IncludeGraphNode *node)
    {
        includedBy.push_back(node);
    }

    [[nodiscard]] dap::parser::ProgramNode *getProgram() const
    {
        return program;
    }

    [[nodiscard]] std::vector<IncludeGraphNode *> getIncludedBy() const
    {
        return includedBy;
    }

    [[nodiscard]] std::vector<IncludeGraphNode *> getIncludes() const
    {
        return includes;
    }

    [[nodiscard]] std::vector<std::string *> getIncludedBy_path() const
    {
        return includedBy_path;
    }

    [[nodiscard]] std::vector<std::string *> getIncludes_path() const
    {
        return includes_path;
    }

    void addIncludedBy_path(std::string *path)
    {
        includedBy_path.push_back(path);
    }

    void addIncludes_path(std::string *path)
    {
        includes_path.push_back(path);
    }

  private:
    std::string name;
    dap::parser::ProgramNode *program{};
    std::vector<IncludeGraphNode *> includedBy{};
    std::vector<IncludeGraphNode *> includes{};
    std::vector<std::string *> includedBy_path{};
    std::vector<std::string *> includes_path{};
};

class IncludeAnalyzer
{
  public:
    void analyze() const;

    [[nodiscard]] std::set<IncludeGraphNode *> getRoots() const
    {
        return roots;
    }

    void generateGraph();

  private:
    std::map<std::string, IncludeGraphNode *> nodes;
    std::set<IncludeGraphNode *> roots;
};
} // namespace inter_gen
} // namespace dap

#endif // INCLUDEANAYLIZE_H