#include "debug_console.hpp"
#include <core/console.hpp>
#include <core/input.hpp>
#include <platform/platform.hpp>

// ЗАДАЧА: на данный момент статически определенное состояние.
// static sDebugConsole* pDebugConsole = nullptr;

DebugConsole::DebugConsole() : LineDisplayCount(10), LineOffset(), lines(), history(), HistoryOffset(), dirty(), visible(false), TextControl(), EntryControl() 
{
    // ПРИМЕЧАНИЕ: обновите текст на основе количества отображаемых строк и количества строк, 
    // смещенных от нижней границы. На данный момент для отображения используется объект UI Text. 
    // Можно позаботиться о цвете в отдельном проходе.
    // Не будем рассматривать перенос слов.
    // ПРИМЕЧАНИЕ: также следует рассмотреть обрезку прямоугольников и новых строк.

    // Зарегистрируйтесь как потребитель консоли.
    Console::RegisterConsumer(this, DebugConsoleConsumerWrite, ConsoleConsumerID);

    EventSystem::Register(EventSystem::KeyPressed, this, OnKey);
    EventSystem::Register(EventSystem::KeyReleased, this, OnKey);
}

bool DebugConsole::DebugConsoleConsumerWrite(void* inst, Log::Level level, const char* message) 
{
    auto pDebugConsole = reinterpret_cast<DebugConsole*>(inst);
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

bool DebugConsole::OnKey(u16 code, void* sender, void* ListenerInst, EventContext context)
{
    auto pDebugConsole = reinterpret_cast<DebugConsole*>(ListenerInst);
    if (!pDebugConsole->visible) {
        return false;
    }
    if (code == EventSystem::KeyPressed) {
        const u16& KeyCode = context.data.u16[0];
        bool ShiftHeld = InputSystem::IsKeyDown(Keys::LSHIFT) || InputSystem::IsKeyDown(Keys::RSHIFT) || InputSystem::IsKeyDown(Keys::SHIFT);

        if (KeyCode == Keys::ENTER) {
            u32 len = pDebugConsole->EntryControl.text.Length();
            if (len > 0) {
                //Сохранить команду в списке истории.
                pDebugConsole->history.PushBack(pDebugConsole->EntryControl.text);

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
                    case static_cast<u16>(Keys::MINUS):
                    CharCode = ShiftHeld ? '_' : '-';
                    break;
                    case static_cast<u16>(Keys::Equal):
                        CharCode = ShiftHeld ? '+' : '=';
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

bool DebugConsole::Load()
{
    // if (!pDebugConsole) {
    //     MFATAL("DebugConsole::Load() вызван до инициализации консоли!");
    //     return false;
    // }

    // Создать текстовый элемент управления пользовательского интерфейса для рендеринга.
    if (!TextControl.Create(TextType::System, "Noto Sans CJK JP", 31, "")) {
        MFATAL("Невозможно создать текстовый элемент управления для консоли отладки.");
        return false;
    }

    TextControl.SetPosition(FVec3(3.F, 30.F, 0.F));

    // Создать еще один текстовый элемент управления пользовательского интерфейса для рендеринга введенного текста.
    if (!EntryControl.Create(TextType::System, "Noto Sans CJK JP", 31, "")) {
        MFATAL("Невозможно создать текстовый элемент управления ввода для консоли отладки.");
        return false;
    }

    EntryControl.SetPosition(FVec3(3.F, 30.F + (31.F * LineDisplayCount), 0.F));

    return true;
}

// void DebugConsole::Unload()
// {
//     if (pDebugConsole) {
//         pDebugConsole->TextControl.~Text(); // Destroy();
//         pDebugConsole->EntryControl.~Text(); // Destroy();
//     }
// }

void DebugConsole::Update()
{
    if (dirty) {
        const u64& LineCount = lines.Length();
        u32 MaxLines = MMIN(LineDisplayCount, MMAX(LineCount, LineDisplayCount));

        // Сначала вычислите минимальную строку, принимая во внимание также смещение строки.
        u32 MinLine = MMAX(LineCount - MaxLines - LineOffset, 0);
        u32 MaxLine = MinLine + MaxLines - 1;

        // Надеюсь, достаточно большой, чтобы справиться с большинством вещей.
        char buffer[16384]{};
        u32 BufferPos = 0;
        for (u32 i = MinLine; i <= MaxLine; ++i) {
            // ЗАДАЧА: вставьте цветовые коды для типа сообщения.

            const auto& line = lines[i];
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
        TextControl.SetText(buffer);

        dirty = false;
    }
}

void DebugConsole::OnLibLoad(bool UpdateConsumer)
{
    if (UpdateConsumer) {
        EventSystem::Register(EventSystem::KeyPressed, this, OnKey);
        EventSystem::Register(EventSystem::KeyReleased, this, OnKey);
        Console::UpdateConsumer(ConsoleConsumerID, this, DebugConsoleConsumerWrite);
    }
} 

void DebugConsole::OnLibUnload()
{
    EventSystem::Unregister(EventSystem::KeyPressed, this, OnKey);
    EventSystem::Unregister(EventSystem::KeyReleased, this, OnKey);
    Console::UpdateConsumer(ConsoleConsumerID, nullptr, nullptr);
}

Text &DebugConsole::GetText()
{
    return TextControl;
}

Text &DebugConsole::GetEntryText()
{
    return EntryControl;
}

bool DebugConsole::Visible()
{
    return visible;
}

void DebugConsole::SetVisible(bool visible)
{
    this->visible = visible;
}

void DebugConsole::MoveUp()
{
    dirty = true;
    const u32& LineCount = lines.Length();
    // Не беспокойтесь о попытках сдвига, просто выполните сброс и загрузитесь.
    if (LineCount <= LineDisplayCount) {
        LineOffset = 0;
        return;
    }
    LineOffset++;
    LineOffset = MMIN(LineOffset, LineCount - LineDisplayCount);

}

void DebugConsole::MoveDown()
{
    if (LineOffset == 0) {
        return;
    }
    
    dirty = true;
    const u32& LineCount = lines.Length();
    // Не беспокойтесь о попытках сдвига, просто выполните сброс и загрузитесь.
    if (LineCount <= LineDisplayCount) {
        LineOffset = 0;
        return;
    }

    LineOffset--;
    LineOffset = MMAX(LineOffset, 0);
}

void DebugConsole::MoveToTop()
{
    dirty = true;
    const u32& LineCount = lines.Length();
    // Не беспокойтесь о попытках сдвига, просто выполните сброс и загрузитесь.
    if (LineCount <= LineDisplayCount) {
        LineOffset = 0;
        return;
    }

    LineOffset = LineCount - LineDisplayCount;

}

void DebugConsole::MoveToBottom()
{
    dirty = true;
    LineOffset = 0;
}

void DebugConsole::HistoryBack()
{
    const u32& lenght = history.Length();
    if (lenght > 0) {
        HistoryOffset = MMAX(HistoryOffset + 1, lenght - 1);
        EntryControl.SetText(history[lenght - HistoryOffset - 1]);
    }
}

void DebugConsole::HistoryForward()
{
    const u32& lenght = history.Length();
    if (lenght > 0) {
        HistoryOffset = MMAX(HistoryOffset - 1, 0);
        EntryControl.SetText(history[lenght - HistoryOffset - 1]);
    }
}
