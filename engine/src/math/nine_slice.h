#pragma once
#include "vector2d_fwd.h"

struct MAPI NineSlice
{
    // Фактический угол ш/в
    Point CornerSize;
    // Выбранный угол ш/в
    Point CornerPxSize;
    
    // Общая ш/в 9-срезового образца.
    Point size;
    
    Point AtlasPxMin;
    Point AtlasPxMax;
    
    Point AtlasPxSize;
    struct Geometry* geometry{};

    bool IsDirty;

    constexpr NineSlice() = default;
    NineSlice(const char *name, Point size, Point AtlasPxSize, Point AtlasPxMin, Point AtlasPxMax, Point CornerPxSize, Point CornerSize);

    void Generate(const char* name);
    void Generate(const char* name, NineSlice& nslice);

    bool Update(struct Vertex2D* vertices = nullptr);
    void RenderFramePrepare(const struct FrameData& rFrameData);

    // void* operator new(u64 size) { return MemorySystem::Allocate(size, Memory::UI); }
    // void operator delete(void* ptr, u64 size) { MemorySystem::Free(ptr, size, Memory::UI); }
};

struct NineSlicePosTc {
    f32 TxMin, TyMin, TxMax, TyMax;
    f32 PosxMin, PosyMin, PosxMax, PosyMax;
};