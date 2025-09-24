#include <entry.h>

#include "platform/platform.hpp"

using PFN_PluginCreate = RendererPlugin*(*)();
// using PFN_CreateGame = void(*)(ApplicationConfig&);

bool LoadGameLib(Application& app) 
{
    // Динамическая загрузка бмблиотеки игры
    if (!PlatformDynamicLibraryLoad("testbed_lib_loaded", app.GameLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("ApplicationBoot", app.GameLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("ApplicationInitialize", app.GameLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("ApplicationUpdate", app.GameLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("ApplicationPrepareRenderPacket", app.GameLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("ApplicationRender", app.GameLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("ApplicationOnResize", app.GameLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("ApplicationShutdown", app.GameLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("ApplicationLibOnLoad", app.GameLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("ApplicationLibOnUnload", app.GameLibrary)) {
        return false;
    }

    // назначить указатели функций
    app.Boot = (bool(*)(Application&))app.GameLibrary.functions[0].pfn;
    app.Initialize = (bool(*)(Application&))app.GameLibrary.functions[1].pfn;
    app.Update = (bool(*)(Application&, const FrameData&))app.GameLibrary.functions[2].pfn;
    app.PrepareRenderPacket = (bool(*)(Application*, RenderPacket&, FrameData&))app.GameLibrary.functions[3].pfn;
    app.Render = (bool(*)(Application&, RenderPacket&, FrameData&))app.GameLibrary.functions[4].pfn;
    app.OnResize = (void(*)(Application&, u32, u32))app.GameLibrary.functions[5].pfn;
    app.Shutdown = (void(*)(Application&))app.GameLibrary.functions[6].pfn;
    app.LibOnLoad = (void(*)(Application&))app.GameLibrary.functions[7].pfn;
    app.LibOnUnload = (void(*)(Application&))app.GameLibrary.functions[8].pfn;

    // Вызовите onload.
    app.LibOnLoad(app);

    return true;
}

bool WatchedFileUpdated(u16 code, void* sender, void* ListenerInst, EventContext context)
{
    if (code == EventSystem::WatchedFileWritten) {
        auto app = reinterpret_cast<Application*>(ListenerInst);
        if (context.data.u32[0] == app->GameLibrary.WatchID) {
            MINFO("Горячая перезагрузка библиотеки игры.");
        }

        // Сообщите приложению, что оно будет выгружено.
        app->LibOnUnload(*app);

        // Фактически выгрузите библиотеку приложения.
        if (!PlatformDynamicLibraryUnload(app->GameLibrary)) {
            MERROR("Не удалось выгрузить библиотеку игры.");
            return false;
        }

        // Подождите немного, прежде чем пытаться скопировать файл.
        PlatformSleep(100);

        const char* prefix = PlatformDynamicLibraryPrefix();
        const char* extension = PlatformDynamicLibraryExtension();
        char SourceFile[260]{};
        char TargetFile[260]{};
        MString::Format(SourceFile, "%stestbed_lib%s", prefix, extension);
        MString::Format(TargetFile, "%stestbed_lib_loaded%s", prefix, extension);

        PlatformError::Code ErrorCode = PlatformError::FileLocked;
        while (ErrorCode == PlatformError::FileLocked) {
            ErrorCode = PlatformCopyFile(SourceFile, TargetFile, true);
            if (ErrorCode == PlatformError::FileLocked) {
                PlatformSleep(100);
            }
        }
        if (ErrorCode != PlatformError::Success) {
            MERROR("Копирование файла не удалось!");
            return false;
        }

        if (!LoadGameLib(*app)) {
            MERROR("Перезагрузка библиотеки игры не удалась.");
            return false;
        }
    }
    return false;
}

bool CreateApplication(Application& OutApplication)
{
    OutApplication.AppConfig.StartPosX = 100;
    OutApplication.AppConfig.StartPosY = 100;
    OutApplication.AppConfig.StartWidth = 1280;
    OutApplication.AppConfig.StartHeight = 720;
    OutApplication.AppConfig.name = "Moon Engine";
    // OutApplication.AppConfig.FontConfig = 
    // OutApplication.AppConfig.RenderViews = 
    OutApplication.AppConfig.FrameAllocatorSize = MEBIBYTES(64);

    PlatformError::Code ErrCode = PlatformError::FileLocked;
    while (ErrCode == PlatformError::FileLocked) {
        const char* prefix = PlatformDynamicLibraryPrefix();
        const char* extension = PlatformDynamicLibraryExtension();
        char SourceFile[260]{};
        char TargetFile[260]{};
        MString::Format(SourceFile, "%stestbed_lib%s", prefix, extension);
        MString::Format(TargetFile, "%stestbed_lib_loaded%s", prefix, extension);

        ErrCode = PlatformCopyFile(SourceFile, TargetFile, true);
        if (ErrCode == PlatformError::FileLocked) {
            PlatformSleep(100);
        }
    }

    if (ErrCode != PlatformError::Success) {
        MERROR("Неудалось копировать фаил!");
        return false;
    }

    if(!LoadGameLib(OutApplication)) {
        MERROR("Загрузка библиотеки игры не удалась!");
        return false;
    }

    // OutApplication.engine = nullptr;
    // OutApplication.state = nullptr;

    if (!PlatformDynamicLibraryLoad("vulkan_renderer", OutApplication.RendererLibrary)) {
        return false;
    }

    if (!PlatformDynamicLibraryLoadFunction("PluginCreate", OutApplication.RendererLibrary)) {
        return false;
    } 
    auto PlaginCreate = reinterpret_cast<PFN_PluginCreate>(OutApplication.RendererLibrary.functions[0].pfn);
    OutApplication.AppConfig.plugin = PlaginCreate(); 

    return true;
}

bool InitializeApplication(Application &app)
{
    if (!EventSystem::Register(EventSystem::Code::WatchedFileWritten, &app, WatchedFileUpdated)) {
        return false;
    }

    const char* prefix = PlatformDynamicLibraryPrefix();
    const char* extension = PlatformDynamicLibraryExtension();
    char path[260]{};
    MString::Format(path, "%s%s%s", prefix, "testbed_lib", extension);

    if (!PlatformWatchFile(path, app.GameLibrary.WatchID)) {
        MERROR("Не удалось просмотреть библиотеку тестового стенда!");
        return false;
    }

    return true;
}