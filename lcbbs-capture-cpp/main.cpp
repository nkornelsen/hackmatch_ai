#include "server.hpp"
#include <windows.h>
#include "pch.h"
#include "main.hpp"
#include "shared.hpp"
#include "mem_helpers.hpp"
#include <TlHelp32.h>
#include <vector>
#include <winternl.h>
#include <Psapi.h>
#include <DbgEng.h>
#include <condition_variable>
#include <mutex>

using namespace winrt::Windows::Graphics::Capture;
using namespace winrt::Windows::Graphics::DirectX::Direct3D11;
using namespace winrt::Windows::Graphics::DirectX;
using namespace Windows::Graphics::DirectX::Direct3D11;
void send_100ms_keystroke_blocking(uint8_t keystroke, HWND hWnd);
uint32_t identify_digits(D3D11_MAPPED_SUBRESOURCE &image);
static BOOL enum_callback(HWND hWnd, LPARAM state) {
    HWND* target_window = (HWND*) state;
    char window_title[50] = {0};

    GetWindowTextA(hWnd, (LPSTR) &window_title, 49);
    
    if (std::string(window_title) == "Last Call BBS") {
        printf("Found window\n");
        *target_window = hWnd;
    }

    return true;
}


int main(int argc, char* argv[]) {
    winrt::init_apartment();
    // cuda_init();

    if (!GraphicsCaptureSession::IsSupported()) {
        printf("Capture not supported\n");
        return -1;
    }

    HWND target_window = 0;

    EnumWindows(enum_callback, (LPARAM) &target_window);

    if (target_window == 0) {
        printf("Window not found\n");
        return -1;
    }

    printf("%d\n", target_window);
    
    // get access to process
    DWORD pid = 0;
    GetWindowThreadProcessId(target_window, &pid);
    if (pid == 0) {
        printf("Failed to get pid\n");
        return -1;
    }

    // MEMORY ACCESS ATTEMPT FOR SCORE TRACKING
    // HANDLE process_handle = OpenProcess(PROCESS_ALL_ACCESS, FALSE, pid);
    // if (process_handle == NULL) {
    //     printf("Failed to open target: %d\n", GetLastError());
    //     return -1;
    // }

    // // open thread 0
    // HANDLE h = CreateToolhelp32Snapshot(TH32CS_SNAPTHREAD, pid);
    // if (h == INVALID_HANDLE_VALUE) {
    //     printf("Snapshot failed: %d\n", GetLastError());
    //     return -1;
    // } else {
    //     printf("Snapshot: %d\n", h);
    // }
    // THREADENTRY32 te{0};
    // te.dwSize = sizeof(THREADENTRY32);
    // BOOL thread_status = Thread32First(h, &te);
    // while (te.th32OwnerProcessID != pid) {
    //     thread_status = Thread32Next(h, &te);
    //     if (thread_status == FALSE) {
    //         printf("Failed to copy thread data: %d\n", GetLastError());
    //     }
    // }

    // printf("Thread id: %d of %d\n", te.th32ThreadID, te.th32OwnerProcessID);

    
    // HANDLE thread_handle = OpenThread(THREAD_ALL_ACCESS, FALSE, te.th32ThreadID);
    // if (thread_handle == NULL) {
    //     printf("Failed to open thread: %d\n", GetLastError());
    //     return -1;
    // }

	// HMODULE module = GetModuleHandle("ntdll.dll");
    // NTSTATUS(__stdcall *NtQueryInformationThread)(HANDLE ThreadHandle, THREADINFOCLASS ThreadInformationClass, PVOID ThreadInformation, ULONG ThreadInformationLength, PULONG ReturnLength);
	// NtQueryInformationThread = reinterpret_cast<decltype(NtQueryInformationThread)>(GetProcAddress(module, "NtQueryInformationThread"));
    
    // THREAD_BASIC_INFORMATION tbi{0};
    // NTSTATUS status = NtQueryInformationThread(thread_handle, (THREADINFOCLASS) 0, (void*) &tbi, sizeof(tbi), NULL);
    // if (status != 0) {
    //     printf("Failed to query thread info\n");
    //     return -1;
    // }
    // uint64_t thread_base_address{0};
    // NT_TIB tib{0};
    // ReadProcessMemory(process_handle, (void*) tbi.TebBaseAddress, (void*) &tib, sizeof(tbi), NULL);
    // thread_base_address = (uint64_t) tib.StackBase;
    // printf("Thread base address: 0x%llx\n", thread_base_address);

    // uint64_t base_address{0};
    // MODULEINFO mi;
	// GetModuleInformation(process_handle, GetModuleHandle("kernel32.dll"), &mi, sizeof(mi));
    // uint64_t buf[512] = {0};
    // ReadProcessMemory(process_handle, (void*) (thread_base_address - (512*sizeof(uint64_t))), (void*) buf, 512*sizeof(uint64_t), NULL);
    // for (int i = 511; i >= 0; i--) {
    //     if (buf[i] >= (uint64_t) mi.lpBaseOfDll && buf[i] <= ((uint64_t) mi.lpBaseOfDll) + ((uint64_t) mi.SizeOfImage)) {
    //         base_address = thread_base_address - (512*sizeof(uint64_t)) + i * sizeof(uint64_t);
    //         break;
    //     }
    // }
    // printf("Base address: 0x%llx\n", base_address);

    // connect to WSL2
    WSLConnection conn;
    int res = connect_to_wsl(conn);
    if (res == -1) {
        printf("Failed to connect to WSL\n");
        return -1;
    }

    auto factory = winrt::get_activation_factory<GraphicsCaptureItem, IGraphicsCaptureItemInterop>();
    GraphicsCaptureItem item = { nullptr };
    winrt::check_hresult(factory->CreateForWindow(target_window, winrt::guid_of<GraphicsCaptureItem>(), winrt::put_abi(item)));

    ID3D11Device* device_d3d;

    winrt::check_hresult(D3D11CreateDevice(
        nullptr,
        D3D_DRIVER_TYPE_HARDWARE,
        nullptr,
        D3D11_CREATE_DEVICE_BGRA_SUPPORT | D3D11_CREATE_DEVICE_DEBUG ,
        nullptr,
        0,
        D3D11_SDK_VERSION,
        &device_d3d,
        nullptr,
        nullptr
    ));

    D3D11_TEXTURE2D_DESC desc{0};
    desc.Width = 83+145;
    desc.Height = 240;
    desc.MipLevels = desc.ArraySize = 1;
    desc.Format = DXGI_FORMAT_B8G8R8A8_UNORM;
    desc.SampleDesc.Count = 1;
    desc.Usage = D3D11_USAGE_STAGING;
    desc.BindFlags = 0;
    desc.CPUAccessFlags = D3D11_CPU_ACCESS_READ;
    desc.MiscFlags = 0;

    ID3D11Texture2D *pTexture = NULL;
    winrt::check_hresult(device_d3d->CreateTexture2D( &desc, NULL, &pTexture ));

    IDXGIDevice* dxgi_device;
    device_d3d->QueryInterface(__uuidof(IDXGIDevice), (void **)&dxgi_device);
    
    winrt::Windows::Graphics::DirectX::Direct3D11::IDirect3DDevice device {nullptr};
    winrt::check_hresult(CreateDirect3D11DeviceFromDXGIDevice(dxgi_device, reinterpret_cast<IInspectable**>(winrt::put_abi(device))));
    
    
    printf("Created d3d device\n");

    IDXGIAdapter* adapter;
    dxgi_device->GetAdapter(&adapter);
    // CudaState cuda_state = new_cuda_state(adapter);
    // set_graphics_resource(cuda_state, pTexture);

    Direct3D11CaptureFramePool frame_pool = Direct3D11CaptureFramePool::CreateFreeThreaded(
        device,
        DirectXPixelFormat::B8G8R8A8UIntNormalized,
        2,
        item.Size()
    );
    
    printf("Creating capture session\n");
    GraphicsCaptureSession session = frame_pool.CreateCaptureSession(item);
    printf("Starting capture\n");

    ID3D11DeviceContext* context;
    device_d3d->GetImmediateContext(&context);

    D3D11_BOX sourceRegion;
    printf("%d x %d\n", item.Size().Width, item.Size().Height);
    sourceRegion.left = 409-83;
    sourceRegion.right = sourceRegion.left + 83 + 145;
    sourceRegion.top = 184;
    sourceRegion.bottom = sourceRegion.top + 240;
    sourceRegion.front = 0;
    sourceRegion.back = 1;
    int i = 0;

    uint8_t* img_buffer = new uint8_t[240*(145)*3];
    
    std::mutex event_mutex;
    std::mutex data_mutex;
    std::condition_variable event_cv;
    bool game_over{false};
    std::atomic<uint32_t> current_score{0};
    frame_pool.FrameArrived([&](Direct3D11CaptureFramePool f, winrt::Windows::Foundation::IInspectable d) {
        Direct3D11CaptureFrame frame = f.TryGetNextFrame();
        Direct3D11CaptureFrame frame_next = f.TryGetNextFrame();
        while (frame_next != NULL) {
            frame.Close();
            frame = frame_next;
            frame_next = f.TryGetNextFrame();
        }
        // printf("Frame %d\n", ++i);
        // auto surface = GetDXGIInterfaceFromObject<ID3D11Texture2D>(frame.Surface());
        
        // print score

        auto surf = frame.Surface();
        auto surf_abi = (IInspectable**) (&surf);
        IDirect3DDxgiInterfaceAccess* access;
        winrt::check_hresult((*surf_abi)->QueryInterface(&access));
        ID3D11Texture2D* surface;
        winrt::check_hresult(access->GetInterface(__uuidof(ID3D11Texture2D), reinterpret_cast<void**>(&surface)));
        
        context->CopySubresourceRegion(pTexture, 0, 0, 0, 0, surface, 0, &sourceRegion);

        D3D11_MAPPED_SUBRESOURCE texture_mapped{0};
        i++;
        winrt::check_hresult(context->Map(pTexture, 0, D3D11_MAP_READ, 0, &texture_mapped));

        const int BYTES_PER_PIXEL = 4;
        uint32_t score = (int) identify_digits(texture_mapped);
        data_mutex.lock();
        for (int y = 0; y < 240; y++) {
            for (int x = 83; x < 83+145; x++) {
                for (int c = 0; c < 3; c++) {
                    // if (i == 3) {
                    //     printf("%d ", ((uint8_t*) texture_mapped.pData)[(y * texture_mapped.RowPitch) + (x * BYTES_PER_PIXEL) + c]);
                    // }
                    img_buffer[(y*(145)*3) + (3*(x-83)) + c] = ((uint8_t*) texture_mapped.pData)[(y * texture_mapped.RowPitch) + (x * BYTES_PER_PIXEL) + c];
                }
            }
        }
        if (score != current_score) {
            current_score = score;
            printf("score: %d\n", score);
        }
        data_mutex.unlock();

        context->Unmap(pTexture, 0);
   
        {
            uint8_t go_red = img_buffer[(239 * 145 * 3) + (0 * BYTES_PER_PIXEL) + 2];
            uint8_t go_green = img_buffer[(239 * 145 * 3) + (0 * BYTES_PER_PIXEL) + 1];
            uint8_t go_blue = img_buffer[(239 * 145 * 3) + (0 * BYTES_PER_PIXEL) + 0];

            if (go_red == 59 && go_green == 38 && go_blue == 39) {
                if (!game_over) {
                    game_over = true;
                    event_cv.notify_one();
                }
            }
        }
        // if (i == 3) {
        //     for (int y = 0; y < 240; y++) {
        //         for (int x = 0; x < 145; x++) {
        //             for (int c = 0; c < 3; c++) {
        //                 // printf("%d ", img_buffer[(y*145*3) + (3*x) + c]);
        //             }
        //         }
        //     }
        // }

        // send the buffer
        uint32_t send_score = htonl(score);
        send_data(conn, (uint8_t *) &send_score, 4);
        send_data(conn, img_buffer, 240*145*3);

        frame.Close();
        return;
    });

    session.StartCapture();
    while (true) {
        std::unique_lock lk(event_mutex);
        event_cv.wait(lk, [&]{ return game_over; });
        
        send_100ms_keystroke_blocking(VK_F1, target_window);
        send_100ms_keystroke_blocking(0x5A, target_window);

        game_over = false;
        printf("Game over detected\n");
        lk.unlock();
    }

    return 0;
}


