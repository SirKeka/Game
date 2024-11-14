#include "keymap.hpp"
#include "mmemory.hpp"

Keymap::~Keymap()
{
}

void Keymap::BindingAdd(Keys key, EntryBindType type, Modifier modifiers, void *UserData, Callback callback)
{
    auto& entry = entries[static_cast<u16>(key)];
    auto node = entry.bindings;
    auto previous = entry.bindings;
    while (node) {
        previous = node;
        node = node->next;
    }

    auto NewEntry = reinterpret_cast<Binding*>(MemorySystem::Allocate(sizeof(Binding), Memory::Unknown));
    NewEntry->callback = callback;
    NewEntry->modifiers = static_cast<ModifierBits>(modifiers);
    NewEntry->type = type;
    NewEntry->next = nullptr;
    NewEntry->UserData = UserData;

    if (previous) {
        previous->next = NewEntry;
    } else {
        entry.bindings = NewEntry;
    }
}

void Keymap::BindingRemove(Keys key, EntryBindType type, Modifier modifiers, Callback callback)
{
    auto& entry = entries[static_cast<u16>(key)];
    auto node = entry.bindings;
    auto previous = entry.bindings;
    while (node) {
        if (node->callback == callback && node->modifiers == modifiers && node->type == type) {
            // Удалить
            previous->next = node->next;
            MemorySystem::Free(node, sizeof(Binding), Memory::Unknown);
            return;
        }
        previous = node;
        node = node->next;
    }
}
