#include "textbox.h"
#include <core/event.h>
#include <core/systems_manager.h>
#include <math/geometry_utils.h>
#include <renderer/rendering_system.h>
#include <resources/geometry.h>
#include <systems/geometry_system.h>
#include <systems/font_system.h>
#include <systems/shader_system.h>

Textbox::Textbox(const char *name, FontType type, const char *FontName, u16 FontSize, const char *text, UiElement *parent) :
UiElement(name, parent), size(Point(200, FontSize + 10)), colour(FVec4::One()), 
ContentLabel(MString(name, "_textbox_internal_label"), type, FontName, FontSize, text, parent),
cursor(MString(name, "_textbox_cursor_panel"), FVec2(1.F, FontSize - 4.F), FVec4::One()),                   // Использовать панель в качестве курсора.
HighlightBox(MString(name, "_textbox_highlight_panel"), FVec2(1.F, FontSize), FVec4(0.F, 0.5F, 0.9F, 0.5F)) // Поле подсветки(выделения)
{
    // char buffer[512]{};
    // MString::Format(buffer, "%s_textbox_internal_label", name);
    // ContentLabel = Label(buffer, type, FontName, FontSize, text, parent);

    // MString::Zero(buffer);
    // MString::Format(buffer, "%s_textbox_cursor_panel", name);
    // cursor = Panel(buffer, FVec2(1.F, FontSize - 4.F), FVec4::One());

    // Поле подсветки(выделения)
    // MString::Zero(buffer);
    // MString::Format(buffer, "%s_textbox_highlight_panel", name);
    // HighlightBox = Panel(buffer, FVec2(1.F, FontSize), FVec4(0.F, 0.5F, 0.9F, 0.5F));
    UiElement::OnMouseDown = OnMouseDown;
    UiElement::OnMouseUp = OnMouseUp;
}

bool Textbox::Create(const char *name, FontType type, const char *FontName, u16 FontSize, const char *text, UiElement *parent)
{
    this->name = name;
    SetParent(parent);
    size.Set(200, FontSize + 10);
    colour = FVec4::One();
    ContentLabel = Label(MString(name, "_textbox_internal_label"), type, FontName, FontSize, text, parent);
    cursor = Panel(MString(name, "_textbox_cursor_panel"), FVec2(1.F, FontSize - 4.F), FVec4::One());                       // Использовать панель в качестве курсора.
    HighlightBox = Panel(MString(name, "_textbox_highlight_panel"), FVec2(1.F, FontSize), FVec4(0.F, 0.5F, 0.9F, 0.5F));    // Поле подсветки(выделения)
    return true;
}

constexpr Textbox::Textbox(UiElement *parent)
{
    MemorySystem::ZeroMem(this, sizeof(Textbox));
}

bool Textbox::SetSize(i32 width, i32 height)
{
    size.x = width;
    size.y = height;
    nslice.size.x = width;
    nslice.size.y = height;

    bounds.height = height;
    bounds.width = width;

    nslice.Update();

    return true;
}

