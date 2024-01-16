# Objectives 
* Establish (**SYN**) and release (**FIN**) connection
* Send data packets (**DAT**) and acknowledge received packets (**ACK**). Use acknowledgement packet to identify and **retransmit** lost packets.
  - RDP cannot be stopped to wait for acknowledgement
* Support **flow control** using **window size**

## Packet Format
**COMMAND** (**SYN|FIN|DAT|ACK|RST**) <br>
**HEADER**: Value (vary based on the COMMAND) <br>
... <br>
**Header**: Value (vary based on the COMMAND) <br>
**PAYLOAD** <br>
