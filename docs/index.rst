.. ESP32_CAN_Interface documentation master file, created by
   sphinx-quickstart on Fri Jul 12 10:42:14 2019.

CAN-Interface ESP32!
====================
.. toctree::
	:maxdepth: 2
	:caption: Contents:

.. _header-n0:

CAN-Interface ESP32
===================

| Wireless CAN-Interface for the ESP32 by Espressif. 
| Which uses the cannelloni protocol to send CAN-Frames via UDP over an
  Wi-Fi tunnel. 

.. _header-n3:

Features
--------

-  CAN-Frame accumulation in a UDP-Package

-  send and receive CAN-Frames from a connected CAN-Bus

-  CAN Driver configuration

-  Arbitration ID filter (hardware and software)

-  python-can support

-  CAN-FD support

-  UDP support

.. _header-n19:

Getting Started
---------------

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

.. figure:: https://github.com/PhilippFux/ESP32_CAN_Interface/blob/master/circuit_diagram.PNG
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

.. _header-n106:

Usage
-----

.. _header-n107:

Important Notice 
~~~~~~~~~~~~~~~~~

CAN-Interface is not suited for production purposes. It is not
guaranteed that CAN-Frames will reach their destination or reach in the
right order. Use the CAN-Interface only for purposes where packet loss
is tolerable.

.. _header-n109:

Example
~~~~~~~

.. _header-n110:

Preparation
^^^^^^^^^^^

-  Connect the SUB-D connector from the CAN-Interface with a CAN-Bus

-  Make a Wi-Fi connection between a Computer and the access point of
   the CAN-Interface

-  To start the communication with the CAN-Interface send an
   initialization message to the UDP-Socket

.. _header-n118:

Sending
^^^^^^^

1. Open an UDP-Socket

2. Build a Cannelloni Data Package containing the CAN-Frames to send
   (Header + encoded CAN-Frames)

   .. code:: python

      class CANNELLONIDataPacket(object):
          """
          Header for Cannelloni Data Packet
          """
          def __init__(self):
              self.version = 0
              self.op_code = 0
              self.seq_no = 0
              self.count = 0

3. Send the Cannelloni Data Package via UDP to the CAN-Interface

.. _header-n127:

Receiving
^^^^^^^^^

1. Open an UDP-Socket

2. Receive the UDP-Packages sent from the CAN-Interface

3. Unpack the CAN-Frames using the Cannelloni Data Package

4. Decode the CAN-Frames

.. _header-n137:

Use with python-can
-------------------

The CAN-Interface can also be used with the library python-can which
provides CAN support for Python.

.. _header-n139:

Important Notice 
~~~~~~~~~~~~~~~~~

In order to use the CAN-Interface with python-can, the library with
support for the CAN-Interface has to be installed

**TODO: Describe the use with python-can (entered in library? y/n)**

.. _header-n143:

Authors
-------

-  **Philipp Fuxen** - *Initial work* -
   `PhilippFux <https://github.com/PhilippFux>`__

.. _header-n147:

Acknowledgments
---------------

-  **cannelloni** - *SocketCAN over Ethernet tunnel* -
   `mguentner/\ cannelloni <https://github.com/mguentner/cannelloni>`__
