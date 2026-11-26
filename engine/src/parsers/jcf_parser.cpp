#include "jcf_parser.h"
#include "core/asserts.h"
#include "containers/parray.h"
#include "containers/stack.h"

enum class jcf_tokenize_mode 
{
    UNKNOWN,
    DEFINING_IDENTIFIER,
    WHITESPACE,
    STRING_LITERAL,
    NUMERIC_LITERAL,
    BOOLEAN,
    OPERATOR,
};

// Сбрасывает как текущий тип токена, так и режим токенизации в значение «неизвестно».
static void RESET_CURRENT_TOKEN_AND_MODE(jcf_token& CurrentToken, jcf_tokenize_mode& mode) 
{
    CurrentToken.type = jcf_token_type::Unknown;
    CurrentToken.start = 0;
    CurrentToken.end = 0;

    mode = jcf_tokenize_mode::UNKNOWN;
}

#ifdef MOON_DEBUG
static void _populate_token_content(jcf_token& token, MString& source) {
    MASSERT_MSG(token.start <= token.end, "Начало токена идёт после его конца, балбес!");
    char buffer[512] {};
    MASSERT_MSG((token.end - token.start) < 512, "Токен не помещается в буфер.");
    source.Mid(token.start, token.end - token.start);
    token.content = buffer;
}
#define POPULATE_TOKEN_CONTENT(t, source) _populate_token_content(t, source)
#else
// No-op (пустая операция)
#define POPULATE_CURRENT_TOKEN_CONTENT(t, source)
#endif

// Добавляет текущий токен, если его тип известен.
void jcf_parser::PushToken(jcf_token &token)
{
    if (token.type != jcf_token_type::Unknown) {
        POPULATE_TOKEN_CONTENT(token, FileContent);
        tokens.PushBack(token);
    }
}

