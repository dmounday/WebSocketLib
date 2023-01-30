# WebSocket library based on Boost Beast examples.

### Library created in support of TR-369 USP test controller.

### See the tests folder for examples of use for client and server.
The cmake script fetches plog from github.com. 

~~~
$ cmake -S . -B build
$ cd build; make -j 8
~~~

Start Server Test
~~~
$ cd tests
$ ./WSServerTest ../../tests/websocket.config debug
~~~

Start Client
~~~
$ cd tests
$ ./WSClientTest 0.0.0.0 8183 /ws-srvr
~~~
