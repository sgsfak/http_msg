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

For an example check `chat.cpp` in the `examples` folder.
