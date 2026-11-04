#include "textbox.h"
#include <core/event.h>
#include <core/systems_manager.h>
#include <math/geometry_utils.h>
#include <renderer/rendering_system.h>
#include <resources/geometry.h>
#include <systems/geometry_system.h>
#include <systems/font_system.h>
#include <systems/shader_system.h>

static bool on_key(u16 code, void* sender, void* ListenerInst, const EventContext& context);
static f32 CalculateCursorOffset(Textbox* self, u32 StringPosition/*, MString& FullString, FontData* font*/);
static void UpdateCursorPosition(Textbox* self);
static void UpdateHighlightBox(Textbox* self);

constexpr Textbox::Textbox(UiElement *parent) : UiElement(parent), ContentLabel(), cursor(), HighlightBox()
{
    UiElement::OnMouseDown = OnMouseDown;
    UiElement::OnMouseUp = OnMouseUp;
}

constexpr Textbox::Textbox(u32 BufferSize, UiElement *parent) : UiElement(parent), ContentLabel(128), cursor(), HighlightBox()
{
    UiElement::OnMouseDown = OnMouseDown;
    UiElement::OnMouseUp = OnMouseUp;
}

Textbox::Textbox(const char *name, FontType type, const char *FontName, u16 FontSize, const char *text, UiElement *parent) 
: UiElement(name, parent), size(Point(200, FontSize + 10)), colour(FVec4::One()),
  ContentLabel(MString(name, "_textbox_internal_label"), type, FontName, FontSize, text),
  cursor(MString(name, "_textbox_cursor_panel"), FVec2(1.F, FontSize - 4.F), FVec4::One()),                   // Использовать панель в качестве курсора.
  HighlightBox(MString(name, "_textbox_highlight_panel"), FVec2(1.F, FontSize), FVec4(0.F, 0.5F, 0.9F, 0.5F)) // Поле подсветки(выделения)
{
    UiElement::OnMouseDown = OnMouseDown;
    UiElement::OnMouseUp = OnMouseUp;
}

