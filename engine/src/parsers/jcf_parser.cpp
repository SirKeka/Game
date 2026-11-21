#include "jcf_parser.h"
#include "core/asserts.h"

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
static void _populate_token_content(jcf_token& t, const char* source) {
    MASSERT_MSG(t.start <= t.end, "Начало токена идёт после его конца, балбес!");
    char buffer[512] = {0};
    MASSERT_MSG((t.end - t.start) < 512, "Токен не помещается в буфер.");
    MString::Mid(buffer, source, t.start, t.end - t.start);
    t.content = string_duplicate(buffer);
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
                // Identifiers may be made up of upper/lowercase a-z, underscores and numbers (although
                // a number cannot be the first character of an identifier). Note that the number cases
                // are handled above as numeric literals, and can/will be combined into identifiers
                // if there are identifiers without whitespace next to numerics.
                if ((codepoint >= 'A' && codepoint <= 'z') || codepoint == '_') {
                    if (mode == jcf_tokenize_mode::DEFINING_IDENTIFIER) {
                        // Start a new identifier token.
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
                    // If any other character is come across here that isn't part of a string, it's unknown
                    // what should happen here. So, throw an error regarding this and boot if this is the
                    // case.
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
