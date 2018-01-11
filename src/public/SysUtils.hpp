#pragma once

#include <stdio.h>
#include <cstdlib>

#if defined( OS_ANDROID )
#    include <android/log.h>
#elif defined( OS_APPLE )
#    import <Foundation/Foundation.h>
#    import <UIKit/UIKit.h>
#elif defined( OS_WINDOWS )
#    include <stdarg.h>
#    include <Windows.h>
#elif defined( OS_LINUX )
#    include <stdarg.h>
#endif

/*
================================================================================================================================

System level functionality

================================================================================================================================
*/

static void* AllocAlignedMemory( size_t size, size_t alignment )
{
    alignment = ( alignment < sizeof( void* ) ) ? sizeof( void* ) : alignment;
#if defined( OS_WINDOWS )
    return _aligned_malloc( size, alignment );
#elif defined( OS_APPLE )
    void* ptr = nullptr;
    return ( posix_memalign( &ptr, alignment, size ) == 0 ) ? ptr : nullptr;
#else
    return memalign( alignment, size );
#endif
}

static void FreeAlignedMemory( void* ptr )
{
#if defined( OS_WINDOWS )
    _aligned_free( ptr );
#else
    free( ptr );
#endif
}

static void Print( const char* format, ... )
{
#if defined( OS_WINDOWS )
    char    buffer[4096];
    va_list args;
    va_start( args, format );
    vsnprintf_s( buffer, 4096, _TRUNCATE, format, args );
    va_end( args );

    OutputDebugStringA( buffer );
#elif defined( OS_LINUX )
    va_list args;
    va_start( args, format );
    vprintf( format, args );
    va_end( args );
    fflush( stdout );
#elif defined( OS_APPLE )
    char    buffer[4096];
    va_list args;
    va_start( args, format );
    vsnprintf( buffer, 4096, format, args );
    va_end( args );

    NSLog( @"%s", buffer );
#elif defined( OS_ANDROID )
    char    buffer[4096];
    va_list args;
    va_start( args, format );
    vsnprintf( buffer, 4096, format, args );
    va_end( args );

    __android_log_print( ANDROID_LOG_VERBOSE, "atw", "%s", buffer );
#endif
}

static void Error( const char* format, ... )
{
#if defined( OS_WINDOWS )
    char    buffer[4096];
    va_list args;
    va_start( args, format );
    vsnprintf_s( buffer, 4096, _TRUNCATE, format, args );
    va_end( args );

    OutputDebugStringA( buffer );

    MessageBoxA( nullptr, buffer, "ERROR", MB_OK | MB_ICONINFORMATION );
#elif defined( OS_LINUX )
    va_list args;
    va_start( args, format );
    vprintf( format, args );
    va_end( args );
    printf( "\n" );
    fflush( stdout );
#elif defined( OS_APPLE_MACOS )
    char    buffer[4096];
    va_list args;
    va_start( args, format );
    int length = vsnprintf( buffer, 4096, format, args );
    va_end( args );

    NSLog( @"%s\n", buffer );

    if ( [NSThread isMainThread] )
    {
        NSString* string =
            [[NSString alloc] initWithBytes:buffer length:length encoding:NSASCIIStringEncoding];
        NSAlert* alert = [[NSAlert alloc] init];
        [alert addButtonWithTitle:@"OK"];
        [alert setMessageText:@"Error"];
        [alert setInformativeText:string];
        [alert setAlertStyle:NSWarningAlertStyle];
        [alert runModal];
    }
#elif defined( OS_APPLE_IOS )
    char    buffer[4096];
    va_list args;
    va_start( args, format );
    int length = vsnprintf( buffer, 4096, format, args );
    va_end( args );

    NSLog( @"%s\n", buffer );

    if ( [NSThread isMainThread] )
    {
        NSString* string =
            [[NSString alloc] initWithBytes:buffer length:length encoding:NSASCIIStringEncoding];
        UIAlertController* alert =
            [UIAlertController alertControllerWithTitle:@"Error"
                                                message:string
                                         preferredStyle:UIAlertControllerStyleAlert];
        [alert addAction:[UIAlertAction actionWithTitle:@"OK"
                                                  style:UIAlertActionStyleDefault
                                                handler:^( UIAlertAction* action ){
                                                }]];
        [UIApplication.sharedApplication.keyWindow.rootViewController presentViewController:alert
                                                                                   animated:YES
                                                                                 completion:nil];
    }
#elif defined( OS_ANDROID )
    char    buffer[4096];
    va_list args;
    va_start( args, format );
    vsnprintf( buffer, 4096, format, args );
    va_end( args );

    __android_log_print( ANDROID_LOG_ERROR, "atw", "%s", buffer );
#endif
    // Without exiting, the application will likely crash.
    if ( format != nullptr )
    {
        exit( 0 );
    }
}
