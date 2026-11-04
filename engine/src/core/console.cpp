#include "console.h"
#include "memory/linear_allocator.h"
#include "asserts.h"
#include "containers/stack.h"
//#include <new>

static constexpr u32 MAX_CONSUMER_COUNT = 10;

Console* Console::Initialize(u64& MemoryRequirement, void* memory)
{
    MemoryRequirement = sizeof(Console) + sizeof(Consumer) * MAX_CONSUMER_COUNT;
    
    if (!memory) {
        return nullptr;
    }

    auto ConsumerMem = reinterpret_cast<Consumer*>(reinterpret_cast<u8*>(memory) + sizeof(Console));

    state = new(memory) Console(ConsumerMem);
    if (!state) {
        return nullptr;
    }
    
    return state;
}

void Console::Shutdown()
{
    if (state) {
        state->RegisteredCommands.~DArray();

        MemorySystem::ZeroMem(state, sizeof(Console) + (sizeof(Consumer) * MAX_CONSUMER_COUNT));
    }

    state = nullptr;
}

void Console::RegisterConsumer(void *inst, PFN_ConsoleConsumerWrite callback, u8& OutConsumerID)
{
    if (state) {
        MASSERT_MSG(state->ConsumerCount + 1 < MAX_CONSUMER_COUNT, "Достигнуто максимальное количество потребителей консолей.");

        Consumer& consumer = state->consumers[state->ConsumerCount];
        consumer.instance = inst;
        consumer.callback = callback;
        OutConsumerID = state->ConsumerCount;
        state->ConsumerCount++;
    }
}

void Console::UpdateConsumer(u8 ConsumerID, void *inst, PFN_ConsoleConsumerWrite callback)
{
    if (state) {
        MASSERT_MSG(ConsumerID < state->ConsumerCount, "Идентификатор потребителя недействителен.");

        auto& consumer = state->consumers[ConsumerID];
        consumer.instance = inst;
        consumer.callback = callback;
    }
}

void Console::WriteLine(Log::Level level, const char *message)
{
    if (state) {
        // Сообщите каждому потребителю, что строка была добавлена.
        for (u8 i = 0; i < state->ConsumerCount; ++i) {
            auto& consumer = state->consumers[i];
            if (consumer.callback) {
                consumer.callback(consumer.instance, level, message);
            }
        }
    }
}

bool Console::RegisterCommand(const char *command, u8 ArgCount, PFN_ConsoleCommand func)
{
    MASSERT_MSG(state && command, "Console::RegisterCommand требует состояние и допустимую команду");

    // Убедитесь, что она еще не существует.
    const u32 CommandCount = state->RegisteredCommands.Length();
    for (u64 i = 0; i < CommandCount; ++i) {
        if (state->RegisteredCommands[i].name.Comparei(command)) {
            MERROR("Команда уже зарегистрирована: %s", command);
            return false;
        }
    }

    Command* NewCommand = state->RegisteredCommands.PushBack();
    NewCommand->name = command;
    NewCommand->ArgCount = ArgCount;
    NewCommand->func = func;

    return true;
}

bool Console::UnregisterCommand(const char *command)
{
    MASSERT_MSG(state && command, "Console::UnregisterCommand требует состояния и допустимой команды");

    // Убедитесь, что его еще нет.
    const u32 CommandCount = state->RegisteredCommands.Length();
    for (u32 i = 0; i < CommandCount; ++i) {
        if (state->RegisteredCommands[i].name.Comparei(command)) {
            // Команда найдена, удалите ее.
            state->RegisteredCommands.PopAt(i);
            return true;
        }
    }

    return false;
}

Console::Object *Console::GetObject(const MString &name, Object *parent)
{
    if (parent) {
        u32 PropertyCount = parent->properties.Length();
        for (u32 i = 0; i < PropertyCount; ++i) {
            Object& obj = parent->properties[i];
            if (name.Comparei(obj.name)) {
                return &obj;
            }
        }
    } else {
        u32 RegisteredObjectLen = RegisteredObjects.Length();
        for (u32 i = 0; i < RegisteredObjectLen; ++i) {
            Object& obj = RegisteredObjects[i];
            if (name.Comparei(obj.name)) {
                return &obj;
            }
        }
    }
    return nullptr;
}

Console::Object *Console::GetObject(const char *name, Object *parent)
{
    if (parent) {
        u32 PropertyCount = parent->properties.Length();
        for (u32 i = 0; i < PropertyCount; ++i) {
            Object& obj = parent->properties[i];
            if (obj.name.Comparei(name)) {
                return &obj;
            }
        }
    } else {
        u32 RegisteredObjectLen = RegisteredObjects.Length();
        for (u32 i = 0; i < RegisteredObjectLen; ++i) {
            Object& obj = RegisteredObjects[i];
            if (obj.name.Comparei(name)) {
                return &obj;
            }
        }
    }
    return nullptr;
}

