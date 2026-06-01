#include "rendergraph.h"
#include "rendering_system.h"
#include "core/systems_manager.h"
#include "resources/texture.h"

MINLINE static RendergraphNode* FindPass(const char *PassName, const DArray<RendergraphNode*>& passes)
{
    RendergraphNode* pass = nullptr;
    const u32& PassCount = passes.Length();
    for (u32 i = 0; i < PassCount; ++i) {
        if (passes[i]->name == PassName) {
            pass = passes[i];
            break;
        }
    }
    return pass;
}

static bool RegenerateRenderTargets(Rendergraph* graph, RendergraphNode* pass, u16 width, u16 height);

bool Rendergraph::Create(const char *name, Application &app)
{
    this->name = name;
    this->app = &app;
    return true;
}

void Rendergraph::Destroy()
{
    app = nullptr;
    name.Destroy();
    
    if (passes) {
        auto renderer = SystemsManager::GetRenderingSystem();
        const u32 PassCount = passes.Length();
        for (u32 i = 0; i < PassCount; i++) {
            auto pass = passes[i];

            // Уничтожение целей рендеринга
            for (u32 p = 0; p < pass->pass.RenderTargetCount; p++) {
                auto& target = pass->pass.targets[p];

                // Уничтожьте цель, если она существует.
                renderer->RenderTargetDestroy(target, true);
            }
            // Уничтожаем сам проход.
            pass->Destroy();
        }
        passes.Destroy();
    }

    GlobalSources.Destroy();
}

bool Rendergraph::AddGlobalSource(const char *name, RendergraphSourceType type, RendergraphSourceOrigin origin)
{
    GlobalSources.EmplaceBack(name, type, origin);

    return true;
}

bool Rendergraph::AddPass(RendergraphNode &RPass)
{
    // Убедитесь, что прохода с таким именем еще не существует.
    const u32 PassCount = passes.Length();
    for (u32 i = 0; i < PassCount; ++i) {
        if (passes[i]->name == name) {
            MERROR("Невозможно добавить прохода, так как прохода с именем «%s» уже существует.", name.c_str());
            return false;
        }
    }
    passes.PushBack(&RPass);
    return true;
}

bool Rendergraph::AddPassSource(const char *PassName, const char *SourceName, RendergraphSourceType type, RendergraphSourceOrigin origin)
{
    // Найдите проход.
    auto pass = FindPass(PassName, passes);

    if (!pass) {
        MERROR("Не удалось найти проход рендерграфа с именем «%s».", PassName);
        return false;
    }

    // Убедитесь, что у прохода нет источника с таким же именем.
    const u32 SourceCount = pass->sources.Length();
    for (u32 i = 0; i < SourceCount; ++i) {
        const auto& source = pass->sources[i];
        if (source.name == SourceName) {
            MERROR("У прохода «%s» уже есть источник с именем «%s». Источник не добавлен.", PassName, SourceName);
            return false;
        }
    }

    pass->sources.EmplaceBack(SourceName, type, origin);

    return true;
}

bool Rendergraph::AddPassSink(const char *PassName, const char *SinkName)
{
    // Найдите проход.
    auto pass = FindPass(PassName, passes);

    if (!pass) {
        MERROR("Не удалось найти проход рендерграфа с именем «%s».", PassName);
        return false;
    }

    // Убедитесь, что у прохода ещё нет приёмника с таким же именем.
    const u32 SinkCount = pass->sinks.Length();
    for (u32 i = 0; i < SinkCount; ++i) {
        if (pass->sinks[i].name == SinkName) {
            MERROR("У прохода «%s» уже есть приёмник с именем «%s». Приёмник не добавлен.", PassName, SinkName);
            return false;
        }
    }

    pass->sinks.EmplaceBack(SinkName);

    return true;
}

void Rendergraph::DisableRederpassNode(const char *PassName)
{
}

void Rendergraph::EnableRenderpassNode(const char *PassName)
{
}

