#include <napi.h>
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <atomic>
#include <chrono>

struct DisplayInfo {
    int index;
    RECT rect;
    HMONITOR hMonitor;
};

class Direct2DDisplay : public Napi::ObjectWrap<Direct2DDisplay> {
private:
    struct RenderConfig {
        std::wstring text;
        float x = 0.0f;
        float y = 0.0f;
        float fontSize = 48.0f;
        DWRITE_FONT_WEIGHT fontWeight = DWRITE_FONT_WEIGHT_BOLD;
        bool fontDirty = true;
    };

    // 双缓冲配置
    RenderConfig frontConfig;  // 渲染线程使用
    RenderConfig backConfig;   // JS线程更新
    std::mutex configMutex;
    std::atomic<bool> configDirty{false};

    // Direct2D 资源
    ID2D1Factory* pD2DFactory = nullptr;
    IDWriteFactory* pDWriteFactory = nullptr;
    ID2D1HwndRenderTarget* pRenderTarget = nullptr;
    IDWriteTextFormat* pTextFormat = nullptr;
    ID2D1SolidColorBrush* pBrush = nullptr;
    
    // 窗口和状态
    HWND hwnd = nullptr;
    std::vector<DisplayInfo> displays;
    std::thread renderThread;
    std::atomic<bool> isRunning{false};

public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "Direct2DDisplay", {
            InstanceMethod("start", &Direct2DDisplay::Start),
            InstanceMethod("stop", &Direct2DDisplay::Stop),
            InstanceMethod("updateAll", &Direct2DDisplay::UpdateAll),
        });

        Napi::FunctionReference* constructor = new Napi::FunctionReference();
        *constructor = Napi::Persistent(func);
        env.SetInstanceData(constructor);

        exports.Set("Direct2DDisplay", func);
        return exports;
    }

    Direct2DDisplay(const Napi::CallbackInfo& info) 
        : Napi::ObjectWrap<Direct2DDisplay>(info) {
        Napi::Env env = info.Env();
        
        // 初始化 COM 和 Direct2D
        CoInitialize(nullptr);
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), 
                          reinterpret_cast<IUnknown**>(&pDWriteFactory));

        // 创建默认画笔颜色
        if (pD2DFactory && SUCCEEDED(pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(),
            D2D1::HwndRenderTargetProperties(GetConsoleWindow(), D2D1::SizeU(1,1)),
            &pRenderTarget))) {
            pRenderTarget->CreateSolidColorBrush(
                D2D1::ColorF(D2D1::ColorF::White), &pBrush);
        }
    }

    ~Direct2DDisplay() {
        Stop(Napi::CallbackInfo(Env(), {})); // 清理线程
        CleanupResources();
        CoUninitialize();
    }

