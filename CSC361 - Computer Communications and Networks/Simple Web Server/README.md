# Objectives
* Support **GET /filename HTTP/1.0 command** command, **Connection: keep-alive** header, and **“Connection: close** header
  - If unsupported commands are received, respond with **HTTP/1.0 400 Bad Request** and **close the connection**
  - If the file cannot be found, respond with **HTTP/1.0 404 Not Found** followed by the header of the request
  - If the request is successful, respond with **HTTP/1.0 200 OK** followed by the header of the request and the content of the file

* Support both **persistent** and **non-persistent** connections