bool Textbox::Load()
{
    auto uisys = (UiSystem*)SystemsManager::GetState(M_SYSTEM_TYPE_STANDARD_UI_EXT);

    // ХАК: ЗАДАЧА: удалить жестко закодированные данные.
    // Point AtlasSize{UiAtlas.texture->width, UiAtlas.texture->height};
    Point AtlasSize{512, 512};
    Point AtlasMin{180, 31};
    Point AtlasMax{193, 43};
    Point CornerPxSize{3, 3};
    Point CornerSize{10, 10};
    nslice = NineSlice(name.c_str(), size, AtlasSize, AtlasMin, AtlasMax, CornerPxSize, CornerSize);

    bounds.x = 0.F;
    bounds.y = 0.F;
    bounds.width = size.x;
    bounds.height = size.y;

    // Настройка геометрии обтравочной маски текстового поля.
    ClipMask.ReferenceID = 1;  // ЗАДАЧА: Создание перемещения/Назначение reference_id.

    GeometryConfig ClipConfig;
    Math::Geometry::GenerateQuad2D("textbox_clipping_box", size.x - (CornerSize.x * 2), size.y, 0, 0, 0, 0, ClipConfig);
    ClipMask.ClipGeometry = GeometrySystem::Acquire(ClipConfig, false);

    ClipMask.RenderData.model = Matrix4D::MakeIdentity();
    ClipMask.RenderData.UniqueID = ClipMask.ReferenceID;
    ClipMask.RenderData.geometry = ClipMask.ClipGeometry;
    
    // ClipMask.RenderData.material = nullptr;
    // ClipMask.RenderData.VertexCount = ClipMask.ClipGeometry->VertexCount;
    // ClipMask.RenderData.VertexElementSize = ClipMask.ClipGeometry->VertexElementSize;
    // ClipMask.RenderData.VertexBufferOffset = ClipMask.ClipGeometry->VertexBufferOffset;

    // ClipMask.RenderData.IndexCount = ClipMask.ClipGeometry->IndexCount;
    // ClipMask.RenderData.IndexElementSize = ClipMask.ClipGeometry->IndexElementSize;
    // ClipMask.RenderData.IndexBufferOffset = ClipMask.ClipGeometry->IndexBufferOffset;

    ClipMask.RenderData.DiffuseColour = FVec4();  // Прозрачный.;

    ClipMask.ClipXform = Transform(FVec3(CornerSize.x, 0.F, 0.F));
    ClipMask.ClipXform.SetParent(&xform);

    // Получите ресурсы экземпляра для этого элемента управления.
    TextureMap* maps[1] = {&uisys->Atlas()};
    auto shader = ShaderSystem::GetShader("Shader.StandardUI");
    u16 AtlasLocation = shader->uniforms[shader->InstanceSamplerIndices[0]].index;
    // Известно количество карт этого типа.
    ShaderInstanceUniformTextureConfig AtlasTexture{
        .UniformLocation = AtlasLocation,
        .TextureMapCount = 1,
        .TextureMaps = maps
    };
    ShaderInstanceResourceConfig InstanceResourceConfig{ 1, &AtlasTexture };

    if(!SystemsManager::GetRenderingSystem()->ShaderAcquireInstanceResources(shader, InstanceResourceConfig, InstanceID)) {
        MFATAL("Не удалось получить ресурсы шейдера для текстурной карты текстового поля.");
        return false;
    }

    // Загрузите элемент управления меткой, который будет использоваться в качестве текста.
    if (!ContentLabel.Load()) {
        MERROR("Не удалось настроить метку в текстовом поле.");
        return false;
    }

    if (!uisys->RegisterControl(&ContentLabel)) {
        MERROR("Не удалось зарегистрировать элемент управления.");
    } else {
        // ПРИМЕЧАНИЕ: Родительским элементом является только преобразование, элемент управления. 
        // Это позволяет контролировать присоединение и отрисовку обтравочной маски. 
        // См. функцию рендеринга для получения дополнительной информации.
        // ЗАДАЧА: Регулируемый отступ
        ContentLabel.xform.SetPosition(FVec3(nslice.CornerSize.x, ContentLabel.data->LineHeight - 5.F, 0.F));  // padding/2 для y
        ContentLabel.xform.SetParent(&xform);
        ContentLabel.IsActive = true;
        if (!uisys->UpdateActive(&ContentLabel)) {
            MERROR("Не удалось обновить активное состояние системного текстового поля.");
        }
    }

    // Загрузите элемент управления панели для курсора.
    if (!cursor.Load()) {
        MERROR("Не удалось установить курсор в текстовом поле.");
        return false;
    }

    // Создайте курсор и прикрепите его как дочерний элемент.
    if (!uisys->RegisterControl(&cursor)) {
        MERROR("Не удалось зарегистрировать элемент управления.");
    } else {
        if (!AddChild(&cursor)) {
            MERROR("Не удалось сделать системный текст родительским для текстового поля.");
        } else {
            // Задайте начальную позицию.
            cursor.xform.SetPosition(FVec3(nslice.CornerSize.x, ContentLabel.data->LineHeight - 4.F, 0.F));
            cursor.IsActive = true;
            if (!uisys->UpdateActive(&cursor)) {
                MERROR("Не удалось обновить активное состояние курсора текстового поля.");
            }
        }
    }

    // Убедитесь, что положение курсора правильное.
    UpdateCursorPosition(); 

    // Загрузите элемент управления панели для выделения.
    if (!HighlightBox.Load()) {
        MERROR("Не удалось настроить область подсветки в текстовом поле.");
        return false;
    }

    // Создайте область подсветки и прикрепите её как дочерний элемент.
    if (!uisys->RegisterControl(&HighlightBox)) {
        MERROR("Не удалось зарегистрировать элемент управления.");
    } else {
        // ПРИМЕЧАНИЕ: Родительский элемент управления — только элемент управления преобразования. 
        // Это необходимо для управления присоединением и отрисовкой обтравочной маски. 
        // См. функцию рендеринга для получения дополнительной информации.
        // Задайте начальное положение.
        HighlightBox.xform.SetPosition(FVec3(nslice.CornerSize.x, ContentLabel.data->LineHeight - 4.F, 0.F));
        HighlightBox.xform.SetParent(&ContentLabel.xform);
        HighlightBox.IsActive = true;
        HighlightBox.IsVisible = false;
        if (!uisys->UpdateActive(&HighlightBox)) {
            MERROR("Не удалось обновить активное состояние области подсветки текстового поля.");
        }
    }

    // Убедитесь, что размер и положение области подсветки верны.
    UpdateHighlightBox();

    EventSystem::Register(EventSystem::KeyPressed, this, OnKey);
    EventSystem::Register(EventSystem::KeyReleased, this, OnKey);

    return true;
}

