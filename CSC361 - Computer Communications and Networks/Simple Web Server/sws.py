#!/usr/bin/env python3
# encoding: utf-8
#
# Copyright (c) 2019 Zhiming Huang
#

import select
import socket
import sys
import queue
import time
import re
import os
import errno

# Create a TCP/IP socket
server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
server.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1) #to use the same port
server.setblocking(0)
serverPort = int(sys.argv[2])
# Bind the socket to the port
server_address = ('', serverPort)
#print('starting up on {} port {}'.format(*server_address), file=sys.stderr)
server.bind(server_address)

# Listen for incoming connections
server.listen(5)

# Sockets from which we expect to read
inputs = [server]

# Sockets to which we expect to write
outputs = []

# Outgoing message queues (socket:Queue) client side
message_queues = {}

# request message server side
request_message = {}

buffer = ""
client_ip = ""
client_port = ""
timeout = 30
client = None

def time_log():#time for server log
    current = time.strftime("%a %b %d %H:%M:%S %Z %Y", time.localtime())
    return current

while inputs:
    # Wait for at least one of the sockets to be
    # ready for processing
    #print('waiting for the next event', file=sys.stderr)
    readable, writable, exceptional = select.select(inputs,
                                                    outputs,
                                                    inputs,
                                                    30)

    # Handle inputs
    for s in readable:
        if s is server:
            # A "readable" socket is ready to accept a connection
            connection, client_address = s.accept()
            client_ip = str(client_address[0])
            client_port = str(client_address[1])
            c_socket = connection
            connection.setblocking(0)
            inputs.append(connection)
            message_queues[connection] = queue.Queue()
            request_message[connection] = queue.Queue()
        else:
            pre_check = re.compile(r"GET /.* HTTP/1.0\n")
            head_check = re.compile(r"connection:\s*keep-alive\n", re.IGNORECASE)
            head_check_c = re.compile(r"connection:\s*close\n", re.IGNORECASE)

            try:
                while True:
                    message = s.recv(1024).decode("utf-8")
                    check = re.findall(pre_check, message)
                    check_h = re.findall(head_check, message)
                    check_hc = re.findall(head_check_c, message)
                    if message.count("\n\n") > 0: #if it's back to back, store it and break out of the loop
                        buffer += message
                        break
                    else:
                        if buffer.count("\n") >= 1: #check if it has \n\n at the end
                            break
                        elif message != "\n":#check the case if message is not just \n
                            if(check == [] and check_h == [] and check_hc == []):#check if it matches proper request format
                                buffer += message
                                break
                            else:
                                buffer += message
                        elif message[-1:] == "\n" and buffer.count("\n") == 0:
                            buffer += message
                            break
            except socket.error:
                continue

            else:
                    client_request = buffer.split("\n\n")
                    client_request[:] = (request for request in client_request if request != "")
                    buffer = ""
                    req_line = re.compile(r"GET /.* HTTP/1.0")
                    header_open = re.compile(r"Connection:\s*keep-alive", re.IGNORECASE)
                    header_close = re.compile(r"Connection:\s*close", re.IGNORECASE)
                    for request in client_request:
                        lines = request.split("\n") #break it into request line and header
                        lines[:] = (line for line in lines if line != "")
                        if len(lines) == 2: #2 items mean request has both request line and header
                            res1 = re.findall(req_line, lines[0])
                            res2 = re.findall(header_open, lines[1])
                            res3 = re.findall(header_close, lines[1])
                            if(res1 == [] or res2 == [] and res3 == []): #400 bad request
                                client_bad_request = "HTTP/1.0 400 Bad Request\r\nConnection:close\r\n\r\n"
                                server_bad_request = time_log() + ": " + client_ip + ":" + client_port + " " + lines[0] + "; HTTP/1.0 400 Bad Request"
                                message_queues[s].put(client_bad_request)
                                request_message[s].put(server_bad_request)

                            else: #we check if this file exists or not
                                file_end = 5
                                for letter in lines[0][5:]: #get the file name
                                    if letter != " ":
                                        file_end += 1
                                    else:
                                        break

                                if os.path.isfile(lines[0][5:file_end]): #file found, 200 OK
                                #open the file and store it to client response
                                    file = open(lines[0][5:file_end], "r")
                                    server_ok_response = time_log() + ": " + client_ip + ":" + client_port + " " + lines[0] + "; HTTP/1.0 200 OK"
                                    client_ok_response = "HTTP/1.0 200 OK\r\n" + lines[1] + "\r\n\r\n" + file.read()
                                    file.close()
                                    message_queues[s].put(client_ok_response)
                                    request_message[s].put(server_ok_response)

                                else: #404 Not found
                                    client_not_found = "HTTP/1.0 404 Not Found\r\n" + lines[1] + "\r\n\r\n"
                                    server_not_found = time_log() + ": " + client_ip + ":" + client_port + " " + lines[0] + "; HTTP/1.0 404 Not Found"
                                    message_queues[s].put(client_not_found)
                                    request_message[s].put(server_not_found)

                        elif len(lines) == 1: #1 item mean just the request line
                            res1 = re.findall(req_line, lines[0])
                            if(res1 == []): #bad request
                                client_bad_request = "HTTP/1.0 400 Bad Request\r\nConnection: close\r\n\r\n"
                                server_bad_request = time_log() + ": " + client_ip + ":" + client_port + " " + lines[0] + "; HTTP/1.0 400 Bad Request"
                                message_queues[s].put(client_bad_request)
                                request_message[s].put(server_bad_request)

                            else:
                                file_end = 5
                                for letter in lines[0][5:]: #get the file name
                                    if letter != " ":
                                        file_end += 1
                                    else:
                                        break

                                if os.path.isfile(lines[0][5:file_end]): #file found, 200 OK
                                    #open the file and store it to client response
                                    file = open(lines[0][5:file_end], "r")
                                    server_ok_response = time_log() + ": " + client_ip + ":" + client_port + " " + lines[0] + "; HTTP/1.0 200 OK"
                                    client_ok_response = "HTTP/1.0 200 OK\r\n" + "Connection: close\r\n\r\n" + file.read()
                                    file.close()
                                    message_queues[s].put(client_ok_response)
                                    request_message[s].put(server_ok_response)

                                else: #404 Not found
                                    client_not_found = "HTTP/1.0 404 Not Found\r\n" + "Connection: close\r\n\r\n"
                                    server_not_found = time_log() + ": " + client_ip + ":" + client_port + " " + lines[0] + "; HTTP/1.0 404 Not Found"
                                    message_queues[s].put(client_not_found)
                                    request_message[s].put(server_not_found)
                        if s not in outputs:
                            outputs.append(s)

    for s in writable:
            try:
                response = message_queues[s].get_nowait()
                log = request_message[s].get_nowait()
            except queue.Empty:
            # No messages need to be sent so stop watching
                if s not in inputs:
                    s.close()
                    del message_queues[s]
                    del request_message[s]
            else:
                #print logs and send messages
                print(log)
                s.send(response.encode("UTF-8"))
                header_close = re.compile(r"Connection:\s*close", re.IGNORECASE)#check if should close the connection or not
                bad_request = re.compile(r"HTTP/1.0 400 Bad Request")
                res2 = re.findall(header_close, response)
                res3 = re.findall(bad_request, response)
                if(res3 != []): #close connection due to 400 bad request
                    outputs.remove(s)
                    inputs.remove(s)
                    if s not in inputs:
                        s.close()
                        del message_queues[s]
                        del request_message[s]
                elif(res2 != []): #connection close header
                    outputs.remove(s)
                    inputs.remove(s)
                    if s not in inputs:
                        s.close()
                        del message_queues[s]
                        del request_message[s]
                
    # Handle "exceptional conditions"
    for s in exceptional:
        #print('exception condition on', s.getpeername(),
         #     file=sys.stderr)
        # Stop listening for input on the connection
        inputs.remove(s)
        if s in outputs:
            outputs.remove(s)
        s.close()

        # Remove message queue
        del message_queues[s]
        del request_message[s]



