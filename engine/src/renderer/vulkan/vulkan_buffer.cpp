#include "vulkan_buffer.hpp"

#include "vulkan_api.hpp"
#include "vulkan_device.hpp"
#include "vulkan_command_buffer.hpp"

VulkanBuffer::VulkanBuffer()
    : 
    TotalSize(), 
    handle(), 
    usage(), 
    IsLocked(false), 
    memory(), 
    MemoryIndex(), 
    MemoryPropertyFlags(), 
    FreeListMemoryRequirement(), 
    FreeListBlock(nullptr), 
    BufferFreeList(),
    HasFreelist(false) {}

VulkanBuffer::VulkanBuffer(VulkanAPI *VkAPI, u64 size, VkBufferUsageFlagBits usage, u32 MemoryPropertyFlags, bool BindOnCreate, bool UseFreelist)
    :   
    TotalSize(size), 
    handle(), 
    usage(usage), 
    IsLocked(false), 
    memory(), 
    MemoryIndex(), 
    MemoryPropertyFlags(MemoryPropertyFlags), 
    FreeListMemoryRequirement(FreeList::GetMemoryRequirement(size)), 
    FreeListBlock(UseFreelist ? MMemory::Allocate(FreeListMemoryRequirement, MemoryTag::Renderer) : nullptr), 
    BufferFreeList(),
    HasFreelist(UseFreelist)
{

    // Создание свободного списка, если нужно
    if (UseFreelist) {
        BufferFreeList.Create(size, FreeListBlock);
    }

    VkBufferCreateInfo BufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    BufferInfo.size = size;
    BufferInfo.usage = usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // ПРИМЕЧАНИЕ: используется только в одной очереди.

    VK_CHECK(vkCreateBuffer(VkAPI->Device.LogicalDevice, &BufferInfo, VkAPI->allocator, &this->handle));

    // Сбор требований к памяти.
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(VkAPI->Device.LogicalDevice, this->handle, &requirements);
    this->MemoryIndex = VkAPI->FindMemoryIndex(requirements.memoryTypeBits, this->MemoryPropertyFlags);
    if (this->MemoryIndex == -1) {
        MERROR("Не удалось создать буфер vulkan, поскольку не был найден индекс требуемого типа памяти.");
        // Обязательно уничтожьте свободный список.
        CleanupFreelist();
        return;
    }

    // Выделить информацию о памяти
    VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    AllocateInfo.allocationSize = requirements.size;
    AllocateInfo.memoryTypeIndex = this->MemoryIndex;

    // Выделите память.
    VkResult result = vkAllocateMemory(
        VkAPI->Device.LogicalDevice,
        &AllocateInfo,
        VkAPI->allocator,
        &this->memory);

    if (result != VK_SUCCESS) {
        MERROR("Не удалось создать буфер vulkan из-за сбоя выделения требуемой памяти. Ошибка: %i", result);
        // Обязательно уничтожьте свободный список.
        CleanupFreelist();
        return;
    }

    if (BindOnCreate) {
        Bind(VkAPI, 0);
    }

    return;
}

VulkanBuffer::~VulkanBuffer()
{
    if (FreeListBlock) {
        // Обязательно уничтожьте свободный список.
        CleanupFreelist();
    }
    TotalSize = 0;
    usage = {};
    IsLocked = false;
}