void Textbox::Unload()
{
    EventSystem::Unregister(EventSystem::KeyPressed, this, OnKey);
    EventSystem::Unregister(EventSystem::KeyReleased, this, OnKey);
}

bool Textbox::Render(FrameData &rFrameData, UiRenderData &RenderData)
{
    if (nslice.geometry) {
        GeometryRenderData rdata {};
        rdata.UniqueID = id.UniqueID;
        rdata.geometry = nslice.geometry;
        rdata.model = xform.GetWorld();
        rdata.DiffuseColour = colour;

        RenderData.renderables.EmplaceBack(
            &InstanceID,
            &FrameNumber,
            nullptr,
            &DrawIndex,
            rdata,
            nullptr
        );
    }

    // Отобразите метку содержимого вручную, чтобы к ней можно было прикрепить маску обрезки.
    // Это гарантирует, что метка содержимого будет отрисована и обрезана до появления курсора или других дочерних элементов.
    if (!ContentLabel.Render(rFrameData, RenderData)) {
        MERROR("Не удалось отобразить метку содержимого для текстового поля «%s».", name.c_str());
        return false;
    }

    // Маску обрезки добавляйте только в том случае, если метка содержимого фактически содержит... содержимое.
    if (ContentLabel.GetText()) {
        // Маску обрезки добавляйте к тексту, который будет последним добавленным элементом.
        const u32 RenderableCount = RenderData.renderables.Length();
        ClipMask.RenderData.model = ClipMask.ClipXform.GetWorld();
        RenderData.renderables[RenderableCount - 1].ClipMaskRenderData = &ClipMask.RenderData;
    }

    // Выполняйте логику HighlightBox только в том случае, если он видим.
    if (HighlightBox.IsVisible) {
        // Отрисуйте выделенный блок вручную, чтобы к нему можно было прикрепить маску обрезки.
        // Это гарантирует, что выделенный блок будет отрисован и обрезан до того, как будут отрисованы курсор или другие дочерние элементы.
        if (!HighlightBox.Render(rFrameData, RenderData)) {
            MERROR("Не удалось отрисовать выделенный блок для текстового поля «%s».", name.c_str());
            return false;
        }

        // Прикрепите маску обрезки к тексту, который будет последним добавленным элементом.
        u32 RenderableCount = RenderData.renderables.Length();
        ClipMask.RenderData.model = ClipMask.ClipXform.GetWorld();
        RenderData.renderables[RenderableCount - 1].ClipMaskRenderData = &ClipMask.RenderData;
    }

    return true;
}

void Textbox::SetText(const char *text)
{
    ContentLabel.SetText(text);
    CursorPosition = 0;
    UpdateCursorPosition();
}

void Textbox::SetText(MString &text, bool copy)
{
    ContentLabel.SetText(text, copy);
    CursorPosition = 0;
    UpdateCursorPosition();
}

void Textbox::OnMouseDown(UiElement *self, UiMouseEvent event)
{
    if (!self) { return; }
    auto& nslice = ((Textbox*)self)->nslice;
    nslice.AtlasPxMin.x = 151;
    nslice.AtlasPxMin.y = 21;
    nslice.AtlasPxMax.x = 158;
    nslice.AtlasPxMax.y = 28;
    nslice.Update();
}

void Textbox::OnMouseUp(UiElement* self, UiMouseEvent event)
{
    if (!self) { return; }
    auto& nslice = ((Textbox*)self)->nslice;
    if (self->IsHovered) {
        nslice.AtlasPxMin.x = 151;
        nslice.AtlasPxMin.y = 31;
        nslice.AtlasPxMax.x = 158;
        nslice.AtlasPxMax.y = 37;
    } else {
        nslice.AtlasPxMin.x = 151;
        nslice.AtlasPxMin.y = 31;
        nslice.AtlasPxMax.x = 158;
        nslice.AtlasPxMax.y = 37;
    }
    nslice.Update();
}

