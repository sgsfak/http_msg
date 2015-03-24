# README #

This is a C++ messaging library using `channels` as the underlying abstraction. There are two types of channels:

* An `OutChannel` is for output. It has a single `put` method for sending some
  data to the channel. This method returns immediately.

* An `InChannel` is for input. You can only "read" from these channels, using
  the `get` or the `try_get` methods. `get` is blocking i.e. if there's nothing
  to be read from this channle, the calling program waits indefinitely, until
  there's something to be retreived from the channel.


An output channel is usually connected to an input channel by using the same
"communication identifier". Under the hood this id is used to identify the
message queue in the message broker component that delivers messages.

### How do I get set up? ###

The library requires [QDB](http://qdb.io) as the message broker receiving and
dispatching the messages.

### Examples ###

There are some examples in the `examples` folder...

#### Example: two "hypomodels" interacting in a master-worker fashion

Check the `oncosim.cpp` (producing the `os` executable) and the
`biomechanics.cpp` (producing the `bm` executable). The `os` is a complex
"model" that has some initial state, working in a time-driven loop (per hour
iteration), that needs to communicate with the `bm` model. The `bm` model is
the "worker" waiting for some request, computing some results that are sent
back to "master" i.e. the `os`. The communication is implemented using the
library with the `qdb` message broker behind the scenes. A typical execution
script is the following:

* Start the `qdb` message broker and create a `foo` queue :

     curl http://127.0.0.1:9554/q/foo -d maxSize=10g

* Start the "master" providing the tumor radius, the simulation time in hours, the input and the output channel:

    ./os 100 96 bm-to-os os-to-bm

* Start the "worker" providing the input and the output channel in the reverse order so that the input of `os` is the output of `bm` and vice versa:

    ./bm os-to-bm bm-to-os