bool Rendergraph::LoadResource()
{
    u32 PassCount = passes.Length();
    for (u32 i = 0; i < PassCount; ++i) {
        auto pass = passes[i];

        // Перед загрузкой ресурсов убедитесь, что для всех источников, подключенных самостоятельно, загружены текстуры.
        u32 SourceCount = pass->sources.Length();
        for (u32 j = 0; j < SourceCount; ++j) {
            auto& source = pass->sources[j];
            if (source.origin == RendergraphSourceOrigin::Self) {
                // Если источник является собственным, подключите текстуры к источнику.
                if (pass->SourcePopulate) {
                    if (!pass->SourcePopulate(pass, source)) {
                        MERROR("Не удалось заполнить источник '%s'.", source.name.c_str());
                    }
                } else {
                    MERROR("Проход графа рендеринга '%s': источник '%s' установлен в RENDERGRAPH_SOURCE_ORIGIN_SELF, но для него не определена функция source_populate.");
                    return false;
                }
            }
        }

        if (pass->LoadResources) {
            pass->LoadResources(pass);
        }
        
    }

    return true;
}

bool Rendergraph::PassSetSinkLinkage(const char *PassName, const char *SinkName, const char *SourcePassName, const char *SourceName)
{
    // Найти целевой проход.
    auto pass = FindPass(PassName, passes);

    if (!pass) {
        MERROR("Не удалось найти целевой проход рендерграфа с именем «%s».", PassName);
        return false;
    }

    // Найти целевой приемник.
    RendergraphSink* sink = nullptr;
    const u32 SinkCount = pass->sinks.Length();
    for (u32 i = 0; i < SinkCount; ++i) {
        if (pass->sinks[i].name == SinkName) {
            sink = &pass->sinks[i];
            break;
        }
    }

    if (!sink) {
        MERROR("Не удалось найти приемник с именем «%s» на целевом проходе рендерграфа с именем «%s».", SinkName, PassName);
        return false;
    }

    RendergraphSource* source = nullptr;
    if (!SourcePassName) {
        // Глобальный источник.
        const u32 SourceCount = GlobalSources.Length();
        for (u32 i = 0; i < SourceCount; ++i) {
            if (GlobalSources[i].name == SourceName) {
                source = &GlobalSources[i];
                break;
            }
        }
    } else {
        // Источник прохода.
        auto Sourcepass = FindPass(SourcePassName, passes);
        
        if (!Sourcepass) {
            MERROR("Не удалось найти исходный проход с именем «%s».", SourcePassName);
            return false;
        }

        const u32 SourceCount = Sourcepass->sources.Length();
        for (u32 i = 0; i < SourceCount; ++i) {
            if (Sourcepass->sources[i].name == SourceName) {
                source = &Sourcepass->sources[i];
                break;
            }
        }
    }

    if (!source) {
        MERROR("Не удалось найти источник с именем «%s».", SourceName);
        return false;
    }

    // Всё необходимое для выполнения ссылки присутствует, выполните это.
    sink->BoundSource = source;

    return true;
}

