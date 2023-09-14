#include "shared.hpp"
#include <cuda.h>
#include <cuda_d3d11_interop.h>
#include <cudaD3D11.h>
#include <d3d11.h>
#include <stdio.h>

void cuda_init() {
    return;
}

CudaState new_cuda_state(IDXGIAdapter* adapter) {

    CudaState s{0};
    cudaD3D11GetDevice(&s.dev, adapter);

    cudaDeviceProp prop;
    cudaGetDeviceProperties(&prop, s.dev);
    printf("CUDA Device: %s\n", prop.name);
    return s;
}

void set_graphics_resource(CudaState &s, ID3D11Resource* resource) {
    auto e = cudaGraphicsD3D11RegisterResource(&s.resource, resource, cudaGraphicsRegisterFlagsNone);
    printf("CUDA Buffer Access: %s\n", cudaGetErrorName(e));
}