#include "util/data_io.h"

void BytesDataOutput::writeString(const std::string& v) {
    int length = (int)v.length() & 0x7fff;
    writeShort((short)length);
    writeBytes(v.c_str(), length);
}

std::string BytesDataInput::readString() {
    int len = readShort();
    if (len < 0) len = 0;
    if (len > MAX_STRING_LENGTH - 1) len = MAX_STRING_LENGTH - 1;
    char* buffer = new char[len + 1];
    readBytes(buffer, len);
    buffer[len] = 0;
    std::string out(buffer, len);
    delete[] buffer;
    return out;
}