bool Textbox::Create(const char *name, FontType type, const char *FontName, u16 FontSize, const char *text, UiElement *parent)
{
    this->name = name;
    if (parent) parent->AddChild(this);
    size.Set(200, FontSize + 10);
    colour = FVec4::One();
    MString nname = MString(name, "_internal_label");
    ContentLabel.Create(nname, type, FontName, FontSize, text/*, this*/);
    cursor.Create(nname = MString(name, "_cursor_panel"), FVec2(1.F, FontSize - 4.F), FVec4::One()/*, this*/); // Использовать панель в качестве курсора.
    HighlightBox.Create(nname = MString(name, "_highlight_panel"), FVec2(1.F, FontSize), FVec4(0.F, 0.5F, 0.9F, 0.5F)/*, &ContentLabel*/); // Поле подсветки(выделения)
    // if (text) {
    //     CursorPosition = ContentLabel.text.Length();
    //     UpdateCursorPosition(this);
    // }
    
    return true;
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
    nslice = NineSlice(name.c_str(), CornerSize, CornerPxSize, size, AtlasMin, AtlasMax, AtlasSize);

    bounds.x = 0.F;
    bounds.y = 0.F;
    bounds.width = size.x;
    bounds.height = size.y;

    // Настройка геометрии обтравочной маски текстового поля.
    ClipMask.ReferenceID = 1;  // ЗАДАЧА: Создание перемещения/Назначение reference_id.

    GeometryConfig ClipConfig = Math::Geometry::GenerateQuad2D("textbox_clipping_box", size.x - (CornerSize.x * 2), size.y, 0, 0, 0, 0);
    ClipMask.ClipGeometry = GeometrySystem::Acquire(ClipConfig, false);
    Geometry* ClipGeometry = ClipMask.ClipGeometry;

    GeometryRenderData& grd = ClipMask.RenderData;
    grd.model    = Matrix4D::MakeIdentity();
    grd.UniqueID = ClipMask.ReferenceID;
    
    // grd.material = nullptr;
    grd.VertexCount        = ClipGeometry->VertexCount;
    grd.VertexElementSize  = ClipGeometry->VertexElementSize;
    grd.VertexBufferOffset = ClipGeometry->VertexBufferOffset;

    grd.IndexCount         = ClipGeometry->IndexCount;
    grd.IndexElementSize   = ClipGeometry->IndexElementSize;
    grd.IndexBufferOffset  = ClipGeometry->IndexBufferOffset;

    grd.DiffuseColour = FVec4();  // Прозрачный.;

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

    // if (!uisys->RegisterControl(&ContentLabel)) {
    //     MERROR("Не удалось зарегистрировать элемент управления.");
    // } else {
        // ПРИМЕЧАНИЕ: Родительским элементом является только преобразование, элемент управления. 
        // Это позволяет контролировать присоединение и отрисовку обтравочной маски. 
        // См. функцию рендеринга для получения дополнительной информации.
        // ЗАДАЧА: Регулируемый отступ
        ContentLabel.xform.SetPosition(FVec3(nslice.CornerSize.x, ContentLabel.data->LineHeight - 5.F, 0.F));  // padding/2 для y
        ContentLabel.xform.SetParent(&xform);
        ContentLabel.IsActive = true;
    // }

    // Загрузите элемент управления панели для курсора.
    if (!cursor.Load()) {
        MERROR("Не удалось установить курсор в текстовом поле.");
        return false;
    }

    // Создайте курсор и прикрепите его как дочерний элемент.
    // if (!uisys->RegisterControl(&cursor)) {
    //     MERROR("Не удалось зарегистрировать элемент управления.");
    // } else {
        // Задайте начальную позицию.
        cursor.xform.SetPosition(FVec3(nslice.CornerSize.x, ContentLabel.data->LineHeight - 4.F, 0.F));
        cursor.xform.SetParent(&xform);
        cursor.IsActive = true;
    // }

    // Убедитесь, что положение курсора правильное.
    UpdateCursorPosition(this); 

    // Загрузите элемент управления панели для выделения.
    if (!HighlightBox.Load()) {
        MERROR("Не удалось настроить область подсветки в текстовом поле.");
        return false;
    }

    // Создайте область подсветки и прикрепите её как дочерний элемент.
    // if (!uisys->RegisterControl(&HighlightBox)) {
    //     MERROR("Не удалось зарегистрировать элемент управления.");
    // } else {
        // ПРИМЕЧАНИЕ: Родительский элемент управления — только элемент управления преобразования. 
        // Это необходимо для управления присоединением и отрисовкой обтравочной маски. 
        // См. функцию рендеринга для получения дополнительной информации.
        // Задайте начальное положение.
        HighlightBox.xform.SetPosition(FVec3(nslice.CornerSize.x, ContentLabel.data->LineHeight - 4.F, 0.F));
        HighlightBox.xform.SetParent(&ContentLabel.xform);
        HighlightBox.IsActive = true;
        HighlightBox.IsVisible = false;
    // }

    // Убедитесь, что размер и положение области подсветки верны.
    UpdateHighlightBox(this);

    EventSystem::Register(EventSystem::KeyPressed, this, on_key);
    EventSystem::Register(EventSystem::KeyReleased, this, on_key);

    return true;
}

void Textbox::Unload()
{
    EventSystem::Unregister(EventSystem::KeyPressed, this, on_key);
    EventSystem::Unregister(EventSystem::KeyReleased, this, on_key);
}

void Textbox::RenderPrepare(const FrameData &rFrameData)
{
    ContentLabel.RenderPrepare(rFrameData);
    cursor.RenderPrepare(rFrameData);
    HighlightBox.RenderPrepare(rFrameData);
    
}

