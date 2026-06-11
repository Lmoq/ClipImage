#include <iostream>
#include <ctime>
#include <clip.hpp>
#include <vector>
#include <opencv2/opencv.hpp>

int system_screen_width;
int system_screen_height;

bool fullscreen_only;
bool spawned_imageThread = false;

long update_interval = 100;
std::deque<PBITMAPINFO> BInfo_Queue;

bool getLatestImage()
{
    UINT format = 0;
 
    char format_string[] = "CF_DIB";
    UINT target_format = CF_DIB;

    if ( !IsClipboardFormatAvailable( target_format ) ) {
        printf( "Format [%s] not available\n", format_string );
        return false;
    }
    else {
        printf( "Format [%s] available\n", format_string );
    }
    if ( !OpenClipboard( NULL ) ) {
        printf( "OpenClipboard failed, error code : %d\n", GetLastError() );
        return false;
    }

    // Populate the BInfo_Queue with bitmapinfo objects
    bool result = GetBits( target_format );

    if ( !CloseClipboard() ) {
        printf( "CloseClip failed, code : %d\n", GetLastError() );
        return false;
    }
    return result;
}

bool GetBits( UINT8 CF_FORMAT )
{
    HANDLE bHandle = GetClipboardData( CF_FORMAT );

    if ( bHandle == NULL )
    {
        printf( "Handle is null : %d\n", GetLastError() );
        return false;
    }
    else
    {
        PBITMAPINFO pBinfo = (PBITMAPINFO)GlobalLock( bHandle );
        if ( pBinfo == NULL ) {
            printf( "GlobalLock failed, code : %d\n", GetLastError() );
            return false;
        }
        GlobalUnlock( bHandle );

        // Append BITMAPINFO object to queue
        BInfo_Queue.push_back( pBinfo );
    }
    return true;
}

bool bitmapToImage( PBITMAPINFO pBinfo )
{
    BITMAPINFOHEADER header = pBinfo->bmiHeader;
    if ( fullscreen_only )
    {
        if ( !( system_screen_width == header.biWidth && system_screen_height == header.biHeight ) ) {
            printf( "Image is not fullscreen\n" );
            return false;
        }
    }
    // Read the bitmap as Mat image with the flag CV_8UC4 
    // to match the data type and include the alpha channel 
    cv::Mat image( (int)header.biHeight, (int)header.biWidth, CV_8UC4, (cv::uint8_t *)pBinfo->bmiColors );

    // Flip the bitmap image back to original
    cv::flip( image, image, 0 );

    // Set file destination
    std::string filename; filename.resize( 128 );

    std::time_t t = std::time( nullptr );
    size_t filename_size = std::strftime( filename.data(), filename.size(), "%Y-%b-%d %H-%M-%S.png", std::localtime( &t ) );

    // Retrieve username env variable
    std::string username; username.resize( 128 );

    DWORD result = GetEnvironmentVariable( "username", username.data(), username.size() );
    if ( result == 0 ) 
    {
        if ( GetLastError() == ERROR_ENVVAR_NOT_FOUND ) {
            printf( "Variable <%s> not found\n", "username" );
            spawned_imageThread = false;
            return false;
        }
    }
    else if ( username.size() < result )
    {
        printf( "Insufficient storage, bytes to write : %d\n", result );
        spawned_imageThread = false;
        return false;
    }

    std::string dst; dst.resize( result + filename_size + 18 );
    if ( !sprintf( dst.data(), "C:/Users/%s/Desktop/%s", username.c_str(), filename.c_str() ) ) 
    {
        printf( "Sprintf failed\n" );
        spawned_imageThread = false;
        return false;
    }
 
    if ( !cv::imwrite( dst, image ) ) {
        printf( "CV IMWRITE failed : \n" );
        spawned_imageThread = false;
        return false;
    }

    printf( "Saved to : %s\n", dst.c_str() );
    spawned_imageThread = false;
    return true;
}

void imageWriteThread()
{
    // Clipboard will be checked after few milliseconds
    // to catch sudden mutltiple clipboard update messages
    std::this_thread::sleep_for( std::chrono::milliseconds( update_interval ) );

    bitmapToImage( BInfo_Queue.back() );
    BInfo_Queue.clear();
}