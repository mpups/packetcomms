#ifndef ABSTRACTSOCKET_H
#define ABSTRACTSOCKET_H

#include <cstddef>

class AbstractWriter
{
public:
    AbstractWriter() {}
    virtual ~AbstractWriter() {}
    virtual void setBlocking( bool )                       = 0;
    virtual int  write( const char*, std::size_t )         = 0;
};

class AbstractReader
{
public:
    AbstractReader() {}
    virtual ~AbstractReader() {}
    virtual void setBlocking( bool )                       = 0;
    virtual int  read( char*, std::size_t )                = 0;
    virtual bool readyForReading( int milliseconds ) const = 0;
};

class AbstractSocket : public AbstractWriter, public AbstractReader
{
public:
    AbstractSocket() {}
    virtual ~AbstractSocket() {}

    virtual void setBlocking( bool )                       = 0;
    virtual int  write( const char*, std::size_t )         = 0;
    virtual int  read( char*, std::size_t )                = 0;
    virtual bool readyForReading( int milliseconds ) const = 0;
};


#endif // ABSTRACTSOCKET_H
