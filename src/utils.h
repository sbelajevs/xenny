#pragma once

#include "system.h"

template <class Element, int MaxCount>
class FixedVec
{
public:
    FixedVec(): count(0)
    {
    }

    int size() const
    {
        return count;
    }

    void clear()
    {
        count = 0;
    }

    template <class OtherVec>
    void transfer(OtherVec& other, int amount)
    {
        for (int i=0; i<amount; i++) {
            other.push((*this)[count-amount+i]);
        }
        count -= amount;
    }

    bool empty() const
    {
        return count == 0;
    }

    void push(const Element& e)
    {
        coll[count++] = e;
    }

    void pop()
    {
        if (count > 0) {
            count--;
        }
    }

    Element& top()
    {
        return coll[count-1];
    }

    const Element& top() const
    {
        return coll[count-1];
    }

    Element& operator[](int idx)
    {
        if (idx < 0) {
            return coll[count+idx];
        } else {
            return coll[idx];
        }
    }

    const Element& operator[](int idx) const
    {
        return coll[idx];
    }

private:
    Element coll[MaxCount];
    int count;
};

template <class T>
inline T max(const T& a, const T& b)
{
    return a > b ? a : b;
}
template <class T>
inline T doMax(T& a, const T& b)
{
    return a = max(a, b);
}

template <class T>
inline T min(const T&a, const T& b)
{
    return a < b ? a : b;
}
template <class T>
inline T doMin(T&a, const T& b)
{
    return a = min(a, b);
}

inline void doFlip(bool& a)
{
    a = a == false;
}

inline void doFloor(float& arg)
{
    arg = Sys_Floor(arg);
}
