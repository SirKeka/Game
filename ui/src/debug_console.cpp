#include "debug_console.h"
#include "core/console.h"
#include "core/event.h"
#include "core/input.h"
#include "core/systems_manager.h"
#include "platform/platform.hpp"

#define FONT_SIZE 31

// ЗАДАЧА: на данный момент статически определенное состояние.
// static sDebugConsole* pDebugConsole = nullptr;

void TextboxOnKey(UiElement* self, UiKeyboardEvent event);

DebugConsole::DebugConsole() : 
                                            // Учет отступов и текстового поля внизу
panel("debug_console_bg_panel", FVec2(1280.F, 50.F + (FONT_SIZE * 10 + 1)), FVec4(0.F, 0.F, 0.F, 0.75F)), TextControl(),
EntryTextbox(128), lines(), history(), LineDisplayCount(10), LineOffset(), HistoryOffset(-1), loaded(), dirty(), visible(false)
{
    // ПРИМЕЧАНИЕ: обновите текст на основе количества отображаемых строк и количества строк, 
    // смещенных от нижней границы. На данный момент для отображения используется объект UI Text. 
    // Можно позаботиться о цвете в отдельном проходе.
    // Не будем рассматривать перенос слов.
    // ПРИМЕЧАНИЕ: также следует рассмотреть обрезку прямоугольников и новых строк.

    // Зарегистрируйтесь как потребитель консоли.
    Console::RegisterConsumer(this, DebugConsoleConsumerWrite, ConsoleConsumerID);

    EventSystem::Register(EventSystem::Resized, this, OnResize);
}

bool DebugConsole::DebugConsoleConsumerWrite(void* inst, Log::Level level, const char* message) 
{
    auto pDebugConsole = (DebugConsole*)inst;
    // Не обязательно сбой, но продолжайте, если не загружено.
    if (!pDebugConsole->loaded) {
        return true;
    }
    // Для высокоприоритетных сообщений об ошибках/фатальных ошибках не беспокойтесь о разделении, 
    // просто выведите их, потому что что-то действительно ужасное может помешать этому разделению произойти.
    if (level <= Log::Level::Error) {
        pDebugConsole->lines.PushBack(message);
        pDebugConsole->dirty = true;
        return true;
    }
    // Создайте новую копию строки и попробуйте разбить ее по новым строкам, 
    // чтобы каждая из них считалась новой строкой.
    // ПРИМЕЧАНИЕ: отсутствие очистки строк здесь намеренное, поскольку строки должны жить, 
    // чтобы к ним можно было получить доступ с помощью этой консоли отладки. 
    DArray<MString> SplitMessage;
    u32 count = MString::Split(message, '\n', SplitMessage, true, false);
    // Поместите каждую в массив как новую строку.
    for (u32 i = 0; i < count; i++) {
        pDebugConsole->lines.PushBack((MString&&)SplitMessage[i]);
    }

    // ОЧИСТИТЕ сам временный массив (только не его содержимое в этом случае).
    // [[maybe_unused]]auto str = SplitMessage.MovePtr();
    pDebugConsole->dirty = true;
    return true;
}

bool DebugConsole::OnResize(u16 code, void* sender, void* ListenerInst, const EventContext& context)
{
    u16 width = context.data.u16[0];
    /* u16 height = context.data.u16[1]; */

    auto state = (DebugConsole*)ListenerInst;
    FVec2 size = state->panel.Size();
    state->panel.Resize(FVec2 (width, size.y));

    return false;
}

