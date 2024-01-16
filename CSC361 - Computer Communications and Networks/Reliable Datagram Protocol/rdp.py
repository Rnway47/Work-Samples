#!/usr/bin/env python3
import select
import socket
import sys
import queue
import time

# Create a UDP socket
udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
udp_socket.setblocking(0)
serverPort = int(sys.argv[2])
# Bind the socket to the port
server_address = (sys.argv[1], serverPort)
udp_socket.bind(server_address)
input = [udp_socket]
output = [udp_socket]
#send buffer
snd_buf = queue.Queue()
#snd_data_buf = [] #keep track of data, for error control
#"receive" buffer for data
rcv_buf = []
pak_buf = []

window_size = 5120 #receiver buffer size, maximum amount of DAT can be sent without acknowledgement
last_window = 5120
payload_list = []
state = "closed"
line = 0

#Todo//
#  implement a payload variable with max 1024 bytes

#break the input into one or more payloads
def build_payload(input_file):
    file = open(input_file, "rb")
    list = []   
    payload = file.read(1024)
    while payload != b'':
        list.append(payload)
        payload = file.read(1024)
    file.close()
    return list

def write_file(output_file, data): #write to the output file
    global line
    if line == 0:
        file = open(output_file, "wb")
        file.write(data)
        line += 1
    else:
        file = open(output_file, "ab")
        file.write(data)
        line += 1

def format_packet(command, header=None, payload=None): #format RDP packets, payload is optional
    packet = command + header
    if payload: #if payload presents
        packet += "\n" + payload.decode()
    return packet

def time_log(event, com_head):#time for server log
    count = 0
    current = time.strftime("%a %b %d %H:%M:%S %Z %Y: ", time.localtime()) #time
    current += event #Send; or Receive;
    for item in com_head:
        if item != "" and count < 3:
            current += item + "; " #all the headers
            count += 1
    print(current)