u32 Console::ObjectToU32(const Object& obj) 
{
    return *((u32*)obj.block);
}

i32 Console::ObjectToI32(const Object& obj) 
{
    return *((i32*)obj.block);
}

f32 Console::ObjectToF32(const Object& obj) 
{
    return *((f32*)obj.block);
}

bool Console::ObjectToBool(const Object& obj) 
{
    return *((bool*)obj.block);
}

void Console::ObjectPrint(u8 indent, Object& obj) {
    char IndentBuffer[513] {};
    u16 idx = 0;
    for (; idx < (indent * 2); idx += 2) {
        IndentBuffer[idx + 0] = ' ';
        IndentBuffer[idx + 1] = ' ';
    }
    IndentBuffer[idx] = 0;

    switch (obj.type) {
        case Console::INT32:
            MINFO("%s%i", IndentBuffer, *((i32*)obj.block));
            break;
        case Console::UINT32:
            MINFO("%s%u", IndentBuffer, *((u32*)obj.block));
            break;
        case Console::F32:
            MINFO("%s%f", IndentBuffer, *((f32*)obj.block));
            break;
        case Console::BOOL: {
            bool val = *((bool*)obj.block);
            MINFO("%s%s", IndentBuffer, val ? "true" : "false");
        }   break;
        case Console::STRUCT:
            if (obj.properties) {
                MINFO("%s", obj.name.c_str());
                indent++;
                u32 len = obj.properties.Length();
                for (u32 i = 0; i < len; ++i) {
                    ObjectPrint(indent, obj.properties[i]);
                }
            }
            break;
    }
}

bool Console::OperatorTypeFromToken(const Token token, const char* source, OperatorType& OutType) 
{
    if (!source || token.type != TokenType::OPERATOR) {
        MERROR("Функция Console::OperatorTypeFromToken требует наличия корректного указателя на исходные данные и токена типа оператора.");
        return false;
    }

    // ПРИМЕЧАНИЕ: Убедитесь, что длина токена задана здесь, чтобы её не пришлось проверять где-либо далее.
    if (token.length < 1 || token.length > 2) {
        MERROR("Операторы всегда состоят из 1 или 2 символов.");
        return false;
    }

    char c = source[token.StartIndex];
    char next = source[token.StartIndex + 1];
    OutType = OperatorType::UNKNOWN;
    u8 ErrorOffset = 0;
    switch (c) {
        case '=':
            if (token.length == 1) {
                OutType = OperatorType::ASSIGN;
            } else if (next == '=') {
                OutType = OperatorType::EQUALITY;
            } else {
                ErrorOffset = 1;
            }
            break;
        case '!': {
            if (token.length == 1) {
                OutType = OperatorType::NOT;
            } else if (next == '=') {
                OutType = OperatorType::INEQUALITY;
            } else {
                ErrorOffset = 1;
            }
            break;
        }
        case '<': {
            if (token.length == 1) {
                OutType = OperatorType::LESSTHAN;
            } else if (next == '=') {
                OutType = OperatorType::LESSTHAN_OR_EQUAL;
            } else {
                ErrorOffset = 1;
            }
            break;
        }
        case '>': {
            if (token.length == 1) {
                OutType = OperatorType::GREATERTHAN;
            } else if (next == '=') {
                OutType = OperatorType::GREATERTHAN_OR_EQUAL;
            } else {
                ErrorOffset = 1;
            }
            break;
        }
        case '+': {
            if (token.length == 1) {
                OutType = OperatorType::ADD;
            } else if (next == '=') {
                OutType = OperatorType::ADD_ASSIGN;
            } else if (next == '+') {
                OutType = OperatorType::INCREMENT;
            } else {
                ErrorOffset = 1;
            }
            break;
        }
        case '-': {
            if (token.length == 1) {
                OutType = OperatorType::SUBTRACT;
            } else if (next == '=') {
                OutType = OperatorType::SUBTRACT_ASSIGN;
            } else if (next == '-') {
                OutType = OperatorType::DECREMENT;
            } else {
                ErrorOffset = 1;
            }
            break;
        }
        case '*': {
            if (token.length == 1) {
                OutType = OperatorType::MULTIPLY;
            } else if (next == '=') {
                OutType = OperatorType::MULTIPLY_ASSIGN;
            } else {
                ErrorOffset = 1;
            }
            break;
        }
        case '/': {
            if (token.length == 1) {
                OutType = OperatorType::DIVIDE;
            } else if (next == '=') {
                OutType = OperatorType::DIVIDE_ASSIGN;
            } else {
                ErrorOffset = 1;
            }
            break;
        }
        case '%': {
            if (token.length == 1) {
                OutType = OperatorType::MODULUS;
            } else if (next == '=') {
                OutType = OperatorType::MODULUS_ASSIGN;
            } else {
                ErrorOffset = 1;
            }
            break;
        }
        default:
            ErrorOffset = 0;
            break;
    }

    bool result = (OutType != OperatorType::UNKNOWN);
    if (!result) {
        MERROR("Неожиданный символ «%c» в позиции %u.", source[token.StartIndex + ErrorOffset], token.StartIndex + ErrorOffset);
    }
    return result;
}

