#pragma once

#include <d3d11.h>
#include <cuda.h>
#include <cuda_d3d11_interop.h>

typedef struct {
    struct cudaGraphicsResource* resource;
    int dev;
} CudaState;

CudaState new_cuda_state(IDXGIAdapter* adapter);
void cuda_init();
void set_graphics_resource(CudaState &s, ID3D11Resource* resource);