#include <napi.h>
#include <windows.h>
#include <d2d1.h>
#include <dwrite.h>
#include <vector>
#include <string>
#include <thread>
#include <mutex>
#include <stdexcept>
#include <memory>

struct DisplayInfo {
    int index;
    RECT rect;
    HMONITOR hMonitor;
};

// std::vector<DisplayInfo> displays;
ID2D1Factory* pD2DFactory = nullptr;
IDWriteFactory* pDWriteFactory = nullptr;

// 添加一个结构体来存储文字渲染的配置
struct TextRenderConfig {
    std::wstring text;
    float x;
    float y;
    float fontSize;
    DWRITE_FONT_WEIGHT fontWeight;
    D2D1::ColorF color;
    bool needsUpdate;

    // 添加默认构造函数
    TextRenderConfig() : 
        text(L""),
        x(0.0f),
        y(0.0f),
        fontSize(48.0f),
        fontWeight(DWRITE_FONT_WEIGHT_BOLD),
        color(D2D1::ColorF::White),
        needsUpdate(true) {}
};



class Direct2DDisplay : public Napi::ObjectWrap<Direct2DDisplay> {
private:
    struct TextRenderConfig {
        std::wstring text;
        float x;
        float y;
        float fontSize;
        DWRITE_FONT_WEIGHT fontWeight;
        D2D1::ColorF color;
        bool needsUpdate;

        // 添加默认构造函数
        TextRenderConfig() : 
            text(L""),
            x(0.0f),
            y(0.0f),
            fontSize(48.0f),
            fontWeight(DWRITE_FONT_WEIGHT_BOLD),
            color(D2D1::ColorF::White),
            needsUpdate(true) {}
    };