bool Textbox::Render(FrameData &rFrameData, UiRenderData &RenderData)
{
    if (nslice.geometry) {
        UiRenderable* renderable = RenderData.renderables.PushBack();
        MemorySystem::ZeroMem(renderable, sizeof(UiRenderable));
        renderable->InstanceID = &InstanceID;
        renderable->FrameNumber = &FrameNumber;
        renderable->DrawIndex = &DrawIndex;
        GeometryRenderData& grd = renderable->RenderData;

        Geometry* g = nslice.geometry;
        grd.model              = xform.GetWorld();
        grd.material           = g->material;
        grd.UniqueID           = id.UniqueID;
        grd.VertexCount        = g->VertexCount;
        grd.VertexElementSize  = g->VertexElementSize;
        grd.VertexBufferOffset = g->VertexBufferOffset;
        grd.IndexCount         = g->IndexCount;
        grd.IndexElementSize   = g->IndexElementSize;
        grd.IndexBufferOffset  = g->IndexBufferOffset;
        grd.DiffuseColour      = colour;

    }

    // Отобразите метку содержимого вручную, чтобы к ней можно было прикрепить маску обрезки.
    // Это гарантирует, что метка содержимого будет отрисована и обрезана до появления курсора или других дочерних элементов.
    if (!ContentLabel.Render(rFrameData, RenderData)) {
        MERROR("Не удалось отобразить метку содержимого для текстового поля «%s».", name.c_str());
        return false;
    }
    
    // Маску обрезки добавляйте только в том случае, если метка содержимого фактически содержит... содержимое.
    // if (ContentLabel.GetText()) {
    //     // Маску обрезки добавляйте к тексту, который будет последним добавленным элементом.
    //     u32 RenderableCount = RenderData.renderables.Length();
    //     ClipMask.RenderData.model = ClipMask.ClipXform.GetWorld();
    //     RenderData.renderables[RenderableCount - 1].ClipMaskRenderData = &ClipMask.RenderData;
    // }
    
    if (!cursor.Render(rFrameData, RenderData)) {
        MERROR("Не удалось отобразить курсор текстового поля «%s».", name.c_str());
        return false;
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
    UpdateCursorPosition(this);
}

void Textbox::SetText(MString &text, bool copy)
{
    ContentLabel.SetText(text, copy);
    CursorPosition = 0;
    UpdateCursorPosition(this);
}

void Textbox::OnMouseDown(UiElement *self, UiMouseEvent event)
{
    /*if (!self) { return; }
    auto& nslice = ((Textbox*)self)->nslice;
    nslice.AtlasPxMin.x = 151;
    nslice.AtlasPxMin.y = 21;
    nslice.AtlasPxMax.x = 158;
    nslice.AtlasPxMax.y = 28;
    nslice.Update();*/
}

void Textbox::OnMouseUp(UiElement* self, UiMouseEvent event)
{
    /*if (!self) { return; }
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
    nslice.Update();*/
}

bool on_key(u16 code, void *sender, void *ListenerInst, const EventContext &context)
{
    auto self = (Textbox*)ListenerInst;
    auto sys = (UiSystem*)SystemsManager::GetState(M_SYSTEM_TYPE_STANDARD_UI_EXT);
    if (sys->FocusedID() != self->id.UniqueID) {
        return false; 
    }
    using enum Keys;
    u16 KeyCode = context.data.u16[0];
    if (code == EventSystem::KeyPressed) {
        bool ShiftHeld = InputSystem::IsKeyDown(LSHIFT) || InputSystem::IsKeyDown(RSHIFT) || InputSystem::IsKeyDown(SHIFT);
        bool CtrlHeld = InputSystem::IsKeyDown(LCONTROL) || InputSystem::IsKeyDown(RCONTROL) || InputSystem::IsKeyDown(CONTROL);

        MString& EntryControlText = self->ContentLabel.GetText();
        u32 len = EntryControlText.Length();
        if (KeyCode == BACKSPACE) {
            if (len > 0 && (self->CursorPosition > 0 || self->HighlightRange.size > 0)) {
                if (self->HighlightRange.size > 0) {
                    if (self->HighlightRange.size == (i32)len) {
                        self->CursorPosition = 0;
                    } else {
                        self->ContentLabel.RemovePart(self->HighlightRange.offset, self->HighlightRange.size);
                        self->CursorPosition = self->HighlightRange.offset;
                    }
                    // Очистить выделенный диапазон.
                    self->HighlightRange.offset = 0;
                    self->HighlightRange.size = 0;
                    UpdateHighlightBox(self);
                } else {
                    self->ContentLabel.RemovePart(self->CursorPosition - 1, 1);
                    self->CursorPosition--;
                }
                UpdateCursorPosition(self);
            }
        } else if (KeyCode == DELETE) {
            if (len > 0) {
                if (self->CursorPosition == len && self->HighlightRange.size == (i32)len) {
                    self->CursorPosition = 0;
                    // Очистить выделенный диапазон.
                    self->HighlightRange.offset = 0;
                    self->HighlightRange.size = 0;
                    UpdateHighlightBox(self);
                    MString str;
                    self->ContentLabel.SetText(str);
                    UpdateCursorPosition(self);
                } else if (self->CursorPosition <= len) {
                    MString str;
                    if (self->HighlightRange.size > 0) {
                        EntryControlText.RemovePart(self->HighlightRange.offset, self->HighlightRange.size);
                        self->CursorPosition = self->HighlightRange.offset;
                        // Очистить выделенный диапазон.
                        self->HighlightRange.offset = 0;
                        self->HighlightRange.size = 0;
                        UpdateHighlightBox(self);
                    } else {
                        EntryControlText.CopyPart(self->CursorPosition, 1);
                    }
                    self->ContentLabel.SetText(str, false);
                    UpdateCursorPosition(self);
                }
            }
        } else if (KeyCode == LEFT) {
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
                UpdateHighlightBox(self);
                UpdateCursorPosition(self);
            }
        } else if (KeyCode == RIGHT) {
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

                UpdateHighlightBox(self);
                UpdateCursorPosition(self);
            }
        } else if (KeyCode == HOME) {
            if (ShiftHeld) {
                self->HighlightRange.offset = 0;
                self->HighlightRange.size = self->CursorPosition;
            } else {
                self->HighlightRange.offset = 0;
                self->HighlightRange.size = 0;
            }
            self->CursorPosition = 0;
            UpdateHighlightBox(self);
            UpdateCursorPosition(self);
        } else if (KeyCode == END) {
            if (ShiftHeld) {
                self->HighlightRange.offset = self->CursorPosition;
                self->HighlightRange.size = len - self->CursorPosition;
            } else {
                self->HighlightRange.offset = 0;
                self->HighlightRange.size = 0;
            }
            self->CursorPosition = len;
            UpdateHighlightBox(self);
            UpdateCursorPosition(self);
        } else {
            // Используйте символы A–Z и 0–9 без изменений.
            char CharCode = KeyCode;
            if ((KeyCode >= (u16)A && KeyCode <= (u16)Z)) {
                if (CtrlHeld) {
                    if (KeyCode == A) {
                        CharCode = 0;
                        // Выделите всё и установите курсор в конец.
                        self->HighlightRange.size = len;
                        self->HighlightRange.offset = 0;
                        self->CursorPosition = len;
                        UpdateHighlightBox(self);
                        UpdateCursorPosition(self);
                    }
                }
                // ЗАДАЧА: проверьте Caps Lock.
                if (!ShiftHeld && !CtrlHeld) {
                    CharCode = KeyCode + 32;
                }
            } else if ((KeyCode >= (u16)KEY_0 && KeyCode <= (u16)KEY_9)) {
                if (ShiftHeld) {
                    // ПРИМЕЧАНИЕ: эта версия поддерживает стандартные раскладки клавиатуры США.
                    // Потребуется также поддержка других раскладок.
                    switch ((Keys)KeyCode) {
                        case KEY_0:
                            CharCode = ')';
                            break;
                        case KEY_1:
                            CharCode = '!';
                            break;
                        case KEY_2:
                            CharCode = '@';
                            break;
                        case KEY_3:
                            CharCode = '#';
                            break;
                        case KEY_4:
                            CharCode = '$';
                            break;
                        case KEY_5:
                            CharCode = '%';
                            break;
                        case KEY_6:
                            CharCode = '^';
                            break;
                        case KEY_7:
                            CharCode = '&';
                            break;
                        case KEY_8:
                            CharCode = '*';
                            break;
                        case KEY_9:
                            CharCode = '(';
                            break;
                        default:
                            break;
                    }
                }
            } else {
                switch ((Keys)KeyCode) {
                    case SPACE:
                        CharCode = KeyCode;
                        break;
                    case MINUS:
                        CharCode = ShiftHeld ? '_' : '-';
                        break;
                    case Equal:
                        CharCode = ShiftHeld ? '+' : '=';
                        break;
                    case PERIOD:
                        CharCode = ShiftHeld ? '>' : '.';
                        break;
                    case COMMA:
                        CharCode = ShiftHeld ? '<' : ',';
                        break;
                    case SLASH:
                        CharCode = ShiftHeld ? '?' : '/';
                        break;
                    case QUOTE:
                        CharCode = ShiftHeld ? '"' : '\'';
                        break;
                    case SEMICOLON:
                        CharCode = ShiftHeld ? ':' : ';';
                        break;
                    default:
                        // Недействительно для входа, используйте 0
                        CharCode = 0;
                        break;
                }
            }

            if (CharCode != 0) {
                u32 len = EntryControlText.Length();

                // Если текст выделен, удалите выделенный текст, а затем вставьте его в позицию курсора.
                if (self->HighlightRange.size > 1) {
                    if (self->HighlightRange.size == (i32)len) {
                        self->CursorPosition = 0;
                    } else {
                        EntryControlText.RemovePart(self->HighlightRange.offset, self->HighlightRange.size);
                        self->CursorPosition = self->HighlightRange.offset;
                    }
                }

                EntryControlText.InsertChar(self->CursorPosition, CharCode);

                self->ContentLabel.SetText(EntryControlText, false);
                if (self->HighlightRange.size > 0) {
                    // Очистить выделенный диапазон.
                    self->HighlightRange.offset = 0;
                    self->HighlightRange.size = 0;
                    UpdateHighlightBox(self);
                } else {
                    self->CursorPosition++;
                }
                
                UpdateCursorPosition(self);
            }
        }
    }

    if (self->OnKey) {
        UiKeyboardEvent event {
            .key = (Keys)KeyCode,
            .type = code == EventSystem::KeyPressed ? UiKeyboardEventType::PRESS : UiKeyboardEventType::RELEASE
        };
        self->OnKey(self, event);
    }

    return false;
}

