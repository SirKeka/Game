#include "vulkan_utils.hpp"

const char *VulkanResultString(VkResult result, bool GetExtended)
{
    // Из: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
    // Коды успеха
    switch (result) {
        default:
        case VK_SUCCESS:
            return !GetExtended ? "VK_SUCCESS" : "VK_SUCCESS Команда успешно выполнена";
        case VK_NOT_READY:
            return !GetExtended ? "VK_NOT_READY" : "VK_NOT_READY Забор или запрос еще не завершенd";
        case VK_TIMEOUT:
            return !GetExtended ? "VK_TIMEOUT" : "VK_TIMEOUT Операция ожидания не была завершена в указанное время";
        case VK_EVENT_SET:
            return !GetExtended ? "VK_EVENT_SET" : "VK_EVENT_SET Сигнализируется о событии";
        case VK_EVENT_RESET:
            return !GetExtended ? "VK_EVENT_RESET" : "VK_EVENT_RESET Событие не просигнализировано";
        case VK_INCOMPLETE:
            return !GetExtended ? "VK_INCOMPLETE" : "VK_INCOMPLETE Возвращаемый массив был слишком мал для результата";
        case VK_SUBOPTIMAL_KHR:
            return !GetExtended ? "VK_SUBOPTIMAL_KHR" : "VK_SUBOPTIMAL_KHR Цепочка подкачки больше не точно соответствует свойствам поверхности, но все еще может быть использована для успешного представления на поверхности.";
        case VK_THREAD_IDLE_KHR:
            return !GetExtended ? "VK_THREAD_IDLE_KHR" : "VK_THREAD_IDLE_KHR Отложенная операция не завершена, но в настоящее время этот поток не выполняет никакой работы во время этого вызова.";
        case VK_THREAD_DONE_KHR:
            return !GetExtended ? "VK_THREAD_DONE_KHR" : "VK_THREAD_DONE_KHR Отложенная операция не завершена, но не осталось никакой работы, которую можно было бы назначить дополнительным потокам.";
        case VK_OPERATION_DEFERRED_KHR:
            return !GetExtended ? "VK_OPERATION_DEFERRED_KHR" : "VK_OPERATION_DEFERRED_KHR Была запрошена отложенная операция, и по крайней мере часть работы была отложена.";
        case VK_OPERATION_NOT_DEFERRED_KHR:
            return !GetExtended ? "VK_OPERATION_NOT_DEFERRED_KHR" : "VK_OPERATION_NOT_DEFERRED_KHR Была запрошена отложенная операция, и никакие операции не были отложены.";
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
            return !GetExtended ? "VK_PIPELINE_COMPILE_REQUIRED_EXT" : "VK_PIPELINE_COMPILE_REQUIRED_EXT Запрошенное создание конвейера потребовало бы компиляции, но приложение запросило, чтобы компиляция не выполнялась.";

        // Коды ошибок
        case VK_ERROR_OUT_OF_HOST_MEMORY:
            return !GetExtended ? "VK_ERROR_OUT_OF_HOST_MEMORY" : "VK_ERROR_OUT_OF_HOST_MEMORY Произошел сбой при выделении памяти хоста.";
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
            return !GetExtended ? "VK_ERROR_OUT_OF_DEVICE_MEMORY" : "VK_ERROR_OUT_OF_DEVICE_MEMORY Произошла ошибка выделения памяти устройства.";
        case VK_ERROR_INITIALIZATION_FAILED:
            return !GetExtended ? "VK_ERROR_INITIALIZATION_FAILED" : "VK_ERROR_INITIALIZATION_FAILED Инициализация объекта не может быть завершена по причинам, зависящим от конкретной реализации.";
        case VK_ERROR_DEVICE_LOST:
            return !GetExtended ? "VK_ERROR_DEVICE_LOST" : "VK_ERROR_DEVICE_LOST Логическое или физическое устройство было потеряно. Смотрите раздел Потерянное устройство";
        case VK_ERROR_MEMORY_MAP_FAILED:
            return !GetExtended ? "VK_ERROR_MEMORY_MAP_FAILED" : "VK_ERROR_MEMORY_MAP_FAILED Не удалось выполнить сопоставление объекта памяти.";
        case VK_ERROR_LAYER_NOT_PRESENT:
            return !GetExtended ? "VK_ERROR_LAYER_NOT_PRESENT" : "VK_ERROR_LAYER_NOT_PRESENT Запрошенный слой отсутствует или не может быть загружен.";
        case VK_ERROR_EXTENSION_NOT_PRESENT:
            return !GetExtended ? "VK_ERROR_EXTENSION_NOT_PRESENT" : "VK_ERROR_EXTENSION_NOT_PRESENT Запрошенное расширение не поддерживается.";
        case VK_ERROR_FEATURE_NOT_PRESENT:
            return !GetExtended ? "VK_ERROR_FEATURE_NOT_PRESENT" : "VK_ERROR_FEATURE_NOT_PRESENT Запрошенная функция не поддерживается.";
        case VK_ERROR_INCOMPATIBLE_DRIVER:
            return !GetExtended ? "VK_ERROR_INCOMPATIBLE_DRIVER" : "VK_ERROR_INCOMPATIBLE_DRIVER Запрашиваемая версия Vulkan не поддерживается драйвером или иным образом несовместима по причинам, зависящим от конкретной реализации.";
        case VK_ERROR_TOO_MANY_OBJECTS:
            return !GetExtended ? "VK_ERROR_TOO_MANY_OBJECTS" : "VK_ERROR_TOO_MANY_OBJECTS Слишком много объектов этого типа уже создано.";
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
            return !GetExtended ? "VK_ERROR_FORMAT_NOT_SUPPORTED" : "VK_ERROR_FORMAT_NOT_SUPPORTED Запрашиваемый формат не поддерживается на данном устройстве.";
        case VK_ERROR_FRAGMENTED_POOL:
            return !GetExtended ? "VK_ERROR_FRAGMENTED_POOL" : "VK_ERROR_FRAGMENTED_POOL Не удалось выделить пул из-за фрагментации памяти пула. Это значение должно быть возвращено только в том случае, если не было предпринято попыток выделить память хоста или устройства для размещения нового распределения. Это значение должно быть возвращено в предпочтении к VK_ERROR_OUT_OF_POOL_MEMORY, но только в том случае, если реализация уверена, что сбой выделения пула произошел из-за фрагментации.";
        case VK_ERROR_SURFACE_LOST_KHR:
            return !GetExtended ? "VK_ERROR_SURFACE_LOST_KHR" : "VK_ERROR_SURFACE_LOST_KHR Поверхность больше недоступна.";
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
            return !GetExtended ? "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR" : "VK_ERROR_NATIVE_WINDOW_IN_USE_KHR Запрошенное окно уже используется Vulkan или другим API таким образом, что оно не может быть использовано повторно.";
        case VK_ERROR_OUT_OF_DATE_KHR:
            return !GetExtended ? "VK_ERROR_OUT_OF_DATE_KHR" : "VK_ERROR_OUT_OF_DATE_KHR Поверхность изменилась таким образом, что она больше не совместима с цепочкой обмена, и дальнейшие запросы представления с использованием цепочки обмена не будут выполнены. Приложения должны запросить новые свойства поверхности и воссоздать свою цепочку обмена, если они хотят продолжить представление на поверхность.";
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
            return !GetExtended ? "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR" : "VK_ERROR_INCOMPATIBLE_DISPLAY_KHR Дисплей, используемый цепочкой обмена, не использует один и тот же презентабельный макет изображения или несовместим, что предотвращает совместное использование изображения.";
        case VK_ERROR_INVALID_SHADER_NV:
            return !GetExtended ? "VK_ERROR_INVALID_SHADER_NV" : "VK_ERROR_INVALID_SHADER_NV Не удалось скомпилировать или связать один или несколько шейдеров. Более подробная информация передается обратно в приложение через VK_EXT_debug_report, если он включен.";
        case VK_ERROR_OUT_OF_POOL_MEMORY:
            return !GetExtended ? "VK_ERROR_OUT_OF_POOL_MEMORY" : "VK_ERROR_OUT_OF_POOL_MEMORY Не удалось выделить память пула. Это значение должно быть возвращено только в том случае, если не было предпринято попыток выделить память хоста или устройства для размещения нового распределения. Если сбой определенно произошел из-за фрагментации пула, вместо этого следует вернуть VK_ERROR_FRAGMENTED_POOL.";
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
            return !GetExtended ? "VK_ERROR_INVALID_EXTERNAL_HANDLE" : "VK_ERROR_INVALID_EXTERNAL_HANDLE Внешний дескриптор не является допустимым дескриптором указанного типа.";
        case VK_ERROR_FRAGMENTATION:
            return !GetExtended ? "VK_ERROR_FRAGMENTATION" : "VK_ERROR_FRAGMENTATION Не удалось создать пул дескрипторов из-за фрагментации.";
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
            return !GetExtended ? "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT" : "VK_ERROR_INVALID_DEVICE_ADDRESS_EXT Не удалось создать буфер, поскольку запрошенный адрес недоступен.";
        // NOTE: То же, что и выше
        //case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        //    return !GetExtended ? "VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS" :"VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS Не удалось создать буфер или выделить память, поскольку запрошенный адрес недоступен. Не удалось назначить дескриптор группы шейдеров, поскольку запрошенная информация об дескрипторе группы шейдеров больше недействительна.";
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
            return !GetExtended ? "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT" : "VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT Операция с цепочкой подкачки, созданной с помощью VK_FULL_SCREEN_EXCLUSIVE_APPLICATION_CONTROLLED_EXT, завершилась неудачей, поскольку у нее не было исключительного полноэкранного доступа. Это может произойти по причинам, зависящим от реализации и находящимся вне контроля приложения.";
        case VK_ERROR_UNKNOWN:
            return !GetExtended ? "VK_ERROR_UNKNOWN" : "VK_ERROR_UNKNOWN Произошла неизвестная ошибка; либо приложение предоставило неверные входные данные, либо произошел сбой реализации.";
    }
}

