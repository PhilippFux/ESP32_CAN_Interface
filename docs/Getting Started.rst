===============
Getting Started
===============

These following instructions will help you to install and run the
project on the ESP32 for development and testing purposes.

.. _header-n21:

Prerequisites 
~~~~~~~~~~~~~~

Which hardware components and software are needed and how to install
them

.. _header-n23:

Hardware components 
^^^^^^^^^^^^^^^^^^^^

-  ESP32 by Espressif

-  CAN-Transceiver

-  cables

-  D-SUB connector (male)

.. _header-n33:

Software 
^^^^^^^^^

-  `Arduino IDE <https://www.arduino.cc/en/main/software>`__ - The tool
   for flashing the ESP32

.. _header-n37:

Connecting the hardware
~~~~~~~~~~~~~~~~~~~~~~~

.. image:: https://user-images.githubusercontent.com/49685484/61203591-1a489300-a6eb-11e9-91c2-b7c0e92b0e2e.PNG
	:alt:

Connect the following Pins from the ESP32 with the specified Pins of the
CAN-Transceiver

======== ===== ===============
\        ESP32 CAN-Transceiver
======== ===== ===============
3.3 Volt 3V3   3V3
Ground   GND   GND
CTX      GPIO5 CTX
CRX      GPIO4 CRX
======== ===== ===============

Connect the following Pins from the CAN-Transceiver with the specified
Pins of the D-SUB connector

======== =============== ===============
\        CAN-Transceiver D-SUB connector
======== =============== ===============
CAN High CANH            Pin 7
CAN Low  CANL            Pin 2
======== =============== ===============

.. _header-n77:

Install and configure the Arduino IDE
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

1. Download and install the `Arduino
   IDE <https://www.arduino.cc/en/main/software>`__

2. Open Preferences in the Arduino IDE ( File -> Preferences )

3. Include https://dl.espressif.com/dl/package\ *esp32*\ index.json
   under Additional Boards Manager URLs

4. Open the Board Manager ( Tools -> Board -> Boards Manager... )

5. Search for esp32 and install the esp32 by Espressif Systems

6. Finish! The installation should be completed in a few seconds

.. _header-n91:

Upload the Project to the ESP32
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

Connect the ESP32 via USB with a Computer and follow the instructions

1. Open the Arduino IDE

2. Choose a ESP32 Board ( Tools -> Board -> ESP32 Arduino)

3. Choose the used port ( Tools -> Port: -> COMx )

4. Download the Project and open the "CAN_Interface.ino" ( File ->
   Open...)

5. Press the Upload Button in the Arduino IDE

6. If everything went as expected a "Done uploading." message should
   appear