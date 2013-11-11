#pragma once

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
        return coll[idx];
    }

    const Element& operator[](int idx) const
    {
        return coll[idx];
    }

private:
    Element coll[MaxCount];
    int count;
};