bool DebugConsole::Load()
{
    UiSystem* uiSystem = nullptr;
    if (!panel.Load()) {
        MERROR("Не удалось загрузить фоновую панель.");
    } else {
        // panel.xform.Translate(FVec3(500,F, 100.F));
        uiSystem = (UiSystem*)SystemsManager::GetState(M_SYSTEM_TYPE_STANDARD_UI_EXT);
        if (!uiSystem->RegisterControl(&panel)) {
            MERROR("Невозможно зарегистрировать элемент управления.");
        } else {
            if (!uiSystem->AddChild(&panel)) {
                MERROR("Не удалось создать родительскую фоновую панель.");
            } else {
                panel.IsActive = true;
                panel.IsVisible = false;
            }
        }
    }
    
    // Создаем текстовый элемент управления пользовательского интерфейса для рендеринга.
    if(!TextControl.Create("debug_console_log_text", FontType::System, "Noto Sans CJK JP", FONT_SIZE, "", &panel)) {
        MFATAL("Не удалось создать текстовый элемент управления для отладочной консоли.");
        return false;
    } else {
        if (!TextControl.Load()) {
        MERROR("Не удалось загрузить текстовый элемент управления.");
        } else {
            if (!uiSystem->RegisterControl(&TextControl)) {
                MERROR("Невозможно зарегистрировать элемент управления.");
            } else {
                TextControl.IsActive = true;
                TextControl.IsVisible = false;
            }
        }
    }
    TextControl.SetPosition(FVec3(3.F, FONT_SIZE, 0.F));

    // Создаем еще один элемент управления текстом пользовательского интерфейса для отображения введенного текста.  Some really long test text in the textbox.
    if (!EntryTextbox.Create("debug_console_entry_textbox", FontType::System, "Noto Sans CJK JP", FONT_SIZE, "Очень длинный тестовый текст в текстовом поле.", &panel)) { 
        MFATAL("Не удалось создать текстовое поле для ввода данных в консоль отладки.");
        return false;
    } else {
        if (!EntryTextbox.Load()) {
            MERROR("Не удалось загрузить текстовое поле ввода для консоли отладки.");
        } else {
            EntryTextbox.UserData = this;
            EntryTextbox.UserDataSize = sizeof(DebugConsole*);
            EntryTextbox.OnKey = TextboxOnKey;
            if (!uiSystem->RegisterControl(&EntryTextbox)) {
                MERROR("Невозможно зарегистрировать элемент управления.");
            } else {
                EntryTextbox.IsActive = true;
                EntryTextbox.IsVisible = false;
            }
        }
    }
    
    EntryTextbox.SetPosition(FVec3(3.F, 10.F + (FONT_SIZE * LineDisplayCount), 0.F));
    loaded = true;

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
        const u32 LineCount = lines.Length();
        u32 MaxLines = MMIN(LineDisplayCount, MMAX(LineCount, LineDisplayCount));

        // Сначала вычислите минимальную строку, принимая во внимание также смещение строки.
        u32 MinLine = MMAX(LineCount - MaxLines - LineOffset, 0);
        u32 MaxLine = MinLine + MaxLines - 1;

        // Надеюсь, достаточно большой, чтобы справиться с большинством вещей.
        char buffer[16384]{};
        u32 BufferPos = 0;
        for (u32 i = MinLine; i <= MaxLine; ++i) {
            // ЗАДАЧА: вставьте цветовые коды для типа сообщения.
            u32 CharCount;
            const char* line = lines[i].Data(&CharCount);
            CharCount--;
            
            for (u32 c = 0; c < CharCount; c++, BufferPos++) {
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
        EventSystem::Register(EventSystem::KeyReleased, this, OnResize);
        Console::UpdateConsumer(ConsoleConsumerID, this, DebugConsoleConsumerWrite);
    }
} 

void DebugConsole::OnLibUnload()
{
    EventSystem::Unregister(EventSystem::KeyReleased, this, OnResize);
    Console::UpdateConsumer(ConsoleConsumerID, nullptr, nullptr);
}

void DebugConsole::SetVisible(bool visible)
{
    this->visible          = visible;
    panel.IsVisible        = visible;
    TextControl.IsVisible  = visible;
    EntryTextbox.IsVisible = visible;
    UiSystem* uisys = (UiSystem*)SystemsManager::GetState(M_SYSTEM_TYPE_STANDARD_UI_EXT);
    uisys->FocusControl(visible ? &EntryTextbox : nullptr);
    InputSystem::RepeatsEnable(visible);
}

void DebugConsole::MoveUp()
{
    dirty = true;
    const u32 LineCount = lines.Length();
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
    const u32 LineCount = lines.Length();
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
    const u32 LineCount = lines.Length();
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
    const u32 lenght = history.Length();
    if (lenght > 0) {
        HistoryOffset = MMAX(HistoryOffset + 1, lenght - 1);
        EntryTextbox.SetText(history[lenght - HistoryOffset - 1]);
    }
}

void DebugConsole::HistoryForward()
{
    const u32 lenght = history.Length();
    if (lenght > 0) {
        HistoryOffset = MMAX(HistoryOffset - 1, 0);
        EntryTextbox.SetText(history[lenght - HistoryOffset - 1]);
    }
}

void TextboxOnKey(UiElement* self, UiKeyboardEvent event)
{
    if (event.type == UiKeyboardEventType::PRESS) {
        auto s = (Textbox*)self;
        u16 KeyCode = (u16)event.key;
        // bool ShiftHeld = InputSystem::IsKeyDown(Keys::SHIFT) || InputSystem::IsKeyDown(Keys::RSHIFT) || InputSystem::IsKeyDown(Keys::SHIFT);

        if (KeyCode == Keys::ENTER) {
            const auto& EntryControlText = s->ContentLabel.GetText();
            u32 len = EntryControlText.Length();
            if (len > 0) {
                // Сохранить команду в списке истории.
                CommandHistoryEntry entry;
                entry = EntryControlText;
                if (entry) {
                    ((DebugConsole*)s->UserData)->History().PushBack(entry);

                    // Выполнить команду и очистить текст.
                    if (!Console::ExecuteCommand(EntryControlText)) {
                        // ЗАДАЧА: Обработать ошибку?
                    }
                }
                // Очистить текст.
                s->ContentLabel.SetText("");
            }
        }
    }
}

