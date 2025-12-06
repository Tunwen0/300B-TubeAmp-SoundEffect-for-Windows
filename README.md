***请向下滚动页面以浏览中文版***

This Windows application recreates the exact sound effect of the  legendary 300B vacuum tubes for your computer audio. Through precise  physical modeling derived from actual APX555 measurements of a Manley Neo-Classic 300B Preamplifier, it brings the magic of tube sound within  reach - allowing every Windows user to experience that celebrated 300B sonic character in an immersive digital environment.

# **I. Program Introduction**

Unlike other tube emulation software, this program's advantage lies  in its physical modeling based on an actual Manley 300B tube  preamplifier, treating the amplifier's overall output characteristics as an integrated system.

Traditional emulation designers often focus solely on a tube's  nominal electrical parameters and performance—such as harmonic  distortion introduced by specific tube models or asymmetric  amplification of soundwave positive and negative half-cycles. However,  in the real world, tubes operate within amplifier circuits, and the  electrical properties of the circuit components themselves also impact  sonic performance. For example, its output transformer and capacitors  cannot perfectly transmit all frequencies, often introducing their own  sonic character. This is why conventional tube emulation software often  sounds "somewhat artificial".

In contrast, this program not only recreates the operational behavior of a 300B tube functioning within a complete circuit, but also captures  the coloration imparted by the output transformer and capacitors,  allowing users to genuinely experience legendary tube sound at a  physical level.

![Manley Neo-Classic 300B Preamplifier](pictures\Manley Neo-Classic 300B Preamplifier.jpg)

*Fig.1 Manley Neo-Classic 300B Preamplifier*



![APX555B](pictures\APX555B.jpg)

*Fig. 2 Audio Precision APX555 analyzer*



# **II. Usage Instructions**

## **1. System Requirements**

This program is compatible with most computers running Windows 10 and Windows 11.

It operates at a sample rate of 48kHz, 24-bit.

## **2. Usage Instructions**

**2.1** This program requires the VB-CABLE Virtual Audio Device (an open-source virtual audio driver software) to function. You  must install the VB-CABLE Driver and restart your computer before  installing this program.
Download link for VB-CABLE Driver:

https://download.vb-audio.com/Download_CABLE/VBCABLE_Driver_Pack45.zip

**2.2** To ensure stable operation, after installing this program, please configure the following settings:
 Open *Control Panel* (the classic Control Panel, not the Windows 10/11 Settings app) → navigate to *Hardware and Sound* → *Sound*. Select **CABLE Input (VB-Audio Virtual Cable)** from the list, then click the **Properties** button in the lower-right corner. In the Properties window, switch to the **Advanced** tab. Set the sample rate to **48kHz, 24-bit**, and check the boxes for **"Allow applications to take exclusive control of this device"** and **"Give exclusive mode applications priority"**.

Next, using the same method in the Control Panel's Sound settings,  set your physical output device's (e.g., Bluetooth headphones, audio  interface, powered speakers, DAC, etc.) sample rate to **48kHz, 24-bit**.

**2.3** When using common music playback software (e.g., Foobar2000, Tidal Music, etc.), please select this program's virtual  audio device **（WASAPI: CABLE Input (VB-Audio Virtual Cable)）** in the output/audio device settings within those players. Then, within  this software's output device settings, select your actual physical  playback device (e.g., headphones, speakers, DAC, external audio  interface, etc.).

**2.4** For playback scenarios that don't allow direct  output device configuration (e.g., watching live streams via Chrome  browser), please use this method:
 While playing the video, right-click the **speaker icon** in the taskbar notification area → select **"Open Sound settings"**. Scroll to the bottom and click **"App volume and device preferences"**. Locate **"Google Chrome"** in the list—you'll see two dropdown menus. The first is **"Output"**, defaulting to "Default". Click this dropdown and directly select **CABLE Input (VB-Audio Virtual Cable)**. Then, within this software's output device settings, select your actual physical playback device.

## **3. Troubleshooting**

**3.1 Program fails to start**
 Please refer to Usage Instructions 2.1 and 2.2.

**3.2 Program runs but no audio output**
 Please refer to Usage Instruction 2.3.

If the issue persists, go to the program's "Settings", adjust the  output device to your actual physical playback device, then click the  "Start" button in the lower-right corner of the program window.

If still not resolved, navigate to:
 *Control Panel* → *Hardware and Sound* → *Sound*. Select this program's virtual speaker, click **Properties** → **Advanced** tab. In the window that opens, **uncheck** the box for **"Allow applications to take exclusive control of this device"**.

