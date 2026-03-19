## Modern Arduino IDE package

This repository includes an adapted LinkIt ONE package for modern Arduino IDE versions on Windows.

Boards Manager URL:

`https://raw.githubusercontent.com/jaredsalazar/Linkitone_Arduino/main/package_linkitone-community_index.json`

Release assets for version `v1.5.6-linkit-modern.1`:

- `linkit-one-platform-1.5.6-linkit-modern.1.zip`
- `linkit-one-toolchain-arm-none-eabi-1.5.6-linkit-modern.1.zip`
- `linkit-one-tool-mtk-1.5.6-linkit-modern.1.zip`

Install:

1. Open Arduino IDE.
2. Go to `File > Preferences`.
3. Add the Boards Manager URL above to `Additional boards manager URLs`.
4. Open `Boards Manager`.
5. Search for `MediaTek LinkIt ONE`.
6. Install the package.

Notes:

- This package is Windows-only.
- Remove old manual installs of `hardware\arduino\mtk` or `hardware\mediatek\mtk` before using the Boards Manager package.

Arduino IDE for LinkIt ONE
-----------
The LinkIt ONE development board is an open source, high performance board for prototyping Wearables and IoT devices. It's based on the world’s leading SoC for Wearables, MediaTek Aster (**MT2502**) combined with high performance Wi-Fi (**MT5931**) and GPS (**MT3332**) chipsets to provide you with access to all the features of MediaTek LinkIt. It also provides similar pin-out features to Arduino boards, making it easy for you to connect to various sensors, peripherals, and Arduino shields.

LinkIt ONE is a co-design product by Seeed Studio and MediaTek. It brings together the parties’ knowledge in open hardware and industry leading reference designs for Wearables and IoT devices to create this powerful development board.
![](http://www.seeedi.com/depot/includes/templates/bootstrap/images/Linkit-one-page.jpg)




<br>
Instal LinkIt ONE driver
---
When first insert LinkIt ONE to your PC, you will find two COM port that need to instal driver:

![](http://www.seeedstudio.com/wiki/images/2/2b/LinkIt_ONE_Wiki_Temp1.jpg)

The driver path: ..\Arduino-IDE-LinkIt-ONE\drivers\mtk

After instaled driver, you can find 2 ports:

![](http://www.seeedstudio.com/wiki/images/f/f9/LinkIt_ONE_Wiki_Temp2.jpg)


- MTK USB Debug Port used for uploading code
- MTK USB Modem Port used for printing message, such as Serial.println()





<br>
Blink
-----
If you had used an Arduino, you will be not unfamiliar with Blink!

Here we will show you a Blink in LinkIt ONE.

First open Arduino IDE for LinkIt ONE, then Click File -> Examples -> Basics -> Blink
to open code of Blink

Then Board choose LinkIt ONE, Port choose the number of MTK USB Debug Port.

Before Uploading the codem make sure the switch is right:

![](http://www.seeedstudio.com/wiki/images/f/fc/LinkIt_ONE_Wiki_Temp3.jpg)

Then click Upload to program the code to LinkIt ONE. You can find the led blinking now. 

