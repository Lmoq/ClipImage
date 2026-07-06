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

DWORD lastSequenceNumber = 0;

cv::Mat image_array{};
std::deque<std::vector<BYTE>> BInfo_Queue;

bool getLatestImage()
{
    UINT target_format = CF_DIB;
    char format_string[] = "CF_DIB";

    savedImageArray = false;

    if ( !CountClipboardFormats() ) {
        printf( "Clipboard number of items retrieval failed, Error code : [%d]\n", GetLastError() );
        return false;
    }
    
    DWORD sequenceNumber = GetClipboardSequenceNumber();
    DWORD diff = sequenceNumber - lastSequenceNumber;
    lastSequenceNumber = sequenceNumber;
    
    // Difference of current and last sequence number greater than the value 2 indicates that new content is pasted to clipboard
    // The latter only shows that an app that previously accessed clipboard, closes
    printf( "Last sequence number : %d\n", lastSequenceNumber );
    if ( diff <= 2 ) {
        return false;
    }

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
        LPVOID pMem = GlobalLock( bHandle );
        if ( pMem == NULL ) {
            printf( "GlobalLock failed, code : %d\n", GetLastError() );
            return false;
        }
        SIZE_T size = GlobalSize( bHandle );

        // Create own copy
        std::vector<BYTE> dib( size );
        memcpy( dib.data(), pMem, size);

        GlobalUnlock( bHandle );

        // Append bitmap to queue
        BInfo_Queue.push_back( dib );
    }
    return true;
}

bool bitmapToImage( std::vector<BYTE> &dib )
{
    PBITMAPINFO pBinfo = reinterpret_cast<PBITMAPINFO>( dib.data() );
    BITMAPINFOHEADER header = pBinfo->bmiHeader;

    if ( fullscreen_only ) 
    {
        if ( !( system_screen_width == header.biWidth && system_screen_height == header.biHeight ) ) {
            printf( "Image is not fullscreen\n" );
            return false;
        }
    }
    cv::Mat image;

    BYTE *pixel_data;
    if ( header.biCompression == BI_RGB ) 
    {
        pixel_data = reinterpret_cast<BYTE *>( pBinfo->bmiColors );
        if ( header.biBitCount == 24 ) {
            // If bitmap is 24 bpp, step in bytes argument should be specified, if not, distorted or misplaced pixels will occur
            image = cv::Mat( static_cast<int>( header.biHeight ), static_cast<int>( header.biWidth ), CV_8UC3, pixel_data, ( ( header.biWidth * 24 + 31 ) / 32 ) * 4 );
        }
        else if ( header.biBitCount == 32 ) {
            image = cv::Mat( static_cast<int>( header.biHeight ), static_cast<int>( header.biWidth ), CV_8UC4, pixel_data );
        }
    }
    else if ( header.biCompression == BI_BITFIELDS )
    {
        // At this compression type, pBinfo contains all the info where the pixel data address is at ( + headerSize + rgb masks memory width )
        pixel_data = reinterpret_cast<BYTE *>( pBinfo ) + pBinfo->bmiHeader.biSize + ( 3 * sizeof( DWORD ) );
        image = cv::Mat( static_cast<int>( header.biHeight ), static_cast<int>( header.biWidth ), CV_8UC4, pixel_data );
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