bool Console::TokenizeSource(const char* source, Stack<Token>& OutTokens) 
{
    if (!source) {
        MERROR("Для работы TokenizeSource требуются корректные указатели на source и OutTokens.");
        return false;
    }

    u32 CharLength = MString::Length(source);

    using enum TokenType;

    Token CurrentToken {};
    for (u32 i = 0; i < CharLength; ++i) {
        i32 codepoint;
        u8 advance;
        if (!MString::BytesToCodepoint(source, i, codepoint, advance)) {
            // Если это не удастся, не пытайтесь продолжать синтаксический разбор, поскольку символы не распознаны; это может привести к непредсказуемому поведению.
            MERROR("Ошибка при обработке выражения — не удалось декодировать кодовую точку в позиции: %u", i);
            return false;
        }

        switch (codepoint) {
            case ' ':
            case '\t': {
                if (CurrentToken.type == WHITESPACE) {
                    CurrentToken.length++;
                } else {
                    // Если тип неизвестен, начать создание токена пробельных символов.
                    if (CurrentToken.type == UNKNOWN) {
                        CurrentToken.length = 1;
                        CurrentToken.StartIndex = i;
                        CurrentToken.type = WHITESPACE;
                    } else if (CurrentToken.type == STRING_LITERAL) {
                        // Для строк — просто увеличить длину.
                        CurrentToken.length++;
                    } else {
                        // Если тип токена известен, добавить текущий токен в список.
                        if (CurrentToken.type != UNKNOWN) {
                            OutTokens.Push(CurrentToken);
                        }

                        // Затем начать создание нового токена пробельных символов.
                        CurrentToken.length = 1;
                        CurrentToken.StartIndex = i;
                        CurrentToken.type = WHITESPACE;
                    }
                }
            } break;
            case '{': {
                // Если тип токена известен, добавить текущий токен в список.
                if (CurrentToken.type != UNKNOWN) {
                    OutTokens.Push(CurrentToken);
                }

                // Добавить новый токен открывающей скобки.
                CurrentToken.length = 1;
                CurrentToken.StartIndex = i;
                CurrentToken.type = OPEN_PAREN;
                OutTokens.Push(CurrentToken);

                // Сбросить тип в «неизвестный».
                CurrentToken.type = UNKNOWN;
                CurrentToken.length = 0;
                CurrentToken.StartIndex = 0;
            } break;
            case '}': {
                // Добавить текущий токен, если его тип — не «неизвестный».
                if (CurrentToken.type != UNKNOWN) {
                    OutTokens.Push(CurrentToken);
                }

                // Добавить новый токен закрывающей скобки.
                CurrentToken.length = 1;
                CurrentToken.StartIndex = i;
                CurrentToken.type = CLOSE_PAREN;
                OutTokens.Push(CurrentToken);

                // Установить тип обратно в «неизвестный».
                CurrentToken.type = UNKNOWN;
                CurrentToken.length = 0;
                CurrentToken.StartIndex = 0;
            } break;
            case '\n': {
                // Добавьте текущий токен, если его тип — не «неизвестный».
                if (CurrentToken.type != UNKNOWN) {
                    OutTokens.Push(CurrentToken);
                }

                // Добавить новый токен новой строки.
                CurrentToken.length = 1;
                CurrentToken.StartIndex = i;
                CurrentToken.type = NEWLINE;
                OutTokens.Push(CurrentToken);

                // Установить значение "неизвестно" обратно.
                CurrentToken.type = UNKNOWN;
                CurrentToken.length = 0;
                CurrentToken.StartIndex = 0;
            } break;
            case '"': {
                // ЗАДАЧА: обработать управляющие последовательности, такие как «\"».
                //
                // Если мы не внутри строки, сохранить текущий токен и начать новый токен-строку.
                if (CurrentToken.type != STRING_LITERAL && CurrentToken.type != UNKNOWN) {
                    OutTokens.Push(CurrentToken);

                    CurrentToken.length = 1;
                    CurrentToken.StartIndex = i;
                    CurrentToken.type = STRING_LITERAL;
                } else if (CurrentToken.type == STRING_LITERAL) {
                    // Если мы уже внутри строки, закрыть её и сохранить токен.
                    CurrentToken.length++;
                    OutTokens.Push(CurrentToken);

                    // Установить значение «неизвестно»
                    CurrentToken.type = UNKNOWN;
                    CurrentToken.length = 0;
                    CurrentToken.StartIndex = 0;
                }
            } break;
            case '=':
            case '!':
            case '<':
            case '>':
            case '+':
            case '-':
            case '*':
            case '/':
            case '%': {
                // Операторы.

                if (CurrentToken.type != OPERATOR) {
                    // Добавить текущий токен, если его тип известен.
                    if (CurrentToken.type != UNKNOWN) {
                        OutTokens.Push(CurrentToken);
                    }

                    // Начать новый токен оператора.
                    CurrentToken.StartIndex = i;
                    CurrentToken.length = 1;
                    CurrentToken.type = OPERATOR;
                    // Оператор будет завершен и добавлен на следующей итерации, если встретится символ другого типа.
                } else {
                    // Уже внутри токена оператора.
                    // Проверить, чтобы длина не превышала 2 — в противном случае токен считается недопустимым.
                    if (CurrentToken.length >= 2) {
                        MERROR("Недопустимый символ в позиции %u.", i);
                        return false;
                    }

                    // Допустимы только некоторые из них.
                    switch (codepoint) {
                        case '=': {
                            // Допустимо, если предыдущим символом был !, =, +, -, /, * или %.
                            char prev = source[CurrentToken.StartIndex];
                            switch (prev) {
                                case '!':
                                case '=':
                                case '+':
                                case '-':
                                case '/':
                                case '*':
                                case '%':
                                    CurrentToken.length++;
                                    break;
                                default:
                                    MERROR("Неожиданный символ в позиции %u.", i);
                                    return false;
                            }
                        }
                        case '+':
                        case '-': {
                            // Допустимо, только если предыдущий символ совпадает.
                            char prev = source[CurrentToken.StartIndex];
                            if (((char)codepoint) == prev) {
                                CurrentToken.length++;
                            } else {
                                MERROR("Неожиданный символ в позиции %u.", i);
                                return false;
                            }
                        } break;
                        case '!':
                        case '%':
                        case '/':
                        case '*':
                        case '<':
                        case '>': {
                            // Недопустим в качестве второго символа.
                            MERROR("Неожиданный символ в позиции %u.", i);
                            return false;
                        }
                    }
                }
            } break;
            case '0':
            case '1':
            case '2':
            case '3':
            case '4':
            case '5':
            case '6':
            case '7':
            case '8':
            case '9': {
                if (CurrentToken.type != NUMERIC_LITERAL) {
                    // Добавить текущий токен, если его тип известен.
                    if (CurrentToken.type != UNKNOWN) {
                        OutTokens.Push(CurrentToken);
                    }

                    // Начать новый токен числового литерала.
                    CurrentToken.length = 1;
                    CurrentToken.StartIndex = i;
                    CurrentToken.type = NUMERIC_LITERAL;

                } else {
                    // Добавить символ к текущему числовому литералу. Безопасно перейти к следующему символу.
                    CurrentToken.length++;
                }
            } break;
            case '.': {
                if (CurrentToken.type == NUMERIC_LITERAL || CurrentToken.type == STRING_LITERAL) {
                    // Добавить символ к текущему токену. Безопасно перейти к следующему символу.
                    CurrentToken.length++;
                } else {
                    // Добавить текущий токен, если его тип известен.
                    if (CurrentToken.type != UNKNOWN) {
                        OutTokens.Push(CurrentToken);
                    }

                    // Создайте новый токен «точка» и добавьте его в список.
                    CurrentToken.length = 1;
                    CurrentToken.StartIndex = i;
                    CurrentToken.type = DOT;
                    OutTokens.Push(CurrentToken);

                    // Установите тип обратно в «неизвестный».
                    CurrentToken.type = UNKNOWN;
                    CurrentToken.length = 0;
                    CurrentToken.StartIndex = 0;
                }
            } break;
            case ',': {
                if (CurrentToken.type == STRING_LITERAL) {
                    // Добавьте символ к текущему токену. Сохраните его, продвинувшись на один символ вперёд.
                    CurrentToken.length++;
                } else {
                    // Если тип текущего токена известен, добавьте его в список.
                    if (CurrentToken.type != UNKNOWN) {
                        OutTokens.Push(CurrentToken);
                    }

                    // Создайте новый токен «запятая» и добавьте его в список.
                    CurrentToken.length = 1;
                    CurrentToken.StartIndex = i;
                    CurrentToken.type = COMMA;
                    OutTokens.Push(CurrentToken);

                    // Установите тип обратно в «неизвестный».
                    CurrentToken.type = UNKNOWN;
                    CurrentToken.length = 0;
                    CurrentToken.StartIndex = 0;
                }
            } break;
            case ';': {
                if (CurrentToken.type == STRING_LITERAL) {
                    // Добавьте символ к текущему токену.
                    CurrentToken.length++;
                } else {
                    // Если тип текущего токена известен (не «неизвестный»), добавьте его в список токенов.
                    if (CurrentToken.type != UNKNOWN) {
                        OutTokens.Push(CurrentToken);
                    }

                    // Создайте новый токен завершения инструкции и добавьте его в список.
                    CurrentToken.length = 1;
                    CurrentToken.StartIndex = i;
                    CurrentToken.type = END_STATEMENT;
                    OutTokens.Push(CurrentToken);

                    // Сбросьте тип токена на «неизвестный».
                    CurrentToken.type = UNKNOWN;
                    CurrentToken.length = 0;
                    CurrentToken.StartIndex = 0;
                }
            } break;
            default: {
                // Любой другой символ следует рассматривать как часть строки или идентификатора — в зависимости от того, какой токен формируется в данный момент.
                // ПРИМЕЧАНИЕ: используйте значение `advance` (сдвиг), чтобы корректно увеличить длину токена при обработке идентификаторов или строк в кодировке UTF-8.
                if (CurrentToken.type == STRING_LITERAL) {
                    CurrentToken.length += advance;
                } else if (CurrentToken.type == IDENTIFIER) {
                    CurrentToken.length += advance;
                } else if (CurrentToken.type == UNKNOWN) {
                    // Если текущий тип — «неизвестный», считайте символ началом идентификатора.
                    CurrentToken.type = IDENTIFIER;
                    CurrentToken.StartIndex = i;
                    CurrentToken.length = advance;
                } else {
                    // Если же в данный момент формируется токен другого типа, такая ситуация является недопустимой.
                    MERROR("Неожиданный символ в позиции %u", i);
                    return false;
                }
            } break;
        }
        // Сдвиньте итератор вперед на величину `advance`.
        i += advance;
    }

    // Если последний токен имеет известный тип, добавьте его в список.
    if (CurrentToken.type != UNKNOWN) {
        OutTokens.Push(CurrentToken);
    }

    // В завершение добавьте токен конца файла (EOF).
    CurrentToken.type = EOF;
    CurrentToken.StartIndex += CurrentToken.length;
    CurrentToken.length = 0;
    OutTokens.Push(CurrentToken);

    return true;
}

