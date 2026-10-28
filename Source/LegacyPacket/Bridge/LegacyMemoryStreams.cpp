#include "Client_PCH.h"

#include "Packet.h"
#include "SocketInputStream.h"
#include "SocketOutputStream.h"

#include <algorithm>

SocketOutputStream::SocketOutputStream(Socket* sock, uint BufferLen)
    : m_Socket(sock), m_Buffer(nullptr), m_BufferLen(BufferLen), m_Head(0), m_Tail(0)
{
    if (m_BufferLen == 0)
    {
        m_BufferLen = DefaultSocketOutputBufferSize;
    }
    m_Buffer = new char[m_BufferLen];
}

SocketOutputStream::~SocketOutputStream()
{
    delete[] m_Buffer;
    m_Buffer = nullptr;
}

uint SocketOutputStream::write(const char* buf, uint len)
{
    if (buf == nullptr && len != 0)
    {
        throw InvalidProtocolException("SocketOutputStream::write null buffer");
    }
    if (len == 0)
    {
        return 0;
    }
    if (m_Head != 0)
    {
        const auto current = length();
        std::memmove(m_Buffer, m_Buffer + m_Head, current);
        m_Head = 0;
        m_Tail = current;
    }
    if (m_Tail + len > m_BufferLen)
    {
        resize(static_cast<int>((m_Tail + len) - m_BufferLen));
    }
    std::memcpy(m_Buffer + m_Tail, buf, len);
    m_Tail += len;
    return len;
}

void SocketOutputStream::write(const Packet* pPacket)
{
    if (pPacket == nullptr)
    {
        throw InvalidProtocolException("SocketOutputStream::write null packet");
    }
    PacketID_t packetID = pPacket->getPacketID();
    write(reinterpret_cast<const char*>(&packetID), szPacketID);
    PacketSize_t packetSize = pPacket->getPacketSize();
    write(reinterpret_cast<const char*>(&packetSize), szPacketSize);
    pPacket->write(*this);
}

uint SocketOutputStream::flush()
{
    const auto flushed = length();
    m_Head = 0;
    m_Tail = 0;
    return flushed;
}

void SocketOutputStream::resize(int size)
{
    if (size <= 0)
    {
        return;
    }
    const auto current = length();
    const auto newLength = std::max<uint>(m_BufferLen + static_cast<uint>(size), current + static_cast<uint>(size) + 1U);
    auto* resized = new char[newLength];
    if (current > 0)
    {
        std::memcpy(resized, m_Buffer + m_Head, current);
    }
    delete[] m_Buffer;
    m_Buffer = resized;
    m_BufferLen = newLength;
    m_Head = 0;
    m_Tail = current;
}

uint SocketOutputStream::length() const
{
    return m_Tail >= m_Head
        ? m_Tail - m_Head
        : (m_BufferLen - m_Head) + m_Tail;
}

SocketInputStream::SocketInputStream(Socket* sock, uint BufferSize)
    : m_pSocket(sock), m_Buffer(nullptr), m_BufferLen(BufferSize), m_Head(0), m_Tail(0)
{
    if (m_BufferLen == 0)
    {
        m_BufferLen = DefaultSocketInputBufferSize;
    }
    m_Buffer = new char[m_BufferLen];
}

SocketInputStream::~SocketInputStream()
{
    delete[] m_Buffer;
    m_Buffer = nullptr;
}

void SocketInputStream::reset(const char* buf, uint len)
{
    if (buf == nullptr && len != 0)
    {
        throw InvalidProtocolException("SocketInputStream::reset null buffer");
    }
    if (len > m_BufferLen)
    {
        resize(static_cast<int>(len - m_BufferLen));
    }
    if (len > 0)
    {
        std::memcpy(m_Buffer, buf, len);
    }
    m_Head = 0;
    m_Tail = len;
}

uint SocketInputStream::read(char* buf, uint len)
{
    if (buf == nullptr && len != 0)
    {
        throw InvalidProtocolException("SocketInputStream::read null buffer");
    }
    if (len > length())
    {
        throw InsufficientDataException(len - length());
    }
    if (len == 0)
    {
        return 0;
    }
    std::memcpy(buf, m_Buffer + m_Head, len);
    m_Head += len;
    if (m_Head == m_Tail)
    {
        m_Head = 0;
        m_Tail = 0;
    }
    return len;
}

uint SocketInputStream::read(std::string& str, uint len)
{
    str.resize(len);
    if (len == 0)
    {
        return 0;
    }
    return read(str.data(), len);
}

void SocketInputStream::read(Packet* p)
{
    if (p == nullptr)
    {
        throw InvalidProtocolException("SocketInputStream::read null packet");
    }
    p->read(*this);
}

void SocketInputStream::peek(char* buf, uint len, uint pos)
{
    if (buf == nullptr && len != 0)
    {
        throw InvalidProtocolException("SocketInputStream::peek null buffer");
    }
    if (pos + len > length())
    {
        throw InsufficientDataException((pos + len) - length());
    }
    std::memcpy(buf, m_Buffer + m_Head + pos, len);
}

void SocketInputStream::skip(uint len)
{
    if (len > length())
    {
        throw InsufficientDataException(len - length());
    }
    m_Head += len;
    if (m_Head == m_Tail)
    {
        m_Head = 0;
        m_Tail = 0;
    }
}

uint SocketInputStream::fill()
{
    return 0;
}

uint SocketInputStream::fill_RAW()
{
    return 0;
}

void SocketInputStream::resize(int size)
{
    if (size <= 0)
    {
        return;
    }
    const auto current = length();
    const auto newLength = std::max<uint>(m_BufferLen + static_cast<uint>(size), current + static_cast<uint>(size) + 1U);
    auto* resized = new char[newLength];
    if (current > 0)
    {
        std::memcpy(resized, m_Buffer + m_Head, current);
    }
    delete[] m_Buffer;
    m_Buffer = resized;
    m_BufferLen = newLength;
    m_Head = 0;
    m_Tail = current;
}

uint SocketInputStream::length() const
{
    return m_Tail >= m_Head
        ? m_Tail - m_Head
        : (m_BufferLen - m_Head) + m_Tail;
}

std::string SocketInputStream::toString() const
{
    return "SocketInputStream(memory)";
}
