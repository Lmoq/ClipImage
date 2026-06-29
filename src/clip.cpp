#include <iostream>
#include <ctime>
#include <clip.hpp>
#include <vector>
#include <opencv2/opencv.hpp>

int system_screen_width;
int system_screen_height;
long update_interval = 100;

bool fullscreen_only;
bool autoSave = true;

bool spawned_imageThread = false;
bool savedImageArray = false;

cv::Mat image_array{};
std::deque<PBITMAPINFO> BInfo_Queue;

bool getLatestImage()
{
    UINT target_format = CF_DIB;
    char format_string[] = "CF_DIB";

    savedImageArray = false;

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
    image_array.release();
    cv::Mat image;
    image.release();

    if ( header.biCompression == BI_RGB ) 
    {
        // For BI_RGB format, additional parameter step is added( width in bytes accounting the padding )
        // Distortion or misplace of pixels happens if not param is not filled, 
        // since cv::Mat() initialization doesn't compensate for the additional bytes padding
        image = cv::Mat( static_cast<int>( header.biHeight ), static_cast<int>( header.biWidth ), CV_8UC3, reinterpret_cast<cv::uint8_t *>(pBinfo->bmiColors), ( ( header.biWidth * 24 + 31 ) / 32 ) * 4 );
    }
    else if ( header.biCompression == BI_BITFIELDS )
    {
        // Read the bitmap as Mat image with the flag CV_8UC4 
        // to match the data type and include the alpha channel
        image = cv::Mat( static_cast<int>( header.biHeight ), static_cast<int>( header.biWidth ), CV_8UC4, reinterpret_cast<cv::uint8_t *>(pBinfo->bmiColors) );
    }
    image.copyTo( image_array );

    // Flip bitmap vertically since the image is bottom-up DIB, where pixel origin is from bottom left
    cv::flip( image_array, image_array, 0 );
    spawned_imageThread = false;
    return true;
}

void writeImageToFile()
{
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
            return;
        }
    }
    else if ( username.size() < result )
    {
        printf( "Insufficient storage, bytes to write : %d\n", result );
        return;
    }

    std::string dst; dst.resize( result + filename_size + 18 );
    if ( !sprintf( dst.data(), "C:/Users/%s/Desktop/%s", username.c_str(), filename.c_str() ) )
    {
        printf( "Sprintf failed\n" );
        return;
    }
    if ( !cv::imwrite( dst, image_array ) ) {
        printf( "cv::imwrite() failed : \n" );
        spawned_imageThread = false;
        return;
    }
    printf( "Saved to : %s\n", dst.c_str() );
}

void imageWriteThread()
{
    // Thread will sleep for an interval to catch the latest clipboard update,
    // since some program dispatch multiple updates at very short amount of time
    std::this_thread::sleep_for( std::chrono::milliseconds( update_interval ) );

    if ( bitmapToImage( BInfo_Queue.back() ) ) {
        savedImageArray = true;
        
        if ( autoSave ) {
            writeImageToFile();
        }
    }
    BInfo_Queue.clear();
}