bool Console::LookAheadToken(const Stack<Token>& tokens, Token& OutToken, bool SkipWhitespace, bool SkipNewlines) 
{
    if (!tokens.Peek(OutToken)) {
        return false;
    }

    if (SkipWhitespace) {
        while (OutToken.type == TokenType::WHITESPACE) {
            if (!tokens.Peek(OutToken)) {
                return false;
            }
        }
    }

    if (SkipNewlines) {
        while (OutToken.type == TokenType::NEWLINE) {
            if (!tokens.Peek(OutToken)) {
                return false;
            }
        }
    }

    return true;
}

bool Console::NumericTokenParse(const char* source, const Token& token, ObjectType& OutType, Numeric& OutResult) 
{
    bool result = false;
    switch (token.type)
    {
    case TokenType::NUMERIC_LITERAL: {
        // Извлеките значение в виде строки, чтобы определить его тип. 
        // При разборе числовых литералов они всегда интерпретируются либо как
        // числа с плавающей точкой (если присутствует символ «.»), либо как целые числа типа i32.
        char* value = AllocateTemp<char>(sizeof(char) * token.length + 1);
        if (MString::IndexOf(value, '.')) {
            if (!MString::ToF32(value, OutResult.f)) {
                MERROR("Не удалось распознать токен как число с плавающей запятой в позиции %i.", token.StartIndex);
                result = false;
            }
            OutType = ObjectType::F32;
            result = true;
        } else {
            if (!MString::ToInt(value, OutResult.i)) {
                MERROR("Не удалось распознать токен как целое число в позиции %i.", token.StartIndex);
                result = false;
            }
            OutType = ObjectType::INT32;
            result = true;
        }
        // MemorySystem::Free(value, sizeof(char) * token.length + 1, Memory::String);
    }   break;
    
    case TokenType::IDENTIFIER: {
        // Извлеките имя и попытайтесь найти объект консоли.
        char* name = AllocateTemp<char>(sizeof(char) * token.length + 1);
        MString::Mid(name, source, token.StartIndex, token.length);
        Object* obj = GetObject(name);
        if (!obj) {
            MERROR("Идентификатор «%s» не определён.", name);
            result = false;
        } else {
            switch (obj->type) {
                case ObjectType::INT32:
                    OutResult.i = ObjectToI32(*obj);
                    OutType = obj->type;
                    result = true;
                    break;
                case ObjectType::UINT32:
                    OutResult.u = ObjectToU32(*obj);
                    OutType = obj->type;
                    result = true;
                    break;
                case ObjectType::F32:
                    OutResult.f = ObjectToF32(*obj);
                    OutType = obj->type;
                    result = true;
                    break;
                default:
                    //Неверный тип токена.
                    MERROR("В позиции %i ожидался числовой токен (i32, u32 или f32)", token.StartIndex);
                    result = false;
                    break;
            }
        }
        // MemorySystem::Free(name, sizeof(char) * token.length + 1, Memory::String);
    } break;
    
    default:
        break;
    }

    return result;
}