bool jcf_parser::Tokenize(const char *source)
{
    if (!source) {
        MERROR("Функция jcf_parser_tokenize требует корректного указателя на источник");
        return false;
    }

    FileContent = source;

    // Убедитесь, что массив токенов парсера пуст.
    tokens.Clear();

    u32 CharLength = MString::Length(source);
    // u32 TextLengthUTF8 = MString::Length(source, true);

    jcf_tokenize_mode mode = jcf_tokenize_mode::DEFINING_IDENTIFIER;
    jcf_token CurrentToken{};
    // Предыдущий кодовый элемент (codepoint).
    i32 PrevCodepoint = 0;
    // Кодовый элемент, полученный 2 итерации назад.
    i32 PrevCodepoint2 = 0;

    bool EofReached = false;

    // Определите длину в символах и извлеките из неё соответствующий кодовый элемент.
    i32 codepoint = 0;
    for (u32 c = 0; c < CharLength; PrevCodepoint2 = PrevCodepoint, PrevCodepoint = codepoint) {
        if (EofReached) {
            break;
        }

        codepoint = source[c];
        // На сколько байтов продвинуться.
        u8 advance = 1;
        // ПРИМЕЧАНИЕ: Обработка кодовых точек UTF-8.
        if (!MString::BytesToCodepoint(source, c, codepoint, advance)) {
            MWARN("В строке обнаружен некорректный UTF-8; используется неизвестный кодовый символ -1");
            codepoint = -1;
        }

        if (mode == jcf_tokenize_mode::STRING_LITERAL) {
            // Обработать разбор строкового литерала.
            // Завершайте строку только в том случае, если предыдущий кодовый символ не был обратной косой чертой, 
            // ЛИБО если предыдущий символ был обратной косой чертой, а символ перед ним — тоже обратной косой чертой. 
            // Иными словами, необходимо убедиться, что обратная косая черта сама не экранирована, а кавычка также не является экранированной.
            if (codepoint == '"' && (PrevCodepoint != '\\' || PrevCodepoint2 == '\\')) {
                // Завершить строку, добавить токен в массив и вернуться к предыдущему режиму.
                PushToken(CurrentToken);
                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } else {
                // Остальные символы следует рассматривать как часть строки.
                CurrentToken.end += advance;
            }
            // ЗАДАЧА: Возможно, потребуется предусмотреть обработку других управляющих последовательностей, таких как \t, \n и т. д.

            // На данном этапе этот кодовый символ уже обработан, поэтому можно сразу перейти к следующему шагу.
            c += advance;
            continue;
        }

        // Символ не является частью строки, идентификатора, числового значения и т. д. — нужно определить, что делать дальше.
        switch (codepoint) {
            case '\n': {
                PushToken(CurrentToken);

                // Просто создайте новый токен и вставьте его.
                jcf_token NewlineToken { .type = jcf_token_type::Newline, .start = c, .end = c + advance };

                PushToken(NewlineToken);  // old

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case '\t':
            case '\r':
            case ' ': {
                if (mode == jcf_tokenize_mode::WHITESPACE) {
                    // Присоедините его к пробельному символу.
                    CurrentToken.end++;
                } else {
                    // Перед переходом в режим пробельных символов сохраните текущий токен.
                    PushToken(CurrentToken);
                    mode = jcf_tokenize_mode::WHITESPACE;
                    CurrentToken.type = jcf_token_type::Whitespace;
                    CurrentToken.start = c;
                    CurrentToken.end = c + advance;
                }
            } break;
            case '{': {
                PushToken(CurrentToken);

                // Создайте и отправьте новый токен для этого.
                jcf_token OpenBraceToken { .type = jcf_token_type::CurlyBraceOpen, .start = c, .end = c + advance };

                PushToken(OpenBraceToken);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case '}': {
                PushToken(CurrentToken);

                // Создайте и отправьте новый токен для этого.
                jcf_token CloseBraceToken { .type = jcf_token_type::CurlyBraceClose, .start = c, .end = c + advance };
                PushToken(CloseBraceToken);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case '[': {
                PushToken(CurrentToken);

                // Создайте и отправьте новый токен для этого.
                jcf_token OpenBracketToken = { .type = jcf_token_type::BracketOpen, .start = c, .end = c + advance };
                PushToken(OpenBracketToken);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case ']': {
                PushToken(CurrentToken);

                // Создайте и отправьте новый токен для этого.
                jcf_token CloseBracketToken = { .type = jcf_token_type::BracketClose, .start = c, .end = c + advance };
                PushToken(CloseBracketToken);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case '"': {
                PushToken(CurrentToken);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);

                // Переключиться в режим синтаксического разбора строк.
                mode = jcf_tokenize_mode::STRING_LITERAL;
                CurrentToken.type = jcf_token_type::StringLiteral;
                CurrentToken.start = c + advance;
                CurrentToken.end = c + advance;
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
                if (mode == jcf_tokenize_mode::NUMERIC_LITERAL) {
                    CurrentToken.end++;
                } else {
                    // Добавьте текущий токен.
                    PushToken(CurrentToken);

                    // Переключитесь в режим числового разбора.
                    mode = jcf_tokenize_mode::NUMERIC_LITERAL;
                    CurrentToken.type = jcf_token_type::NumericLiteral;
                    CurrentToken.start = c;
                    CurrentToken.end = c + advance;
                }
            } break;
            case '-': {
                // ПРИМЕЧАНИЕ: Всегда рассматривайте знак «минус» как оператор вычитания, 
                // независимо от того, как именно он используется (за исключением случая 
                // со строками, упомянутого выше). А уж как именно он будет применен — 
                // для отрицания числового литерала, для выполнения операции вычитания 
                // и т. д. — это определяется правилами грамматики.

                PushToken(CurrentToken);

                // Создайте и отправьте новый токен для этого.
                jcf_token MinusToken { .type = jcf_token_type::OperatorMinus, .start = c, .end = c + advance };
                PushToken(MinusToken);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case '+': {
                // ПРИМЕЧАНИЕ: Всегда рассматривайте символ «плюс» как оператор сложения, 
                // независимо от контекста его использования (за исключением случая со строками, 
                // упомянутого выше). А уже правила грамматики определяют, служит ли он в данном 
                // конкретном случае для указания положительного значения числового литерала, 
                // для выполнения операции сложения и т. д.

                PushToken(CurrentToken);

                // Создайте и отправьте новый токен для этого.
                jcf_token PlusToken = { .type = jcf_token_type::OperatorPlus, .start = c, .end = c + advance };
                PushToken(PlusToken);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case '/': {
                PushToken(CurrentToken);
                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);

                // Посмотрите вперёд: если за ним следует ещё одна косая черта, то остальная часть строки является комментарием. Пропустите всё до символа новой строки.
                if (source[c + 1] == '/') {
                    i32 cm = c + 2;
                    char ch = source[cm];
                    while (ch != '\n' && ch != '\0') {
                        cm++;
                        ch = source[cm];
                    }
                    if (cm > 0) {
                        // Перейдите к позиции за один символ до символа новой строки, чтобы этот символ новой строки был обработан.
                        // Это делается потому, что комментарий не должен подвергаться токенизации — вместо этого его следует игнорировать.
                        c = cm;
                    }
                    continue;
                } else {
                    // В противном случае его следует рассматривать как оператор косой черты.
                    // Создайте и отправьте новый токен для этого.
                    jcf_token SlashToken = { .type = jcf_token_type::OperatorSlash, .start = c, .end = c + advance };
                    PushToken(SlashToken);
                }

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case '*': {
                PushToken(CurrentToken);

                // Создайте и отправьте новый токен для этого.
                jcf_token asterisk_token = { .type = jcf_token_type::OperatorAsterisk, .start = c, .end = c + advance };
                PushToken(asterisk_token);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case '=': {
                PushToken(CurrentToken);

                // Создайте и отправьте новый токен для этого.
                jcf_token equal_token = { .type = jcf_token_type::OperatorEqual, .start = c, .end = c + advance };
                PushToken(equal_token);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case '.': {
                // NOTE: Always treat this as a dot token, regardless of use. It's up to the grammar
                // rules in the parser as to whether or not it's to be used as part of a numeric literal
                // or something else.

                PushToken(CurrentToken);

                // Создайте и отправьте новый токен для этого.
                jcf_token dot_token = { .type = jcf_token_type::OperatorDot, .start = c, .end = c + advance };
                PushToken(dot_token);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);
            } break;
            case '\0': {
                // Reached the end of the file.
                PushToken(CurrentToken);

                // Создайте и отправьте новый токен для этого.
                jcf_token eof_token = {.type = jcf_token_type::EOF, .start = c, .end = c + advance };
                PushToken(eof_token);

                RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);

                EofReached = true;
            } break;

            default: {
                // Идентификаторы могут состоять из строчных и прописных латинских букв (a–z), 
                // символов подчеркивания и цифр (при этом цифра не может быть первым символом идентификатора). 
                // Заметим, что последовательности цифр, упомянутые выше, рассматриваются как числовые литералы; 
                // однако, если они располагаются непосредственно рядом с идентификаторами (без пробелов), 
                // то могут объединяться с ними в единые идентификаторы.
                if ((codepoint >= 'A' && codepoint <= 'z') || codepoint == '_') {
                    if (mode == jcf_tokenize_mode::DEFINING_IDENTIFIER) {
                        // Создайте новый токен-идентификатор.
                        if (CurrentToken.type == jcf_token_type::Unknown) {
                            CurrentToken.type = jcf_token_type::Identifier;
                            CurrentToken.start = c;
                            CurrentToken.end = c;
                        }
                        // Добавьте к существующему идентификатору.
                        CurrentToken.end += advance;
                    } else {
                        // Сначала проверьте, не является ли это логическим определением.
                        const char* str = source + c;
                        u8 bool_advance = 0;
                        if (MString::Comparei(str, "true", 4)) {
                            bool_advance = 4;
                        } else if (MString::Comparei(str, "false", 5)) {
                            bool_advance = 5;
                        }

                        if (bool_advance) {
                            PushToken(CurrentToken);

                            // Создайте и добавьте логический токен.
                            jcf_token bool_token = { .type = jcf_token_type::Boolean, .start = c, .end = c + bool_advance};
                            PushToken(bool_token);

                            RESET_CURRENT_TOKEN_AND_MODE(CurrentToken, mode);

                            // Продвигайтесь вперед на размер токена.
                            advance = bool_advance;
                        } else {
                            // Рассматривайте как начало определения идентификатора.
                            // Добавьте существующий токен.
                            PushToken(CurrentToken);

                            // Переключитесь в режим анализа идентификаторов.
                            mode = jcf_tokenize_mode::DEFINING_IDENTIFIER;
                            CurrentToken.type = jcf_token_type::Identifier;
                            CurrentToken.start = c;
                            CurrentToken.end = c + advance;
                        }
                    }
                } else {
                    // Если здесь встречается какой-либо другой символ, не являющийся частью строки, 
                    // то дальнейшие действия неопределены. В таком случае следует вызвать ошибку и прервать выполнение программы.
                    MERROR("Неожиданный символ «%c» в позиции %u. Разбор на токены не удался.", c + advance);
                    // Очистите массив токенов, так как в данном случае с ними ничего нельзя сделать.
                    tokens.Clear();
                    return false;
                }

            } break;
        }

        // Теперь продвигайтесь дальше.
        c += advance;
    }
    PushToken(CurrentToken);
    // Создайте и отправьте новый токен для этого.
    jcf_token EofToken = { .type = jcf_token_type::EOF, .start = CharLength, .end = CharLength + 1};
    PushToken(EofToken);

    return true;
}

#define NEXT_TOKEN()                            \
    {                                           \
        index++;                                \
        CurrentToken = &tokens[index]; \
    }

#define ENSURE_IDENTIFIER(tokenString)                                                                          \
    {                                                                                                            \
        if (ExpectIdentifier) {                                                                                 \
            MERROR("Ожидался идентификатор, но найдено «%s». Позиция: %u", tokenString, CurrentToken->start); \
            return false;                                                                                        \
        }                                                                                                        \
    }

jcf_token *jcf_parser::GetLastNonWhitespaceToken(u32 CurrentIndex)
{
    if (CurrentIndex == 0) {
        return nullptr;
    }

    jcf_token* token = &tokens[CurrentIndex - 1];
    while (CurrentIndex > 0 && token && token->type == jcf_token_type::Whitespace) {
        CurrentIndex--;
        token = &tokens[CurrentIndex];
    }

    // Это означает, что последний доступный в файле токен — пробельный символ.
    // Вернуть токен, не являющийся пробельным символом, невозможно.
    if (token->type == jcf_token_type::Whitespace) {
        return nullptr;
    }

    return token;
}

#define NUMERIC_LITERAL_STR_MAX_LENGTH 25

Substring StringFromJcfToken(MString& FileContent, const jcf_token* token) {
    i32 length = (i32)token->end - (i32)token->start;
    MASSERT_MSG(length > 0, "Длина токена должна быть равна единице.");

    return FileContent.Mid(token->start, length);;
}

bool jcf_parser::Parse(jcf_tree &OutThree)
{
    if (!tokens) {
        MERROR("Невозможно выполнить синтаксический разбор пустого набора токенов!");
        return false;
    }

    jcf_token* CurrentToken = nullptr;

    Stack<jcf_object*> scope;

    // Первым делом ожидается идентификатор.
    bool ExpectIdentifier = true;
    bool ExpectValue = false;
    bool ExpectOperator = false;
    bool ExpectNumeric = false;

    char NumericLiteralStr[NUMERIC_LITERAL_STR_MAX_LENGTH]{};
    u32 NumericLiteralStrPos = 0;
    i32 NumericDecimalPos = -1;

    u32 index = 0;
    CurrentToken = &tokens[index];

    // Создайте дерево.
    OutThree.root.type = jcf_object_type::OBJECT;

    // Установите его в качестве текущего объекта.
    jcf_object* CurrentObject = &OutThree.root;
    if (!scope.Push(CurrentObject)) {
        MERROR("Не удалось поместить базовый объект в стек.");
        return false;
    }
    jcf_property* CurrentProperty = nullptr;

    while (CurrentToken && CurrentToken->type != jcf_token_type::EOF) {
        switch (CurrentToken->type) {
            case jcf_token_type::CurlyBraceOpen: {
                // ЗАДАЧА: Может потребоваться для проверки того, что объект начинается в нужном месте.
                /* ENSURE_IDENTIFIER("{") */
                // Начало блока.
                jcf_object NewObj {
                    .type = jcf_object_type::OBJECT,
                    .properties = PArrayCreate(jcf_property)
                };

                if (CurrentObject->type == jcf_object_type::ARRAY) {
                    // Присвойте значение непосредственно новому, безымянному свойству, которое добавляется к `CurrentObject`.
                    auto UnnamedArrayProp = (jcf_property*)AcquireBack(CurrentObject->properties);
                    UnnamedArrayProp->type = jcf_property_type::OBJECT;
                    UnnamedArrayProp->value.o = NewObj;
                    // Добавьте свойство-массив к текущему объекту.
                    // Теперь текущим объектом является `new_obj`.
                    CurrentObject = &UnnamedArrayProp->value.o;
                } else {
                    // Объект становится значением текущего свойства
                    CurrentProperty->value.o = NewObj;
                    // Теперь текущим объектом является new_obj.
                    CurrentObject = &CurrentProperty->value.o;

                    // Это также означает, что текущему свойству в качестве значения присваивается объект, поэтому следует пометить это свойство как имеющее тип «объект».
                    CurrentProperty->type = jcf_property_type::OBJECT;
                }

                // Добавьте обновлённый CurrentObject в стек.
                scope.Push(CurrentObject);

                ExpectIdentifier = true;
            } break;
            case jcf_token_type::CurlyBraceClose: {
                // ЗАДАЧА: может потребоваться для проверки того, что объект заканчивается в нужном месте.
                /* ENSURE_IDENTIFIER("}") */
                // Завершение блока.

                jcf_object* PoppedObj = nullptr;
                if (!scope.Pop(PoppedObj)) {
                    MERROR("Не удалось извлечь элемент из стека областей видимости.");
                    return false;
                }

                // Посмотрите на следующий объект в стеке и сделайте его текущим объектом.
                if (!scope.Peek(CurrentObject)) {
                    MERROR("Не удалось получить верхний элемент стека областей видимости.");
                    return false;
                }

                ExpectValue = CurrentObject->type == jcf_object_type::ARRAY;
            } break;
            case jcf_token_type::BracketOpen: {
                // ЗАДАЧА: Может потребоваться проверка того, что массив начинается в нужном месте.
                /* ENSURE_IDENTIFIER("[") */

                // создание массива.
                jcf_object NewArr = { .type = jcf_object_type::ARRAY };

                if (CurrentObject->type == jcf_object_type::ARRAY) {
                    // Присвойте значение непосредственно новому, безымянному свойству, которое добавляется к `CurrentObject`.
                    // Добавьте свойство к текущему объекту.
                    auto UnnamedArrayProp = (jcf_property*)AcquireBack(CurrentObject->properties);
                    UnnamedArrayProp->type = jcf_property_type::ARRAY;
                    UnnamedArrayProp->value.o = NewArr;
                    // Теперь текущим объектом становится `new_arr`. Он всегда будет первым элементом этого массива.
                    CurrentObject = &UnnamedArrayProp->value.o;
                } else {
                    // Объект становится значением текущего свойства.
                    CurrentProperty->value.o = NewArr;
                    // Теперь текущим объектом является `new_obj`.
                    CurrentObject = &CurrentProperty->value.o;

                    // Это также означает, что текущему свойству в качестве значения присваивается массив, поэтому пометьте это свойство как имеющее тип «массив».
                    CurrentProperty->type = jcf_property_type::ARRAY;
                }

                // Добавьте объект в стек.
                scope.Push(CurrentObject);

                ExpectValue = true;

            } break;
            case jcf_token_type::BracketClose: {
                // ЗАДАЧА: может потребоваться для проверки того, что массив заканчивается в нужном месте.
                /* ENSURE_IDENTIFIER("]") */

                // Завершение массива.
                jcf_object* PoppedObj = nullptr;
                if (!scope.Pop(PoppedObj)) {
                    MERROR("Не удалось извлечь элемент из стека областей видимости.");
                    return false;
                }

                // Посмотреть на следующий объект в стеке и сделать его текущим объектом.
                if (!scope.Peek(CurrentObject)) {
                    MERROR("Не удалось получить верхний элемент стека областей видимости.");
                    return false;
                }

                ExpectValue = CurrentObject->type == jcf_object_type::ARRAY;
            } break;
            case jcf_token_type::Identifier: {
                char buf[512] {};
                MString::Mid(buf, FileContent, CurrentToken->start, CurrentToken->end - CurrentToken->start);
                if (!ExpectIdentifier) {
                    MERROR("Неожиданный идентификатор «%s» в позиции %u.", buf, CurrentToken->start);
                    return false;
                }
                // Добавление нового свойства и установка его в качестве текущего.
                CurrentProperty = (jcf_property*)AcquireBack(CurrentObject->properties);
                CurrentProperty->type = jcf_property_type::UNKNOWN;

                // Идентификатор больше не ожидается
                ExpectIdentifier = false;
                ExpectOperator = true;
            } break;
            case jcf_token_type::Whitespace:
            case jcf_token_type::Comment: {
                NEXT_TOKEN();
                continue;
            }
            case jcf_token_type::Unknown:
            default: {
                MERROR("Обнаружен неожиданный или неизвестный токен. Разбор не удался.");
                return false;
            }
            case jcf_token_type::OperatorEqual: {
                ENSURE_IDENTIFIER("=")
                // Предыдущий токен должен быть идентификатором.
                jcf_token* t = GetLastNonWhitespaceToken(index);
                if (!t) {
                    MERROR("Неожиданный токен перед оператором присваивания. Позиция: %u", CurrentToken->start);
                    return false;
                } else if (t->type != jcf_token_type::Identifier) {
                    MERROR("Перед оператором присваивания ожидался идентификатор. Позиция: %u", CurrentToken->start);
                    return false;
                }

                ExpectOperator = false;

                // Следующий токен (не являющийся пробельным символом) должен представлять собой какое-либо значение.
                ExpectValue = true;
            } break;
            case jcf_token_type::OperatorMinus: {
                if (ExpectNumeric) {
                    MERROR("Уже выполняется разбор числового значения; использование знака «минус» (отрицательного значения) внутри числа недопустимо. Позиция: %u", CurrentToken->start);
                    return false;
                }

                // Если этот токен является числовым литералом, его следует обработать как число.
                // ПРИМЕЧАНИЕ: знак «минус» допустим только в качестве первого символа числового литерала.
                if (tokens[index + 1].type == jcf_token_type::NumericLiteral ||
                    (tokens[index + 1].type == jcf_token_type::OperatorDot && tokens[index + 2].type == jcf_token_type::NumericLiteral)) {
                    // Начало численного процесса.
                    ExpectNumeric = true; 
                    MemorySystem::ZeroMem(NumericLiteralStr, sizeof(char) * NUMERIC_LITERAL_STR_MAX_LENGTH);

                    NumericLiteralStr[0] = '-';
                    NumericLiteralStrPos++;
                } else {
                    // ЗАДАЧА: Это следует рассматривать как оператор вычитания. Убедитесь, что предыдущий токен является корректным, и т. д.
                    MERROR("Операция вычитания в данный момент не поддерживается.");
                    return false;
                }
            } break;
            case jcf_token_type::OperatorPlus:
                MERROR("Операция сложения в данный момент не поддерживается.");
                return false;
                break;
            case jcf_token_type::OperatorDot:
                // Это может быть первым элементом в последовательности токенов, составляющих числовой литерал.
                if (!ExpectNumeric) {
                    // Проверьте следующий токен: является ли он числовым? Чтобы текущий токен мог быть частью числового литерала, следующий токен обязательно должен быть числовым.
                    // Наличие пробельных символов между ними не допускается.
                    if (tokens[index + 1].type == jcf_token_type::NumericLiteral) {
                        // Начало числового литерала.
                        NumericLiteralStr[0] = '.';
                        ExpectNumeric = true;
                        MemorySystem::ZeroMem(NumericLiteralStr, sizeof(char) * NUMERIC_LITERAL_STR_MAX_LENGTH);
                        NumericDecimalPos = 0;
                        NumericLiteralStrPos++;
                    } else {
                        // ЗАДАЧА: Добавить поддержку именованных свойств объектов, например «sponza.name».
                        MERROR("Оператор доступа через точку не поддерживается. Позиция: %u", CurrentToken->start);
                        return false;
                    }
                } else {
                    // Просто убедитесь, что десятичная точка еще не присутствует.
                    if (NumericDecimalPos != -1) {
                        MERROR("В числовом литерале не может быть более одной десятичной точки. Первое вхождение: %i, позиция: %u", NumericDecimalPos, CurrentToken->start);
                        return false;
                    }

                    // Добавьте это к строке.
                    NumericLiteralStr[NumericLiteralStrPos] = '.';
                    NumericDecimalPos = NumericLiteralStrPos;
                    NumericLiteralStrPos++;
                }
                break;
            case jcf_token_type::OperatorAsterisk:
            case jcf_token_type::OperatorSlash:
                MERROR("Неожиданный токен в позиции %u. Ошибка при разборе.", CurrentToken->start);
                return false;
            case jcf_token_type::NumericLiteral: {
                if (!ExpectNumeric) {
                    ExpectNumeric = true;
                    MemorySystem::ZeroMem(NumericLiteralStr, sizeof(char) * NUMERIC_LITERAL_STR_MAX_LENGTH);
                }
                u32 length = CurrentToken->end - CurrentToken->start;
                MString::Copy(NumericLiteralStr + NumericLiteralStrPos, FileContent + CurrentToken->start, length);
                NumericLiteralStrPos += length;
            } break;
            case jcf_token_type::StringLiteral:
                if (!ExpectValue) {
                    MERROR("Неожиданный строковый токен в позиции: %u", CurrentToken->start);
                    return false;
                }

                if (CurrentObject->type == jcf_object_type::ARRAY) {
                    // Присвойте значение непосредственно новому, безымянному свойству, которое добавляется к объекту CurrentObject.
                    auto p = (jcf_property*)AcquireBack(CurrentObject->properties);
                    p->type = jcf_property_type::STRING;
                    p->value.s = StringFromJcfToken(FileContent, CurrentToken);
                } else {
                    CurrentProperty->type = jcf_property_type::STRING;
                    CurrentProperty->value.s = StringFromJcfToken(FileContent, CurrentToken);
                }

                ExpectValue = CurrentObject->type == jcf_object_type::ARRAY;
                break;
            case jcf_token_type::Boolean: {
                if (!ExpectValue) {
                    MERROR("Неожиданный логический токен в позиции: %u", CurrentToken->start);
                    return false;
                }

                Substring TokenString = StringFromJcfToken(FileContent, CurrentToken);
                bool BoolValue = false;
                if (!TokenString.ToBool(BoolValue)) {
                    MERROR("Не удалось распознать логическое значение из токена. Позиция: %u", CurrentToken->start);
                }

                if (CurrentObject->type == jcf_object_type::ARRAY) {
                    // Присвойте значение непосредственно новому, безымянному свойству, которое добавляется к объекту `CurrentObject`.
                    auto p = (jcf_property*)AcquireBack(CurrentObject->properties);
                    p->type = jcf_property_type::BOOLEAN;
                    p->value.b = BoolValue;
                } else {
                    CurrentProperty->type = jcf_property_type::BOOLEAN;
                    CurrentProperty->value.b = BoolValue;
                }

                ExpectValue = CurrentObject->type == jcf_object_type::ARRAY;
            } break;
            case jcf_token_type::Newline:
                if (ExpectNumeric) {
                    // Завершите формирование числового значения и присвойте его текущему свойству.
                    jcf_property p;
                    // Определите, является ли это значение числом с плавающей точкой или целым числом.
                    if (MString::IndexOf(NumericLiteralStr, '.') != -1) {
                        f32 f_value = 0;
                        if (!MString::ToF32(NumericLiteralStr, f_value)) {
                            MERROR("Не удалось преобразовать строку в число с плавающей точкой: «%s», позиция: %u", NumericLiteralStr, CurrentToken->start);
                            return false;
                        }
                        p.value.f = f_value;
                        p.type = jcf_property_type::FLOAT;
                    } else {
                        i64 i_value = 0;
                        if (!MString::ToInt(NumericLiteralStr, i_value)) {
                            MERROR("Не удалось преобразовать строку в целое число со знаком: «%s», позиция: %u", NumericLiteralStr, CurrentToken->start);
                            return false;
                        }
                        p.value.i = i_value;
                        p.type = jcf_property_type::INT;
                    }

                    if (CurrentObject->type == jcf_object_type::ARRAY) {
                        // Присвойте значение непосредственно новому, безымянному свойству, которое добавляется к объекту CurrentObject.
                        PArrayPush(CurrentObject->properties, p);
                    } else {
                        CurrentProperty->type = p.type;
                        CurrentProperty->value = p.value;
                    }

                    // Сбросьте состояние строки числового разбора.
                    u32 NumLitLen = MString::Length(NumericLiteralStr);
                    MemorySystem::ZeroMem(NumericLiteralStr, sizeof(char) * NumLitLen);
                    ExpectNumeric = false;
                    NumericDecimalPos = -1;
                    NumericLiteralStrPos = 0;

                    // Текущее значение задано, поэтому теперь ожидается следующий идентификатор или элемент массива.
                }

                // Не ожидайте значения после символа новой строки.
                ExpectValue = CurrentObject->type == jcf_object_type::ARRAY;
                ExpectIdentifier = !ExpectValue;
                break;
            case jcf_token_type::EOF: {
                bool valid = true;
                // Убедитесь, что в данный момент не выполняется операция присваивания.
                if (ExpectValue || ExpectOperator || ExpectNumeric) {
                    valid = false;
                }
                // Убедитесь, что текущий уровень вложенности равен 1 (с учетом базового объекта).
                if (scope.ElementCount > 1) {
                    valid = false;
                }

                if (!valid) {
                    MERROR("Неожиданный конец файла в позиции: %u", CurrentToken->start);
                    return false;
                }

            } break;
        }
        index++;
        CurrentToken = &tokens[index];
    }

    return true;
}
