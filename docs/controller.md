
# Controller Setup Guide

During development, you'll use a local controller (e.g., XBox) plugged into your PC that sends data in UDP packets to either a local instance of QEMU running the emulator or to the remote hardware.

## Getting Started with the Emulator

### Prerequisites
- Controller (XBox or other XInput-compatible controller) connected to your PC
- Visual Studio
- Sandpiper SDK

### Setup Steps

1. **Copy the sample folder**
   - Copy the `controller_sample` folder into your `sandpiper_sdk/samples` folder

2. **Configure network forwarding**
   - Modify the `-net` part of your `boot_emulator` script to forward a local UDP port:
   ```
   -net user,hostfwd=tcp::2222-:22,hostfwd=udp::39811-:39811
   ```

3. **Connect your controller**
   - Plug an XBox controller (or other XInput-compatible controller) into your PC

4. **Build and test the transmitter**
   - Build and run the `controller_xmit` application under Visual Studio
   - This should detect your controller and display the LEFT TRIGGER axis value

5. **Deploy to the emulator**
   - Build the `controller_sample` application
   - Copy it into the emulator using `scp`
   - SSH into the emulator and run the application
   - You should see a "Pong"-like bat that is moveable using your XBox controller



