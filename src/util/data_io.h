
#ifndef MCPSP_UTIL_DATA_IO_H
#define MCPSP_UTIL_DATA_IO_H

#include <cstring>
#include <climits>
#include <string>
#include <vector>

class IDataOutput {
public:
    virtual ~IDataOutput() {}
    virtual void writeString(const std::string& v) = 0;
    virtual void writeFloat(float v)          = 0;
    virtual void writeDouble(double v)        = 0;
    virtual void writeByte(char v)            = 0;
    virtual void writeShort(short v)          = 0;
    virtual void writeInt(int v)              = 0;
    virtual void writeLongLong(long long v)   = 0;
    virtual void writeBytes(const void* data, int bytes) = 0;
};

class IDataInput {
public:
    virtual ~IDataInput() {}
    virtual std::string readString()  = 0;
    virtual float     readFloat()     = 0;
    virtual double    readDouble()    = 0;
    virtual char      readByte()      = 0;
    virtual short     readShort()     = 0;
    virtual int       readInt()       = 0;
    virtual long long readLongLong()  = 0;
    virtual void      readBytes(void* data, int bytes) = 0;
};

class BytesDataOutput : public IDataOutput {
public:
    virtual void writeString(const std::string& v);
    virtual void writeFloat(float v)        { writeBytes(&v, 4); }
    virtual void writeDouble(double v)      { writeBytes(&v, 8); }
    virtual void writeByte(char v)          { writeBytes(&v, 1); }
    virtual void writeShort(short v)        { writeBytes(&v, 2); }
    virtual void writeInt(int v)            { writeBytes(&v, 4); }
    virtual void writeLongLong(long long v) { writeBytes(&v, 8); }
    virtual void writeBytes(const void* data, int bytes) = 0;
};

class BytesDataInput : public IDataInput {
public:
    virtual std::string readString();
    virtual float     readFloat()    { float o;     readBytes(&o, 4); return o; }
    virtual double    readDouble()   { double o;    readBytes(&o, 8); return o; }
    virtual char      readByte()     { char o;      readBytes(&o, 1); return o; }
    virtual short     readShort()    { short o;     readBytes(&o, 2); return o; }
    virtual int       readInt()      { int o;       readBytes(&o, 4); return o; }
    virtual long long readLongLong() { long long o; readBytes(&o, 8); return o; }
    virtual void      readBytes(void* data, int bytes) = 0;
protected:
    static const int MAX_STRING_LENGTH = SHRT_MAX;
};

class MemWriter : public BytesDataOutput {
public:
    std::vector<unsigned char> buf;
    virtual void writeBytes(const void* data, int bytes) {
        if (bytes <= 0) return;
        const unsigned char* p = (const unsigned char*)data;
        buf.insert(buf.end(), p, p + bytes);
    }
};

class MemReader : public BytesDataInput {
public:
    MemReader(const unsigned char* data, int size)
        : fail(false), _buf(data), _size(size), _pos(0) {}
    virtual void readBytes(void* data, int bytes) {
        if (bytes <= 0) return;
        if (_pos + bytes > _size) { fail = true; memset(data, 0, bytes); _pos = _size; return; }
        memcpy(data, &_buf[_pos], bytes);
        _pos += bytes;
    }
    bool fail;
private:
    const unsigned char* _buf;
    int _size;
    int _pos;
};

#endif
