#include "filesystem.hpp"

#include "core/logger.hpp"
#include "core/mmemory.hpp"

#include <iostream>
#include <cstring>
#include <sys/stat.h>

bool Filesystem::Exists(const char *path)
{
#ifdef _MSC_VER
    struct _stat buffer;
    return _stat(path, &buffer);
#else
    struct stat buffer;
    return stat(path, &buffer) == 0;
#endif
}

bool Filesystem::Open(const char *path, FileModes mode, bool binary, FileHandle *OutHandle)
{
    OutHandle->IsValid = false;
    OutHandle->handle = 0;
    MString ModeStr;

    if ((mode & FileModes::Read) != 0 && (mode & FileModes::Write) != 0) {
        ModeStr = binary ? "w+b" : "w+";
    } else if ((mode & FileModes::Read) != 0 && (mode & FileModes::Write) == 0) {
        ModeStr = binary ? "rb" : "r";
    } else if ((mode & FileModes::Read) == 0 && (mode & FileModes::Write) != 0) {
        ModeStr = binary ? "wb" : "w";
    } else {
        MERROR("При попытке открыть файл был пройден недопустимый режим: '%s'", path);
        return false;
    }

    // Попытайтесь открыть файл.
    FILE* file = fopen(path, ModeStr.c_str());
    if (!file) {
        MERROR("Ошибка при открытии файла: '%s'", path);
        return false;
    }

    OutHandle->handle = file;
    OutHandle->IsValid = true;

    return true;
}

void Filesystem::Close(FileHandle *handle)
{
    if (handle->handle) {
        fclose(reinterpret_cast<FILE*>(handle->handle));
        handle->handle = 0;
        handle->IsValid = false;
    }
}

bool Filesystem::Size(FileHandle *handle, u64 &OutSize)
{
    if (handle->handle) {
        fseek(reinterpret_cast<FILE*>(handle->handle), 0, SEEK_END);
        OutSize = ftell(reinterpret_cast<FILE*>(handle->handle));
        rewind(reinterpret_cast<FILE*>(handle->handle));
        return true;
    }
    return false;
}

bool Filesystem::ReadLine(FileHandle *handle, u64 MaxLength, char** LineBuf, u64& OutLineLength)
{
    if (handle->handle && LineBuf && MaxLength > 0) {
        char* buf = *LineBuf;
        if (fgets(buf, MaxLength, reinterpret_cast<FILE*>(handle->handle)) != 0) {
            OutLineLength = MString::Lenght(*LineBuf);
            return true;
        }
    }
    return false;
}

bool Filesystem::WriteLine(FileHandle *handle, const char *text)
{
     if (handle->handle) {
        i32 result = fputs(text, reinterpret_cast<FILE*>(handle->handle));
        if (result != EOF) {
            result = fputc('\n', reinterpret_cast<FILE*>(handle->handle));
        }

        // Обязательно очистите поток, чтобы он был немедленно записан в файл.
        // Это предотвращает потерю данных в случае сбоя.
        fflush(reinterpret_cast<FILE*>(handle->handle));
        return result != EOF;
    }
    return false;
}

bool Filesystem::Read(FileHandle *handle, u64 DataSize, void *OutData, u64 &OutBytesRead)
{
    if (handle->handle && OutData) {
        OutBytesRead = fread(OutData, 1, DataSize, reinterpret_cast<FILE*>(handle->handle));
        if (OutBytesRead != DataSize) {
            return false;
        }
        return true;
    }
    return false;
}

bool Filesystem::ReadAllBytes(FileHandle *handle, u8 *OutBytes, u64 &OutBytesRead)
{
    if (handle->handle && OutBytes && OutBytesRead) {
        // Размер файла
        u64 size = 0;
        if(!Filesystem::Size(handle, size)) {
            return false;
        }

        OutBytesRead = fread(OutBytes, 1, size, reinterpret_cast<FILE*>(handle->handle));
        return OutBytesRead == size;
    }
    return false;
}

bool Filesystem::ReadAllText(FileHandle *handle, char *OutText, u64 &OutBytesRead)
{
    if (handle->handle && OutText && OutBytesRead) {
        // Размер файла
        u64 size = 0;
        if(!Filesystem::Size(handle, size)) {
            return false;
        }
        OutBytesRead = fread(OutText, 1, size, reinterpret_cast<FILE*>(handle->handle));
        return OutBytesRead == size;
    }
    return false;
}

bool Filesystem::Write(FileHandle *handle, u64 DataSize, const void *data, u64 &OutBytesWritten)
{
    if (handle->handle) {
        OutBytesWritten = fwrite(data, 1, DataSize, reinterpret_cast<FILE*>(handle->handle));
        if (OutBytesWritten != DataSize) {
            return false;
        }
        fflush(reinterpret_cast<FILE*>(handle->handle));
        return true;
    }
    return false;
}