bool Textbox::OnKey(u16 code, void *sender, void *ListenerInst, EventContext &context)
{
    auto self = (Textbox*)ListenerInst;
    auto sys = (UiSystem*)SystemsManager::GetState(M_SYSTEM_TYPE_STANDARD_UI_EXT);
    if (sys->FocusedID() != self->id.UniqueID) {
        return false; 
    }
    u16 KeyCode = context.data.u16[0];
    if (code == EventSystem::KeyPressed) {
        bool ShiftHeld = InputSystem::IsKeyDown(Keys::LSHIFT) || InputSystem::IsKeyDown(Keys::RSHIFT) || InputSystem::IsKeyDown(Keys::SHIFT);
        bool CtrlHeld = InputSystem::IsKeyDown(Keys::LCONTROL) || InputSystem::IsKeyDown(Keys::RCONTROL) || InputSystem::IsKeyDown(Keys::CONTROL);

        const auto& EntryControlText = self->ContentLabel.GetText();
        u32 len = EntryControlText.Length();
        if (KeyCode == Keys::BACKSPACE) {
            if (len > 0 && (self->CursorPosition > 0 || self->HighlightRange.size > 0)) {
                MString str;
                if (self->HighlightRange.size > 0) {
                    if (self->HighlightRange.size == (i32)len) {
                        self->CursorPosition = 0;
                    } else {
                        str = (MString&&)EntryControlText.CopyPart(self->HighlightRange.offset, self->HighlightRange.size);
                        self->CursorPosition = self->HighlightRange.offset;
                    }
                    // Очистить выделенный диапазон.
                    self->HighlightRange.offset = 0;
                    self->HighlightRange.size = 0;
                    self->UpdateHighlightBox();
                } else {
                    str = (MString&&)EntryControlText.CopyPart(self->CursorPosition - 1, 1);
                    self->CursorPosition--;
                }
                self->ContentLabel.SetText(str, false);
                self->UpdateCursorPosition();
            }
        } else if (KeyCode == Keys::DELETE) {
            if (len > 0) {
                if (self->CursorPosition == len && self->HighlightRange.size == (i32)len) {
                    MString str;
                    self->CursorPosition = 0;
                    // Очистить выделенный диапазон.
                    self->HighlightRange.offset = 0;
                    self->HighlightRange.size = 0;
                    self->UpdateHighlightBox();
                    self->ContentLabel.SetText(str);
                    self->UpdateCursorPosition();
                } else if (self->CursorPosition <= len) {
                    MString str;
                    if (self->HighlightRange.size > 0) {
                        str = EntryControlText.CopyPart(self->HighlightRange.offset, self->HighlightRange.size);
                        self->CursorPosition = self->HighlightRange.offset;
                        // Очистить выделенный диапазон.
                        self->HighlightRange.offset = 0;
                        self->HighlightRange.size = 0;
                        self->UpdateHighlightBox();
                    } else {
                        str = EntryControlText.CopyPart(self->CursorPosition, 1);
                    }
                    self->ContentLabel.SetText(str, false);
                    self->UpdateCursorPosition();
                }
            }
        } else if (KeyCode == Keys::LEFT) {
            if (self->CursorPosition > 0) {
                if (ShiftHeld) {
                    if (self->HighlightRange.size == 0) {
                        self->HighlightRange.offset = (i32)self->CursorPosition;
                    }
                    if ((i32)self->CursorPosition == self->HighlightRange.offset) {
                        self->HighlightRange.offset--;
                        self->HighlightRange.size = MCLAMP(self->HighlightRange.size + 1, 0, (i32)len);
                    } else {
                        self->HighlightRange.size = MCLAMP(self->HighlightRange.size - 1, 0, (i32)len);
                    }
                    self->CursorPosition--;
                } else {
                    if (self->HighlightRange.size > 0) {
                        self->CursorPosition = self->HighlightRange.offset;
                    } else {
                        self->CursorPosition--;
                    }
                    self->HighlightRange.offset = 0;
                    self->HighlightRange.size = 0;
                }
                self->UpdateHighlightBox();
                self->UpdateCursorPosition();
            }
        } else if (KeyCode == Keys::RIGHT) {
            // ПРИМЕЧАНИЕ: позиция курсора может выходить за пределы конца строки, поэтому возврат на одну позицию работает правильно.
            if (self->CursorPosition < len) {
                if (ShiftHeld) {
                    if (self->HighlightRange.size == 0) {
                        self->HighlightRange.offset = (i32)self->CursorPosition;
                    }
                    if ((i32)self->CursorPosition == self->HighlightRange.offset + self->HighlightRange.size) {
                        self->HighlightRange.size = MCLAMP(self->HighlightRange.size + 1, 0, (i32)len);
                    } else {
                        self->HighlightRange.offset = MCLAMP(self->HighlightRange.offset + 1, 0, (i32)len);
                        self->HighlightRange.size = MCLAMP(self->HighlightRange.size - 1, 0, (i32)len);
                    }
                    self->CursorPosition++;
                } else {
                    if (self->HighlightRange.size > 0) {
                        self->CursorPosition = self->HighlightRange.offset + self->HighlightRange.size;
                    } else {
                        self->CursorPosition++;
                    }
                    self->HighlightRange.offset = 0;
                    self->HighlightRange.size = 0;
                }

                self->UpdateHighlightBox();
                self->UpdateCursorPosition();
            }
        } else if (KeyCode == Keys::HOME) {
            if (ShiftHeld) {
                self->HighlightRange.offset = 0;
                self->HighlightRange.size = self->CursorPosition;
            } else {
                self->HighlightRange.offset = 0;
                self->HighlightRange.size = 0;
            }
            self->CursorPosition = 0;
            self->UpdateHighlightBox();
            self->UpdateCursorPosition();
        } else if (KeyCode == Keys::END) {
            if (ShiftHeld) {
                self->HighlightRange.offset = self->CursorPosition;
                self->HighlightRange.size = len - self->CursorPosition;
            } else {
                self->HighlightRange.offset = 0;
                self->HighlightRange.size = 0;
            }
            self->CursorPosition = len;
            self->UpdateHighlightBox();
            self->UpdateCursorPosition();
        } else {
            // Используйте символы A–Z и 0–9 без изменений.
            char CharCode = KeyCode;
            if ((KeyCode >= (u16)Keys::A && KeyCode <= (u16)Keys::Z)) {
                if (CtrlHeld) {
                    if (KeyCode == Keys::A) {
                        CharCode = 0;
                        // Выделите всё и установите курсор в конец.
                        self->HighlightRange.size = len;
                        self->HighlightRange.offset = 0;
                        self->CursorPosition = len;
                        self->UpdateHighlightBox();
                        self->UpdateCursorPosition();
                    }
                }
                // ЗАДАЧА: проверьте Caps Lock.
                if (!ShiftHeld && !CtrlHeld) {
                    CharCode = KeyCode + 32;
                }
            } else if ((KeyCode >= (u16)Keys::KEY_0 && KeyCode <= (u16)Keys::KEY_9)) {
                if (ShiftHeld) {
                    // ПРИМЕЧАНИЕ: эта версия поддерживает стандартные раскладки клавиатуры США.
                    // Потребуется также поддержка других раскладок.
                    switch ((Keys)KeyCode) {
                        case Keys::KEY_0:
                            CharCode = ')';
                            break;
                        case Keys::KEY_1:
                            CharCode = '!';
                            break;
                        case Keys::KEY_2:
                            CharCode = '@';
                            break;
                        case Keys::KEY_3:
                            CharCode = '#';
                            break;
                        case Keys::KEY_4:
                            CharCode = '$';
                            break;
                        case Keys::KEY_5:
                            CharCode = '%';
                            break;
                        case Keys::KEY_6:
                            CharCode = '^';
                            break;
                        case Keys::KEY_7:
                            CharCode = '&';
                            break;
                        case Keys::KEY_8:
                            CharCode = '*';
                            break;
                        case Keys::KEY_9:
                            CharCode = '(';
                            break;
                        default:
                            break;
                    }
                }
            } else {
                switch ((Keys)KeyCode) {
                    case Keys::SPACE:
                        CharCode = KeyCode;
                        break;
                    case Keys::MINUS:
                        CharCode = ShiftHeld ? '_' : '-';
                        break;
                    case Keys::Equal:
                        CharCode = ShiftHeld ? '+' : '=';
                        break;
                    default:
                        // Недействительно для входа, используйте 0
                        CharCode = 0;
                        break;
                }
            }

            if (CharCode != 0) {
                const auto& EntryControlText = self->ContentLabel.GetText();
                u32 len = EntryControlText.Length();
                MString str{len + 2};

                // Если текст выделен, удалите выделенный текст, а затем вставьте его в позицию курсора.
                if (self->HighlightRange.size > 0) {
                    if (self->HighlightRange.size == (i32)len) {
                        self->CursorPosition = 0;
                    } else {
                        str = EntryControlText.CopyPart(self->HighlightRange.offset, self->HighlightRange.size);
                        self->CursorPosition = self->HighlightRange.offset;
                    }
                } else {
                    str = EntryControlText;
                }

                str.InsertChar(self->CursorPosition, CharCode);
                // MString::Format(str, "%s%c", EntryControlText, CharCode);

                self->ContentLabel.SetText(str, false);
                if (self->HighlightRange.size > 0) {
                    // Очистить выделенный диапазон.
                    self->HighlightRange.offset = 0;
                    self->HighlightRange.size = 0;
                    self->UpdateHighlightBox();
                } else {
                    self->CursorPosition++;
                }
                self->UpdateCursorPosition();
            }
        }
    }
    // if (self->OnKey()) {
        UiKeyboardEvent event {};
        event.key = (Keys)KeyCode;
        event.type = code == EventSystem::KeyPressed ? UiKeyboardEventType::PRESS : UiKeyboardEventType::RELEASE;
        self->OnKey(event);
    // }

    return false;
}

