# **Networking Project**

**Note**: _do not use threads_ for this assignment. We will not uses the `pthread` library for this project. That is, we are going to use single thread.

## Threads start wearing out

One of the common ways of handling multiple clients at a time is to use threads. Sounds simple enough. A client connects, we spawn a new thread to handle it, and then that thread can clean up after itself once it's done. What's the catch?

Well, that usually works okay up until a point. After that, your server won't scale as fast. And in the modern world, you have to do things web scale (TM).

## Non blocking I/O

Well, what can we do about this? Maybe we could keep a thread pool, that is, have a fixed number of threads, and have them service the connections. However, it's an M:N mapping this time (M connections, N threads). But wait, how do I multiplex all these different connections, and handle all those threads?

Non-blocking I/O is your friend here. Remember how regular calls to read(), accept() etc. block until there's data or a new connection available? (If you don't, try it out! Trace how your server code blocks in read() until your client sends some data!). Well, non-blocking I/O does exactly what it says, you structure your application in such a way that works with the data that's already present (in the TCP buffers), not block for data that may or may not arrive! Functions that help you with this are `select()`, `poll()`, and `epoll()`.

Think of it as an event driven system. At a high level, you maintain a set of file descriptors (could map to files, pipes, network sockets, etc.) that you're interested in, and call the appropriate wait() function on that set of descriptors. Your program waits until one (or more) of those descriptors have some data available (in a server scenario, when a client has actually sent data). When data is available, it's like an 'event' occurred, so your program exits the wait() call, and can iterate over the descriptors that have data, process each of them, and then go back to wait()-ing for additional data to arrive.

## The Problem

You'll be writing the client and server for a simplified file sharing application. TCP is used for everything here, so reliability is taken care of. The server uses non-blocking I/O (with epoll) to handle concurrent requests. The application supports four basic operations - `GET`, `PUT`, `LIST` and `DELETE`. Their functions are as follows:

`GET` - Client downloads (GETs) a file from the server
`PUT` - Client uploads (PUTs) a file to the server
`LIST` - Client receives a list of files from the server
`DELETE` - Client deletes a file from the server

For simplicity, you can assume that there are no conflicting requests (that is, nobody will upload a file while someone else is downloading or deleting it, etc.)

## The Protocol

This is a text-based protocol (similar to HTTP and FTP). The client sends plaintext requests along with some binary data (depending on the operation), and then the server responds with plaintext containing either error messages or optional binary data. The binary data in this case is the file being transferred, if it is a `GET` or `PUT` request. The maximum header length (header is part before data) for both the request and response is 1024 bytes.  The format for the protocol is as follows:
