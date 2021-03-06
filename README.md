# Minecraft Command Center

## Wrapper

The wrapper is a multithreaded C Programm which wraps Minecraft, or any other 
Program, redirecting it's stdio (*stdin*, *stdout*, *stderr*) to a unix domain socket.
Up to 4 clients can connect simultaneously to the socket and read minecraft's 
output and send minecraft commands.

After forking and before the child process environment is replaced, the stdio 
streams are redirected to a pipe, created before the fork. The parent process 
ataches to these pipes. The parent process spawns a sending thread, which broadcasts 
the child's stdout to any connected client. When a client connects to the socket, 
the parent process spawns a thread which recieves the input from the client and 
redirects it to the pipe.

Clients may send commands directly to the wrapper. Only simple well defined commands 
without arguments are supportet at the moment. To distinguish between data for 
minecraft's stdin and a command each command is prepended with the magic string 
`::mc::`.

Supported commands are:

 - `::mc::bye` to end the connection gracefully.
 - `::mc::test` for testing purpose. 

Also a simple client is included in the wrapper. It compiles to a own binary. 
Like the wrapper, it uses a thread to read input. The output is directly written 
in the main process.

For convenience, a leading colon (`:`) on a line is converted to the magic string
`::mc::`. So entering the sequence `:bye`+enter disconnects the client.

### Wrapper example

```bash
./minecraft-wrapper -s /tmp/minecraft.socket -b /home/jojo/Desktop/minecraft ./ServerStart.sh
```

Use the `--help` switch to get more information. Passing arguments to the command
executed as child process is currently not supported. Use a startup script instead.

Start the client like this:

```bash
./minecraft-console /tmp/minecraft.socket
```

To connect directly to the pipe, you can use `socat`, 
which is a utility to convert between, different stream types:

```bash
socat - UNIX-CONNECT:/tmp/minecraft.socket
```

### Building the wrapper

```bash
make wrapper
make client
```


## Web frontend

The web frontend is a client for the wrapper using web technologies. 

A *node.js* server translates between the socket and the web page using *socket.io*.

This is fairly basic at the moment, but can grow very useful.

Install the dependencies and start the server:

```
npm install
bower install
node server.js
```

Then open in a browser localhost at port 3000: http://localhost:3000

The streams are automatically reconnected on error. 

Currently the web page keeps only the 50 last lines of the log, because otherwise 
the browser may become unresponsive if the page keeps growing.

Planned features:

- highlight the log using highlightjs
- list connected players using the query port and list command as fallback
- construct minecraft server commands graphically (by using the user list)
- responsive design
- login