/*
  000
 1 2 3
 1 2 3
  444
 5   6
 5   6
  777
*/

uint32_t identify_digits(D3D11_MAPPED_SUBRESOURCE &image) {
    int off_x = 0;
    int off_y = 22;

    const int offsets[8][2] = {
        {4, 0},
        {2, 3},
        {3, 2},
        {6, 2},
        {5, 3},
        {1, 5},
        {6, 5},
        {2, 7}
    };

    const uint8_t digits[10] = {
        0x6b, 0x85, 0xb9, 0xf9, 0x12, 0xd3, 0xf3, 0x99, 0xfb, 0xcb
    };
    uint32_t result = 0;
    uint32_t fac = 100000000;
    for (int d = 0; d < 9; d++) {
        uint8_t active_segment[8] = {0};
        uint8_t digit_mask{0};
        for (int i = 0; i < 8; i++) {
            uint8_t red = ((uint8_t*) image.pData)[(off_y + offsets[i][1])*image.RowPitch + (off_x + offsets[i][0])*4 + 2];

            // ((uint8_t*) image.pData)[(off_y + offsets[i][1])*image.RowPitch + (off_x + offsets[i][0])*4 + 0] = 255;
            // ((uint8_t*) image.pData)[(off_y + offsets[i][1])*image.RowPitch + (off_x + offsets[i][0])*4 + 1] = 255;
            // ((uint8_t*) image.pData)[(off_y + offsets[i][1])*image.RowPitch + (off_x + offsets[i][0])*4 + 2] = 0;

            if (red > 200) red = 1; else red = 0;
            digit_mask |= (red) << i;
        }
        // printf("0x%02x\n", digit_mask);
        int digit{0};
        for (int i = 0; i < 10; i++) {
            if (digit_mask == digits[i]) {
                digit = i; 
                break;
            }
        }
        result += digit * fac;
        fac /= 10;
        off_x += 8;
    }

    return result;
}

void send_100ms_keystroke_blocking(uint8_t keystroke, HWND hWnd) {

        uint32_t repeatCount = 0;
        uint32_t scanCode = 0;
        uint32_t extended = 0;
        uint32_t context = 0;
        uint32_t previousState = 0;
        uint32_t transition = 0;

        uint32_t lParamDown;
        uint32_t lParamUp;

        scanCode = MapVirtualKey(keystroke, MAPVK_VK_TO_VSC);
        lParamDown = repeatCount
            | (scanCode << 16)
            | (extended << 24)
            | (context << 29)
            | (previousState << 30)
            | (transition << 31);
        previousState = 1;
        transition = 1;
        lParamUp = repeatCount
            | (scanCode << 16)
            | (extended << 24)
            | (context << 29)
            | (previousState << 30)
            | (transition << 31);
        // reset the game
        PostMessage(hWnd, WM_KEYDOWN, keystroke, lParamDown);
        Sleep(50);
        PostMessage(hWnd, WM_KEYUP, keystroke, lParamUp);
        Sleep(50);
}