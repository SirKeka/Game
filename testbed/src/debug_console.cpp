#include "debug_console.hpp"
#include <core/console.hpp>
#include <core/input.hpp>

struct sDebugConsole 
{
    i32 LineDisplayCount;   // Количество строк, отображаемых одновременно.
    i32 LineOffset;         // Количество строк, смещенных от низа списка.
    DArray<MString> lines;

    bool dirty;
    bool visible;

    Text TextControl;
    Text EntryControl;

    sDebugConsole() : LineDisplayCount(10), LineOffset(), lines(), dirty(), visible(false), TextControl(), EntryControl() {}

    void* operator new(u64 size) {
        return MMemory::Allocate(size, Memory::Game);
    }
    void operator delete(void* ptr, u64 size){
        MMemory::Free(ptr, size, Memory::Game);
    }
};

// ЗАДАЧА: на данный момент статически определенное состояние.
static sDebugConsole* pDebugConsole = nullptr;

namespace DebugConsole
{
    bool DebugConsoleConsumerWrite(void* inst, LogLevel level, const char* message) 
    {
        // Создайте новую копию строки и попробуйте разбить ее по новым строкам, 
        // чтобы каждая из них считалась новой строкой.
        // ПРИМЕЧАНИЕ: отсутствие очистки строк здесь намеренное, поскольку строки должны жить, 
        // чтобы к ним можно было получить доступ с помощью этой консоли отладки. 
        DArray<MString> SplitMessage;
        u32 count = MString::Split(message, '\n', SplitMessage, true, false);
        // Поместите каждую в массив как новую строку.
        for (u32 i = 0; i < count; i++) {
            pDebugConsole->lines.PushBack(static_cast<MString&&>(SplitMessage[i]));
        }

        // ОЧИСТИТЕ сам временный массив (только не его содержимое в этом случае).
        // [[maybe_unused]]auto str = SplitMessage.MovePtr();
        pDebugConsole->dirty = true;
        return true;
    }

    bool OnKey(u16 code, void* sender, void* ListenerInst, EventContext context)
    {
        if (!pDebugConsole->visible) {
            return false;
        }
        if (code == EVENT_CODE_KEY_PRESSED) {
            const u16& KeyCode = context.data.u16[0];
            bool ShiftHeld = Input::IsKeyDown(Keys::LSHIFT) || Input::IsKeyDown(Keys::RSHIFT) || Input::IsKeyDown(Keys::SHIFT);

            if (KeyCode == Keys::ENTER) {
                u32 len = pDebugConsole->EntryControl.text.Length();
                if (len > 0) {
                    // Выполните команду и очистите текст.
                    if (!Console::ExecuteCommand(pDebugConsole->EntryControl.text)) {
                        // ЗАДАЧА: обработать ошибку?
                    }
                    // Очистить текст.
                    pDebugConsole->EntryControl.SetText("");
                }
            } else if (KeyCode == Keys::BACKSPACE) {
                u32 len = pDebugConsole->EntryControl.text.Length();
                if (len > 0) {
                    pDebugConsole->EntryControl.DeleteLastSymbol();
                }
            } else {
                // Используйте A-Z и 0-9 как есть.
                char CharCode = KeyCode;
                if ((KeyCode >= static_cast<u16>(Keys::A) && KeyCode <= static_cast<u16>(Keys::Z))) {
                    // ЗАДАЧА: проверьте Caps Lock.
                    if (!ShiftHeld) {
                        CharCode = KeyCode + 32;
                    }
                } else if ((KeyCode >= static_cast<u16>(Keys::KEY_0) && KeyCode <= static_cast<u16>(Keys::KEY_9))) {
                    if (ShiftHeld) {
                        // ПРИМЕЧАНИЕ: это обрабатывает стандартные раскладки клавиатуры США.
                        // Потребуется также обрабатывать другие раскладки.
                        switch(KeyCode) {
                            case static_cast<u16>(Keys::KEY_0): CharCode = ')'; break;
                            case static_cast<u16>(Keys::KEY_1): CharCode = '!'; break;
                            case static_cast<u16>(Keys::KEY_2): CharCode = '@'; break;
                            case static_cast<u16>(Keys::KEY_3): CharCode = '#'; break;
                            case static_cast<u16>(Keys::KEY_4): CharCode = '$'; break;
                            case static_cast<u16>(Keys::KEY_5): CharCode = '%'; break;
                            case static_cast<u16>(Keys::KEY_6): CharCode = '^'; break;
                            case static_cast<u16>(Keys::KEY_7): CharCode = '&'; break;
                            case static_cast<u16>(Keys::KEY_8): CharCode = '*'; break;
                            case static_cast<u16>(Keys::KEY_9): CharCode = '('; break;
                        }
                    }
                } else {
                    switch (KeyCode) {
                        case static_cast<u16>(Keys::SPACE):
                            CharCode = KeyCode;
                            break;
                        default:
                            // Недействительно для ввода, используйте 0
                            CharCode = 0;
                            break;
                    }
                }

                if (CharCode != 0) {
                    // u32 len = pDebugConsole->EntryControl.text.Length();
                    //MString NewText;
                    //MString::Format(NewText, "%s%c", pDebugConsole->EntryControl.text, CharCode);
                    //pDebugConsole->EntryControl.SetText(MString(text, ));
                    pDebugConsole->EntryControl.Append(CharCode);
                    // kfree(NewText, len + 1, MEMORY_TAG_STRING);
                }
            }

            // ЗАДАЧА: сохранить историю команд, вверх/вниз для навигации.
        }

        return false;
    }

