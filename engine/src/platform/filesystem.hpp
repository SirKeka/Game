#pragma once

#include "defines.hpp"

// Содержит дескриптор файла.
struct FileHandle {
    // Непрозрачный дескриптор для внутреннего дескриптора файла.
    void* handle;
    bool IsValid;
};

enum FileModes {
    FILE_MODE_READ = 0x1,
    FILE_MODE_WRITE = 0x2
};

class MAPI Filesystem
{
private:
    /* data */
public:
    Filesystem(/* args */);
    ~Filesystem();

    /**
 * Проверяет, существует ли файл с заданным путем.
 * @param path путь к файлу, который необходимо проверить.
 * @returns true, если существует; в противном случае false.
 */
bool FilesystemExists(const char* path);

/** 
 * Попытайтесь открыть файл, расположенный по адресу path.
 * @param path путь к файлу, который нужно открыть.
 * @param mode Флаги режима для файла при открытии (чтение/запись). Смотрите перечисление FileModes в filesystem.h.
 * @param binary указывает, следует ли открывать файл в двоичном режиме.
 * @param OutHandle указатель на структуру FileHandle, которая содержит информацию о дескрипторе.
 * @returns true, если открыто успешно; в противном случае false.
 */
bool FilesystemOpen(const char* path, FileModes mode, bool binary, FileHandle* OutHandle);

/** 
 * Закрывает предоставленный дескриптор файла.
 * @param handle указатель на структуру FileHandle, которая содержит дескриптор, подлежащий закрытию.
 */
void FilesystemClose(FileHandle* handle);

/** 
 * Считывает до новой строки или EOF. Выделяет *LineBuf, который должен быть освобожден вызывающей стороной.
 * @param handle указатель на структуру FileHandle.
 * @param LineBuf указатель на массив символов, который будет выделен и заполнен этим методом.
 * @returns true в случае успеха; в противном случае false.
 */
bool FilesystemReadLine(FileHandle* handle, char** LineBuf);

/** 
 * Записывает текст в предоставленный файл, добавляя после этого '\n'.
 * @param handle указатель на структуру FileHandle.
 * @param text текст, который должен быть написан.
 * @returns true в случае успеха; в противном случае false.
 */
bool FilesystemWriteLine(FileHandle* handle, const char* text);

/** 
 * Считывает данные размером до DataSize в OutBytesRead. 
 * Выделяет *OutData, который должен быть освобожден вызывающей стороной.
 * @param handle указатель на структуру FileHandle.
 * @param DataSize количество байт для чтения.
 * @param OutData указатель на блок памяти, который будет заполнен этим методом.
 * @param OutBytesRead указатель на число, которое будет заполнено количеством байт, фактически считанных из файла.
 * @returns true в случае успеха; в противном случае false.
 */
bool FilesystemRead(FileHandle* handle, u64 DataSize, void* OutData, u64* OutBytesRead);

/** 
 * Reads up to data_size bytes of data into out_bytes_read. 
 * Allocates *out_bytes, which must be freed by the caller.
 * @param handle Указатель на структуру FileHandle.
 * @param out_bytes A pointer to a byte array which will be allocated and populated by this method.
 * @param out_bytes_read A pointer to a number which will be populated with the number of bytes actually read from the file.
 * @returns True if successful; otherwise false.
 */
bool filesystem_read_all_bytes(FileHandle* handle, u8** out_bytes, u64* out_bytes_read);

/** 
 * Writes provided data to the file.
 * @param handle Указатель на структуру FileHandle.
 * @param data_size The size of the data in bytes.
 * @param data The data to be written.
 * @param out_bytes_written A pointer to a number which will be populated with the number of bytes actually written to the file.
 * @returns True if successful; otherwise false.
 */
bool filesystem_write(FileHandle* handle, u64 data_size, const void* data, u64* out_bytes_written);
};
