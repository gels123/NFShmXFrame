// -------------------------------------------------------------------------
//    @FileName         :    NFObjectIterator.h
//    @Author           :    gaoyi
//    @Date             :    23-6-13
//    @Email			:    445267987@qq.com
//    @Module           :    NFObjectIterator
//
// -------------------------------------------------------------------------

#pragma once

#include "NFComm/NFCore/NFPlatform.h"

class NFObject;

template<class Tp, class Ref, class Ptr, class Container>
struct NFObjectIterator
{
    typedef size_t size_type;
    typedef ptrdiff_t difference_type;
    typedef std::bidirectional_iterator_tag iterator_category;

    typedef NFObjectIterator<Tp, Tp &, Tp *, Container> iterator;
    typedef NFObjectIterator<Tp, const Tp &, const Tp *, Container> const_iterator;
    typedef NFObjectIterator self;

    typedef Tp value_type;
    typedef Ptr pointer;
    typedef Ref reference;

    Container *m_pContainer;
    int m_type;
    size_t m_pos;

    explicit NFObjectIterator(const Container *pContainer, int type, size_t pos)
            : m_pContainer(const_cast<Container *>(pContainer)), m_type(type), m_pos(pos)
    {
    }

    NFObjectIterator() : m_pContainer(nullptr), m_type(0), m_pos(0) {}

    NFObjectIterator(const iterator& x) : NFObjectIterator(x.m_pContainer, x.m_type, x.m_pos)
    {

    }

    template<class Tpx>
    NFObjectIterator(const NFObjectIterator<Tpx, Tpx&, Tpx*, Container>& x) : NFObjectIterator(x.m_pContainer, x.m_type, x.m_pos)
    {

    }

    reference operator*() const { return *(dynamic_cast<pointer>(m_pContainer->GetIterObj(m_type, m_pos))); }

    pointer operator->() const { return dynamic_cast<pointer>(m_pContainer->GetIterObj(m_type, m_pos)); }

    pointer GetObj() const { return dynamic_cast<pointer>(m_pContainer->GetIterObj(m_type, m_pos)); }

    self& operator++()
    {
        this->Incr();
        return *this;
    }

    self operator++(int)
    {
        self tmp = *this;
        this->Incr();
        return tmp;
    }

    self& operator--()
    {
        this->Decr();
        return *this;
    }

    self operator--(int)
    {
        self tmp = *this;
        this->Decr();
        return tmp;
    }

    void Incr() { m_pos = m_pContainer->IterIncr(m_type, m_pos); }

    void Decr() { m_pos = m_pContainer->IterDecr(m_type, m_pos); }

    bool operator==(const NFObjectIterator& x) const
    {
        return m_pContainer == x.m_pContainer && m_type == x.m_type && m_pos == x.m_pos;
    }

    bool operator!=(const NFObjectIterator& x) const
    {
        return !(m_pContainer == x.m_pContainer && m_type == x.m_type && m_pos == x.m_pos);
    }

    NFObjectIterator& operator=(const NFObjectIterator& x)
    {
        if (this != &x)
        {
            m_pContainer = x.m_pContainer;
            m_type = x.m_type;
            m_pos = x.m_pos;
        }

        return *this;
    }
};