#SYN packet from sender to open a connection
command = "SYN\n"
header = "Sequence: 0\n" + "Length: 0\n"
packet = format_packet(command, header)
snd_buf.put(packet)
payload_list = build_payload(sys.argv[3])
#print(payload_list)
while True:
    # ready for processing
    readable, writable, exceptional = select.select(input,
                                                    output,
                                                    input,
                                                    0.5)

    
    if udp_socket in writable: #sender, send what we have in the send buffer
        message = snd_buf.get() #get what we have in the send buffer
        pak_buf.append(message)
        udp_socket.sendto(message.encode(), ("10.10.1.100", 8888)) #send the packet to receiver
        packet = message.split("\n\n") #By Default, there will an empty line after the last header
        com_head = packet[0].split("\n") #acquire all the command and headers
        time_log("Send; ", com_head) #log for sender
        output.remove(udp_socket)
        time.sleep(0.1)

    if udp_socket in readable: #receiver
        message = udp_socket.recvfrom(2048) #payload is maximum 1024 bytes, actual packet may exceed 1024 bytes
        if (len(message[0]) < 1): #timeout, re-send lost packet
            snd_buf.put(pak_buf.pop())
            print("timeout, send again")
            output.append(udp_socket)

        else:
            chunk = message[0].decode()
            packet = chunk.split("\n\n", 1) #new line indicating the presence of all headers
            com_head = packet[0].split("\n") #acquire all the command and headers
            
            if len(packet) > 1: #payload exists
                data = packet[1]
            else:
                data = []
            
            command = com_head[0] #SYN|ACK|DAT|FIN

            if command != "DAT":
                time_log("Receive; ", com_head) #log for receiver (receive what specific packets from sender)

            if (command == "SYN"):
                    state = "SYN-Sent" #state changes to SYN-sent, need to send ACK packet
                    command = "ACK\n"
                    header = "Acknowledgement: 1\n" + "Window: {}\n".format(window_size)
                    packet = format_packet(command, header) #ACK packet, no payload
                    snd_buf.put(packet)
                    output.append(udp_socket)

            elif (command == "ACK"): #3 scenario, send DAT, send FIN, the end of transmission
                    if state == "SYN-Sent":
                        command = "DAT\n"
                        state = "open" #set state to open, sending DAT
                        ack_list = com_head[1].split(": ")
                        seq_num = ack_list[1] #next seq num is the current ack num
                        header = "Sequence: {}\nLength: {}\n".format(seq_num, len(payload_list[0]))
                        packet = format_packet(command, header, payload_list[0])
                        #last_window = window_size
                        window_size -= len(payload_list[0]) #keep track of the window size
                        snd_buf.put(packet)
                        rcv_buf.append(packet)
                        output.append(udp_socket)
                        del payload_list[0]

                    elif state == "open": #send DAT if some data remains. If not, send FIN packet
                        window_size = 5120 #reset window_size
                        if payload_list: #still data left
                                command = "DAT\n"
                                ack_list = com_head[1].split(": ")
                                seq_num = ack_list[1] #next seq num is the current ack num
                                header = "Sequence: {}\nLength: {}\n".format(seq_num, len(payload_list[0]))
                                packet = format_packet(command, header, payload_list[0])
                                #last_window = window_size
                                window_size -= len(payload_list[0]) #keep track of the window size
                                snd_buf.put(packet)
                                rcv_buf.append(packet)
                                output.append(udp_socket)
                                del payload_list[0]

                        else: #ready to close connection
                            command = "FIN\n"
                            ack_list = com_head[1].split(": ") #the ack header line
                            seq_num = ack_list[1] #ack num is the seq number of the next packet
                            header = "Sequence: {}\nLength: 0\n".format(seq_num)
                            packet = format_packet(command, header)
                            snd_buf.put(packet)
                            output.append(udp_socket)

                    elif state == "FIN-Sent": #connection needs to be closed
                        state = "closed"
                        udp_socket.close()
                        sys.exit()
                        break
                    
            elif (command == "DAT"): #write data to write file and send DAT if any data remains OR send ACK when window size is full or no data remains
                if payload_list: #data remains
                    if window_size > 0:
                        command = "DAT\n"
                        seq_list = com_head[1].split(": ")
                        len_list = com_head[2].split(": ")
                        seq_num = int(seq_list[1]) + int(len_list[1]) #seq num + len num = next seq num
                        header = "Sequence: {}\nLength: {}\n".format(seq_num, len(payload_list[0]))
                        packet = format_packet(command, header, payload_list[0])
                        #last_window = window_size
                        window_size -= len(payload_list[0]) #keep track of the window size
                        snd_buf.put(packet)
                        rcv_buf.append(packet)
                        output.append(udp_socket)
                        del payload_list[0]

                    else: #send ACK to free window size
                        while rcv_buf:
                            packet = rcv_buf.pop(0).split("\n\n", 1) #new line indicating the presence of all headers
                            com_head = packet[0].split("\n") #acquire all the command and headers
                            time_log("Receive; ", com_head)
                            write_file(sys.argv[4], packet[1].encode())
                            #print("writing here")
                        
                        command = "ACK\n"
                        seq_list = com_head[1].split(": ")
                        len_list = com_head[2].split(": ")
                        ack_num = int(seq_list[1]) + int(len_list[1])
                        header = "Acknowledgment: {}\nWindows: {}\n".format(ack_num, window_size)
                        window_size = 5120
                        packet = format_packet(command, header)
                        snd_buf.put(packet)
                        output.append(udp_socket)

                else: #no data left, send ACK
                    while rcv_buf:
                        packet = rcv_buf.pop(0).split("\n\n", 1) #new line indicating the presence of all headers
                        com_head = packet[0].split("\n") #acquire all the command and headers
                        time_log("Receive; ", com_head)
                        write_file(sys.argv[4], packet[1].encode())
                        #print("writing here")
                    
                    command = "ACK\n"
                    seq_list = com_head[1].split(": ")
                    len_list = com_head[2].split(": ")
                    ack_num = int(seq_list[1]) + int(len_list[1])
                    header = "Acknowledgment: {}\nWindows: {}\n".format(ack_num, window_size)
                    window_size = 5120
                    packet = format_packet(command, header)
                    snd_buf.put(packet)
                    output.append(udp_socket)
            elif(command == "FIN"): #Send ACK packet
                state = "FIN-Sent" #ok to close the connection after FIN
                command = "ACK\n"
                seq_list = com_head[1].split(": ")
                ack_num = int(seq_list[1]) + 1
                header = "Acknowledgment: {}\nWindows: {}\n".format(ack_num, window_size)
                packet = format_packet(command, header)
                snd_buf.put(packet)
                output.append(udp_socket)