bool Rendergraph::Finalize()
{
    // Получите глобальные ссылки на текстуры для глобальных источников.
    const u32 GlobalSourceCount = GlobalSources.Length();
    auto renderer = SystemsManager::GetRenderingSystem();
    for (u32 i = 0; i < GlobalSourceCount; ++i) {
        auto& source = GlobalSources[i];
        if (source.origin == RendergraphSourceOrigin::Global) {
            u32 AttachmentCount = renderer->GetWindowAttachmentCount();
            source.textures = (Texture**)MemorySystem::Allocate(sizeof(void*) * AttachmentCount, Memory::Array);
            for (u32 j = 0; j < AttachmentCount; ++j) {
                if (source.type == RendergraphSourceType::RenderTargetColour) {
                    source.textures[i] = renderer->GetWindowAttachment(i);
                } else if (source.type == RendergraphSourceType::RenderTargetDepthStencil) {
                    source.textures[i] = renderer->GetDepthAttachment(i);
                } else {
                    MERROR("Неподдерживаемый тип источника: 0x%x", source.type);
                    return false;
                }
            }
        } 
    }

    // Проверьте, что что-то связано с глобальным источником цвета.
    RendergraphNode* BackbufferFirstUser = nullptr;
    const u32 PassCount = passes.Length();
    for (u32 i = 0; i < PassCount; ++i) {
        const u32& SinkCount = passes[i]->sinks.Length();
        for (u32 j = 0; j < SinkCount; ++j) {
            auto source = passes[i]->sinks[j].BoundSource;
            if (source) {
                if (source->origin == RendergraphSourceOrigin::Global && source->type == RendergraphSourceType::RenderTargetColour) {
                    // найдено.
                    BackbufferFirstUser = passes[i];
                    break;
                }
            }
        }
    }
    if (!BackbufferFirstUser) {
        MERROR("Ошибка конфигурации рендерграфа: Ссылка на глобальный источник заднего буфера отсутствует.");
        return false;
    }

    // Пройдите весь список всех проходов и убедитесь, что все приёмники имеют привязанные источники.
    // Затем определите, какой выход источника является последним целевым рендером, и свяжите его с глобальным приёмником backbuffer_global_sink.
    for (u32 i = 0; i < PassCount; ++i) {
        auto pass = passes[i];

        // Найдите источник «заднего буфера» для этого прохода, если он есть.
        const u32 SourceCount = pass->sources.Length();
        for (u32 j = 0; j < SourceCount; ++j) {
            auto& source = pass->sources[j];
            if (source.type == RendergraphSourceType::RenderTargetColour) {
                if (source.origin == RendergraphSourceOrigin::Other) {
                    // Other — ссылка на выход источника другого прохода.
                    // Найдите все приёмники других проходов, чтобы узнать, есть ли у кого-либо этот источник в качестве привязанного источника.
                    for (u32 k = 0; k < PassCount; ++k) {
                        auto RefCheckPass = passes[k];
                        const u32& SinkCount = RefCheckPass->sinks.Length();
                        bool found = false;
                        for (u32 s = 0; s < SinkCount; ++s) {
                            if (RefCheckPass->sinks[s].BoundSource == &source) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            // Конец строки. Этот источник должен быть подключен к backbuffer_global_sink.
                            BackbufferGlobalSink.BoundSource = &source;
                            pass->PresentsAfter = true;
                            break;
                        }
                    }
                }
                } else if (source.type == RendergraphSourceType::RenderTargetDepthStencil) {
                if (source.origin == RendergraphSourceOrigin::Other) {
                    // «Другое» — это ссылка на выходной сигнал другого прохода.
                    // Просмотрите все остальные приемники прохода, чтобы узнать, есть ли среди них этот источник в качестве связанного источника.
                    for (u32 k = 0; k < PassCount; ++k) {
                        auto RefCheckPass = passes[k];
                        u32 SinkCount = RefCheckPass->sinks.Length();
                        bool found = false;
                        for (u32 s = 0; s < SinkCount; ++s) {
                            if (RefCheckPass->sinks[s].BoundSource == &source) {
                                found = true;
                                break;
                            }
                        }
                        if (!found) {
                            MWARN("Источник с доступной текстурой трафарета глубины не найден.");
                            break;
                        }
                    }
                } else if (source.origin == RendergraphSourceOrigin::Self) {
                    // Если начало координат — self, подключите текстуры к источнику.
                    if (pass->SourcePopulate) {
                        if (!pass->SourcePopulate(pass, source)) {
                            MERROR("Не удалось заполнить источник '%s'.", source.name.c_str());
                        }
                    } else {
                        MERROR("Проход графа рендеринга '%s': источник '%s' установлен в RENDERGRAPH_SOURCE_ORIGIN_SELF, но для него не определена функция source_populate.");
                        return false;
                    }
                }
            }
            if (BackbufferGlobalSink.BoundSource) {
                break;
            }
        }
        if (BackbufferGlobalSink.BoundSource) {
            break;
        }
    }

    if (!BackbufferGlobalSink.BoundSource) {
        MERROR("Не удалось связать backbuffer_global_sink с источником, так как источник не найден.");
        return false;
    }

    // После завершения всех связей инициализируйте каждый проход.
    for (u32 i = 0; i < PassCount; ++i) {
        auto pass = passes[i];
        if (!pass->Initialize()) {
            MERROR("Ошибка инициализации прохода. Проверьте логи для получения дополнительной информации.");
            return false;
        }

        // Также сгенерируйте цели рендеринга.
        // ЗАДАЧА: Получите разрешение по умолчанию.
        if (!RegenerateRenderTargets(this, pass, 1280, 720)) {
            MERROR("Не удалось повторно сгенерировать цели рендеринга.");
            return false;
        }
    }

    return true;
}