bool Console::EvaluateNumericalExpression(const char* source, const Token& a, const Token& v_operator, const Token* b, ObjectType& OutType, Numeric& OutResult) 
{
    Numeric NumericA, NumericB;
    ObjectType TypeA, TypeB;
    OperatorType OperatorType;

    bool IsUnary = true;

    // Определить тип оператора.
    if (!OperatorTypeFromToken(v_operator, source, OperatorType)) {
        MERROR("Не удалось вычислить числовое выражение из-за недопустимого оператора.");
        return false;
    }

    // Получить значения токенов.
    if (NumericTokenParse(source, a, TypeA, NumericA)) {
        MERROR("Не удалось вычислить числовое выражение. Подробности см. в журналах.");
        return false;
    }

    // Если существует `b`, то оператор не является унарным.
    if (b) {
        if (NumericTokenParse(source, *b, TypeB, NumericB)) {
            MERROR("Не удалось вычислить числовое выражение. Подробности см. в журналах.");
            return false;
        }
        IsUnary = false;
    }

    if (IsUnary) {
        if (TypeA == ObjectType::F32) {
            if (OperatorType == OperatorType::INCREMENT) {
                MWARN("Инкремент значения типа float в позиции %i. Может привести к неожиданным результатам.", a.StartIndex);
                OutResult.f = NumericA.f + 1;
            } else if (OperatorType == OperatorType::DECREMENT) {
                MWARN("Декремент значения типа float в позиции %i. Может привести к неожиданным результатам.", a.StartIndex);
                OutResult.f = NumericA.f - 1;
            } else {
                MERROR("Неожиданный унарный оператор в позиции %i", a.StartIndex);
                return false;
            }
        } else {
            // Рассматривать как `int`.
            if (OperatorType == OperatorType::INCREMENT) {
                OutResult.i = NumericA.i + 1;
            } else if (OperatorType == OperatorType::DECREMENT) {
                OutResult.i = NumericA.i - 1;
            } else {
                MERROR("Неожиданный унарный оператор в позиции %i", a.StartIndex);
                return false;
            }
        }
    } else {
        // Бинарный оператор.

        // Если хотя бы один из операндов — `float`, то всё выражение следует рассматривать как `float`.
        if (TypeA == ObjectType::F32 || TypeB == ObjectType::F32) {
            OutType = ObjectType::F32;
            // Убедиться, что оба операнда приведены к типу `float`, если они ещё не представлены в этом формате.
            f32 f_a, f_b;
            if (TypeA == ObjectType::F32) {
                f_a = NumericA.f;
            } else if (TypeA == ObjectType::INT32) {
                f_a = (f32)NumericA.i;
            } else if (TypeA == ObjectType::UINT32) {
                f_a = (f32)NumericA.u;
            } else {
                MERROR("Неподдерживаемый тип объекта в позиции %i.", a.StartIndex);
                return false;
            }
            if (TypeB == ObjectType::F32) {
                f_b = NumericB.f;
            } else if (TypeB == ObjectType::INT32) {
                f_b = (f32)NumericB.i;
            } else if (TypeB == ObjectType::UINT32) {
                f_b = (f32)NumericB.u;
            } else {
                MERROR("Неподдерживаемый тип объекта в позиции %i.", b->StartIndex);
                return false;
            }

            switch (OperatorType) {
                case OperatorType::ADD:
                case OperatorType::ADD_ASSIGN:
                    OutResult.f = f_a + f_b;
                    break;
                case OperatorType::SUBTRACT:
                case OperatorType::SUBTRACT_ASSIGN:
                    OutResult.f = f_a - f_b;
                    break;
                    // ЗАДАЧА: остальные типы.
                default:
                    break;
            }

        } else {
            // В противном случае рассматривать как `i32`.
            OutType = ObjectType::INT32;
        }
    }
    
    return true;
}

