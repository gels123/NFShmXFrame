// -------------------------------------------------------------------------
//    @FileName         :    NFCircularBuffer.h
//    @Author           :    gaoyi
//    @Date             :    24-8-8
//    @Email            :    445267987@qq.com
//    @Module           :    NFCircularBuffer
//
// -------------------------------------------------------------------------

#pragma once

#include <stddef.h>
#include <string.h>
#include <assert.h>

template<typename HeadType>
struct BufferHead
{
    HeadType m_head;
    size_t m_iSize;
};

template<typename HeadType, typename Container>
class CircularBufferIterator
{
protected:
    typedef CircularBufferIterator<HeadType, Container>                _Self;
    typedef BufferHead<HeadType> HEADTYPE;
public:
    typedef ptrdiff_t                          DifferenceType;
    typedef size_t                                     SizeType;

    CircularBufferIterator()
        : m_pContainer(NULL)
        , m_iStart(0)
    {
    }

    explicit
    CircularBufferIterator(Container *pContainer, SizeType iPos)
        : m_pContainer(pContainer)
        , m_iStart(iPos)
    {
    }

    int Read(HeadType &head, char *pDataOut, size_t *piLen)
    {
        assert(m_pContainer);
        return m_pContainer->Read(m_iStart, head, pDataOut, piLen);
    }


    int Read(HeadType &head)
    {
        assert(m_pContainer);
        m_pContainer->Read((char *)&head, m_iStart, sizeof(head));

        return 0;
    }

    _Self & operator++()
    {
        assert(m_pContainer);
        m_iStart = m_pContainer->GetNextStartPos(m_iStart);
        return *this;
    }

    _Self operator++(int)
    {
        _Self __tmp = *this;
        m_iStart = m_pContainer->GetNextStartPos(m_iStart);
        return __tmp;
    }

    bool operator==(const _Self &other) const
    {
        assert(m_pContainer == other.m_pContainer);
        return m_iStart == other.m_iStart;
    }

    bool operator!=(const _Self &other) const
    {
        assert(m_pContainer == other.m_pContainer);
        return m_iStart != other.m_iStart;
    }

    Container *m_pContainer;
    size_t m_iStart;
};

//most SIZE-1 data
template<typename HeadType, size_t SIZE>
class CircularBuffer
{
public:
    typedef BufferHead<HeadType> HEADTYPE;
    typedef CircularBufferIterator<HeadType, CircularBuffer<HeadType, SIZE> > Iterator;

    void Initialize()
    {
        Clear();
    }

    bool IsEmpty()
    {
        return (m_iFront == m_iRear);
    }

    bool CanWrite(size_t iDataLen);

    size_t Size()
    {
        if (m_iRear >= m_iFront)
        {
            return m_iRear - m_iFront;
        }
        else
        {
            return SIZE - m_iFront + m_iRear;
        }
    }

    size_t MaxSize()
    {
        return SIZE-1-sizeof(HEADTYPE);
    }

    Iterator Begin()
    {
        return Iterator(this, m_iFront);
    }

    Iterator End()
    {
        return Iterator(this, m_iRear);
    }

    int Push(const HeadType &head, const char *pData, size_t iDataLen);
    int Pop();

    //first data head
    int Front(HeadType &head);

    //first data head and data
    int Front(HeadType &head, char *pData, size_t *iDataLen);

    //last data head
    int Back(HeadType &head);

    //last data head and data
    int Back(HeadType &head, char *pData, size_t *iDataLen);
    void Clear()
    {
        m_iFront = m_iRear = m_iBack = 0;
    }

protected:
    int Front(HEADTYPE &head)
    {
        Read((char *)&head, m_iFront, sizeof(head));
        return 0;
    }

    int Back(HEADTYPE &head)
    {
        Read((char *)&head, m_iBack, sizeof(head));
        return 0;
    }

    int GetNextStartPos(size_t iStartPos)
    {
        HEADTYPE bufferHead;
        Read((char *)&bufferHead, iStartPos, sizeof(bufferHead));
        return (iStartPos+sizeof(bufferHead)+bufferHead.m_iSize)%SIZE;
    }

    int Read(size_t iStartPos, HeadType &head, char *pData, size_t *iDataLen);

    //store data in buff
    void Write(const char *pData, size_t iSize);

    //takeout data from buff
    void Read(char *pData, size_t iStart, size_t iSize);

    size_t m_iFront; //head, first data start pos
    size_t m_iBack; //back, last data start pos
    size_t m_iRear; //end of buff, next data will store base on rear

    char m_szBuff[SIZE];

    friend class CircularBufferIterator<HeadType, CircularBuffer<HeadType, SIZE> >;
};

