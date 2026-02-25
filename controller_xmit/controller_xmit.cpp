#include <iostream>
#include <string>
#include <thread>
#include <chrono>
#include <iomanip>
#define WIN32_LEAN_AND_MEAN
#include <windows.h>
#include <WinSock2.h>
#include <Xinput.h>
#include <stdint.h>

#pragma comment(lib, "Xinput.lib")

#define CONTROLLER_PORT             39811

double normalizeThumb(short v)
{
    if (v >= 0) return v / 32767.0;
    return v / 32768.0; // negative side
}

int main(int argc, char** argv)
{
    SOCKET              sock;
    int                 i;
    WSADATA             wsaData;

    i = WSAStartup(MAKEWORD(2, 1), &wsaData);
    if (i)
    {
        std::cout << "failed to init winsock\n";
        exit(-1);
    }

    std::string axis = "lt"; // default: left trigger

    if (argc < 2)
    {
        std::cout << "usage: controller_xmit <ip_address>\n";
        exit(-1);
    }

    int             a, b, c, d;
    i = sscanf_s(argv[1], "%d.%d.%d.%d", &a, &b, &c, &d);
    if (i != 4)
    {
        std::cout << "bad ip\n";
        exit(-1);
    }
    struct sockaddr_in          sin;
    memset(&sin, 0, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.S_un.S_un_b.s_b1 = (uint8_t)a;
    sin.sin_addr.S_un.S_un_b.s_b2 = (uint8_t)b;
    sin.sin_addr.S_un.S_un_b.s_b3 = (uint8_t)c;
    sin.sin_addr.S_un.S_un_b.s_b4 = (uint8_t)d;
    sin.sin_port = htons(CONTROLLER_PORT);

    sock = socket(AF_INET, SOCK_DGRAM, 0);
    if (sock == INVALID_SOCKET)
    {
        std::cout << "failed to create socket";
        exit(-1);
    }


    const int controllerIndex = 0;
    const std::chrono::milliseconds interval(10);

    std::cout << "Reading axis '" << axis << "' from controller " << controllerIndex << " (Ctrl-C to quit)" << std::endl;

    while (true)
    {
        XINPUT_STATE state;
        ZeroMemory(&state, sizeof(state));
        DWORD res = XInputGetState(controllerIndex, &state);

        if (res == ERROR_SUCCESS)
        {
            auto &g = state.Gamepad;
            double value = 0.0;

            if (axis == "lx") value = normalizeThumb(g.sThumbLX);
            else if (axis == "ly") value = normalizeThumb(g.sThumbLY);
            else if (axis == "rx") value = normalizeThumb(g.sThumbRX);
            else if (axis == "ry") value = normalizeThumb(g.sThumbRY);
            else if (axis == "lt") value = g.bLeftTrigger / 255.0;
            else if (axis == "rt") value = g.bRightTrigger / 255.0;
            else
            {
                std::cerr << "Unknown axis '" << axis << "'. Use lx, ly, rx, ry, lt, rt." << std::endl;
                return 1;
            }
            std::cout << "Value: " << std::fixed << std::setprecision(3) << value << "    \r" << std::flush;

            int         i;
            uint8_t     buf[ 5 ];
            memcpy(buf, "CTRL", 4);
            buf[ 4 ] = (uint8_t)(value * 255.0);
            i = sendto(sock, (char*)buf, 5, 0, (struct sockaddr*)&sin, sizeof(sin));
            if (i != 5)
            {
                std::cout << "Network send error\n";
            }
        } else {
            std::cout << "Controller not connected. Waiting...    \r" << std::flush;
        }

        std::this_thread::sleep_for(interval);
    }

    return 0;
}
