#pragma once

#include "GpuDevice.hpp"
#include "GpuContext.hpp"
#include "GpuSwapChain.hpp"
#include "GpuBuffer.hpp"

namespace lxd
{
enum GpuWindowEvent
{
    GPU_WINDOW_EVENT_NONE,
    GPU_WINDOW_EVENT_ACTIVATED,
    GPU_WINDOW_EVENT_DEACTIVATED,
    GPU_WINDOW_EVENT_EXIT
};

struct GpuWindowInput
{
    bool keyInput[256];
    bool mouseInput[8];
    int  mouseInputX[8];
    int  mouseInputY[8];
};

#if defined( OS_WINDOWS )
typedef enum
{
    KEY_A            = 0x41,
    KEY_B            = 0x42,
    KEY_C            = 0x43,
    KEY_D            = 0x44,
    KEY_E            = 0x45,
    KEY_F            = 0x46,
    KEY_G            = 0x47,
    KEY_H            = 0x48,
    KEY_I            = 0x49,
    KEY_J            = 0x4A,
    KEY_K            = 0x4B,
    KEY_L            = 0x4C,
    KEY_M            = 0x4D,
    KEY_N            = 0x4E,
    KEY_O            = 0x4F,
    KEY_P            = 0x50,
    KEY_Q            = 0x51,
    KEY_R            = 0x52,
    KEY_S            = 0x53,
    KEY_T            = 0x54,
    KEY_U            = 0x55,
    KEY_V            = 0x56,
    KEY_W            = 0x57,
    KEY_X            = 0x58,
    KEY_Y            = 0x59,
    KEY_Z            = 0x5A,
    KEY_RETURN       = VK_RETURN,
    KEY_TAB          = VK_TAB,
    KEY_ESCAPE       = VK_ESCAPE,
    KEY_SHIFT_LEFT   = VK_LSHIFT,
    KEY_CTRL_LEFT    = VK_LCONTROL,
    KEY_ALT_LEFT     = VK_LMENU,
    KEY_CURSOR_UP    = VK_UP,
    KEY_CURSOR_DOWN  = VK_DOWN,
    KEY_CURSOR_LEFT  = VK_LEFT,
    KEY_CURSOR_RIGHT = VK_RIGHT
} KeyboardKey;

typedef enum
{
    MOUSE_LEFT  = 0,
    MOUSE_RIGHT = 1
} MouseButton;

#endif

class GpuWindow
{
  public:
    GpuWindow( GpuInstance* instance, const GpuQueueInfo* queueInfo, const int queueIndex,
               const GpuSurfaceColorFormat colorFormat, const GpuSurfaceDepthFormat depthFormat,
               const GpuSampleCount sampleCount, const int width, const int height,
               const bool fullscreen );
    ~GpuWindow();

  public:
    //
    // Rendering
    //
    void          SwapInterval( const int swapInterval );
    void          SwapBuffers();
    ksNanoseconds GetNextSwapTimeNanoseconds();
    ksNanoseconds GetFrameTimeNanoseconds();
    void          DelayBeforeSwap( const ksNanoseconds delay );
    bool          ConsumeKeyboardKey( GpuWindowInput* input, const KeyboardKey key );
    bool          ConsumeMouseButton( GpuWindowInput* input, const MouseButton button );
    bool          CheckKeyboardKey( GpuWindowInput* input, const KeyboardKey key );

  public:
    //
    // Platform special
    //
    static LRESULT APIENTRY WndProc( HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam );
    bool                    SupportedResolution( const int width, const int height );
    void                    Exit();
    GpuWindowEvent          ProcessEvents();

  private:
    struct WindowHandles
    {
        HINSTANCE hInstance;
        HWND      hWnd;
    };
    WindowHandles GetWindowHandle( bool fullScreen, float* refreshRate, int width, int height );
    VkSurfaceKHR  CreateSurface( GpuInstance*                       instance,
                                 const VkWin32SurfaceCreateInfoKHR* pCreateInfo,
                                 const VkAllocationCallbacks*       pAllocator );

  public:
    GpuDevice             device;
    GpuContext            context;
    GpuSurfaceColorFormat colorFormat;
    GpuSurfaceDepthFormat depthFormat;
    GpuSampleCount        sampleCount;
    int                   windowWidth;
    int                   windowHeight;
    int                   windowSwapInterval;
    float                 windowRefreshRate;
    bool                  windowFullscreen;
    bool                  windowActive;
    bool                  windowExit;
    GpuWindowInput        input;
    ksNanoseconds         lastSwapTime;

    // The swapchain and depth buffer could be stored on the context like OpenGL but this makes more sense.
    VkSurfaceKHR   surface;
    int            swapchainCreateCount;
    GpuSwapchain   swapchain;
    GpuDepthBuffer depthBuffer;

#if defined( OS_WINDOWS )
    WindowHandles               windowHandles;
    bool                        windowActiveState;
    VkWin32SurfaceCreateInfoKHR win32SurfaceCreateInfo;

#elif defined( OS_LINUX_XLIB )
    Display* xDisplay;
    int      xScreen;
    Window   xRoot;
    Window   xWindow;
    int      desktopWidth;
    int      desktopHeight;
    float    desktopRefreshRate;
#elif defined( OS_LINUX_XCB )
    xcb_connection_t*  connection;
    xcb_screen_t*      screen;
    xcb_window_t       window;
    xcb_atom_t         wm_delete_window_atom;
    xcb_key_symbols_t* key_symbols;
    int                desktopWidth;
    int                desktopHeight;
    float              desktopRefreshRate;
#elif defined( OS_APPLE_MACOS )
    CGDirectDisplayID display;
    CGDisplayModeRef  desktopDisplayMode;
    NSWindow*         nsWindow;
    NSView*           nsView;
#elif defined( OS_APPLE_IOS )
    UIWindow* uiWindow;
    UIView*   uiView;
#elif defined( OS_ANDROID )
    struct android_app* app;
    Java_t              java;
    ANativeWindow*      nativeWindow;
    bool                resumed;
#elif defined( OS_NEUTRAL_DISPLAY_SURFACE )
    VkDisplayKHR     display;
    VkDisplayModeKHR mode;
#endif
};
} // namespace lxd
