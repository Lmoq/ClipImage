#ifndef __HOTKEY_H__
#define __HOTKEY_H__

#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#define WM_EXIT WM_APP + 1

#include <unordered_map>
#include <unordered_set>
#include <thread>
#include <cmath>
#include <iostream>

inline HHOOK hook = NULL;
inline KBDLLHOOKSTRUCT *kbd = NULL;

inline LRESULT CALLBACK KeyProc( int nCode, WPARAM wParam, LPARAM lParam );
inline LRESULT CALLBACK ClipWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam );
inline BOOL WINAPI consoleHandler( DWORD ctrlType );

/// Class that install hook and listen for hotkeys\n
/// \note - This class runs independently in a separate thread that automatically 
/// \note   terminates on system shutdown or via closing console window
/// \note - If compiled with no console flag, and you want to terminate program anytime, 
/// \note   explicitly add a terminate hotkey that calls Hotkey::terminate()
/// \note - Usage :
/// \note   - Populate hotkeymap with add_hotkey()
/// \note   - Run listener with Hotkey::run() and call Hotkey::wait() at the
/// \note     end of main thread if there are no other 
/// \note     thread running besides main to avoid exceptions
/// \note   - Terminate Hotkey sample :
/// \note       - Hotkey::add_hotkey( {VK_NUMPAD0}, Hotkey::terminate, TRUE );
class Hotkey
{
public:

    struct hotkey
    {
        std::unordered_set<DWORD> keyCodes;
        BOOL active = FALSE;
        BOOL ignoreKeypress = FALSE;

        void ( *on_press )( ) = NULL;
        void ( *on_release )( ) = NULL;
    };
    inline static hotkey *current_active = NULL;
    inline static DWORD prime = 199;

    inline static BOOL ignoreKeypress = FALSE;

    inline static std::thread thread_;
    inline static DWORD threadID;

    inline static HWND hWnd;

    inline static std::unordered_set<DWORD> keyPressed;
    inline static std::unordered_map<DWORD, hotkey> map;

    /// @brief Populate hotkey map. Remember to add terminate hotey
    /// @param keyCodes Sets of keyCodes or single keyCode.
    /// @param on_press Callback function to call when keyCodes are pressed, return type should be void.
    /// @param on_release Callback function to call when keyCodes are pressed, return type should be void.
    /// @param ignoreKeypress If true, hotkey or single key presses will not be sent to foreground window.
    inline static void add_hotkey( std::unordered_set<DWORD> keyCodes, void ( *on_press )( ), void ( *on_release )( ), BOOL ignoreKeypress )
    {
        hotkey hkey;
        hkey.keyCodes = keyCodes;

        hkey.on_press = on_press;
        hkey.on_release = on_release;

        hkey.ignoreKeypress = ignoreKeypress;
        DWORD hash = hash_keycodes( keyCodes );
        if ( hotkey_exist( hash ) )
        {
            return;
        }
        map[hash] = hkey;
    }

    inline static DWORD hash_keycodes( std::unordered_set<DWORD> keyCodes )
    {
        DWORD hash = 0;
        for ( auto &key : keyCodes ) {
            hash += ( static_cast<int>( std::pow( key, 4 ) ) ^ prime ) / key;
        }
        return hash % 60000;
    }

    inline static BOOL hotkey_exist( DWORD hash )
    {
        if ( map.find( hash ) != map.end() ) {
            return TRUE;
        }
        return FALSE;
    }

    inline static void keydown( DWORD keyCode )
    {
        keyPressed.insert( keyCode );
        DWORD hash = hash_keycodes( keyPressed );

        if ( !hotkey_exist( hash ) ) {
            return;
        }

        hotkey &hkey = Hotkey::map[hash];
        if ( hkey.active == FALSE )
        {
            current_active = &hkey;
            hkey.active = TRUE;
            hkey.on_press();

            ignoreKeypress = hkey.ignoreKeypress;
        }
    }

    inline static void keyup( DWORD keyCode )
    {
        if ( keyPressed.count( keyCode ) ) {
            keyPressed.erase( keyCode );

            if ( current_active != NULL )
            {
                current_active->active = FALSE;
                if ( current_active->on_release != NULL )
                {
                    current_active->on_release();
                }
                current_active = NULL;
                ignoreKeypress = FALSE;
            }
        }
    }

    inline static void terminate();

    inline static void run() {
        thread_ = std::thread( keyhook );
    }

    inline static void wait()
    {
        if ( thread_.joinable() )
            thread_.join();
    }

    inline static void keyhook()
    {
        const CHAR className[] = "macazudon";
        WNDCLASSA wc = {};

        wc.lpszClassName = className;
        wc.lpfnWndProc = &ClipWndProc;

        RegisterClassA( &wc );

        hWnd = CreateWindowExA(
            0, wc.lpszClassName, "Winproc", 0,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            CW_USEDEFAULT,
            NULL,
            NULL,
            NULL,
            NULL
        );
        if ( !hWnd ) {
            MessageBoxA( NULL, "CreateWindow failed", "Error", MB_OK );
            return;
        }
        hook = SetWindowsHookEx( WH_KEYBOARD_LL, &KeyProc, 0, 0 );
        if ( !hook ) {
            MessageBoxA( NULL, "Hook failed", "Error", MB_OK );
            return;
        }
        /*if ( !SetConsoleCtrlHandler( &consoleHandler, TRUE ) ) {
            MessageBoxA( NULL, "ConsoleHandler failed", "Error", MB_OK );
            return;
        }*/

        ShowWindow( hWnd, SW_HIDE );
        threadID = GetCurrentThreadId();

        msgLoop();
        UnhookWindowsHookEx( hook );
    }

    inline static void msgLoop()
    {
        MSG msg;
        while ( GetMessage( &msg, NULL, 0, 0 ) > 0 )
        {
            if ( msg.message == WM_EXIT ) {
                PostQuitMessage( 0 );
            }
            TranslateMessage( &msg );
            DispatchMessage( &msg );
        }
    }
};

BOOL WINAPI consoleHandler( DWORD ctrlType )
{
    switch ( ctrlType )
    {
    case CTRL_C_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_SHUTDOWN_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_BREAK_EVENT:
        printf( "SigClose\n" );
        Hotkey::terminate();
        Hotkey::wait();
        return TRUE;

    default:
        return FALSE;
    }
}


#endif
