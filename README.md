# PiControl

This is the server used to connect to the iOS/Android applications for PiControl.
The source code for the mobile frontends can be found [here](https://github.com/mackalex/pi-control-frontend).

## Dependencies
- libwebsockets - `sudo apt install libwebsockets-dev`
  - You might have to install the most recent version from source if you get errors:

    ```bash
    git clone https://libwebsockets.org/repo/libwebsockets libwebsockets
    cd libwebsockets
    cmake . && sudo make install
    ```
### (Optional) (Limited functionality)
- libxdo - `sudo apt install libxdo-dev`
  - `USE_XDO=true make picontrol_server`
