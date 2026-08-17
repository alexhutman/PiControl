# PiControl

This is the server used to connect to the iOS/Android applications for PiControl.
The source code for the mobile frontends can be found [here](https://github.com/mackalex/pi-control-frontend).

## Dependencies
- libuv - `sudo apt install libuv1-dev`
- libwebsockets - `sudo apt install libwebsockets-evlib-uv`
  - You might have to install the most recent version from source if you get errors:

    ```bash
    git clone https://libwebsockets.org/repo/libwebsockets
    mkdir libwebsockets/build && cd libwebsockets/build
    cmake -DLWS_WITH_LIBUV=1 .. && make -j && sudo make install && sudo ldconfig
    ```
### (Optional) (Limited functionality)
- libxdo - `sudo apt install libxdo-dev`
  - `USE_XDO=true make picontrol_server`