bool Console::SourceTokensEvaluate(const char* source, DArray<Token>& tokens, ObjectType& OutType, void* OutValue) 
{
    bool result = false;

    u32 TokenCount = tokens.Length();
    // ЗАДАЧА: Пройтись по токенам, чтобы получить результат.
    for (u32 i = 0; i < TokenCount; ++i) {
        Token& token = tokens[i];

        // Пропустить пробельные символы
        if (token.type == TokenType::WHITESPACE) {
            continue;
        }
    }
    //

    return result;
}

bool Console::ExpressionParse2(const char* ExpressionStr, ObjectType& OutType, void* OutValue) 
{
    // Создайте копию строки и выполните её обрезку (trim).
    MString ExpressionCopy { ExpressionStr };
    u32 CharLength = ExpressionCopy.Size() - 1;
    ExpressionCopy.Trim();

    for (u32 i = 0; i < CharLength; ++i) {
        i32 codepoint;
        u8 advance;
        if (!MString::BytesToCodepoint(ExpressionStr, i, codepoint, advance)) {
            // Если операция не удалась, прекратите дальнейший разбор, так как символы не распознаны — это может привести к неопределённому поведению.
            MERROR("Error processing expression - unable to decode codepoint at position: %u", i);
            return false;
        } else {
            // Сдвиньте итератор вперёд на величину advance.
            i += advance;
        }
    }

    return true;
}

