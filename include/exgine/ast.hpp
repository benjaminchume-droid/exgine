#pragma once
#include "exgine/diagnostic.hpp"
#include "exgine/token.hpp"
#include <cstdint>
#include <string>
#include <variant>
#include <vector>
namespace exgine {
enum class AstNodeKind { World,Terrain,Vegetation,Building,Vehicle,Player,NPC,Bridge,Prop };
using AstValue=std::variant<std::int64_t,double,bool,std::string>;
struct AstProperty{std::string name;AstValue value;SourceLocation location{};};
struct AstNode{AstNodeKind kind;std::string name;std::vector<AstProperty> properties;std::vector<AstNode> children;SourceLocation location{};};
struct AstGame{std::string name;std::vector<AstNode> nodes;SourceLocation location{};};
struct AstDocument{std::vector<AstGame> games;};
class Parser{public:explicit Parser(const std::vector<Token>&tokens)noexcept:tokens_(tokens){}[[nodiscard]]AstDocument parse(DiagnosticBag&diagnostics)const;private:const std::vector<Token>&tokens_;};
} // namespace exgine
