#pragma once

#include "containers/darray.h"

/// @brief Функция записи потребителя консоли, которая вызывается каждый раз, когда происходит событие регистрации. Потребители должны реализовать это и обрабатывать ввод таким образом.
using PFN_ConsoleConsumerWrite = bool(*)(void*, Log::Level, const char*);

/// @brief Представляет значение аргумента одной консольной команды. Всегда представлено в виде строки, функция консольной команды должна интерпретировать и преобразовывать его в требуемый тип во время обработки.
struct ConsoleCommandArgument {
    const char* value;
};

/// @brief Контекст, который должен передаваться вместе с выполняемой консольной командой (т. е. аргументы команды).
struct ConsoleCommandContext {
    /// @brief Количество переданных аргументов.
    u8 ArgumentCount;
    /// @brief Массив аргументов.
    ConsoleCommandArgument* arguments;
};

/// @brief Указатель функции, который представляет зарегистрированную консольную команду и вызывается при срабатывании какого-либо средства консольного ввода (обычно консоли отладки).
using PFN_ConsoleCommand = void(*)(ConsoleCommandContext&);

template<typename T> class Stack;

class Console
{
public:
    Console(const Console&) = delete;
    Console& operator=(Console&) = delete;
    
    static Console* Initialize(u64& MemoryRequirement, void* memory);
    void Shutdown();

    MAPI static void RegisterConsumer(void* inst, PFN_ConsoleConsumerWrite callback, u8& OutConsumerID);

    /// @brief Обновляет экземпляр и обратный вызов для потребителя с заданным идентификатором.
    /// @param ConsumerID идентификатор потребителя для обновления.
    /// @param inst экземпляр потребителя.
    /// @param callback новая функция обратного вызова.
    MAPI static void UpdateConsumer(u8 ConsumerID, void* inst, PFN_ConsoleConsumerWrite callback);

    static void WriteLine(Log::Level level, const char* message);

    MAPI static bool RegisterCommand(const char* command, u8 ArgCount, PFN_ConsoleCommand func);
    
    /// @brief Отменяет регистрацию данной команды.
    /// @param command имя команды, регистрация которой отменяется.
    /// @return true в случае успеха; в противном случае false.
    MAPI static bool UnregisterCommand(const char* command);

    MAPI static bool ExecuteCommand(const MString& command);

    enum ObjectType {
        INT32,
        UINT32,
        F32,
        BOOL,
        STRUCT
    };

    MAPI static bool ObjectRegister(const char* ObjectName, void* object, ObjectType type);

    MAPI static bool ObjectUnregister(const char* ObjectName);

    MAPI static bool ObjectAddProperty(const char* ObjectName, const char* PropertyName, void* property, ObjectType type);

    MAPI static bool ObjectRemoveProperty(const char* ObjectName, const char* PropertyName);

private:
    struct Consumer {
        PFN_ConsoleConsumerWrite callback;
        void* instance;
    };

    struct Command {
        MString name;
        u8 ArgCount;
        PFN_ConsoleCommand func;
    };

    struct Object {
        MString name;
        ObjectType type;
        void* block;
        DArray<Object> properties;
        // void Destroy();
    };

    u8 ConsumerCount;
    Consumer* consumers;

    DArray<Command> RegisteredCommands;

    // Массив зарегистрированных консольных объектов.
    DArray<Object> RegisteredObjects;

    constexpr Console(Consumer* consumers) : ConsumerCount(), consumers(consumers), RegisteredCommands(), RegisteredObjects() {}

    static inline Console* state;

    Object* GetObject(const MString& name, Object* parent = nullptr);
    Object* GetObject(const char* name, Object* parent = nullptr);
    u32 ObjectToU32(const Object& obj);
    i32 ObjectToI32(const Object& obj);
    f32 ObjectToF32(const Object& obj);
    bool ObjectToBool(const Object& obj);
    static void ObjectPrint(u8 indent, Object& obj);
    
    enum class TokenType {
        UNKNOWN,
        EOF,
        WHITESPACE,
        NEWLINE,
        OPEN_PAREN,
        CLOSE_PAREN,
        IDENTIFIER,
        OPERATOR,
        STRING_LITERAL,
        NUMERIC_LITERAL,
        DOT,
        COMMA,
        END_STATEMENT
    };

    struct Token {
        TokenType type;
        u32 StartIndex;
        u32 length;
    };

    enum class OperatorType {
        // Нераспознанный оператор.
        UNKNOWN,
        // =
        ASSIGN,
        // ==
        EQUALITY,
        // !=
        INEQUALITY,
        // !
        NOT,
        // <
        LESSTHAN,
        // <=
        LESSTHAN_OR_EQUAL,
        // >
        GREATERTHAN,
        // >=
        GREATERTHAN_OR_EQUAL,
        // +
        ADD,
        // +=
        ADD_ASSIGN,
        // -
        SUBTRACT,
        // -=
        SUBTRACT_ASSIGN,
        // *
        MULTIPLY,
        // *=
        MULTIPLY_ASSIGN,
        // /
        DIVIDE,
        // /=
        DIVIDE_ASSIGN,
        // %
        MODULUS,
        // %=
        MODULUS_ASSIGN,
        // ++
        INCREMENT,
        // --
        DECREMENT,
    };

    struct Expression {
        ObjectType ResultType;
        u32 TokenCount;
        Token* tokens;
    };

    union Numeric {
        i32 i;
        u32 u;
        f32 f;
    };

    bool OperatorTypeFromToken(const Token token, const char* source, OperatorType& OutType);
    bool TokenizeSource(const char* source, Stack<Token>& OutTokens);
    bool LookAheadToken(const Stack<Token>& tokens, Token& OutToken, bool SkipWhitespace, bool SkipNewlines);
    bool NumericTokenParse(const char* source, const Token& token, ObjectType& OutType, Numeric& OutResult);
    bool EvaluateNumericalExpression(const char* source, const Token& a, const Token& v_operator, const Token* b, ObjectType& OutType, Numeric& OutResult);
    static bool ParseExpression(const char* source, const Stack<Token>& tokens, ObjectType& OutResultType) {/* Определите, что это за выражение.*/ return true; }

    static bool EvaluateExpression(const char* source) { return true; }
    bool SourceTokensEvaluate(const char* source, DArray<Token>& tokens, ObjectType& OutType, void* OutValue);
    bool ExpressionParse2(const char* ExpressionStr, ObjectType& OutType, void* OutValue);
    bool ExpressionParse(const char* expression, ObjectType& OutType, void* OutValue);
};