bool VulkanBuffer::Create(
    VulkanAPI *VkAPI,
    u64 size,
    VkBufferUsageFlagBits usage,
    u32 MemoryPropertyFlags,
    bool BindOnCreate,
    bool UseFreelist)
{
    this->TotalSize = size;
    this->usage = usage;
    this->MemoryPropertyFlags = MemoryPropertyFlags;
    this->HasFreelist = UseFreelist;

    // Создание свободного списка, если нужно
    if (UseFreelist) {
        BufferFreeList.GetMemoryRequirement(size, FreeListMemoryRequirement);
        FreeListBlock = MMemory::Allocate(FreeListMemoryRequirement, MemoryTag::Renderer);
        BufferFreeList.Create(size, FreeListBlock);
    }

    VkBufferCreateInfo BufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    BufferInfo.size = size;
    BufferInfo.usage = usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // ПРИМЕЧАНИЕ: используется только в одной очереди.

    VK_CHECK(vkCreateBuffer(VkAPI->Device.LogicalDevice, &BufferInfo, VkAPI->allocator, &this->handle));

    // Сбор требований к памяти.
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(VkAPI->Device.LogicalDevice, this->handle, &requirements);
    this->MemoryIndex = VkAPI->FindMemoryIndex(requirements.memoryTypeBits, this->MemoryPropertyFlags);
    if (this->MemoryIndex == -1) {
        MERROR("Не удалось создать буфер vulkan, поскольку не был найден индекс требуемого типа памяти.");
        // Обязательно уничтожьте свободный список.
        CleanupFreelist();
        return false;
    }

    // Выделить информацию о памяти
    VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    AllocateInfo.allocationSize = requirements.size;
    AllocateInfo.memoryTypeIndex = this->MemoryIndex;

    // Выделите память.
    VkResult result = vkAllocateMemory(
        VkAPI->Device.LogicalDevice,
        &AllocateInfo,
        VkAPI->allocator,
        &this->memory);

    if (result != VK_SUCCESS) {
        MERROR("Не удалось создать буфер vulkan из-за сбоя выделения требуемой памяти. Ошибка: %i", result);
        // Обязательно уничтожьте свободный список.
        CleanupFreelist();
        return false;
    }

    if (BindOnCreate) {
        Bind(VkAPI, 0);
    }

    return true;
}

void VulkanBuffer::Destroy(VulkanAPI *VkAPI)
{
    this->~VulkanBuffer();
    if (memory) {
        vkFreeMemory(VkAPI->Device.LogicalDevice, memory, VkAPI->allocator);
        memory = 0;
    }
    if (handle) {
        vkDestroyBuffer(VkAPI->Device.LogicalDevice, handle, VkAPI->allocator);
        handle = 0;
    }
}

bool VulkanBuffer::Resize(VulkanAPI *VkAPI, u64 NewSize, VkQueue queue, VkCommandPool pool)
{
    // Sanity check.
    if (NewSize < TotalSize) {
        MERROR("VulkanBuffer::Resize требует, чтобы новый размер был больше старого. Невыполнение этого требования может привести к потере данных.");
        return false;
    }

    // Сначала измените размер свободного списка, если он используется.
    if (HasFreelist) {
        u64 NewMemoryRequirement = 0;
        BufferFreeList.GetMemoryRequirement(NewSize, NewMemoryRequirement);
        void* NewBlock = MMemory::Allocate(NewMemoryRequirement, MemoryTag::Renderer);
        void* OldBlock = nullptr;
        if (!BufferFreeList.Resize(NewBlock, NewSize, &OldBlock)) {
            MERROR("VulkanBuffer::Resize не удалось изменить размер внутреннего списка свободных мест.");
            MMemory::Free(NewBlock, NewMemoryRequirement, MemoryTag::Renderer);
            return false;
        }
        // Очистите старую память, затем назначьте новые свойства.
        MMemory::Free(OldBlock, FreeListMemoryRequirement, MemoryTag::Renderer);
        FreeListMemoryRequirement = NewMemoryRequirement;
        FreeListBlock = NewBlock;
        TotalSize = NewSize;
    }
    
    // Создайте новый буфер.
    VkBufferCreateInfo BufferInfo = {VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO};
    BufferInfo.size = NewSize;
    BufferInfo.usage = this->usage;
    BufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;  // ПРИМЕЧАНИЕ: используется только в одной очереди.

    VkBuffer NewBuffer;
    VK_CHECK(vkCreateBuffer(VkAPI->Device.LogicalDevice, &BufferInfo, VkAPI->allocator, &NewBuffer));

    // Сбор требований к памяти.
    VkMemoryRequirements requirements;
    vkGetBufferMemoryRequirements(VkAPI->Device.LogicalDevice, NewBuffer, &requirements);

    // Распределить информацию о памяти
    VkMemoryAllocateInfo AllocateInfo = {VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO};
    AllocateInfo.allocationSize = requirements.size;
    AllocateInfo.memoryTypeIndex = (u32)this->MemoryIndex;

    // Выделение памяти.
    VkDeviceMemory NewMemory;
    VkResult result = vkAllocateMemory(VkAPI->Device.LogicalDevice, &AllocateInfo, VkAPI->allocator, &NewMemory);
    if (result != VK_SUCCESS) {
        MERROR("Не удалось изменить размер буфера vulkan из-за сбоя выделения требуемой памяти. Ошибка: %i", result);
        return false;
    }

    // Привязать память нового буфера
    VK_CHECK(vkBindBufferMemory(VkAPI->Device.LogicalDevice, NewBuffer, NewMemory, 0));

    // Скопируйте данные
    CopyTo(VkAPI, pool, 0, queue, 0, NewBuffer, 0, this->TotalSize);

    // Убедитесь, что все, что потенциально может их использовать, завершено.
    vkDeviceWaitIdle(VkAPI->Device.LogicalDevice);

    // Уничтожьте старое
    if (this->memory) {
        vkFreeMemory(VkAPI->Device.LogicalDevice, this->memory, VkAPI->allocator);
        this->memory = 0;
    }
    if (this->handle) {
        vkDestroyBuffer(VkAPI->Device.LogicalDevice, this->handle, VkAPI->allocator);
        this->handle = 0;
    }

    // Установить новые свойства
    this->TotalSize = NewSize;
    this->memory = NewMemory;
    this->handle = NewBuffer;

    return true;
}

