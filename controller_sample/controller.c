#include <stdio.h>
#include <wchar.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdint.h>
#include <sys/time.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <fcntl.h>
#include <errno.h>
#include <stdbool.h>
#include <dirent.h>

#include "controller.h"


#define DEBUGMSG(format, ...) \
  { printf( format __VA_OPT__(,) __VA_ARGS__ ); printf( "\n" ); }

#define CONTROLLER_PORT     39811

#define DEVICE_NAME         "HID Analog"

static controller_mode      s_mode = CONTROLLER_MODE_INVALID;
static uint8_t              s_lastValue;
static int                  s_udpSocket;
static int                  s_hidHandle;


static int controller_open_hid( void )
{
    FILE*       s = fopen( "/proc/bus/input/devices", "r" );
    char        t[ 256 ];
    char        sf[ 256 ];
    bool        found = false;

    while( !feof( s ) )
    {
        fgets( t, sizeof( t ), s );
        if( !strncmp( t, "N: ", 3 ) )
        {
            if( strstr( t, DEVICE_NAME ) )
            {
                found = true;
            }
        }
        else if( !strncmp( t, "S: ", 3 ) && found )
        {
            char*     c = strstr(t, "Sysfs=" );
            if( c )
            {
                c[ 5 ] =0;
                c += 6;
                c = strtok( c, " \r\t\n" );
                snprintf( sf, sizeof( sf ), "/sys%s/device/hidraw", c );
                break;
            }
            found = false;
        }
    }
    fclose( s );
    if( !found )
    {
        DEBUGMSG( "failed to find device" );
        return( CONTROLLER_GENERIC_ERROR );
    }

    DEBUGMSG( "reading %s folder", sf );
    DIR*    dir = opendir( sf );
    if( !dir )
    {
        DEBUGMSG( "failed to open hid dir" );
        return( CONTROLLER_GENERIC_ERROR );
    }
    found = false;
    while( true )
    {
        struct dirent*      d = readdir( dir );
        if( !d )
        {
            break;
        }
        if( !strncmp( d->d_name, "hidraw", 6 ) )
        {
            found = true;
            sprintf( t, "/dev/%s", d->d_name );
            break;
        }
    }
    closedir( dir );
    
    if( !found )
    {
        DEBUGMSG( "failed to find hid device" );
        return( CONTROLLER_GENERIC_ERROR );
    }

    DEBUGMSG( "trying to open HID device [%s]", t );
    s_hidHandle = open( t, O_RDONLY | O_NONBLOCK );
    if( s_hidHandle < 0 )
    {
        DEBUGMSG( "failed to open HID device = errno %d", errno );
        return( CONTROLLER_GENERIC_ERROR );
    }

    DEBUGMSG( "opened fakehid OK" );
    s_mode = CONTROLLER_MODE_HID;
    return( CONTROLLER_OK );
}


static int controller_open_udp( void )
{
    int                 rc, flags;
    struct sockaddr_in  sin;

    DEBUGMSG( "controller_open_udp" );

    s_udpSocket = socket( AF_INET, SOCK_DGRAM, 0 );
    if( s_udpSocket < 0 )
    {
        DEBUGMSG( "socket() failed" );
        return( CONTROLLER_GENERIC_ERROR );
    }

    memset( &sin, 0, sizeof( sin ) );
    sin.sin_family = AF_INET;
    sin.sin_port = htons( CONTROLLER_PORT );
    rc = bind( s_udpSocket, (struct sockaddr*)&sin, sizeof( sin ) );
    if( rc != 0 )
    {
        DEBUGMSG( "bind() failed" );
        close( s_udpSocket );
        return( CONTROLLER_GENERIC_ERROR );
    }

    flags = fcntl( s_udpSocket, F_GETFL, 0 );
    if( flags < 0 )
    {
        DEBUGMSG( "fcntl() failed" );
        close( s_udpSocket );
        return( CONTROLLER_GENERIC_ERROR );
    }
    flags = fcntl( s_udpSocket, F_SETFL, flags | O_NONBLOCK );
    if( flags < 0 )
    {
        DEBUGMSG( "fcntl() failed" );
        close( s_udpSocket );
        return( CONTROLLER_GENERIC_ERROR );
    }

    DEBUGMSG( "UDP controller opened OK" );

    s_mode = CONTROLLER_MODE_NETWORK;
    return( CONTROLLER_OK );
}


int controller_open( controller_mode mode )
{
    if( s_mode != CONTROLLER_MODE_INVALID )
    {
        DEBUGMSG( "controller_open: device is already opened" );
        return( CONTROLLER_GENERIC_ERROR );
    }

    if( mode == CONTROLLER_MODE_AUTO )
    {
        int     rc = controller_open( CONTROLLER_MODE_HID );
        if( rc != CONTROLLER_OK )
        {
            rc = controller_open( CONTROLLER_MODE_NETWORK );
        }
        return( rc );
    }

    if( mode == CONTROLLER_MODE_NETWORK )
    {
        return( controller_open_udp() );
    }
    else if( mode == CONTROLLER_MODE_HID )
    {
        return( controller_open_hid() );
    }
    else
    {
        DEBUGMSG( "controller_open: bad controller mode" );
        return( CONTROLLER_GENERIC_ERROR );
    }
}


int controller_read( int* value )
{
    int                     s;
    uint8_t                 buf[ 65 ];
    bool                    wouldBlock = false;
    bool                    gotValue = false;

    // read as many reports as possible - don't let them back up
    while( 1 )
    {
        if( s_mode == CONTROLLER_MODE_NETWORK )
        {
            s = recv( s_udpSocket, buf, sizeof( buf), 0 );
            if( s < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) )
            {
                wouldBlock = true;
            }
            else if( s == 5 && !memcmp( buf, "CTRL", 4 ) )
            {
                gotValue = true;
                s_lastValue = buf[ 4 ];
            }
        }
        else if( s_mode == CONTROLLER_MODE_HID )
        {
            s = read( s_hidHandle, buf, sizeof( buf) );
            if( ( s < 0 && ( errno == EAGAIN || errno == EWOULDBLOCK ) ) || s == 0 )
            {
                wouldBlock = true;
            }
            else if( s > 0 )
            {
                gotValue = true;
                s_lastValue = buf[ s-1 ];
            }
        }
        else
        {
            return( CONTROLLER_GENERIC_ERROR );
        }

        if( wouldBlock )
        {
            *value = (int)s_lastValue;
            return( gotValue ? CONTROLLER_OK : CONTROLLER_OLD_VALUE );
        }
        if( s < 0 )
        {
            return( CONTROLLER_GENERIC_ERROR );
        }
    }

}

void controller_close( void )
{
    DEBUGMSG( "controller_close" );

    if( s_mode == CONTROLLER_MODE_NETWORK )
    {
        close( s_udpSocket );
    }
    else if( s_mode == CONTROLLER_MODE_HID )
    {
        close( s_hidHandle );
    }

    s_mode = CONTROLLER_MODE_INVALID;
}

