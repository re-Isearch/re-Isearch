/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include <iostream>
#include <algorithm>

template<class T> class sVector {
private:
    T *data;
    size_t maxlen;
    size_t currlen;
public:
    sVector<T> () : data (nullptr), maxlen(0), currlen(0) { }
    sVector<T> (int maxlen) : data (new T [maxlen]), maxlen(maxlen), currlen(0) { }

    sVector<T> (const sVector& o) {
        std::cout << "copy ctor called" << std::endl;
        data = new T [o.maxlen];
        maxlen = o.maxlen;
        currlen = o.currlen;
        std::copy(o.data, o.data + o.maxlen, data);
    }

    sVector<T> (const sVector<T>&& o) {
        std::cout << "move ctor called" << std::endl;
        data = o.data;
        maxlen = o.maxlen;
        currlen = o.currlen;
    }

    void push_back (const T& i) {
        if (currlen >= maxlen) {
            maxlen *= 2;
            auto newdata = new T [maxlen];

            std::copy(data, data + currlen, newdata);
            if (data) {
                delete[] data;
            }
            data = newdata;
        }
        data[currlen++] = i;
    }

    friend std::ostream& operator<<(std::ostream &os, const sVector<T>& o) {
        auto s = o.data;
        auto e = o.data + o.currlen;;
        while (s < e) {
            os << "[" << *s << "]";
            s++;
        }
        return os;
    }
};

#ifdef EBUG
int main() {
    auto c = new sVector<int>(1);
    for (size_t i = 10; i < 1000000 ; i++)
      c->push_back(i);
  //  std::cout << *c << std::endl;
    std::cout << "DONE: "<<std::endl;
}
#endif
