#include "exgine/ast.hpp"
#include <cstdlib>
#include <optional>
#include <string>
#include <utility>
namespace exgine { namespace {
class ParserImpl {
public:
    ParserImpl(const std::vector<Token>& t,DiagnosticBag& d):tokens_(t),diagnostics_(d){}
    AstDocument parse(){AstDocument document;while(!check(TokenKind::EndOfFile)){if(!match(TokenKind::Game)){error_here("expected 'game' declaration");recover_to(TokenKind::Game);continue;}document.games.push_back(parse_game(previous().location));}return document;}
private:
    const std::vector<Token>& tokens_;DiagnosticBag& diagnostics_;std::size_t current_=0;
    const Token& peek()const{return tokens_[current_];}const Token& next()const{return current_+1<tokens_.size()?tokens_[current_+1]:tokens_.back();}const Token& previous()const{return tokens_[current_-1];}
    bool check(TokenKind k)const{return peek().kind==k;}const Token& advance(){if(!check(TokenKind::EndOfFile))++current_;return previous();}bool match(TokenKind k){if(!check(k))return false;advance();return true;}void error_here(const std::string& m){diagnostics_.error(m,peek().location);}void recover_to(TokenKind k){while(!check(k)&&!check(TokenKind::EndOfFile))advance();}
    bool consume(TokenKind k,const char* m){if(check(k)){advance();return true;}error_here(m);return false;}
    AstGame parse_game(SourceLocation location){AstGame game;game.location=location;if(check(TokenKind::String)||check(TokenKind::Identifier))game.name=advance().lexeme;else error_here("expected game name after 'game'");if(!consume(TokenKind::LeftBrace,"expected '{' after game name"))return game;while(!check(TokenKind::RightBrace)&&!check(TokenKind::EndOfFile)){auto n=parse_node();if(n)game.nodes.push_back(std::move(*n));}consume(TokenKind::RightBrace,"expected '}' at end of game");return game;}
    static AstNodeKind node_kind(TokenKind k){switch(k){case TokenKind::World:return AstNodeKind::World;case TokenKind::Terrain:return AstNodeKind::Terrain;case TokenKind::Vegetation:return AstNodeKind::Vegetation;case TokenKind::Building:return AstNodeKind::Building;case TokenKind::Vehicle:return AstNodeKind::Vehicle;case TokenKind::Player:return AstNodeKind::Player;case TokenKind::NPC:return AstNodeKind::NPC;case TokenKind::Bridge:return AstNodeKind::Bridge;case TokenKind::Prop:return AstNodeKind::Prop;default:return AstNodeKind::World;}}
    bool starts_node()const{return check(TokenKind::World)||check(TokenKind::Terrain)||check(TokenKind::Vegetation)||check(TokenKind::Building)||check(TokenKind::Vehicle)||check(TokenKind::Player)||check(TokenKind::NPC)||check(TokenKind::Bridge)||check(TokenKind::Prop);}
    bool starts_property()const{switch(peek().kind){case TokenKind::Identifier:case TokenKind::Terrain:case TokenKind::Vegetation:case TokenKind::Building:case TokenKind::Vehicle:case TokenKind::Player:case TokenKind::NPC:case TokenKind::Bridge:case TokenKind::Prop:case TokenKind::Procedural:case TokenKind::World:return next().kind==TokenKind::Equals;default:return false;}}
    std::optional<AstNode> parse_node(){if(!starts_node()){error_here("expected a valid EXGINE node block");advance();return std::nullopt;}const Token start=advance();AstNode node{node_kind(start.kind),{}, {}, {},start.location};if(check(TokenKind::String)||check(TokenKind::Identifier))node.name=advance().lexeme;if(!consume(TokenKind::LeftBrace,"expected '{' after node declaration"))return node;while(!check(TokenKind::RightBrace)&&!check(TokenKind::EndOfFile)){if(starts_node()&&!starts_property()){auto child=parse_node();if(child)node.children.push_back(std::move(*child));}else if(starts_property())parse_property(node);else{error_here("expected property assignment or child block");advance();}}consume(TokenKind::RightBrace,"expected '}' at end of node");return node;}
    void parse_property(AstNode& node){const Token name=advance();if(!consume(TokenKind::Equals,"expected '=' after property name"))return;AstValue value;if(match(TokenKind::Integer))value=static_cast<std::int64_t>(std::strtoll(previous().lexeme.c_str(),nullptr,10));else if(match(TokenKind::Number))value=std::strtod(previous().lexeme.c_str(),nullptr);else if(match(TokenKind::True))value=true;else if(match(TokenKind::False))value=false;else if(match(TokenKind::String))value=previous().lexeme;else if(match(TokenKind::Identifier)||match(TokenKind::Procedural))value=previous().lexeme;else{error_here("expected a literal value after '='");return;}node.properties.push_back({name.lexeme,std::move(value),name.location});}
};}
AstDocument Parser::parse(DiagnosticBag& diagnostics)const{ParserImpl parser(tokens_,diagnostics);return parser.parse();}
}
