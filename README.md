# c2usb MCUX SDK sample applications

This repository contains c2usb MCUXpresso SDK sample applications.

## Initialization

The first step is to initialize the workspace folder (``c2usb-workspace``) where
this repository and all MCUXpresso SDK modules will be cloned. Run the following
command:

```shell
# initialize my-workspace for the usb-keyboard (main branch)
west init -m https://github.com/IntergatedCircuits/c2usb-mcux-examples c2usb-workspace
cd c2usb-workspace
# update mcuxsdk modules
west update
west patch
```

You can continue developing with the command line, or with the MCUXpresso for VS Code extension.
In the latter case open the `projects.code-workspace` file in vscode.

## Application Index

### hid-keyboard

A straightforward USB HID keyboard. Use the button on the board to trigger a caps lock press,
and observe as the host changes the caps lock state on the board's LED. When suspended,
the button press triggers a remote wakeup request.

## Building and running

To build an application, run the following command:

```shell
cd c2usb-mcux-examples/hid-keyboard
BOARD=frdmk22f west build -d release -- --preset release
```

Once you have built the application, run the following command to flash it:

```shell
west flash
```