bool Rendergraph::ExecuteFrame(FrameData &rFrameData)
{
    // Пропуска будут выполняться в порядке их добавления.
    const u32 PassCount = passes.Length();
    for (u32 i = 0; i < PassCount; ++i) {
        if (!passes[i]->PassData.DoExecute) {
            continue;
        }
        if (!passes[i]->Render(rFrameData)) {
            MERROR("Ошибка выполнения прохода. Проверьте журналы для получения дополнительной информации.");
            return false;
        }
    }

    return true;
}

bool Rendergraph::OnResize(u16 width, u16 height)
{
    const u32 PassCount = passes.Length();
    for (u32 i = 0; i < PassCount; ++i) {
        RegenerateRenderTargets(this, passes[i], width, height);
    }

    return true;
}

bool RegenerateRenderTargets(Rendergraph *graph, RendergraphNode *pass, u16 width, u16 height)
{
    auto renderer = SystemsManager::GetRenderingSystem();
    for (u32 i = 0; i < pass->pass.RenderTargetCount; ++i) {
        auto& target = pass->pass.targets[i];

        // Уничтожьте старую цель, если она существует.
        renderer->RenderTargetDestroy(target, false);

        // Получение указателей текстур для всех прикрепленных объектов и кадров.
        for (u32 a = 0; a < target.AttachmentCount; ++a) {
            auto& attachment = target.attachments[a];
            if (attachment.source == RenderTargetAttachmentSource::Default) {
                if (attachment.type == RenderTargetAttachmentType::Colour) {
                    attachment.texture = renderer->GetWindowAttachment(i);
                } else if (attachment.type == RenderTargetAttachmentType::Depth) {
                    attachment.texture = renderer->GetDepthAttachment(i);
                } else {
                    MERROR("Неподдерживаемый тип вложения: 0x%x", attachment.type);
                    return false;
                }
            } else if (attachment.source == RenderTargetAttachmentSource::Self) {
                // При необходимости/при поддержке данной функции выполните повторную генерацию.
                if (pass->AttachmentTexturesRegenerate) {
                    if (!pass->AttachmentTexturesRegenerate(pass, width, height)) {
                        MERROR("Не удалось перегенерировать текстуры прикрепления для прохода рендеринга '%s'.", pass->name.c_str());
                    }
                }
                if (!pass->AttachmentPopulate) {
                    MERROR("Была предпринята попытка получить собственную текстуру прикрепления для прохода, который не реализует функцию AttachmentPopulate.");
                    return false;
                }
                if (!pass->AttachmentPopulate(pass, attachment)) {
                    MERROR("Неподдерживаемый тип вложения: 0x%x", attachment.type);
                    return false;
                }
            }
        }

        bool UseCustomSize = target.attachments[0].source == RenderTargetAttachmentSource::Self;

        // Создайте базовую цель рендеринга.
        renderer->RenderTargetCreate(target.AttachmentCount, target.attachments, &pass->pass, 
            UseCustomSize ? target.attachments[0].texture->width : width,
            UseCustomSize ? target.attachments[0].texture->height : height,
            0, target);
    }

    return true;
}
