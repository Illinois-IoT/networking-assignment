# **Networking Project**

**Note**: _do not use threads_ for this assignment. We will not uses the `pthread` library for this project. That is, we are going to use single thread.

## Threads?
[Why would multi threaded applications in general scale bad?](https://stackoverflow.com/questions/10347613/why-would-multi-threaded-applications-in-general-scale-bad)
<br />
[Web-Scale Properties](https://www.nutanix.com/blog/understanding-web-scale-properties#:~:text=Web%2Dscale%20describes%20the%20tendency,re%2Darchitecting%20at%20critical%20moments)

## What is Non blocking I/O?
Non-blocking I/O avoids the client being blocked while waiting for a request to be accepted by the **transport layer** during one-way messaging for connection-oriented protocols(e.g. TCP).
<br />
For connection-oriented protocols, there is a limit to the amount of data that can be put in a network protocol queue. The limit depends on the transport protocols used. When a client sending a request reaches the data limit, this client is blocked from processing until its request has entered the queue. You cannot determine how long a message will wait before being added to the queue.
<br />
In non-blocking I/O, when the transport queue is full, there is an additional buffer available between the client and the transport layer. As requests not accepted by the transport queue can be stored in this buffer, **the client is not blocked**. The client is free to continue processing as soon as it has put the request in the buffer. The client does not wait until the request is put in the queue and does not receive information on the status of the request after the buffer accepts the request.
<br />
By using non-blocking I/O you gain further processing time as compared to two-way and one-way messaging. The client can send requests in succession without being blocked from processing. (i.e. when you `read()` or `write()`)
Functions that help you with this are `select()`, `poll()`, and `epoll()`.
<br />
More information if you want to know:
[epoll](https://en.wikipedia.org/wiki/Epoll#Triggering_modes)
[edge-triggered?](https://en.wikipedia.org/wiki/Interrupt#Edge-triggered)
[level-triggered?](https://en.wikipedia.org/wiki/Interrupt#Level-triggered)

## The Problem

You'll be writing the client and server for a simplified file sharing application. TCP is used for everything here, so reliability is taken care of. The server doesn't have to use non-blocking I/O to handle concurrent requests, as this is way more complicated than our goal of this assignment. However, you are encouraged to try that when you have extra time.
<br />
Your application should support three basic operations - `GET`, `PUT` and `DELETE`. Their functions are as follows:

`GET` - Client downloads (GETs) a file from the server
`PUT` - Client uploads (PUTs) a file to the server
`DELETE` - Client deletes a file from the server

For simplicity, you can assume that there are no conflicting requests (that is, nobody will upload a file while someone else is downloading or deleting it, etc.)

## The Protocol

This is a text-based protocol (similar to HTTP and FTP). The client sends plaintext requests along with some binary data (depending on the operation), and then the server responds with plaintext containing either error messages or optional binary data. The binary data in this case is the file being transferred, if it is a `GET` or `PUT` request. The maximum header length (header is part before data) for both the request and response is 1024 bytes.  The format for the protocol is as follows:
### Client Request:

