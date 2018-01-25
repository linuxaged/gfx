#include "GpuWindow.hpp"
#include "GpuInstance.hpp"

namespace lxd
{

LRESULT APIENTRY WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam )
{
    auto window = reinterpret_cast<GpuWindow*>(GetWindowLongPtrA( hWnd, GWLP_USERDATA ));

    switch ( message )
    {
        case WM_SIZE:
        {
            if ( window != nullptr )
            {
                window->windowWidth  = static_cast<int>( LOWORD( lParam ) );
                window->windowHeight = static_cast<int>( HIWORD( lParam ) );
            }
            return 0;
        }
        case WM_ACTIVATE:
        {
            if ( window != nullptr )
            {
                window->windowActiveState = !HIWORD( wParam );
            }
            return 0;
        }
        case WM_ERASEBKGND:
        {
            return 0;
        }
        case WM_CLOSE:
        {
            PostQuitMessage( 0 );
            return 0;
        }
        case WM_KEYDOWN:
        {
			if (nullptr != window)
			{
				if (static_cast<int>(wParam) >= 0 && static_cast<int>(wParam) < 256)
				{
					if (static_cast<int>(wParam) != KEY_SHIFT_LEFT && static_cast<int>(wParam) != KEY_CTRL_LEFT &&
						static_cast<int>(wParam) != KEY_ALT_LEFT && static_cast<int>(wParam) != KEY_CURSOR_UP &&
						static_cast<int>(wParam) != KEY_CURSOR_DOWN && static_cast<int>(wParam) != KEY_CURSOR_LEFT &&
						static_cast<int>(wParam) != KEY_CURSOR_RIGHT)
					{
						window->input.keyInput[static_cast<int>(wParam)] = true;
					}
				}
			}
            break;
        }
        case WM_LBUTTONDOWN:
        {
            window->input.mouseInput[MOUSE_LEFT]  = true;
            window->input.mouseInputX[MOUSE_LEFT] = LOWORD( lParam );
            window->input.mouseInputY[MOUSE_LEFT] = window->windowHeight - HIWORD( lParam );
            break;
        }
        case WM_RBUTTONDOWN:
        {
            window->input.mouseInput[MOUSE_RIGHT]  = true;
            window->input.mouseInputX[MOUSE_RIGHT] = LOWORD( lParam );
            window->input.mouseInputY[MOUSE_RIGHT] = window->windowHeight - HIWORD( lParam );
            break;
        }
    }
    return DefWindowProcA( hWnd, message, wParam, lParam );
}