bool Console::ExpressionParse(const char* expression, ObjectType& OutType, void* OutValue) 
{
    bool result = true;
    MString ExpressionCopy { expression };
    ExpressionCopy.Trim();

    // Поддерживаются следующие операторы: =, ==, !=, /, *, +, -, %

    bool OperatorFound = false;

    if (!OperatorFound) {
        i32 SpaceIndex = ExpressionCopy.IndexOf(' ');
        if (SpaceIndex != -1) {
            MERROR("Неожиданный токен в позиции %i", SpaceIndex);
            result = false;
            return false;
        }

        // Проверьте наличие оператора «точка», который указывает на свойство структуры.
        bool IdentifierFound = false;
        i32 DotIndex = ExpressionCopy.IndexOf('.');
        if (DotIndex != -1) {
            // Проанализируйте каждую часть и определите иерархию структур и свойств.
            DArray<Substring> parts;
            u32 SplitCount = ExpressionCopy.Split('.', parts, true, false);

            Object* parent = GetObject(parts[0]);
            for (u32 s = 1; s < SplitCount; ++s) {
                Object* obj = GetObject(parts[s], parent);
                if (obj) {
                    parent = obj;
                }
            }
            if (parent) {
                ObjectPrint(0, *parent);
                IdentifierFound = true;
                result = true;
            }
        } else {
            Object* obj = GetObject(ExpressionCopy);
            if (obj) {
                ObjectPrint(0, *obj);
                IdentifierFound = true;
                result = true;
            }
        }

        if (!IdentifierFound) {
            MERROR("Идентификатор не определён: «%s».", ExpressionCopy.c_str());
            result = false;
            return false;
        }
    }

    // ЗАДАЧА:
    // Пример выражения:
    // the_thing = thing_2
    // или:
    // the_thing
    // Если просто ввести имя объекта, его значение будет выведено в консоль.
    // Выражения также можно вычислять непосредственно по месту их записи.

    return result;
}

bool Console::ExecuteCommand(const MString &command)
{
    if (!command) {
        return false;
    }
    DArray<MString> parts;
    // ЗАДАЧА: Если строки когда-либо будут использоваться в качестве аргументов, это приведет к неправильному разделению.
    u32 PartCount = command.Split(' ', parts, true, false);
    if (PartCount < 1) {
        return false;
    }

    // Запишите строку обратно в консоль для справки.
    char temp[512] = {};
    MString::Format(temp, "-->%s", command.c_str());
    WriteLine(Log::Level::Info, temp);

    // Да, строки медленные. Но это консоль. Она не должна быть молниеносной...
    bool HasError = false;
    bool CommandFound = false;
    const u32 CommandCount = state->RegisteredCommands.Length();
    // Просмотрите зарегистрированные команды на предмет совпадения.
    for (u32 i = 0; i < CommandCount; ++i) {
        auto& cmd = state->RegisteredCommands[i];
        if (parts[0].Comparei(cmd.name)) {
            CommandFound = true;
            u8 ArgCount = PartCount - 1;
            // Указанное количество аргументов должно соответствовать ожидаемому количеству аргументов для команды.
            if (state->RegisteredCommands[i].ArgCount != ArgCount) {
                MERROR("Консольная команда '%s' требует %u аргументов, но %u были предоставлены.", cmd.name.c_str(), cmd.ArgCount, ArgCount);
                HasError = true;
            } else {
                // Выполните ее, передав аргументы, если необходимо.
                ConsoleCommandContext context {
                    .ArgumentCount = cmd.ArgCount
                };

                if (cmd.ArgCount > 0) {
                    context.arguments = AllocateTemp<ConsoleCommandArgument>(sizeof(ConsoleCommandArgument) * cmd.ArgCount, true);
                    for (u8 j = 0; j < cmd.ArgCount; ++j) {
                        context.arguments[j].value = parts[j + 1].c_str();
                    }
                }

                cmd.func(context);

                // if (context.arguments) {
                //     DestroyArray(context.arguments, cmd.ArgCount);
                // }
            }
            break;
        }
    }

    if (!CommandFound) {
        MERROR("Команда '%s' не существует.", parts[0].c_str());
        HasError = true;
    }

    return !HasError;
}