template<typename HeadType, size_t SIZE>
bool CircularBuffer<HeadType, SIZE>::CanWrite(size_t iDataLen)
{
    size_t iWritableDataLen = iDataLen+1;//to keep rear not = front again

    if (iWritableDataLen > SIZE)
    {
        return false;
    }

    if (m_iRear >= m_iFront)
    {
        return SIZE - m_iRear + m_iFront >= iWritableDataLen+sizeof(HEADTYPE);
    }
    else
    {
        return m_iFront - m_iRear >= iWritableDataLen+sizeof(HEADTYPE);
    }
}

template<typename HeadType, size_t SIZE>
int CircularBuffer<HeadType, SIZE>::Push(const HeadType &head, const char *pData, size_t iDataLen)
{
    if (!CanWrite(iDataLen))
    {
        return -1;
    }

    HEADTYPE bufferHead;
    bufferHead.m_head = head;
    bufferHead.m_iSize = iDataLen;
    m_iBack = m_iRear;
    Write((char *)&bufferHead, sizeof(bufferHead));
    Write(pData, iDataLen);

    return 0;
}

template<typename HeadType, size_t SIZE>
int CircularBuffer<HeadType, SIZE>::Pop()
{
    if (!IsEmpty())
    {
        HEADTYPE bufferHead;
        Read((char *)&bufferHead, m_iFront, sizeof(bufferHead));
        m_iFront=(m_iFront+bufferHead.m_iSize+sizeof(bufferHead))%SIZE;
    }
    else
    {
        return -1;
    }

    return 0;
}

//first data head
template<typename HeadType, size_t SIZE>
int CircularBuffer<HeadType, SIZE>::Front(HeadType &head)
{
    if (!IsEmpty())
    {
        Read((char *)&head, m_iFront, sizeof(head));
    }
    else
    {
        return -1;
    }

    return 0;
}

template<typename HeadType, size_t SIZE>
int CircularBuffer<HeadType, SIZE>::Front(HeadType &head, char *pData, size_t *iDataLen)
{
    if (!IsEmpty())
    {
        return Read(m_iFront, head, pData, iDataLen);
    }
    else
    {
        return -1;
    }

    return 0;
}

template<typename HeadType, size_t SIZE>
int CircularBuffer<HeadType, SIZE>::Back(HeadType &head)
{
    if (!IsEmpty())
    {
        Read((char *)&head, m_iBack, sizeof(head));
    }
    else
    {
        return -1;
    }

    return 0;
}

template<typename HeadType, size_t SIZE>
int CircularBuffer<HeadType, SIZE>::Back(HeadType &head, char *pData, size_t *iDataLen)
{
    if (!IsEmpty())
    {
        return Read(m_iBack, head, pData, iDataLen);
    }
    else
    {
        return -1;
    }

    return 0;
}

template<typename HeadType, size_t SIZE>
int CircularBuffer<HeadType, SIZE>::Read(size_t iStartPos, HeadType &head, char *pData, size_t *iDataLen)
{
    HEADTYPE bufferHead;
    Read((char *)&bufferHead, iStartPos, sizeof(bufferHead));

    if (*iDataLen < bufferHead.m_iSize)
    {
        return -1;
    }

    size_t iDataStart = (iStartPos+sizeof(bufferHead))%SIZE;
    Read(pData, iDataStart, bufferHead.m_iSize);
    head = bufferHead.m_head;
    *iDataLen = bufferHead.m_iSize;

    return 0;
}

template<typename HeadType, size_t SIZE>
void CircularBuffer<HeadType, SIZE>::Write(const char *pData, size_t iSize)
{
    size_t iRearLeftLen = SIZE - m_iRear;

    if (m_iRear >= m_iFront && iRearLeftLen < iSize)
    {
        memcpy(&m_szBuff[m_iRear], pData, iRearLeftLen);
        memcpy(m_szBuff, &(pData[iRearLeftLen]), iSize-iRearLeftLen);
    }
    else
    {
        memcpy(&(m_szBuff[m_iRear]), pData, iSize);
    }

    m_iRear = (m_iRear + iSize)%SIZE;
}

template<typename HeadType, size_t SIZE>
void CircularBuffer<HeadType, SIZE>::Read(char *pData, size_t iStart, size_t iSize)
{
    if (SIZE-iStart < iSize)
    {
        memcpy(pData, &(m_szBuff[iStart]), SIZE-iStart);
        memcpy(&(pData[SIZE-iStart]), m_szBuff, iSize-SIZE+iStart);
    }
    else
    {
        memcpy(pData, &(m_szBuff[iStart]), iSize);
    }
}