GpuWindow::WindowHandles GpuWindow::GetWindowHandle( bool fullScreen, float* refreshRate, int width,
                                                     int height )
{
    HWND         hwnd;
    HINSTANCE    hinstance;
    const LPCSTR displayDevice = nullptr;

    if ( fullScreen )
    {
        DEVMODEA dmScreenSettings;
        memset( &dmScreenSettings, 0, sizeof( dmScreenSettings ) );
        dmScreenSettings.dmSize       = sizeof( dmScreenSettings );
        dmScreenSettings.dmPelsWidth  = width;
        dmScreenSettings.dmPelsHeight = height;
        dmScreenSettings.dmBitsPerPel = 32;
        dmScreenSettings.dmFields     = DM_PELSWIDTH | DM_PELSHEIGHT | DM_BITSPERPEL;

        if ( ChangeDisplaySettingsExA( displayDevice, &dmScreenSettings, nullptr, CDS_FULLSCREEN,
                                       nullptr ) != DISP_CHANGE_SUCCESSFUL )
        {
            Error( "The requested fullscreen mode is not supported." );
        }
    }

    DEVMODEA lpDevMode;
    memset( &lpDevMode, 0, sizeof( DEVMODEA ) );
    lpDevMode.dmSize        = sizeof( DEVMODEA );
    lpDevMode.dmDriverExtra = 0;

    if ( EnumDisplaySettingsA( displayDevice, ENUM_CURRENT_SETTINGS, &lpDevMode ) != FALSE )
    {
        *refreshRate = (float)lpDevMode.dmDisplayFrequency;
    }

    hinstance = GetModuleHandleA( nullptr );

    WNDCLASSA wc;
    wc.style         = CS_HREDRAW | CS_VREDRAW | CS_OWNDC;
    wc.lpfnWndProc   = (WNDPROC)WndProc;
    wc.cbClsExtra    = 0;
    wc.cbWndExtra    = 0;
    wc.hInstance     = hinstance;
    wc.hIcon         = LoadIcon( nullptr, IDI_WINLOGO );
    wc.hCursor       = LoadCursor( nullptr, IDC_ARROW );
    wc.hbrBackground = nullptr;
    wc.lpszMenuName  = nullptr;
    wc.lpszClassName = "gfx_app";

	auto result = RegisterClassA(&wc);
	assert( 0 != result );

    DWORD dwExStyle = 0;
    DWORD dwStyle   = 0;
    if ( fullScreen )
    {
        dwExStyle = WS_EX_APPWINDOW;
        dwStyle   = WS_POPUP;
        ShowCursor( FALSE );
    }
    else
    {
        // Fixed size window.
        dwExStyle = WS_EX_APPWINDOW | WS_EX_WINDOWEDGE;
        dwStyle   = WS_OVERLAPPED | WS_CAPTION | WS_SYSMENU | WS_MINIMIZEBOX;
    }

    RECT windowRect;
    windowRect.left   = static_cast<LONG>(0);
    windowRect.right  = static_cast<LONG>( width );
    windowRect.top    = static_cast<LONG>(0);
    windowRect.bottom = static_cast<LONG>( height );

    AdjustWindowRectEx( &windowRect, dwStyle, FALSE, dwExStyle );

    if ( !fullScreen )
    {
        RECT desktopRect;
        GetWindowRect( GetDesktopWindow(), &desktopRect );

        const int offsetX = ( desktopRect.right - ( windowRect.right - windowRect.left ) ) / 2;
        const int offsetY = ( desktopRect.bottom - ( windowRect.bottom - windowRect.top ) ) / 2;

        windowRect.left += offsetX;
        windowRect.right += offsetX;
        windowRect.top += offsetY;
        windowRect.bottom += offsetY;
    }

    hwnd = CreateWindowExA( dwExStyle,                          // Extended style for the window
                            "gfx_app",                          // Class name
                            "gfx_app",                          // Window title
                            dwStyle |                           // Defined window style
                                WS_CLIPSIBLINGS |               // Required window style
                                WS_CLIPCHILDREN,                // Required window style
                            windowRect.left,                    // Window X position
                            windowRect.top,                     // Window Y position
                            windowRect.right - windowRect.left, // Window width
                            windowRect.bottom - windowRect.top, // Window height
                            nullptr,                               // No parent window
                            nullptr,                               // No menu
                            hinstance,                          // Instance
                            nullptr );                             // No WM_CREATE parameter
	assert( nullptr != hwnd );

    SetWindowLongPtrA( hwnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(this) );

    return GpuWindow::WindowHandles{ hinstance, hwnd };
}

VkSurfaceKHR GpuWindow::CreateSurface( GpuInstance*                       instance,
                                       const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
                                       const VkAllocationCallbacks*       pAllocator )
{
    VkSurfaceKHR surface;
    VK( instance->vkCreateSurfaceKHR( instance->instance, pCreateInfo, pAllocator, &surface ) );
    return surface;
}
GpuWindow::GpuWindow( GpuInstance* instance, const GpuQueueInfo* queueInfo, const int queueIndex,
                      const GpuSurfaceColorFormat colorFormat,
                      const GpuSurfaceDepthFormat depthFormat, const GpuSampleCount sampleCount,
                      const int width, const int height, const bool fullscreen )
    : windowHandles( GetWindowHandle( fullscreen, &this->windowRefreshRate, width, height ) ),
      win32SurfaceCreateInfo{ VK_STRUCTURE_TYPE_WIN32_SURFACE_CREATE_INFO_KHR, nullptr, 0,
                              windowHandles.hInstance, windowHandles.hWnd },
      surface( CreateSurface( instance, &win32SurfaceCreateInfo, VK_ALLOCATOR ) ),
      device( instance, queueInfo, surface ),
      context( &device, queueIndex ),
      swapchain( &context, surface, colorFormat, width, height, 1 ),
      depthBuffer( &context, depthFormat, sampleCount, width, height, 1 )
{
    this->colorFormat        = colorFormat;
    this->depthFormat        = depthFormat;
    this->sampleCount        = sampleCount;
    this->windowWidth        = width;
    this->windowHeight       = height;
    this->windowSwapInterval = 1;
    this->windowRefreshRate  = 60.0f;
    this->windowFullscreen   = fullscreen;
    this->windowActive       = false;
    this->windowExit         = false;
    this->windowActiveState  = false;
    this->lastSwapTime       = GetTimeNanoseconds();

#if defined( OS_APPLE )
    this->windowWidth  = this->swapchain.width;  // iOS/macOS patch for Retina displays
    this->windowHeight = this->swapchain.height; // iOS/macOS patch for Retina displays
#endif

    assert( this->swapchain.width == this->windowWidth &&
            this->swapchain.height == this->windowHeight );

    this->colorFormat = this->swapchain.format; // May not have acquired the desired format.
    this->depthFormat = this->depthBuffer.format;
    this->swapchainCreateCount++;

    ShowWindow( this->windowHandles.hWnd, SW_SHOW );
    SetForegroundWindow( this->windowHandles.hWnd );
    SetFocus( this->windowHandles.hWnd );
}