f32 Textbox::CalculateCursorOffset(u32 StringPosition, MString &FullString, FontData *font)
{
    if (StringPosition == 0) {
        return 0;
    }

    auto MidTarget = FullString;
    
    MidTarget.Mid(FullString, 0, StringPosition);

    FVec2 size = FontSystem::MeasureString(font, MidTarget.c_str());

    // Используйте ось X измерения для размещения курсора.
    return size.x;
}

void Textbox::UpdateCursorPosition()
{
    // Смещение от начала строки.
    f32 offset = CalculateCursorOffset(CursorPosition, ContentLabel.text, ContentLabel.data);
    f32 padding = nslice.CornerSize.x;

    // Предполагаемая позиция курсора (без учёта отступов).
    FVec3 CursorPosition{};
    CursorPosition.x = offset + TextViewOffset;
    CursorPosition.y = 6.F;  // ЗАДАЧА: настраиваемая

    // Убедитесь, что курсор находится в пределах текстового поля.
    // Пока не учитывайте отступы.
    f32 ClipWidth = size.x - (padding * 2);
    f32 ClipXMin = padding;
    f32 ClipXMax = ClipXMin + ClipWidth;
    f32 diff = 0;
    if (CursorPosition.x > ClipWidth) {
        diff = ClipWidth - CursorPosition.x;
        // Установите курсор вплотную к краю с учётом отступов.
        CursorPosition.x = ClipXMax;
    } else if (CursorPosition.x < 0) {
        diff = 0 - CursorPosition.x;
        // Установите курсор вплотную к краю с учётом отступов.
        CursorPosition.x = ClipXMin;
    } else {
        // Используйте текущую позицию, но добавьте отступы.
        CursorPosition.x += padding;
    }
    // Сохраните смещение представления.
    TextViewOffset+= diff;
    // Переместите метку вперёд/назад, чтобы выровнять её с курсором, с учётом отступов.
    auto LabelPosition = ContentLabel.xform.position;
    ContentLabel.xform.SetPosition(FVec3(padding + TextViewOffset, LabelPosition.y, LabelPosition.z));

    // Переместите курсор в новое положение.
    cursor.xform.SetPosition(CursorPosition);
}

void Textbox::UpdateHighlightBox()
{
    if (HighlightRange.size == 0) {
        HighlightBox.IsVisible = false;
        return;
    }

    HighlightBox.IsVisible = true;

    // Смещение от начала строки.
    f32 OffsetStart = CalculateCursorOffset(HighlightRange.offset, ContentLabel.text, ContentLabel.data);
    f32 OffsetEnd   = CalculateCursorOffset(HighlightRange.offset + HighlightRange.size, ContentLabel.text, ContentLabel.data);
    f32 width = OffsetEnd - OffsetStart;
    // f32 padding = nslice.CornerSize.x;

    FVec3 InitialPosition = HighlightBox.xform.GetPosition();
    InitialPosition.y = -ContentLabel.data->LineHeight + 10.F;
    HighlightBox.xform.SetPosition(FVec3(OffsetStart, InitialPosition.y, InitialPosition.z));
    HighlightBox.xform.SetScale(FVec3(width, 1.F, 1.F));
}
