// ------------------------------------------------------------- Project Headers
#include "CProgCSTVisitor.h"
#include "CProgAST.h"
#include "Writer.h"

// ---------------------------------------------------------- C++ System Headers
#include <iostream>
#include <string>

////////////////////////////////////////////////////////////////////////////////
// class CProgCSTVisitor                                                      //
////////////////////////////////////////////////////////////////////////////////

antlrcpp::Any CProgCSTVisitor::visitProgram(CProgParser::ProgramContext *ctx)
{
    CProgASTProgram *program = new CProgASTProgram();
    for(auto funcdef_ctx : ctx->funcdef())
    {
        program->add_funcdef(visit(funcdef_ctx).as<CProgASTFuncdef*>());
    }
    return program;
}

antlrcpp::Any CProgCSTVisitor::visitFuncdef(CProgParser::FuncdefContext *ctx)
{
    std::string identifier = ctx->IDENTIFIER()->getText();
    CProgASTFuncdef *funcdef = new CProgASTFuncdef(identifier, Type::INT_64);
    for(auto statement_ctx : ctx->block()->statement())
    {
        funcdef->add_statement(visit(statement_ctx).as<CProgASTStatement*>());
    }
    return funcdef;
}

antlrcpp::Any CProgCSTVisitor::visitStatement(CProgParser::StatementContext *ctx)
{
    if(ctx->return_statement() != nullptr)
    {
        return static_cast<CProgASTStatement*>(visit(ctx->return_statement()).as<CProgASTReturn*>());
    }
    else if(ctx->declaration() != nullptr)
    {
        return static_cast<CProgASTStatement*>(visit(ctx->declaration()).as<CProgASTDeclaration*>());
    }
    else if(ctx->expr() != nullptr)
    {
        return static_cast<CProgASTStatement*>(visit(ctx->expr()).as<CProgASTExpression*>());
    }
    else
    {
        Writer::error() << "empty statement currently not supported" << std::endl;
        return nullptr;
    }
}

antlrcpp::Any CProgCSTVisitor::visitReturn_statement(CProgParser::Return_statementContext *ctx)
{
    return new CProgASTReturn(visit(ctx->expr()).as<CProgASTExpression*>());
}

antlrcpp::Any CProgCSTVisitor::visitDeclaration(CProgParser::DeclarationContext *ctx)
{
    Type type;
    if (ctx->type_name()->CHAR_TYPE_NAME())
        type = Type::CHAR;
    else if (ctx->type_name()->INT_16_TYPE_NAME())
        type = Type::INT_16;
    else if (ctx->type_name()->INT_32_TYPE_NAME())
        type = Type::INT_32;
    else if (ctx->type_name()->INT_64_TYPE_NAME() || ctx->type_name()->INT_TYPE_NAME())
        type = Type::INT_64;
    else if (ctx->type_name()->VOID_TYPE_NAME())
    {
        Writer::error() << "can't declare a void variable" << std::endl;
        return nullptr;
    }
    else if (ctx->type_name()->IDENTIFIER())
    {
        Writer::error() << "unknown type name '" << ctx->type_name()->IDENTIFIER()->getText() << "'" << std::endl;
        return nullptr;
    }

    CProgASTDeclaration* declaration = new CProgASTDeclaration(type);
    for(auto declarator_ctx : ctx->declarator())
    {
        CProgASTIdentifier* identifier = nullptr;
        CProgASTAssignment* initializer = nullptr;
        if(declarator_ctx->IDENTIFIER() != nullptr)
        {
            identifier = new CProgASTIdentifier(declarator_ctx->IDENTIFIER()->getText());
        }
        else if(declarator_ctx->assignment() != nullptr)
        {
            initializer = visit(declarator_ctx->assignment()).as<CProgASTAssignment*>();
            identifier = new CProgASTIdentifier(declarator_ctx->assignment()->IDENTIFIER()->getText());
        }
        declaration->add_declarator(new CProgASTDeclarator(identifier, initializer));
    }
    return declaration;
}

antlrcpp::Any CProgCSTVisitor::visitAssignment(CProgParser::AssignmentContext *ctx)
{
    CProgASTIdentifier* identifier = new CProgASTIdentifier(ctx->IDENTIFIER()->getText());
    CProgASTExpression* expression = visit(ctx->expr()).as<CProgASTExpression*>();
    return new CProgASTAssignment(identifier, expression);
}

antlrcpp::Any CProgCSTVisitor::visitExpr(CProgParser::ExprContext *ctx)
{
    CProgASTExpression* rexpr = nullptr;
    size_t op_size = ctx->expr().size();
    if (op_size == 0)
    {
        if(ctx->INT_LITERAL())
        {
            rexpr = new CProgASTIntLiteral(std::stoi(ctx->INT_LITERAL()->getText()));
        }
        else if(ctx->CHAR_LITERAL())
        {
            std::string literal = ctx->CHAR_LITERAL()->getText();
            rexpr = new CProgASTCharLiteral(literal.substr(1, literal.size()-2));
        }
        else if(ctx->IDENTIFIER())
        {
            rexpr = new CProgASTIdentifier(ctx->IDENTIFIER()->getText());
        }
    }
    else if (op_size == 1)
    {
        CProgASTExpression* expr = visit(ctx->expr(0)).as<CProgASTExpression*>();
        if (ctx->PAR_OP)
        {
            rexpr = expr;
        }
        else if (ctx->POSTFIX_OP)
        {
            // TODO
        }
        else if (ctx->ARG_OP)
        {
            // TODO
        }
        else if (ctx->PREFIX_OP)
        {
            if(ctx->OP_PLUS())
            {
                // NOP
            }
            else if(ctx->OP_MINUS())
            {
                rexpr = new CProgASTUnaryMinus(expr);
            }
            else if(ctx->OP_PP())
            {
                // TODO
            }
            else if(ctx->OP_MM())
            {
                // TODO
            }
            else if(ctx->OP_NOT())
            {
                // TODO
            }
            else if(ctx->OP_BNOT())
            {
                // TODO
            }
        }
        else if (ctx->OP_ASGN())
        {
            CProgASTIdentifier *identifier = new CProgASTIdentifier(ctx->IDENTIFIER()->getText());
            rexpr = new CProgASTAssignment(identifier, expr);
        }
    }
    else if (op_size == 2)
    {
        CProgASTExpression* lhs = visit(ctx->expr(0)).as<CProgASTExpression*>();
        CProgASTExpression* rhs = visit(ctx->expr(1)).as<CProgASTExpression*>();
        if(ctx->OP_MUL())
        {
            rexpr = new CProgASTMultiplication(lhs, rhs);
        }
        else if(ctx->OP_DIV())
        {
            rexpr = new CProgASTDivision(lhs, rhs);
        }
        else if(ctx->OP_MOD())
        {
            rexpr = new CProgASTModulo(lhs, rhs);
        }
        else if(ctx->OP_PLUS())
        {
            rexpr = new CProgASTAddition(lhs, rhs);
        }
        else if(ctx->OP_MINUS())
        {
            rexpr = new CProgASTSubtraction(lhs, rhs);
        }else if(ctx->OP_BAND())
        {
            rexpr = new CProgASTBAnd(lhs, rhs);
        }else if(ctx->OP_BOR())
        {
            rexpr = new CProgASTBOr(lhs, rhs);
        }else if(ctx->OP_BXOR())
        {
            rexpr = new CProgASTBXor(lhs, rhs);
        }
    }
    return rexpr;
}

antlrcpp::Any CProgCSTVisitor::visitArg_list(CProgParser::Arg_listContext *ctx)
{
    return visitChildren(ctx);
}