GpuWindow::~GpuWindow()
{
    VC( this->device.instance->vkDestroySurfaceKHR( this->device.instance->instance, this->surface,
                                                    VK_ALLOCATOR ) );

    if ( this->windowFullscreen )
    {
        ChangeDisplaySettingsA( nullptr, 0 );
        ShowCursor( TRUE );
    }

    if ( nullptr != this->windowHandles.hWnd )
    {
        if ( FALSE == DestroyWindow( this->windowHandles.hWnd ) )
        {
            Error( "Failed to destroy the window." );
        }
        this->windowHandles.hWnd = nullptr;
    }

    if ( this->windowHandles.hInstance )
    {
        if ( FALSE == UnregisterClassA( "gfx_app", this->windowHandles.hInstance ) )
        {
            Error( "Failed to unregister window class." );
        }
        this->windowHandles.hInstance = nullptr;
    }
}

bool GpuWindow::SupportedResolution( const int width, const int height )
{
    DEVMODE dm = { 0 };
    dm.dmSize  = sizeof( dm );
    for ( int modeIndex = 0; EnumDisplaySettings( nullptr, modeIndex, &dm ) != 0; modeIndex++ )
    {
        if ( dm.dmPelsWidth == static_cast<DWORD>(width) && dm.dmPelsHeight == static_cast<DWORD>(height) )
        {
            return true;
        }
    }
    return false;
}

void GpuWindow::Exit() { this->windowExit = true; }

GpuWindowEvent GpuWindow::ProcessEvents()
{
    MSG msg;
    while ( PeekMessage( &msg, nullptr, 0, 0, PM_REMOVE ) > 0 )
    {
        if ( msg.message == WM_QUIT )
        {
            this->windowExit = true;
        }
        else
        {
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }

    this->input.keyInput[KEY_SHIFT_LEFT]   = static_cast<bool>(GetAsyncKeyState( KEY_SHIFT_LEFT ));
    this->input.keyInput[KEY_CTRL_LEFT]    = static_cast<bool>(GetAsyncKeyState( KEY_CTRL_LEFT ));
    this->input.keyInput[KEY_ALT_LEFT]     = static_cast<bool>(GetAsyncKeyState( KEY_ALT_LEFT ));
    this->input.keyInput[KEY_CURSOR_UP]    = static_cast<bool>(GetAsyncKeyState( KEY_CURSOR_UP ));
    this->input.keyInput[KEY_CURSOR_DOWN]  = static_cast<bool>(GetAsyncKeyState( KEY_CURSOR_DOWN ));
    this->input.keyInput[KEY_CURSOR_LEFT]  = static_cast<bool>(GetAsyncKeyState( KEY_CURSOR_LEFT ));
    this->input.keyInput[KEY_CURSOR_RIGHT] = static_cast<bool>(GetAsyncKeyState( KEY_CURSOR_RIGHT ));

    if ( this->windowExit )
    {
        return GPU_WINDOW_EVENT_EXIT;
    }
    if ( this->windowActiveState != this->windowActive )
    {
        this->windowActive = this->windowActiveState;
        return ( this->windowActiveState ) ? GPU_WINDOW_EVENT_ACTIVATED
                                           : GPU_WINDOW_EVENT_DEACTIVATED;
    }
    return GPU_WINDOW_EVENT_NONE;
}

} // namespace lxd