void VulkanBuffer::Bind(VulkanAPI *VkAPI, u64 offset)
{
    VK_CHECK(vkBindBufferMemory(VkAPI->Device.LogicalDevice, this->handle, this->memory, offset));
}

void *VulkanBuffer::LockMemory(VulkanAPI *VkAPI, u64 offset, u64 size, u32 flags)
{
    void* data;
    VK_CHECK(vkMapMemory(VkAPI->Device.LogicalDevice, this->memory, offset, size, flags, &data));
    return data;
}

void VulkanBuffer::UnlockMemory(VulkanAPI *VkAPI)
{
    vkUnmapMemory(VkAPI->Device.LogicalDevice, this->memory);
}

bool VulkanBuffer::Allocate(u64 size, u64 &OutOffset)
{
    if (!size) {
        MERROR("VulkanBuffer::Allocate требуется ненулевой размер.");
        return false;
    }
    if (!HasFreelist) {
        MWARN("VulkanBuffer::Allocate вызывается для буфера, не использующего свободные списки. Смещение не будет действительным. Вместо этого вызовите VulkanBuffer::LoadData.");
        OutOffset = 0;
        return true;
    }

    return BufferFreeList.AllocateBlock(size, OutOffset);
}

bool VulkanBuffer::Free(u64 size, u64 offset)
{
    if (!size) {
        MERROR("VulkanBuffer::Free требуется ненулевой размер.");
        return false;
    }
    if (!HasFreelist) {
        MWARN("VulkanBuffer::Free вызывается в буфере, не использующем свободные списки. Ничего не было сделано.");
        return true;
    }

    return BufferFreeList.FreeBlock(size, offset);
}

void VulkanBuffer::LoadData(VulkanAPI *VkAPI, u64 offset, u64 size, u32 flags, const void *data)
{
    void* ptrData;
    VK_CHECK(vkMapMemory(VkAPI->Device.LogicalDevice, this->memory, offset, size, flags, &ptrData));
    MMemory::CopyMem(ptrData, data, size);
    vkUnmapMemory(VkAPI->Device.LogicalDevice, this->memory);
}

void VulkanBuffer::CopyTo(
    VulkanAPI *VkAPI, 
    VkCommandPool pool, 
    VkFence fence, 
    VkQueue queue, 
    u64 SourceOffset, 
    VkBuffer dest, 
    u64 DestOffset, 
    u64 size)
{
    // Создайте одноразовый командный буфер.
    VulkanCommandBuffer TempCommandBuffer;
    VulkanCommandBufferAllocateAndBeginSingleUse(VkAPI, pool, &TempCommandBuffer);

    // Подготовьте команду копирования и добавьте ее в буфер команд.
    VkBufferCopy CopyRegion;
    CopyRegion.srcOffset = SourceOffset;
    CopyRegion.dstOffset = DestOffset;
    CopyRegion.size = size;

    vkCmdCopyBuffer(TempCommandBuffer.handle, handle, dest, 1, &CopyRegion);

    // Отправьте буфер на выполнение и дождитесь его завершения.
    VulkanCommandBufferEndSingleUse(VkAPI, pool, &TempCommandBuffer, queue);
}

void VulkanBuffer::CleanupFreelist()
{
    if(HasFreelist)
    {
        BufferFreeList.~FreeList();
        MMemory::Free(FreeListBlock, FreeListMemoryRequirement, MemoryTag::Renderer);
        FreeListMemoryRequirement = 0;
        FreeListBlock = nullptr;
        }
}