    void Create()
    {
        pDebugConsole = new sDebugConsole();

        // ПРИМЕЧАНИЕ: обновите текст на основе количества отображаемых строк и количества строк, 
        // смещенных от нижней границы. На данный момент для отображения используется объект UI Text. 
        // Можно позаботиться о цвете в отдельном проходе.
        // Не будем рассматривать перенос слов.
        // ПРИМЕЧАНИЕ: также следует рассмотреть обрезку прямоугольников и новых строк.

        // Зарегистрируйтесь как потребитель консоли.
        Console::RegisterConsumer(nullptr, DebugConsoleConsumerWrite);
    }

    bool Load()
    {
        if (!pDebugConsole) {
            MFATAL("DebugConsole::Load() вызван до инициализации консоли!");
            return false;
        }

        // Создать текстовый элемент управления пользовательского интерфейса для рендеринга.
        if (!pDebugConsole->TextControl.Create(TextType::System, "Metrika", 31, "")) {
            MFATAL("Невозможно создать текстовый элемент управления для консоли отладки.");
            return false;
        }

        pDebugConsole->TextControl.SetPosition(FVec3(3.F, 30.F, 0.F));

        // Создать еще один текстовый элемент управления пользовательского интерфейса для рендеринга введенного текста.
        if (!pDebugConsole->EntryControl.Create(TextType::System, "Metrika", 31, "")) {
            MFATAL("Невозможно создать текстовый элемент управления ввода для консоли отладки.");
            return false;
        }

        pDebugConsole->EntryControl.SetPosition(FVec3(3.F, 30.F + (31.F * pDebugConsole->LineDisplayCount), 0.F));

        Event::Register(EVENT_CODE_KEY_PRESSED, nullptr, OnKey);
        Event::Register(EVENT_CODE_KEY_RELEASED, nullptr, OnKey);

        return true;
    }

    void Unload()
    {
        if (pDebugConsole) {
            pDebugConsole->TextControl.~Text(); // Destroy();
            pDebugConsole->EntryControl.~Text(); // Destroy();
        }
    }

    void Update()
    {
        if (pDebugConsole && pDebugConsole->dirty) {
            const u64& LineCount = pDebugConsole->lines.Length();
            u32 MaxLines = MMIN(pDebugConsole->LineDisplayCount, MMAX(LineCount, pDebugConsole->LineDisplayCount));

            // Сначала вычислите минимальную строку, принимая во внимание также смещение строки.
            u32 MinLine = MMAX(LineCount - MaxLines - pDebugConsole->LineOffset, 0);
            u32 MaxLine = MinLine + MaxLines - 1;

            // Надеюсь, достаточно большой, чтобы справиться с большинством вещей.
            char buffer[16384]{};
            u32 BufferPos = 0;
            for (u32 i = MinLine; i <= MaxLine; ++i) {
                // ЗАДАЧА: вставьте цветовые коды для типа сообщения.

                const auto& line = pDebugConsole->lines[i];
                u32 LineSize = line.Size() - 1;
                // ЗАДАЧА: в результате копирования строк в буфер сбивается кодировка.
                for (u32 c = 0; c < LineSize; c++, BufferPos++) {
                    buffer[BufferPos] = line[c];
                }
                // Добавьте новую строку
                buffer[BufferPos] = '\n';
                BufferPos++;
            }

            // Убедитесь, что строка заканчивается нулем
            buffer[BufferPos] = '\0';

            // После того, как строка будет создана, задайте текст.
            pDebugConsole->TextControl.SetText(buffer);

            pDebugConsole->dirty = false;
        }
    }

    Text *GetText()
    {
        if (pDebugConsole) {
            return &pDebugConsole->TextControl;
        }
        return nullptr;
    }

    Text *GetEntryText()
    {
        if (pDebugConsole) {
            return &pDebugConsole->EntryControl;
        }
        return nullptr;
    }

    bool Visible()
    {
        if (!pDebugConsole) {
            return false;
        }

        return pDebugConsole->visible;
    }

    void VisibleSet(bool visible)
    {
        if (pDebugConsole) {
            pDebugConsole->visible = visible;
        }
    }

    void MoveUp()
    {
        if (pDebugConsole) {
            pDebugConsole->dirty = true;
            const u32& LineCount = pDebugConsole->lines.Length();
            // Не беспокойтесь о попытках сдвига, просто выполните сброс и загрузитесь.
            if (LineCount <= pDebugConsole->LineDisplayCount) {
                pDebugConsole->LineOffset = 0;
                return;
            }
            pDebugConsole->LineOffset++;
            pDebugConsole->LineOffset = MMIN(pDebugConsole->LineOffset, LineCount - pDebugConsole->LineDisplayCount);
        }
    }

    void MoveDown()
    {
        if (pDebugConsole) {
            pDebugConsole->dirty = true;
            const u32& LineCount = pDebugConsole->lines.Length();
            // Не беспокойтесь о попытках сдвига, просто выполните сброс и загрузитесь.
            if (LineCount <= pDebugConsole->LineDisplayCount) {
                pDebugConsole->LineOffset = 0;
                return;
            }

            pDebugConsole->LineOffset--;
            pDebugConsole->LineOffset = MMAX(pDebugConsole->LineOffset, 0);
        }
    }

    void MoveToTop()
    {
        if (pDebugConsole) {
            pDebugConsole->dirty = true;
            const u32& LineCount = pDebugConsole->lines.Length();
            // Не беспокойтесь о попытках сдвига, просто выполните сброс и загрузитесь.
            if (LineCount <= pDebugConsole->LineDisplayCount) {
                pDebugConsole->LineOffset = 0;
                return;
            }

            pDebugConsole->LineOffset = LineCount - pDebugConsole->LineDisplayCount;
        }
    }

    void MoveToBottom()
    {
        if (pDebugConsole) {
            pDebugConsole->dirty = true;
            pDebugConsole->LineOffset = 0;
        }
    }
} // namespace name
