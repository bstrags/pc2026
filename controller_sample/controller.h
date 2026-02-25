#ifndef CONTROLLER_H
#define CONTROLLER_H

/*

    Controller library for studio summit 2026 programming competition

    The controller is a single-axis analog input - returning bytes from 0 to 255.

    There are 3 modes in which this lib is likely to be used:

    1) Local development using sandpiper emulator

        In this case, the controller library will receive controller data via UDP network packets from a
        source machine. Spin up QEMU with the additional parameter:

            -net user,hostfwd=tcp::2222-:22,hostfwd=udp::39811-:39811
                                            ^^^^^^^^^^^^^^^^^^^^^^^^^ - new

        On the same development machine, plug in a USB game controller, and run the (separate)
        controller_xmit.exe program with 127.0.0.1 as the parameter.
        This will poll the game controller, and relay the data to your application inside QEMU.

    2)  Remote development using sandpiper hardware

        This works exactly as in (1) - you will still use a local USB game controller. The only difference
        is that you will use controller_xmit.exe to send the UDP packets to the remote sandpiper machine
        itself. You will need to ascertain its IP address, and pass that as the parameter to controller_xmit.
        There may be some network latency... :/
    
    3)  The competition itself

        For the competition, we will use a (as-of-yet) mystery bluetooth analog controller!
        The device will be paired with the sandpiper machine on which the entries are run. The controller
        library in your binary will detect the device automatically (assuming you use CONTROLLER_MODE_AUTO).

*/


//
// return codes
//
typedef enum
{
    CONTROLLER_OK = 0,
    CONTROLLER_GENERIC_ERROR = -1,
    CONTROLLER_DISCONNECTED = -2,
    CONTROLLER_OLD_VALUE = -3,
} controller_code;


//
// modes of operation.
// generally you will want CONTROLLER_MODE_AUTO
//
typedef enum
{
    CONTROLLER_MODE_INVALID = 0,

    // attempt to init controller in HID, then NETWORK order
    CONTROLLER_MODE_AUTO,

    // use received UDP packets
    CONTROLLER_MODE_NETWORK,

    // proper HID mode, discovers device by name
    CONTROLLER_MODE_HID
} controller_mode;


// call this once at startup
int             controller_open( controller_mode mode );

// reads a single byte value from the controller.
// if there is new data, it will return CONTROLLER_OK. if there is no new data
// it will provide the last value read, and return CONTROLLER_OLD_VALUE.
int             controller_read( int* value );

// call this at shutdown. if you like.
void            controller_close( void );


#endif // #ifndef CONTROLLER_H
