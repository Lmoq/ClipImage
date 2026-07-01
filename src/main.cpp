#include <iostream>
#include <algorithm>
#include <thread>
#include <deque>
#include <chrono>
#include <ctime>
#include <hotkey.hpp>
#include <clip.hpp>


static BOOL bListening = FALSE;

int WINAPI WinMain( _In_ HINSTANCE hInstance, _In_opt_ HINSTANCE hPrevInstance, _In_ LPSTR lpCmdLine, _In_ int nShowCmd )
{
    fullscreen_only = false;
    update_interval = 100;

    system_screen_width = GetSystemMetrics( SM_CXSCREEN );
    system_screen_height = GetSystemMetrics( SM_CYSCREEN );

    Hotkey::add_hotkey( { VK_LCONTROL, VK_F10 }, Hotkey::terminate, NULL, TRUE );
    Hotkey::add_hotkey( { VK_LCONTROL, VK_UP, VK_DOWN }, [](){ std::thread( togglePFullscreen ).detach(); }, NULL, TRUE );
    Hotkey::add_hotkey( { VK_OEM_PERIOD, VK_UP }, []() { std::thread( showClipImage ).detach(); }, NULL, TRUE );
    Hotkey::add_hotkey( { VK_LCONTROL, VK_OEM_3 }, [](){ std::thread( togglePAutosave ).detach(); }, NULL, TRUE);
    
    Hotkey::run();
    Hotkey::wait();
    return 0;
}

LRESULT CALLBACK ClipWndProc( HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam )
{
    switch ( uMsg )
    {
        case WM_CREATE:
            bListening = AddClipboardFormatListener( hWnd );
            return bListening ? 0 : -1;

        case WM_DESTROY:
            if ( bListening )
            {
                RemoveClipboardFormatListener( hWnd );
                bListening = FALSE;
            }
            return 0;

        case WM_QUERYENDSESSION:
            Hotkey::terminate();
            return TRUE;

        case WM_ENDSESSION:
            Hotkey::wait();
            return 0;

        case WM_CLIPBOARDUPDATE:
            if ( !getLatestImage() ) {
                return 0;
            }
            if ( !spawned_imageThread ) 
            {
                spawned_imageThread = true;
                std::thread( imageWriteThread ).detach();
            }
            return 0;
    }
    return DefWindowProc( hWnd, uMsg, wParam, lParam );
}

LRESULT CALLBACK KeyProc( int nCode, WPARAM wParam, LPARAM lParam )
{
    if ( nCode == HC_ACTION ) {
        kbd = reinterpret_cast<KBDLLHOOKSTRUCT *>( lParam );

        switch ( wParam )
        {
        case WM_KEYDOWN:
            Hotkey::keydown( kbd->vkCode );
            break;

        case WM_KEYUP:
            Hotkey::keyup( kbd->vkCode );
            break;

        default:
            break;
        }
    }
    return ( Hotkey::ignoreKeypress ) ? -1 : CallNextHookEx( NULL, nCode, wParam, lParam );
}

void togglePFullscreen()
{
    fullscreen_only = !fullscreen_only;
    std::string buffer; buffer.resize( 128 );

    sprintf( buffer.data(), "Save only fullscreen snips : %s", fullscreen_only ? "true" : "false" );
    MessageBox( Hotkey::hWnd, buffer.c_str(), "ClipImage", MB_OK );
}

void togglePAutosave()
{
    autoSave = !autoSave;
    std::string buffer; buffer.resize( 128 );

    sprintf( buffer.data(), "AutoSave : %s", autoSave ? "true" : "false" );
    MessageBox( Hotkey::hWnd, buffer.c_str(), "ClipImage", MB_OK );
}

std::vector<int> abortKeys{ VK_ESCAPE, VK_BACK, -1 };
void showClipImage()
{
    if ( !autoSave && savedImageArray ) 
    {
        cv::imshow( "ClipImage", image_array );
        int keycode = cv::waitKeyEx( 0 );

        if ( std::find( abortKeys.begin(), abortKeys.end(), keycode ) == abortKeys.end() ) {
            writeImageToFile();
        }
        cv::destroyAllWindows();
    }
}

void Hotkey::terminate()
{
    if ( bListening ) 
    {
        RemoveClipboardFormatListener( hWnd );
        bListening = FALSE;
    }
    PostThreadMessage( threadID, WM_EXIT, 0, 0 );
}