**3.3 Audio plays but no processing effect is audible**
 Click the program's "Monitor" button and ensure the "Bypass" button is  not illuminated. If "Bypass" is active, the program's algorithms are not engaged.

You may test the processing effect by playing a 1kHz pure tone signal, where the difference should be clearly audible.

# 一、程序介绍

与其他模拟电子管的程序相比，本程序的优点在于**基于真正的Manley 300B电子管前级放大器进行基于科学测量的物理建模**，并将这个前级放大器的输出表现视为一个整体。

传统的模拟程序的设计者往往只考虑电子管本体的纸面电气参数和性能表现，例如特定型号电子管带来的谐波失真或者对声波的正半周和负半周的不对称放大。但在真实世界中，电子管是安装在电子管放大器内工作的，而电子管放大器电路和元件本身的电气特性也会影响声音表现，例如，它的输出变压器和电容无法完美传输所有频率，往往也会带来声音特征的变化。这就是为什么传统的模拟电子管的程序往往听起来“有点假”。

相比之下，本程序不但还原了运行于一整套电路中的300B电子管的工作特性，能够还原这种由输出变压器和电容带来的音染，让用户真正体验到物理级的传奇电子管之声。

# 二、使用方法

## 1.运行环境

本程序可以在绝大多数Windows10和Windows11电脑内运行。

本程序支持48kHz, 24bit的采样率。

## 2.使用方法

**2.1** 本程序的运行依赖VB-CABLE Virtual Audio Device （一个开源的虚拟声卡驱动软件）。您必须要安装VB-CABLE Driver并重启电脑后才能安装本程序。
VB-CABLE Driver的下载地址：

https://download.vb-audio.com/Download_CABLE/VBCABLE_Driver_Pack45.zip

**2.2** 为了保证程序运行的稳定性，在安装本程序后，请在控制面板（注意是传统控制面板，不是Windows10/11的设置菜单）的声音设置里，点击**CABLE Input (VB-Audio Virtual Cable)**，再点击右下角的“属性”，在打开的设置界面中选择上方的“高级”，在这里将采样率调整为**48kHz，24bit**，并勾选“允许应用程序独占控制该设备”、“给与独占模式应用程序优先”。然后，在控制面板的声音设置里，用同样的方法将您的硬件输出设备（如蓝牙耳机、声卡、有源音箱、DAC解码器等）的采样率调整为**48kHz，24bit**。

**2.3** 如果您使用常见的音乐播放软件聆听音乐，如Foobar2000、网易云音乐等，请您在这些软件的设置菜单的输出设备里选择本软件的虚拟声卡**（WASAPI: CABLE Input (VB-Audio Virtual Cable)）**，然后再在本软件的输出设备里选择您实际聆听的物理设备（如耳机、音箱、DAC音频解码器、外置声卡等）。

**2.4** 如果您使用的播放设备内没有办法直接设置输出设备，例如，使用Chrome浏览器观看网络直播视频，那么请您在播放直播视频时，右键点击任务栏右下角的小喇叭 -> 选择 “打开声音设置”（注意是右下角的任务栏，不是传统控制面板）。滚动到最底部，点击 “应用音量和设备首选项” ，在列表中找到 “Google Chrome”——您会看到两个下拉菜单，第一个是“输出”，默认显示的是“默认”。点击下拉菜单，直接选中**CABLE Input (VB-Audio Virtual Cable)**。接着，再在本软件的输出设备里选择您实际聆听的物理设备（如耳机、音箱、DAC音频解码器、外置声卡等）。

## 3.故障排除

**3.1** 程序无法运行

请参考使用方法2.1和2.2。

**3.2** 程序运行了，但无法播放声音

请参考使用方法2.3。

如果不奏效，请在程序的“设置”里把输出设备调整为您实际聆听的物理设备（如耳机、音箱、DAC音频解码器、外置声卡等），然后点击一下程序窗口右下角的“开始”按钮。

如果还不奏效，请您进入传统控制面板 -> 声音，选中本程序的虚拟扬声器，点击右下角的属性 -> 高级，在打开的窗口内取消勾选“允许应用程序独占控制该设备”。

**3.3** 程序能播放声音，但处理前后的声音听不出变化

请点击程序的“监听”，确保程序的“直通”按钮没有亮起。如果“直通”按钮亮起了，则程序算法不生效。

您可以播放一段1khz的纯音信号来测试程序，可以很容易听出变化。