    // 成员变量
    std::vector<DisplayInfo> displays;
    ID2D1Factory* pD2DFactory;
    IDWriteFactory* pDWriteFactory;
    ID2D1HwndRenderTarget* pRenderTarget;
    IDWriteTextFormat* pTextFormat;
    ID2D1SolidColorBrush* pBrush;
    HWND hwnd;
    bool isRunning;
    TextRenderConfig config;
    std::thread renderThread;
    std::mutex configMutex;

public:
    static Napi::Object Init(Napi::Env env, Napi::Object exports) {
        Napi::Function func = DefineClass(env, "Direct2DDisplay", {
            InstanceMethod("start", &Direct2DDisplay::Start),
            InstanceMethod("stop", &Direct2DDisplay::Stop),
            InstanceMethod("updateText", &Direct2DDisplay::UpdateText),
            InstanceMethod("updatePosition", &Direct2DDisplay::UpdatePosition),
            InstanceMethod("updateStyle", &Direct2DDisplay::UpdateStyle),
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
        
        // 初始化COM和D2D
        CoInitialize(nullptr);
        D2D1CreateFactory(D2D1_FACTORY_TYPE_SINGLE_THREADED, &pD2DFactory);
        DWriteCreateFactory(DWRITE_FACTORY_TYPE_SHARED, __uuidof(IDWriteFactory), (IUnknown**)&pDWriteFactory);
        
        // 使用直接赋值而不是初始化列表
        config.text = L"";
        config.x = 0.0f;
        config.y = 0.0f;
        config.fontSize = 48.0f;
        config.fontWeight = DWRITE_FONT_WEIGHT_BOLD;
        config.color = D2D1::ColorF(D2D1::ColorF::White);
        config.needsUpdate = true;

        isRunning = false;
        pRenderTarget = nullptr;
        pTextFormat = nullptr;
        pBrush = nullptr;
        hwnd = nullptr;
    }

    ~Direct2DDisplay() {
        // 修复 CallbackInfo 构造
        if (isRunning) {
            isRunning = false;
            if (renderThread.joinable()) {
                renderThread.join();
            }
        }
        
        if (pBrush) pBrush->Release();
        if (pTextFormat) pTextFormat->Release();
        if (pRenderTarget) pRenderTarget->Release();
        if (pDWriteFactory) pDWriteFactory->Release();
        if (pD2DFactory) pD2DFactory->Release();
        CoUninitialize();
    }

private:
    static BOOL CALLBACK MonitorEnumProc(HMONITOR hMonitor, HDC hdcMonitor, LPRECT lprcMonitor, LPARAM lParam) {
        Direct2DDisplay* instance = reinterpret_cast<Direct2DDisplay*>(lParam);
        MONITORINFOEX info;
        info.cbSize = sizeof(MONITORINFOEX);
        GetMonitorInfo(hMonitor, &info);
        
        DisplayInfo di;
        di.index = instance->displays.size();
        di.hMonitor = hMonitor;
        di.rect = info.rcMonitor;
        instance->displays.push_back(di); // 添加到实例的 displays 成员中
        
        return TRUE;
    }
    Napi::Value Start(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        
        if (isRunning) {
            return Napi::Boolean::New(env, true);
        }

        // 解析参数
        if (info.Length() < 1) {
            Napi::Error::New(env, "Initial text is required").ThrowAsJavaScriptException();
            return env.Null();
        }

        std::string text = info[0].As<Napi::String>();
        int displayIndex = info.Length() > 1 ? info[1].As<Napi::Number>() : 0;

        // 设置初始文本
        {
            std::lock_guard<std::mutex> lock(configMutex);
            config.text = std::wstring(text.begin(), text.end());
        }

        // 枚举显示器
        displays.clear();
        EnumDisplayMonitors(nullptr, nullptr, MonitorEnumProc, (LPARAM)this);
        
        if (displayIndex >= displays.size()) {
            Napi::Error::New(env, "Invalid display index").ThrowAsJavaScriptException();
            return env.Null();
        }

        // 创建渲染线程
        isRunning = true;
        renderThread = std::thread(&Direct2DDisplay::RenderLoop, this, displayIndex);

        return Napi::Boolean::New(env, true);
    }

    

    Napi::Value Stop(const Napi::CallbackInfo& info) {
        if (isRunning) {
            isRunning = false;
            if (renderThread.joinable()) {
                renderThread.join();
            }
        }

        return Napi::Boolean::New(info.Env(), true);
    }

    Napi::Value UpdateText(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        if (info.Length() < 1) return env.Null();

        std::string utf8Text = info[0].As<Napi::String>();

        // 将 UTF-8 转换为 UTF-16
    int wideLen = MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(), -1, nullptr, 0);
    std::wstring utf16Text(wideLen, 0);
    MultiByteToWideChar(CP_UTF8, 0, utf8Text.c_str(), -1, &utf16Text[0], wideLen);
        {
            std::lock_guard<std::mutex> lock(configMutex);
            config.text = utf16Text;
            config.needsUpdate = true;
        }
        return env.Undefined();
    }

    Napi::Value UpdatePosition(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        if (info.Length() < 2) return env.Null();

        float x = info[0].As<Napi::Number>().FloatValue();
        float y = info[1].As<Napi::Number>().FloatValue();
        {
            std::lock_guard<std::mutex> lock(configMutex);
            config.x = x;
            config.y = y;
            config.needsUpdate = true;
        }
        return env.Undefined();
    }

    Napi::Value UpdateStyle(const Napi::CallbackInfo& info) {
        Napi::Env env = info.Env();
        if (info.Length() < 2) return env.Null();

        float fontSize = info[0].As<Napi::Number>().FloatValue();
        int weight = info[1].As<Napi::Number>().Int32Value();
        {
            std::lock_guard<std::mutex> lock(configMutex);
            config.fontSize = fontSize;
            config.fontWeight = static_cast<DWRITE_FONT_WEIGHT>(weight);
            config.needsUpdate = true;
        }
        return env.Undefined();
    }

    void RenderLoop(int displayIndex) {
        // 创建窗口和初始化D2D资源
        InitializeWindow(displayIndex);
        
        MSG msg = {0};
        while (isRunning) {
            if (PeekMessage(&msg, nullptr, 0, 0, PM_REMOVE)) {
                TranslateMessage(&msg);
                DispatchMessage(&msg);
            }

            std::lock_guard<std::mutex> lock(configMutex);
            if (config.needsUpdate) {
                UpdateTextFormat();
                config.needsUpdate = false;
            }

            Render();
            Sleep(16); // ~60 FPS
        }

        // 清理资源
        CleanupResources();
    }

    void InitializeWindow(int displayIndex) {
        RECT displayRect = displays[displayIndex].rect;
        
        // 创建窗口类
        WNDCLASSEXW wc = {0};
        wc.cbSize = sizeof(WNDCLASSEXW);
        wc.lpfnWndProc = DefWindowProcW;
        wc.hInstance = GetModuleHandle(nullptr);
        wc.lpszClassName = L"D2DDisplayWindow";
        RegisterClassExW(&wc);
        
        // 创建窗口
        hwnd = CreateWindowExW(
            WS_EX_LAYERED | WS_EX_TRANSPARENT | WS_EX_TOPMOST,
            L"D2DDisplayWindow",
            L"Direct2D Display",
            WS_POPUP,
            displayRect.left,
            displayRect.top,
            displayRect.right - displayRect.left,
            displayRect.bottom - displayRect.top,
            nullptr,
            nullptr,
            GetModuleHandle(nullptr),
            nullptr
        );

        if (!hwnd) {
            throw std::runtime_error("Failed to create window");
        }

        // 设置窗口透明
        SetLayeredWindowAttributes(hwnd, RGB(0,0,0), 0, LWA_COLORKEY);
        ShowWindow(hwnd, SW_SHOW);

        // 创建D2D渲染目标
        D2D1_RENDER_TARGET_PROPERTIES props = D2D1::RenderTargetProperties();
        D2D1_HWND_RENDER_TARGET_PROPERTIES hwndProps = D2D1::HwndRenderTargetProperties(
            hwnd,
            D2D1::SizeU(
                displayRect.right - displayRect.left,
                displayRect.bottom - displayRect.top
            )
        );

        HRESULT hr = pD2DFactory->CreateHwndRenderTarget(
            props,
            hwndProps,
            &pRenderTarget
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create render target");
        }

        // 创建画笔
        hr = pRenderTarget->CreateSolidColorBrush(
            config.color,
            &pBrush
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create brush");
        }

        // 初始化文本格式
        UpdateTextFormat();
    }

    void UpdateTextFormat() {
        // 释放旧的文本格式
        if (pTextFormat) {
            pTextFormat->Release();
            pTextFormat = nullptr;
        }

        // 创建新的文本格式
        HRESULT hr = pDWriteFactory->CreateTextFormat(
            L"Arial",                    // fontFamilyName
            nullptr,                     // fontCollection
            config.fontWeight,           // fontWeight
            DWRITE_FONT_STYLE_NORMAL,    // fontStyle
            DWRITE_FONT_STRETCH_NORMAL,  // fontStretch
            config.fontSize,             // fontSize
            L"",                         // localeName
            &pTextFormat
        );

        if (FAILED(hr)) {
            throw std::runtime_error("Failed to create text format");
        }

        // 设置文本对齐方式
        pTextFormat->SetTextAlignment(DWRITE_TEXT_ALIGNMENT_LEADING);
        pTextFormat->SetParagraphAlignment(DWRITE_PARAGRAPH_ALIGNMENT_NEAR);
    }

    void Render() {
        if (!pRenderTarget || !pTextFormat || !pBrush) {
            return;
        }

        pRenderTarget->BeginDraw();
        pRenderTarget->Clear(D2D1::ColorF(0, 0, 0, 0)); // 透明背景

        // 获取渲染目标大小
        D2D1_SIZE_F size = pRenderTarget->GetSize();

        // 创建文本布局矩形
        D2D1_RECT_F layoutRect = D2D1::RectF(
            config.x,
            config.y,
            config.x + size.width,
            config.y + size.height
        );

        // 绘制文本
        pRenderTarget->DrawText(
            config.text.c_str(),
            config.text.length(),
            pTextFormat,
            layoutRect,
            pBrush
        );

        // 结束绘制
        HRESULT hr = pRenderTarget->EndDraw();

        // 如果设备丢失，尝试重新创建资源
        if (hr == D2DERR_RECREATE_TARGET) {
            CleanupD2DResources();
            InitializeWindow(0); // 重新初始化，使用默认显示器
        }
    }

    void CleanupD2DResources() {
        if (pBrush) {
            pBrush->Release();
            pBrush = nullptr;
        }
        if (pTextFormat) {
            pTextFormat->Release();
            pTextFormat = nullptr;
        }
        if (pRenderTarget) {
            pRenderTarget->Release();
            pRenderTarget = nullptr;
        }
    }

    void CleanupResources() {
        CleanupD2DResources();
        if (hwnd) {
            DestroyWindow(hwnd);
            hwnd = nullptr;
        }
    }

    // 添加一个辅助方法用于错误处理
    static void ThrowIfFailed(HRESULT hr, const char* message) {
        if (FAILED(hr)) {
            throw std::runtime_error(message);
        }
    }
};

// 修复 NODE_API_MODULE 宏的使用
Napi::Object Init(Napi::Env env, Napi::Object exports) {
    return Direct2DDisplay::Init(env, exports);
}

NODE_API_MODULE(NODE_GYP_MODULE_NAME, Init)