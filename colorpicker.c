#include <windows.h>
#include <stdio.h>

// 全局变量
HBITMAP g_hBitmap = NULL;
int g_nPhysicalWidth = 0, g_nPhysicalHeight = 0;  // 实际物理分辨率
double g_dScale = 1.0;  // 缩放比例，默认1.0（原始大小）
int g_nOffsetX = 0, g_nOffsetY = 0;  // 截图偏移量，用于中心点缩放
POINT g_ptLastMouse;  // 最后鼠标位置

// 窗口过程
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // 获取实际物理屏幕分辨率
            DEVMODE dm = {0};
            dm.dmSize = sizeof(DEVMODE);
            EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
            g_nPhysicalWidth = dm.dmPelsWidth;
            g_nPhysicalHeight = dm.dmPelsHeight;
            
            // 获取屏幕DC并创建截图
            HDC hScreenDC = GetDC(NULL);
            if (hScreenDC != NULL) {
                HDC hMemDC = CreateCompatibleDC(hScreenDC);
                if (hMemDC != NULL) {
                    g_hBitmap = CreateCompatibleBitmap(hScreenDC, g_nPhysicalWidth, g_nPhysicalHeight);
                    if (g_hBitmap != NULL) {
                        HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, g_hBitmap);
                        BitBlt(hMemDC, 0, 0, g_nPhysicalWidth, g_nPhysicalHeight, 
                               hScreenDC, 0, 0, SRCCOPY);
                        SelectObject(hMemDC, hOldBitmap);
                    }
                    DeleteDC(hMemDC);
                }
                ReleaseDC(NULL, hScreenDC);
            }
            
            // 设置光标为十字线
            SetClassLongPtr(hwnd, GCLP_HCURSOR, 
                (LONG_PTR)LoadCursor(NULL, MAKEINTRESOURCE(32515))); // IDC_CROSS
            return 0;
        }

        case WM_MOUSEMOVE: {
            // 记录鼠标位置
            g_ptLastMouse.x = LOWORD(lParam);
            g_ptLastMouse.y = HIWORD(lParam);
            return 0;
        }

        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_ADD:     // 数字小键盘上的"+"键
                case VK_OEM_PLUS: // 主键盘上的"+"键
                    {
                        // 计算鼠标在截图上的位置
                        double mouseXOnScreen = (g_ptLastMouse.x - g_nOffsetX) / g_dScale;
                        double mouseYOnScreen = (g_ptLastMouse.y - g_nOffsetY) / g_dScale;
                        
                        // 保存当前缩放比例
                        double oldScale = g_dScale;
                        
                        // 调整缩放比例
                        g_dScale *= 1.2;  // 每次放大20%
                        if (g_dScale > 4.0) g_dScale = 4.0;  // 最大放大4倍
                        
                        // 计算新的偏移量，使鼠标位置保持在屏幕中央
                        g_nOffsetX = (int)(g_ptLastMouse.x - mouseXOnScreen * g_dScale);
                        g_nOffsetY = (int)(g_ptLastMouse.y - mouseYOnScreen * g_dScale);
                        
                        // 确保截图完全显示在窗口内
                        int nDrawWidth = (int)(g_nPhysicalWidth * g_dScale);
                        int nDrawHeight = (int)(g_nPhysicalHeight * g_dScale);
                        
                        RECT rcClient;
                        GetClientRect(hwnd, &rcClient);
                        
                        g_nOffsetX = max(rcClient.right - nDrawWidth, min(g_nOffsetX, 0));
                        g_nOffsetY = max(rcClient.bottom - nDrawHeight, min(g_nOffsetY, 0));
                    }
                    InvalidateRect(hwnd, NULL, TRUE);  // 刷新窗口
                    break;
                    
                case VK_SUBTRACT: // 数字小键盘上的"-"键
                case VK_OEM_MINUS: // 主键盘上的"-"键
                    {
                        // 计算鼠标在截图上的位置
                        double mouseXOnScreen = (g_ptLastMouse.x - g_nOffsetX) / g_dScale;
                        double mouseYOnScreen = (g_ptLastMouse.y - g_nOffsetY) / g_dScale;
                        
                        // 保存当前缩放比例
                        double oldScale = g_dScale;
                        
                        // 调整缩放比例
                        g_dScale /= 1.2;  // 每次缩小20%
                        if (g_dScale < 0.25) g_dScale = 0.25;  // 最小缩小到25%
                        
                        // 计算新的偏移量，使鼠标位置保持在屏幕中央
                        g_nOffsetX = (int)(g_ptLastMouse.x - mouseXOnScreen * g_dScale);
                        g_nOffsetY = (int)(g_ptLastMouse.y - mouseYOnScreen * g_dScale);
                        
                        // 确保截图完全显示在窗口内
                        int nDrawWidth = (int)(g_nPhysicalWidth * g_dScale);
                        int nDrawHeight = (int)(g_nPhysicalHeight * g_dScale);
                        
                        RECT rcClient;
                        GetClientRect(hwnd, &rcClient);
                        
                        g_nOffsetX = max(rcClient.right - nDrawWidth, min(g_nOffsetX, 0));
                        g_nOffsetY = max(rcClient.bottom - nDrawHeight, min(g_nOffsetY, 0));
                    }
                    InvalidateRect(hwnd, NULL, TRUE);  // 刷新窗口
                    break;
                    
                case '0':  // "0" 键
                    g_dScale = 1.0;  // 恢复到原始大小
                    g_nOffsetX = 0;
                    g_nOffsetY = 0;
                    InvalidateRect(hwnd, NULL, TRUE);  // 刷新窗口
                    break;
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // 获取窗口客户区尺寸
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            // 计算显示尺寸
            int nDrawWidth = (int)(g_nPhysicalWidth * g_dScale);
            int nDrawHeight = (int)(g_nPhysicalHeight * g_dScale);
            
            // 计算显示位置
            int nDrawX = g_nOffsetX;
            int nDrawY = g_nOffsetY;
            
            // 确保显示在窗口内
            nDrawX = max(rcClient.right - nDrawWidth, min(nDrawX, 0));
            nDrawY = max(rcClient.bottom - nDrawHeight, min(nDrawY, 0));
            
            // 绘制截图
            if (g_hBitmap != NULL) {
                HDC hMemDC = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, g_hBitmap);
                
                SetStretchBltMode(hdc, HALFTONE);  // 高质量缩放
                StretchBlt(hdc, nDrawX, nDrawY, nDrawWidth, nDrawHeight,
                          hMemDC, 0, 0, g_nPhysicalWidth, g_nPhysicalHeight, SRCCOPY);
                
                SelectObject(hMemDC, hOldBitmap);
                DeleteDC(hMemDC);
            }
            
            // 显示当前缩放比例
            char szScale[32];
            sprintf(szScale, "缩放比例: %.1f%%", g_dScale * 100);
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, 10, 10, szScale, strlen(szScale));
            
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            // 获取窗口客户区尺寸
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            // 计算显示尺寸
            int nDrawWidth = (int)(g_nPhysicalWidth * g_dScale);
            int nDrawHeight = (int)(g_nPhysicalHeight * g_dScale);
            
            // 计算显示位置
            int nDrawX = g_nOffsetX;
            int nDrawY = g_nOffsetY;
            
            // 将窗口坐标转换为实际屏幕坐标
            int x = (int)((LOWORD(lParam) - nDrawX) / g_dScale);
            int y = (int)((HIWORD(lParam) - nDrawY) / g_dScale);
            
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
    
    // 获取实际物理屏幕分辨率
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

    // 创建全屏窗口
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        "颜色拾取器 - 按+放大/-缩小/0恢复原始大小",
        WS_POPUP,
        0, 0, nPhysicalWidth, nPhysicalHeight,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd) {
        SetProcessDPIAware();  // 禁用DPI缩放
        
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