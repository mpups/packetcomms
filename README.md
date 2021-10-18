# packetcomms

This library contains building blocks for client/server programs that send arbitrary packets of data muxed onto a single
communication stream. At the receiving end demuxed packets are shared in a zero-copy fashion with any number of subscribers
in a thread safe way. The core library is agnostic to both the serialisation of packets and the underlying transport stream.
However, support for serialistion using the Cereal library and simple TCP socket streams is included so simple applications
can be built quickly with no extra dependencies.
