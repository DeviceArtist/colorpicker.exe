#include <windows.h>
#include <stdio.h>

// 全局变量
HBITMAP g_hBitmap = NULL;
int g_nPhysicalWidth = 0, g_nPhysicalHeight = 0;  // 实际物理分辨率

// 窗口过程
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // 获取实际物理屏幕分辨率（不考虑系统缩放）
            DEVMODE dm = {0};
            dm.dmSize = sizeof(DEVMODE);
            EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
            g_nPhysicalWidth = dm.dmPelsWidth;
            g_nPhysicalHeight = dm.dmPelsHeight;
            
            // 获取屏幕DC（包含整个物理屏幕）
            HDC hScreenDC = GetDC(NULL);
            if (hScreenDC == NULL) break;
            
            // 创建兼容内存DC
            HDC hMemDC = CreateCompatibleDC(hScreenDC);
            if (hMemDC == NULL) {
                ReleaseDC(NULL, hScreenDC);
                break;
            }
            
            // 创建与实际物理分辨率匹配的位图
            g_hBitmap = CreateCompatibleBitmap(hScreenDC, g_nPhysicalWidth, g_nPhysicalHeight);
            if (g_hBitmap != NULL) {
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, g_hBitmap);
                // 1:1复制整个物理屏幕内容
                BitBlt(hMemDC, 0, 0, g_nPhysicalWidth, g_nPhysicalHeight, 
                       hScreenDC, 0, 0, SRCCOPY);
                SelectObject(hMemDC, hOldBitmap);
            }
            
            DeleteDC(hMemDC);
            ReleaseDC(NULL, hScreenDC);
            
            // 设置吸管光标
            SetClassLongPtr(hwnd, GCLP_HCURSOR, 
                (LONG_PTR)LoadCursor(NULL, MAKEINTRESOURCE(32515))); // IDC_CROSS
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 获取窗口客户区尺寸
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            // 计算显示比例（保持截图1:1，不拉伸）
            double scaleX = (double)rcClient.right / g_nPhysicalWidth;
            double scaleY = (double)rcClient.bottom / g_nPhysicalHeight;
            double scale = min(scaleX, scaleY);
            
            // 计算显示位置（居中显示）
            int nDrawWidth = (int)(g_nPhysicalWidth * scale);
            int nDrawHeight = (int)(g_nPhysicalHeight * scale);
            int nDrawX = (rcClient.right - nDrawWidth) / 2;
            int nDrawY = (rcClient.bottom - nDrawHeight) / 2;
            
            // 绘制截图（保持1:1比例，不拉伸）
            if (g_hBitmap != NULL) {
                HDC hMemDC = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, g_hBitmap);
                
                SetStretchBltMode(hdc, HALFTONE); // 高质量缩放
                StretchBlt(hdc, nDrawX, nDrawY, nDrawWidth, nDrawHeight,
                          hMemDC, 0, 0, g_nPhysicalWidth, g_nPhysicalHeight, SRCCOPY);
                
                SelectObject(hMemDC, hOldBitmap);
                DeleteDC(hMemDC);
            }
            
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            // 获取窗口客户区尺寸
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            // 计算显示比例
            double scaleX = (double)rcClient.right / g_nPhysicalWidth;
            double scaleY = (double)rcClient.bottom / g_nPhysicalHeight;
            double scale = min(scaleX, scaleY);
            
            // 将窗口坐标转换为实际屏幕坐标
            int x = (int)((LOWORD(lParam) - (rcClient.right - g_nPhysicalWidth*scale)/2) / scale);
            int y = (int)((HIWORD(lParam) - (rcClient.bottom - g_nPhysicalHeight*scale)/2) / scale);
            
            // 确保坐标在屏幕范围内
            x = max(0, min(x, g_nPhysicalWidth - 1));
            y = max(0, min(y, g_nPhysicalHeight - 1));
            
            // 获取实际屏幕坐标的颜色
            HDC hScreenDC = GetDC(NULL);
            COLORREF color = GetPixel(hScreenDC, x, y);
            ReleaseDC(NULL, hScreenDC);
            
            // 格式化颜色字符串
            char szColor[64];
            sprintf(szColor, "RGB(%d,%d,%d) | #%02X%02X%02X", 
                GetRValue(color), GetGValue(color), GetBValue(color),
                GetRValue(color), GetGValue(color), GetBValue(color));
            
            // 复制到剪贴板
            if (OpenClipboard(NULL)) {
                EmptyClipboard();
                HGLOBAL hMem = GlobalAlloc(GMEM_MOVEABLE, strlen(szColor) + 1);
                if (hMem) {
                    LPSTR lpMem = (LPSTR)GlobalLock(hMem);
                    strcpy(lpMem, szColor);
                    GlobalUnlock(hMem);
                    SetClipboardData(CF_TEXT, hMem);
                }
                CloseClipboard();
            }
            
            PostQuitMessage(0);
            return 0;
        }

        case WM_DESTROY: {
            if (g_hBitmap != NULL) {
                DeleteObject(g_hBitmap);
                g_hBitmap = NULL;
            }
            PostQuitMessage(0);
            return 0;
        }

        default:
            return DefWindowProc(hwnd, uMsg, wParam, lParam);
    }
    return 0;
}

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nCmdShow) {
    const char CLASS_NAME[] = "MinGWColorPicker";
    
    // 获取实际物理屏幕分辨率（提前获取，避免窗口创建后可能的缩放问题）
    DEVMODE dm = {0};
    dm.dmSize = sizeof(DEVMODE);
    EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
    int nPhysicalWidth = dm.dmPelsWidth;
    int nPhysicalHeight = dm.dmPelsHeight;
    
    WNDCLASSEX wc = {0};
    wc.cbSize        = sizeof(WNDCLASSEX);
    wc.lpfnWndProc   = WindowProc;
    wc.hInstance     = hInstance;
    wc.hCursor       = LoadCursor(NULL, MAKEINTRESOURCE(32515)); // IDC_CROSS
    wc.lpszClassName = CLASS_NAME;
    
    if (!RegisterClassEx(&wc)) {
        MessageBox(NULL, "窗口类注册失败！", "错误", MB_ICONERROR);
        return 1;
    }

    // 创建与实际物理分辨率相同的全屏窗口
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        "颜色拾取器",
        WS_POPUP,
        0, 0, nPhysicalWidth, nPhysicalHeight,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd) {
        // 禁用DPI缩放，确保窗口显示实际物理分辨率
        SetProcessDPIAware();
        
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
        
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } else {
        MessageBox(NULL, "窗口创建失败！", "错误", MB_ICONERROR);
    }

    return 0;
}

// Cygwin兼容入口
int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOW);
}