bool VulkanResultIsSuccess(VkResult result)
{
    // Из: https://www.khronos.org/registry/vulkan/specs/1.2-extensions/man/html/VkResult.html
    switch (result) {
            // Коды успеха
        default:
        case VK_SUCCESS:
        case VK_NOT_READY:
        case VK_TIMEOUT:
        case VK_EVENT_SET:
        case VK_EVENT_RESET:
        case VK_INCOMPLETE:
        case VK_SUBOPTIMAL_KHR:
        case VK_THREAD_IDLE_KHR:
        case VK_THREAD_DONE_KHR:
        case VK_OPERATION_DEFERRED_KHR:
        case VK_OPERATION_NOT_DEFERRED_KHR:
        case VK_PIPELINE_COMPILE_REQUIRED_EXT:
            return true;

        // коды ошибок
        case VK_ERROR_OUT_OF_HOST_MEMORY:
        case VK_ERROR_OUT_OF_DEVICE_MEMORY:
        case VK_ERROR_INITIALIZATION_FAILED:
        case VK_ERROR_DEVICE_LOST:
        case VK_ERROR_MEMORY_MAP_FAILED:
        case VK_ERROR_LAYER_NOT_PRESENT:
        case VK_ERROR_EXTENSION_NOT_PRESENT:
        case VK_ERROR_FEATURE_NOT_PRESENT:
        case VK_ERROR_INCOMPATIBLE_DRIVER:
        case VK_ERROR_TOO_MANY_OBJECTS:
        case VK_ERROR_FORMAT_NOT_SUPPORTED:
        case VK_ERROR_FRAGMENTED_POOL:
        case VK_ERROR_SURFACE_LOST_KHR:
        case VK_ERROR_NATIVE_WINDOW_IN_USE_KHR:
        case VK_ERROR_OUT_OF_DATE_KHR:
        case VK_ERROR_INCOMPATIBLE_DISPLAY_KHR:
        case VK_ERROR_INVALID_SHADER_NV:
        case VK_ERROR_OUT_OF_POOL_MEMORY:
        case VK_ERROR_INVALID_EXTERNAL_HANDLE:
        case VK_ERROR_FRAGMENTATION:
        case VK_ERROR_INVALID_DEVICE_ADDRESS_EXT:
        // NOTE: Тоже что и выше
        //case VK_ERROR_INVALID_OPAQUE_CAPTURE_ADDRESS:
        case VK_ERROR_FULL_SCREEN_EXCLUSIVE_MODE_LOST_EXT:
        case VK_ERROR_UNKNOWN:
            return false;
    }
}