private:
    void CleanupResources() {
        if (pBrush) { pBrush->Release(); pBrush = nullptr; }
        if (pTextFormat) { pTextFormat->Release(); pTextFormat = nullptr; }
        if (pRenderTarget) { pRenderTarget->Release(); pRenderTarget = nullptr; }
        if (pDWriteFactory) { pDWriteFactory->Release(); pDWriteFactory = nullptr; }
        if (pD2DFactory) { pD2DFactory->Release(); pD2DFactory = nullptr; }
        if (hwnd) { DestroyWindow(hwnd); hwnd = nullptr; }
    }
    // 显示器枚举回调
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC, LPRECT rect, LPARAM self) {
        auto* instance = reinterpret_cast<Direct2DDisplay*>(self);
        MONITORINFOEX info{};
        info.cbSize = sizeof(MONITORINFOEX);
        GetMonitorInfo(hMonitor, &info);

        instance->displays.push_back({
            static_cast<int>(instance->displays.size()),
            info.rcMonitor,
            hMonitor
        });
        return TRUE;
    }

    // 窗口创建和初始化
    void InitializeWindow(int displayIndex) {
        displays.clear();
        EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, reinterpret_cast<LPARAM>(this));

        if (displayIndex >= displays.size()) {
            throw std::runtime_error("Invalid display index");
        }

        const auto& display = displays[displayIndex];
        const wchar_t CLASS_NAME[] = L"D2DOverlayWindow";

        WNDCLASSEXW wc{};
        wc.cbSize = sizeof(WNDCLASSEX);
        wc.lpfnWndProc = DefWindowProc;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = CLASS_NAME;
        RegisterClassExW(&wc);

        // 使用WS_EX_LAYERED实现透明效果
        hwnd = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST | WS_EX_NOACTIVATE,
            CLASS_NAME,
            nullptr,
            WS_POPUP,
            display.rect.left,
            display.rect.top,
            display.rect.right - display.rect.left,
            display.rect.bottom - display.rect.top,
            nullptr,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        if (!hwnd) {
            throw std::runtime_error("Window creation failed");
        }

        // 配置窗口透明属性
        SetLayeredWindowAttributes(hwnd, 0, int(255 * 0.2), LWA_ALPHA);
        ShowWindow(hwnd, SW_SHOWNA);

        // 创建D2D渲染目标
        D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps{};
        hwndProps.hwnd = hwnd;
        hwndProps.pixelSize = D2D1::SizeU(
            display.rect.right - display.rect.left,
            display.rect.bottom - display.rect.top
        );
        hwndProps.presentOptions = D2D1_PRESENT_OPTIONS_IMMEDIATELY;

        pD2DFactory->CreateHwndRenderTarget(
            D2D1::RenderTargetProperties(
                D2D1_RENDER_TARGET_TYPE_DEFAULT,
                D2D1::PixelFormat(DXGI_FORMAT_B8G8R8A8_UNORM, D2D1_ALPHA_MODE_PREMULTIPLIED)),
            hwndProps,
            &pRenderTarget
        );

        // 创建默认画笔
        pRenderTarget->CreateSolidColorBrush(D2D1::ColorF(D2D1::ColorF::White), &pBrush);
        RecreateTextFormat(frontConfig.fontSize, frontConfig.fontWeight);
    }

    // 字体资源管理
    void RecreateTextFormat(float fontSize, DWRITE_FONT_WEIGHT weight) {
        if (pTextFormat) pTextFormat->Release();
        
        pDWriteFactory->CreateTextFormat(
            L"Arial",
            nullptr,
            weight,
            DWRITE_FONT_STYLE_NORMAL,
            DWRITE_FONT_STRETCH_NORMAL,
            fontSize,
            L"",
            &pTextFormat
        );

        if (pTextFormat) {
            pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
            pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
        }
    }

    // 渲染循环核心逻辑
    void RenderLoop(int displayIndex) {
        try {
            InitializeWindow(displayIndex);
            auto lastFrameTime = std::chrono::steady_clock::now();
            
            MSG msg{};
            while (isRunning) {
                // 处理窗口消息
                while (PeekMessageW(&msg, nullptr, 0, 0, PM_REMOVE)) {
                    TranslateMessage(&msg);
                    DispatchMessage(&msg);
                }

                // 双缓冲配置交换
                if (configDirty.exchange(false)) {
                    std::lock_guard<std::mutex> lock(configMutex);
                    frontConfig = backConfig;
                    
                    if (frontConfig.fontDirty) {
                        RecreateTextFormat(frontConfig.fontSize, frontConfig.fontWeight);
                        frontConfig.fontDirty = false;
                    }
                }

                // 精确帧率控制
                const auto now = std::chrono::steady_clock::now();
                const auto elapsed = now - lastFrameTime;
                if (elapsed < std::chrono::milliseconds(16)) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(1));
                    continue;
                }

                RenderFrame();
                lastFrameTime = now;
            }
        } catch (...) {
            isRunning = false;
        }
        CleanupResources();
    }

    // 单帧渲染实现
    void RenderFrame() {
        if (!pRenderTarget || !pTextFormat || !pBrush) return;

        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(0, 0, 0, 0));

        // 计算文本布局边界
        const D2D1_SIZE_F rtSize = pRenderTarget->GetSize();
        const float maxWidth = rtSize.width - frontConfig.x;
        const float maxHeight = rtSize.height - frontConfig.y;

        const D2D1_RECT_F textRect = D2D1::RectF(
            frontConfig.x,
            frontConfig.y,
            frontConfig.x + maxWidth,
            frontConfig.y + maxHeight
        );

        // 执行文本绘制
        pRenderTarget->DrawText(
            frontConfig.text.c_str(),
            static_cast<UINT32>(frontConfig.text.length()),
            pTextFormat,
            textRect,
            pBrush
        );

        // 处理设备丢失
        const HRESULT hr = pRenderTarget->EndDraw();
        if (hr == D2DERR_RECREATE_TARGET) {
            CleanupResources();
            InitializeWindow(0); // 尝试恢复渲染目标
        }
    }

    // JavaScript 接口方法
    Napi::Value Start(const Napi::CallbackInfo& info) {
        if (isRunning) return info.Env().Undefined();

        const int displayIndex = info.Length() > 0 ? info[0].As<Napi::Number>().Int32Value() : 0;
        isRunning = true;
        renderThread = std::thread(&Direct2DDisplay::RenderLoop, this, displayIndex);
        return info.Env().Undefined();
    }

    Napi::Value Stop(const Napi::CallbackInfo& info) {
        isRunning = false;
        if (renderThread.joinable()) {
            renderThread.join();
        }
        CleanupResources();
        return info.Env().Undefined();
    }

    Napi::Value UpdateAll(const Napi::CallbackInfo& info) {
        if (info.Length() < 3) return info.Env().Undefined();

        // 从JavaScript获取参数
        const float x = info[0].As<Napi::Number>().FloatValue();
        const float y = info[1].As<Napi::Number>().FloatValue();
        const std::string text = info[2].As<Napi::String>();

        // 转换文本编码
        const int bufferSize = MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, nullptr, 0);
        std::wstring wtext(bufferSize, 0);
        MultiByteToWideChar(CP_UTF8, 0, text.c_str(), -1, wtext.data(), bufferSize);

        // 更新后台配置
        {
            std::lock_guard<std::mutex> lock(configMutex);
            backConfig.x = x;
            backConfig.y = y;
            backConfig.text = std::move(wtext);
            configDirty = true;
        }

        return info.Env().Undefined();
    }
};

Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return Direct2DDisplay::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)