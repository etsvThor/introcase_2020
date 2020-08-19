Code for the NodeMCU's used during the TU/e introduction week 2020 in de business case organized by Volundr.

Due to the Coronavirus regulations, it was not allowed to let the kiddo's work together with the same components, therefore this alternative business case was made. Every kiddo has his own kit which includes:
 - 400 tie-point breadboard
 - ~33 jumper wires
 - NodeMCU (already flashed)
 - Passive buzzer module audion 9012 drive
 - DIP switch 4 bit
 - 2 micro tactile switches
 - LED red 5 mm
 - Resistor 220 ohm

They already have got a power bank with a USB cable.

## Features
All NodeMCU's connect together in a mesh network. The DIP switch can be used to select a channel (0 - 15). When pressing one of the buttons the LED or the buzzer gives a signal on every board that has selected the same channel. The frequency of the buzzer is determined as follows: `frequency = (channel + 4) * 30`. All signals are synchronized with each other by using the same time base that is provided by the mesh network.

The onboard LED of the NodeMCU will blink synchronously with the other nodes in the mesh network. If the node is alone in the mesh the onboard LED will not blink.

The NodeMCU boards of the members of the Volundr committee are flashed with `#define VOLUDNR`. When the onboard button on one of these boards is pressed the Thor song refrain will be played on all boards synchronously! If the button is pressed again while playing the song is stopped earlier.

## Working
The code relies heavily on the following two libraries:
 - [https://gitlab.com/painlessMesh/painlessMesh/-/wikis/home](https://gitlab.com/painlessMesh/painlessMesh/-/wikis/home)
 - [https://github.com/arkhipenko/TaskScheduler/wiki](https://github.com/arkhipenko/TaskScheduler/wiki)

Next to that documentation the comments in 'scr/main.cpp' will explain the detailed working of the code.