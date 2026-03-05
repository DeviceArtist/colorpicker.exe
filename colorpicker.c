#include <windows.h>
#include <stdio.h>

// Global variables
HBITMAP g_hBitmap = NULL;
int g_nPhysicalWidth = 0, g_nPhysicalHeight = 0;  // Actual physical screen resolution
double g_dScale = 1.0;  // Zoom scale, default 1.0 (original size)
int g_nOffsetX = 0, g_nOffsetY = 0;  // Screenshot offset for center-based zoom
POINT g_ptLastMouse;  // Last mouse position

// Window procedure
LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
    switch (uMsg) {
        case WM_CREATE: {
            // Get actual physical screen resolution
            DEVMODE dm = {0};
            dm.dmSize = sizeof(DEVMODE);
            EnumDisplaySettings(NULL, ENUM_CURRENT_SETTINGS, &dm);
            g_nPhysicalWidth = dm.dmPelsWidth;
            g_nPhysicalHeight = dm.dmPelsHeight;
            
            // Get screen DC and create screenshot
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
            
            // Set cursor to crosshair
            SetClassLongPtr(hwnd, GCLP_HCURSOR, 
                (LONG_PTR)LoadCursor(NULL, MAKEINTRESOURCE(32515))); // IDC_CROSS
            return 0;
        }

        case WM_MOUSEMOVE: {
            // Record mouse position
            g_ptLastMouse.x = LOWORD(lParam);
            g_ptLastMouse.y = HIWORD(lParam);
            return 0;
        }

        case WM_KEYDOWN: {
            switch (wParam) {
                case VK_ADD:     // "+" key on numeric keypad
                case VK_OEM_PLUS: // "+" key on main keyboard
                    {
                        // Calculate mouse position on the original screen
                        double mouseXOnScreen = (g_ptLastMouse.x - g_nOffsetX) / g_dScale;
                        double mouseYOnScreen = (g_ptLastMouse.y - g_nOffsetY) / g_dScale;
                        
                        // Save current scale
                        double oldScale = g_dScale;
                        
                        // Adjust zoom scale
                        g_dScale *= 1.2;  // Zoom in by 20% each time
                        if (g_dScale > 4.0) g_dScale = 4.0;  // Maximum zoom is 4x
                        
                        // Calculate new offset to keep mouse position centered
                        g_nOffsetX = (int)(g_ptLastMouse.x - mouseXOnScreen * g_dScale);
                        g_nOffsetY = (int)(g_ptLastMouse.y - mouseYOnScreen * g_dScale);
                        
                        // Ensure screenshot is fully visible within window
                        int nDrawWidth = (int)(g_nPhysicalWidth * g_dScale);
                        int nDrawHeight = (int)(g_nPhysicalHeight * g_dScale);
                        
                        RECT rcClient;
                        GetClientRect(hwnd, &rcClient);
                        
                        g_nOffsetX = max(rcClient.right - nDrawWidth, min(g_nOffsetX, 0));
                        g_nOffsetY = max(rcClient.bottom - nDrawHeight, min(g_nOffsetY, 0));
                    }
                    InvalidateRect(hwnd, NULL, TRUE);  // Refresh window
                    break;
                    
                case VK_SUBTRACT: // "-" key on numeric keypad
                case VK_OEM_MINUS: // "-" key on main keyboard
                    {
                        // Do nothing if already at original size
                        if (g_dScale <= 1.0) {
                            break;
                        }
                        
                        // Calculate mouse position on the original screen
                        double mouseXOnScreen = (g_ptLastMouse.x - g_nOffsetX) / g_dScale;
                        double mouseYOnScreen = (g_ptLastMouse.y - g_nOffsetY) / g_dScale;
                        
                        // Save current scale
                        double oldScale = g_dScale;
                        
                        // Adjust zoom scale
                        g_dScale /= 1.2;  // Zoom out by 20% each time
                        if (g_dScale < 0.25) g_dScale = 0.25;  // Minimum zoom is 25%
                        
                        // Calculate new offset to keep mouse position centered
                        g_nOffsetX = (int)(g_ptLastMouse.x - mouseXOnScreen * g_dScale);
                        g_nOffsetY = (int)(g_ptLastMouse.y - mouseYOnScreen * g_dScale);
                        
                        // Ensure screenshot is fully visible within window
                        int nDrawWidth = (int)(g_nPhysicalWidth * g_dScale);
                        int nDrawHeight = (int)(g_nPhysicalHeight * g_dScale);
                        
                        RECT rcClient;
                        GetClientRect(hwnd, &rcClient);
                        
                        g_nOffsetX = max(rcClient.right - nDrawWidth, min(g_nOffsetX, 0));
                        g_nOffsetY = max(rcClient.bottom - nDrawHeight, min(g_nOffsetY, 0));
                    }
                    InvalidateRect(hwnd, NULL, TRUE);  // Refresh window
                    break;
                    
                case '0':  // "0" key
                    g_dScale = 1.0;  // Restore to original size
                    g_nOffsetX = 0;
                    g_nOffsetY = 0;
                    InvalidateRect(hwnd, NULL, TRUE);  // Refresh window
                    break;
            }
            return 0;
        }

        case WM_PAINT: {
            PAINTSTRUCT ps;
            HDC hdc = BeginPaint(hwnd, &ps);
            
            // Get client area dimensions
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            // Calculate display dimensions
            int nDrawWidth = (int)(g_nPhysicalWidth * g_dScale);
            int nDrawHeight = (int)(g_nPhysicalHeight * g_dScale);
            
            // Calculate display position
            int nDrawX = g_nOffsetX;
            int nDrawY = g_nOffsetY;
            
            // Ensure display is within window bounds
            nDrawX = max(rcClient.right - nDrawWidth, min(nDrawX, 0));
            nDrawY = max(rcClient.bottom - nDrawHeight, min(nDrawY, 0));
            
            // Draw screenshot
            if (g_hBitmap != NULL) {
                HDC hMemDC = CreateCompatibleDC(hdc);
                HBITMAP hOldBitmap = (HBITMAP)SelectObject(hMemDC, g_hBitmap);
                
                SetStretchBltMode(hdc, HALFTONE);  // High-quality scaling
                StretchBlt(hdc, nDrawX, nDrawY, nDrawWidth, nDrawHeight,
                          hMemDC, 0, 0, g_nPhysicalWidth, g_nPhysicalHeight, SRCCOPY);
                
                SelectObject(hMemDC, hOldBitmap);
                DeleteDC(hMemDC);
            }
            
            // Display current zoom scale
            char szScale[32];
            sprintf(szScale, "Zoom: %.1f%%", g_dScale * 100);
            SetTextColor(hdc, RGB(255, 255, 255));
            SetBkMode(hdc, TRANSPARENT);
            TextOut(hdc, 10, 10, szScale, strlen(szScale));
            
            EndPaint(hwnd, &ps);
            return 0;
        }

        case WM_LBUTTONDOWN: {
            // Get client area dimensions
            RECT rcClient;
            GetClientRect(hwnd, &rcClient);
            
            // Calculate display dimensions
            int nDrawWidth = (int)(g_nPhysicalWidth * g_dScale);
            int nDrawHeight = (int)(g_nPhysicalHeight * g_dScale);
            
            // Calculate display position
            int nDrawX = g_nOffsetX;
            int nDrawY = g_nOffsetY;
            
            // Convert window coordinates to actual screen coordinates
            int x = (int)((LOWORD(lParam) - nDrawX) / g_dScale);
            int y = (int)((HIWORD(lParam) - nDrawY) / g_dScale);
            
            // Ensure coordinates are within screen bounds
            x = max(0, min(x, g_nPhysicalWidth - 1));
            y = max(0, min(y, g_nPhysicalHeight - 1));
            
            // Get color at actual screen coordinates
            HDC hScreenDC = GetDC(NULL);
            COLORREF color = GetPixel(hScreenDC, x, y);
            ReleaseDC(NULL, hScreenDC);
            
            // Format color string
            char szColor[64];
            sprintf(szColor, "RGB(%d,%d,%d) | #%02X%02X%02X", 
                GetRValue(color), GetGValue(color), GetBValue(color),
                GetRValue(color), GetGValue(color), GetBValue(color));
            
            // Copy to clipboard
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
    const char CLASS_NAME[] = "ColorPicker";
    
    // Get actual physical screen resolution
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
        MessageBox(NULL, "Failed to register window class!", "Error", MB_ICONERROR);
        return 1;
    }

    // Create fullscreen window
    HWND hwnd = CreateWindowEx(
        WS_EX_TOPMOST | WS_EX_TOOLWINDOW,
        CLASS_NAME,
        "Color Picker - Press + to zoom in, - to zoom out, 0 to reset",
        WS_POPUP,
        0, 0, nPhysicalWidth, nPhysicalHeight,
        NULL, NULL, hInstance, NULL
    );

    if (hwnd) {
        SetProcessDPIAware();  // Disable DPI scaling
        
        ShowWindow(hwnd, nCmdShow);
        UpdateWindow(hwnd);
        
        MSG msg;
        while (GetMessage(&msg, NULL, 0, 0)) {
            TranslateMessage(&msg);
            DispatchMessage(&msg);
        }
    } else {
        MessageBox(NULL, "Failed to create window!", "Error", MB_ICONERROR);
    }

    return 0;
}

// Cygwin compatible entry point
int main() {
    return WinMain(GetModuleHandle(NULL), NULL, GetCommandLine(), SW_SHOW);
}