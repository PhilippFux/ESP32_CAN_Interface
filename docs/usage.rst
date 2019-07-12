=====
Usage
=====

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