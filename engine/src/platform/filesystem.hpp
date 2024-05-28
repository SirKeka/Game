#pragma once

#include "defines.hpp"

//TODO: переписать

// Содержит дескриптор файла.
struct FileHandle {
    // Непрозрачный дескриптор для внутреннего дескриптора файла.
    void* handle;
    bool IsValid;
};

enum class FileModes {
    Read = 0x1,
    Write = 0x2
};

MINLINE constexpr i32
  operator&(FileModes x, FileModes y)
  {
    return /*static_cast<FileModes>*/
      (static_cast<i32>(x) & static_cast<i32>(y));
  }

namespace Filesystem 
{
    /// Проверяет, существует ли файл с заданным путем.
    /// param path путь к файлу, который необходимо проверить.
    /// returns true, если существует; в противном случае false.
    MAPI bool Exists(const char* path);

    /// Попытайтесь открыть файл, расположенный по адресу path.
    /// @param path путь к файлу, который нужно открыть.
    /// @param mode Флаги режима для файла при открытии (чтение/запись). Смотрите перечисление FileModes в filesystem.h.
    /// @param binary указывает, следует ли открывать файл в двоичном режиме.
    /// @param OutHandle указатель на структуру FileHandle, которая содержит информацию о дескрипторе.
    /// @returns true, если открыто успешно; в противном случае false.   
    MAPI bool Open(const char* path, FileModes mode, bool binary, FileHandle* OutHandle);

    /// Закрывает предоставленный дескриптор файла.
    /// @param handle указатель на структуру FileHandle, которая содержит дескриптор, подлежащий закрытию.
    MAPI void Close(FileHandle* handle);

    /// @brief Пытается прочитать размер файла, к которому прикреплен дескриптор.
    /// @param handle Дескриптор файла.
    /// @param OutSize Указатель на размер файла.
    /// @return 
    MAPI bool Size(FileHandle* handle, u64& OutSize);

    /// Считывает до новой строки или EOF. Выделяет *LineBuf, который должен быть освобожден вызывающей стороной.
    /// @param handle указатель на структуру FileHandle.
    /// @param LineBuf указатель на массив символов, который будет выделен и заполнен этим методом.
    /// @returns true в случае успеха; в противном случае false.
    MAPI bool ReadLine(FileHandle* handle, u64 MaxLength, char** LineBuf, u64& OutLineLength);
 
    /// Записывает текст в предоставленный файл, добавляя после этого '\n'.
    /// @param handle указатель на структуру FileHandle.
    /// @param text текст, который должен быть написан.
    /// @returns true в случае успеха; в противном случае false.
    MAPI bool WriteLine(FileHandle* handle, const char* text);

    /// Считывает данные размером до DataSize в OutBytesRead. 
    /// Выделяет *OutData, который должен быть освобожден вызывающей стороной.
    /// @param handle указатель на структуру FileHandle.
    /// @param DataSize количество байт для чтения.
    /// @param OutData указатель на блок памяти, который будет заполнен этим методом.
    /// @param OutBytesRead указатель на число, которое будет заполнено количеством байт, фактически считанных из файла.
    /// @returns true в случае успеха; в противном случае false.
    MAPI bool Read(FileHandle* handle, u64 DataSize, void* OutData, u64& OutBytesRead);

    /// Считывает все байты данных в OutBytes. 
    /// @param handle указатель на структуру FileHandle.
    /// @param OutBytes указатель на массив байтов, который будет выделен и заполнен этим методом.
    /// @param OutBytesRead указатель на число, которое будет заполнено количеством байт, фактически считанных из файла.
    /// @returns true в случае успеха; в противном случае false.
    MAPI bool ReadAllBytes(FileHandle* handle, u8* OutBytes, u64& OutBytesRead);
    MAPI bool ReadAllText(FileHandle* handle, char* OutText, u64& OutBytesRead);
 
    /// Записывает предоставленные данные в файл.
    /// @param handle Указатель на структуру FileHandle.
    /// @param DataSize pазмер данных в байтах.
    /// @param data данные, подлежащие записи.
    /// @param OutBytesWritten указатель на число, которое будет заполнено количеством байт, фактически записанных в файл.
    /// @returns true в случае успеха; в противном случае false.
    MAPI bool Write(FileHandle* handle, u64 DataSize, const void* data, u64& OutBytesWritten);
} // namespace Filesystem
