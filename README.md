========== WORK IN PROGRESS ==========
---
title: Turn ESP32CAM into a Snapshot Taker, for Selfies and Time Elapsed Pictures
description: Turn ESP32CAM (or equivalent) into a Android phone managed snapshot taker, for snapshots like selfies and time-elapsed pictures
tags: 'esp32cam, snapshots'
cover_image: ./imgs/MAIN.jpg
---


# Turn ESP32CAM into a Snapshot Taker, for Selfies and Time Elapsed Pictures


Just for fun! The purpose of this project is to try to turn ESP32CAM (or equivalent like LILYGO T-Camera / T-Camera Plus) microcontroller board into an Android phone managed snapshot taker, for snapshots like selfies and time-elapsed pictures.

> With your Android phone, connect to the ESP32CAM, make your post and simile.
>  Then go back to your phone to select the pictures you like best and save to your phone directly.

> With your Android phone, connect to the ESP32CAM, set how frequently a snapshot is to be taken.
>  Then disconnect the ESP32CAM and let it does its job of taking time-elapsed pictures.
>  Then reconnect to the ESP32CAM to transfer the taken pictures to your phone.

The microcontroller program (sketch) is developed with Arduino framework using VS Code and PlatformIO, in the similar fashion as described by the post -- [A Way to Run Arduino Sketch With VSCode PlatformIO Directly](https://www.instructables.com/A-Way-to-Run-Arduino-Sketch-With-VSCode-PlatformIO/)

The remote UI -- control panel -- is realized with the help of the DumbDisplay Android app. For a brief description of DumbDisplay, you may want to refer to the post -- [Blink Test With Virtual Display, DumbDisplay](https://www.instructables.com/Blink-Test-With-Virtual-Display-DumbDisplay/)

Please note that the UI is driven by the microcontroller program; i.e., the control flow of the UI is programmed in the sketch. No building of a mobile app is necessary, other than installing the DumbDisplay Android app.