bool Console::ObjectRegister(const char *ObjectName, void *object, ObjectType type)
{
    if (!object || !ObjectName) {
        MERROR("Для Console::ObjectRegister требуется корректный указатель на объект и имя объекта (ObjectName).");
        return false;
    }

    // Убедитесь, что такой объект еще не зарегистрирован.
    u32 ObjectCount = state->RegisteredObjects.Length();
    for (u32 i = 0; i < ObjectCount; ++i) {
        if (state->RegisteredObjects[i].name.Comparei(ObjectName)) {
            MERROR("Объект консоли уже зарегистрирован: «%s».", ObjectName);
            return false;
        }
    }

    Object* NewObject = state->RegisteredObjects.PushBack();
    NewObject->name = ObjectName;
    NewObject->type = type;
    NewObject->block = object;

    return true;
}

bool Console::ObjectUnregister(const char *ObjectName)
{
    if (!ObjectName) {
        MERROR("Для Console::ObjectUnregister требуется корректный указатель object_name.");
        return false;
    }

    // Убедитесь, что объект существует.
    u32 ObjectCount = state->RegisteredObjects.Length();
    for (u32 i = 0; i < ObjectCount; ++i) {
        if (state->RegisteredObjects[i].name.Comparei(ObjectName)) {
            // Объект найден, удаляем его.
            state->RegisteredObjects.PopAt(i);
            return true;
        }
    }

    return false;
}

bool Console::ObjectAddProperty(const char *ObjectName, const char *PropertyName, void *property, ObjectType type)
{
   if (!property || !ObjectName || !PropertyName) {
        MERROR("Для работы Console::ObjectAddProperty требуются корректный указатель на свойство, а также имя свойства и имя объекта.");
        return false;
    }

    // Сначала убедитесь, что объект существует.
    u32 ObjectCount = state->RegisteredObjects.Length();
    for (u32 i = 0; i < ObjectCount; ++i) {
        if (state->RegisteredObjects[i].name.Comparei(ObjectName)) {
            Object& obj = state->RegisteredObjects[i];
            // Объект найден; теперь убедитесь, что свойство с таким именем еще не существует.
            if (obj.properties) {
                u32 PropertyCount = obj.properties.Length();
                for (u32 j = 0; j < PropertyCount; ++j) {
                    if (obj.properties[j].name.Comparei(PropertyName)) {
                        MERROR("У объекта «%s» уже есть свойство с именем «%s».", ObjectName, PropertyName);
                        return false;
                    }
                }
            }

            // Создайте новое свойство, которое, по сути, является еще одним объектом.
            Object* NewObject = obj.properties.PushBack();
            NewObject->name = PropertyName;
            NewObject->type = type;
            NewObject->block = property;

            return true;
        }
    }

    MERROR("Объект консоли не найден: «%s».", ObjectName);
    return false;
}

// void Console::Object::Destroy()
// {
//     name.Destroy();
//     block = nullptr;
//     properties.Destroy();
// }

bool Console::ObjectRemoveProperty(const char *ObjectName, const char *PropertyName)
{
    if (!ObjectName || !PropertyName) {
        MERROR("Для Console::ObjectRemoveProperty требуются корректный указатель на свойство, а также имя свойства и имя объекта.");
        return false;
    }

    // Сначала убедитесь, что объект существует.
    u32 ObjectCount = state->RegisteredObjects.Length();
    for (u32 i = 0; i < ObjectCount; ++i) {
        
        if (state->RegisteredObjects[i].name.Comparei(ObjectName)) {
            Object& obj = state->RegisteredObjects[i];
            // Объект найден; теперь убедитесь, что свойство с таким именем отсутствует.
            if (obj.properties) {
                u32 PropertyCount = obj.properties.Length();
                for (u32 j = 0; j < PropertyCount; ++j) {
                    if (obj.properties[j].name.Comparei(PropertyName)) {
                        //obj.properties[j].Destroy();
                        obj.properties.PopAt(j);
                        return true;
                    }
                }
            }

            MERROR("Свойство «%s» не найдено у объекта консоли «%s».", ObjectName, PropertyName);
            return false;
        }
    }

    MERROR("Объект консоли не найден: «%s».", ObjectName);
    return false;
}