f32 CalculateCursorOffset(Textbox *self, u32 StringPosition/*, MString &FullString, FontData *font*/)
{
    if (StringPosition == 0) {
        return 0;
    }

    Substring MidTarget = self->ContentLabel.text.Mid(0, StringPosition); // FullString;

    FVec2 size = FontSystem::MeasureString(self->ContentLabel.data, MidTarget);

    // Используйте ось X измерения для размещения курсора.
    return size.x;
}

void UpdateCursorPosition(Textbox* self)
{
    // Смещение от начала строки.
    f32 offset = CalculateCursorOffset(self, self->CursorPosition);
    f32 padding = self->nslice.CornerSize.x;

    // Предполагаемая позиция курсора (без учёта отступов).
    FVec3 CursorPosition { offset + self->TextViewOffset, 6.F }; // ЗАДАЧА: настраиваемая

    // Убедитесь, что курсор находится в пределах текстового поля.
    // Пока не учитывайте отступы.
    f32 ClipWidth = self->size.x - (padding * 2);
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
    self->TextViewOffset+= diff;
    // Переместите метку вперёд/назад, чтобы выровнять её с курсором, с учётом отступов.
    const FVec3& LabelPosition = self->ContentLabel.xform.position;
    self->ContentLabel.xform.SetPosition(FVec3(padding + self->TextViewOffset, LabelPosition.y, LabelPosition.z));

    // Переместите курсор в новое положение.
    self->cursor.xform.SetPosition(CursorPosition);
}

void UpdateHighlightBox(Textbox* self)
{
    if (self->HighlightRange.size == 0) {
        self->HighlightBox.IsVisible = false;
        return;
    }

    self->HighlightBox.IsVisible = true;

    // Смещение от начала строки.
    f32 OffsetStart = CalculateCursorOffset(self, self->HighlightRange.offset);
    f32 OffsetEnd   = CalculateCursorOffset(self, self->HighlightRange.offset + self->HighlightRange.size);
    f32 width = OffsetEnd - OffsetStart;
    // f32 padding = nslice.CornerSize.x;

    FVec3 InitialPosition = self->HighlightBox.xform.GetPosition();
    InitialPosition.y = -self->ContentLabel.data->LineHeight + 10.F;
    self->HighlightBox.xform.SetPosition(FVec3(OffsetStart, InitialPosition.y, InitialPosition.z));
    self->HighlightBox.xform.SetScale(FVec3(width, 1.F, 1.F));
}
