libbc - Uniform reliable broadcast library
==========================================
DISCLAIMER. THIS LIBRARY IS NOT USABLE YET

This library is an implementation of uniform reliable broadcast.
It might become a distributed system library algorithm collection later on.


Design
------
-- Please note that since the library is in an early development
-- stage, the design might quickly change from one version to another.

The implementation is intented to avoid dynamic allocation as much as
possible.  It's build around an `struct bc_group` type which is an
opaque type, one can still allocate the data on the stack thanks to
`alloca()` and the provided `size_t bc_group_struct_size(void);`
functions.

The communication is built over the TCP network stack and using file
descriptor polling in an event driven design.

Upon creating or joining a group, one will get two file descriptors in
a pipe fashing to read messages from the group and broadcast messages
to the group.  This choice is motivated by the desire to be the least
intrusive for the programmer; it's very common to work with file
descriptors in C.


How to?
-------
For now the library is not yet usable, but you can still see examples
of use in the `example` folder.  It contains a chat client named
`miaou` using uniform reliable broadcast to send and receive messages.

To build the library you will need cmake:
$ mkdir build && cd build
$ cmake ..
$ make && make install

Api specification can be found in `include/bc.h`.
