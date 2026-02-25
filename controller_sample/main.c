#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include "controller.h"

#define SANDPIPER 1

#if SANDPIPER
#include "platform.h"
#include "vpu.h"

struct SPSizeAlloc framebuffers[ 2 ];
static struct SPPlatform* s_platform = NULL;



int main( int argc, char** argv )
{
    if( controller_open( CONTROLLER_MODE_AUTO ) != CONTROLLER_OK )
    {
        printf( "failed\n" );
        exit( -1 );
    }

	// Initialize platform and video system
	s_platform = SPInitPlatform();
	if (!s_platform) {
		fprintf(stderr, "Failed to initialize platform\n");
		return -1;
	}

	// Grab video buffer
	uint32_t stride = VPUGetStride(EVM_320_240, ECM_16bit_RGB);
    for( int i = 0 ; i < 2 ; i++ )
    {
	    framebuffers[ i ].size = stride*240;
	    SPAllocateBuffer(s_platform, &framebuffers[ i ]);
    }

	// Set up the video mode and frame pointers
	VPUSetVideoMode(s_platform->vx, EVM_320_240, ECM_16bit_RGB, EVS_Enable);
	s_platform->sc->cycle = 0;
	s_platform->sc->framebufferA = &framebuffers[ 0 ];
	s_platform->sc->framebufferB = &framebuffers[ 1 ];
	VPUSwapPages(s_platform->vx, s_platform->sc);
	VPUClear(s_platform->vx, 0x03030303);

    while( 1 )
    {
        int     r, val;
        r = controller_read( &val );

        uint16_t* pixels = (uint16_t*)s_platform->sc->writepage;
        val = ( ( val - 128 ) / 2 ) + 120 - 16;
        uint16_t* row = pixels + ( stride/2 * val );
        for( int i = 0 ; i < 32 ; i++ )
        {
            memset( row + (10 * 2 ), 0xff, 20 );
            row += stride/2;
        }

        VPUWaitVSync(s_platform->vx);
    	VPUSwapPages(s_platform->vx, s_platform->sc);
    	VPUClear(s_platform->vx, 0x00000000);
    }

	return 0;
}
#else
int main( int argc, char** argv )
{
    if( controller_open( CONTROLLER_MODE_AUTO ) != CONTROLLER_OK )
    {
        printf( "failed to open controller\n" );
        exit( -1 );
    }
    while( 1 )
    {
        int     r, val;
        r = controller_read( &val );
        if( r == CONTROLLER_OK )
        {
            printf( "%d\n", val );
        }
    }
    return( 0 );
}

#endif
