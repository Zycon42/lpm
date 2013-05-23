/**
 * @file bitarray.h
 *
 * @author Jan Dušek <xdusek17@stud.fit.vutbr.cz>
 * @date 2013
 *
 * File with class BitArray
 */

#ifndef BITARRAY_H
#define	BITARRAY_H

#include <stdexcept>
#include <cstring>
#include <climits>
#include <iterator>
#include <cassert>
#include <cstddef>

template <size_t N>
class BitArray
{
public:
    class Reference;
    class IteratorBase;
    class ConstIterator;
    class Iterator;

    typedef unsigned char storage_type;
    typedef bool const_reference;
    typedef Reference reference;
    typedef ConstIterator const_iterator;
    typedef Iterator iterator;

    static size_t byteSize() { return N; }

    BitArray() : bitCount(0) { }
    BitArray(unsigned char* bits, size_t nbits) : bitCount(nbits) {
        size_t byteCount = getByteCount(nbits);
        if (byteCount > byteSize())
            throw std::runtime_error("BitArray cannot contain enough bits.");

        memcpy(this->bits, bits, byteCount);
    }
	
	unsigned char* internalStorage() {
		return bits;
	}
	
	void setSize(size_t nbits) {
		assert(nbits <= byteSize() * WORD_BITS);
		bitCount = nbits;
	}

    bool empty() const {
        return bitCount == 0;
    }

    size_t size() const {
        return bitCount;
    }

    iterator begin() {
        return iterator(bits, 0);
    }

    const_iterator begin() const {
        return const_iterator(bits, 0);
    }

    iterator end() {
        return iterator(bits + byteSize(), 0);
    }

    const_iterator end() const {
        return const_iterator(bits + byteSize(), 0);
    }

    BitArray<N> mid(size_t pos, ssize_t n) const {
        assert(pos + n <= byteSize() * CHAR_BIT);

        BitArray<N> ret = *this << pos;
        ret.bitCount = n;
        return ret;
    }

    size_t firstDifferentBit(const BitArray& other, size_t n) const {
        size_t result = 0;
        for (size_t i = 0; i * 8 < n; i++) {
            unsigned int r;
            if ((r = bits[i] ^ other.bits[i]) == 0) {
                result = (i + 1) * 8;
                continue;
            }

            // clz works on int so we need to fit result for char.
            // r is guaranteed to have one's only in last byte
            result = i * 8 + (__builtin_clz(r) - (sizeof(int) - sizeof(char)) * 8);
            break;
        }

        if (result > n)
            result = n;

        return result;
    }

    bool compareBits(const BitArray& other, size_t nbits) const {
        if (nbits > bitCount || nbits > other.bitCount)
            return false;

        size_t n = nbits / WORD_BITS;
        size_t offset = nbits % WORD_BITS;

        if (memcmp(bits, other.bits, n) != 0)
            return false;

        if (offset != 0) {
            storage_type lhsLastByte = bits[n];
            storage_type rhsLastByte = other.bits[n];
            storage_type mask = (~0) << (WORD_BITS - offset);

            if ((lhsLastByte & mask) != (rhsLastByte & mask))
                return false;
        }

        return true;
    }

    const_reference operator[](size_t pos) const {
        size_t n = pos / WORD_BITS;
        size_t offset = pos % WORD_BITS;
        return *const_iterator(const_cast<storage_type*>(bits) + n, offset);
    }

    reference operator[](size_t pos) {
        size_t n = pos / WORD_BITS;
        size_t offset = pos % WORD_BITS;
        return *iterator(bits + n, offset);
    }

    BitArray<N>& operator&=(const BitArray<N>& rhs) {
        for (size_t i = 0; i < byteSize(); i++) {
            bits[i] &= rhs.bits[i];
        }
        return *this;
    }

    BitArray<N>& operator|=(const BitArray<N>& rhs) {
        for (size_t i = 0; i < byteSize(); i++) {
            bits[i] |= rhs.bits[i];
        }
        return *this;
    }

    BitArray<N>& operator^=(const BitArray<N>& rhs) {
        for (size_t i = 0; i < byteSize(); i++) {
            bits[i] ^= rhs.bits[i];
        }
        return *this;
    }

    BitArray<N> operator~() const {
        BitArray<N> tmp(*this);
        for (size_t i = 0; i < byteSize(); i++) {
            tmp.bits[i] = ~tmp.bits[i];
        }

        return tmp;
    }

    BitArray<N>& operator<<=(size_t pos) {
        if (pos != 0) {
            size_t n = pos / WORD_BITS;
            size_t offset = pos % WORD_BITS;

            if (offset == 0) {
                for (size_t i = byteSize() - 1; i >= n; i--)
                    bits[i] = bits[i - n];
            } else {
                size_t suboffset = WORD_BITS - offset;
                for (size_t i = byteSize() - 1; i > n; i--)
                    bits[i] = (bits[i - n] << offset) | (bits[i - n - 1] >> suboffset);
                bits[n] = bits[0] << offset;
            }

            std::fill(bits, bits + n, static_cast<storage_type>(0));
        }

        return *this;
    }

    BitArray<N>& operator>>=(size_t pos) {
        if (pos != 0) {
            size_t n = pos / WORD_BITS;
            size_t offset = pos % WORD_BITS;
            size_t limit = byteSize() - n - 1;

            if (offset == 0) {
                for (size_t i = 0; i <= limit; i++)
                    bits[i] = bits[i + n];
            } else {
                size_t suboffset = WORD_BITS - offset;
                for (size_t i = 0; i <= limit; i++)
                    bits[i] = (bits[i + n] >> offset) | (bits[i + n + 1] << suboffset);
                bits[limit] = bits[byteSize() - 1] >> offset;
            }

            std::fill(bits + limit + 1, bits + byteSize(), static_cast<storage_type>(0));
        }

        return *this;
    }

private:
    template <size_t V>
    friend bool operator<(const BitArray<V>&, const BitArray<V>&);
    template <size_t V>
    friend bool operator==(const BitArray<V>&, const BitArray<V>&);
    template <size_t V>
    friend std::ostream& operator<<(std::ostream&, const BitArray<V>&);

    static size_t getByteCount(size_t nbits) {
        return (nbits + 7) >> 3;
    }

    static const int WORD_BITS = CHAR_BIT * sizeof(storage_type);

    unsigned char bits[N];
    size_t bitCount;
};

template <size_t N>
inline bool operator<(const BitArray<N>& lhs, const BitArray<N>& rhs) {
    if (lhs.bitCount < rhs.bitCount)
        return true;
    if (lhs.bitCount > rhs.bitCount)
        return false;

    // bitCount equal

    int c = strncmp((const char*)lhs.bits, (const char*)rhs.bits, BitArray<N>::getByteCount(lhs.bitCount));
    if (c < 0)
        return true;
    if (c > 0)
        return false;
    return false;
}

template <size_t N>
inline bool operator>(const BitArray<N>& lhs, const BitArray<N>& rhs) {
    return operator<(rhs, lhs);
}

template <size_t N>
inline bool operator>=(const BitArray<N>& lhs, const BitArray<N>& rhs) {
    return !operator<(lhs, rhs);
}

template <size_t N>
inline bool operator<=(const BitArray<N>& lhs, const BitArray<N>& rhs) {
    return operator>(lhs, rhs);
}

template <size_t N>
inline bool operator==(const BitArray<N>& lhs, const BitArray<N>& rhs) {
    if (lhs.bitCount == rhs.bitCount) {
        return lhs.compareBits(rhs, lhs.bitCount);
//        size_t n = lhs.bitCount / BitArray<N>::WORD_BITS;
//        size_t offset = lhs.bitCount % BitArray<N>::WORD_BITS;
//
//        if (memcmp(lhs.bits, rhs.bits, n) != 0)
//            return false;
//
//        if (offset != 0) {
//            typename BitArray<N>::storage_type lhsLastByte = lhs.bits[n + 1];
//            typename BitArray<N>::storage_type rhsLastByte = lhs.bits[n + 1];
//            typename BitArray<N>::storage_type mask =
//                (static_cast<typename BitArray<N>::storage_type>(1) << offset) - 1;
//
//            if (lhsLastByte & mask != rhsLastByte & mask)
//                return false;
//        }
//
//        return true;
    }
    return false;
}

template <size_t N>
inline bool operator!=(const BitArray<N>& lhs, const BitArray<N>& rhs) {
    return !operator==(lhs, rhs);
}

template <size_t N>
inline BitArray<N> operator&(BitArray<N> lhs, const BitArray<N>& rhs) {
    lhs &= rhs;
    return lhs;
}

template <size_t N>
inline BitArray<N> operator|(BitArray<N> lhs, const BitArray<N>& rhs) {
    lhs |= rhs;
    return lhs;
}

template <size_t N>
inline BitArray<N> operator^(BitArray<N> lhs, const BitArray<N>& rhs) {
    lhs ^= rhs;
    return lhs;
}

template <size_t N>
inline BitArray<N> operator<<(BitArray<N> lhs, size_t n) {
    lhs <<= n;
    return lhs;
}

template <size_t N>
inline BitArray<N> operator>>(BitArray<N> lhs, size_t n) {
    lhs >>= n;
    return lhs;
}

template <size_t N>
inline std::ostream& operator<<(std::ostream& stream, const BitArray<N>& val) {
    for (size_t i = 0; i < N; i++) {
        stream << (int)val.bits[i];
        stream << ".";
    }
    stream << "/" << val.bitCount;
    return stream;
}

template <size_t N>
class BitArray<N>::Reference
{
public:
    Reference(storage_type* p, storage_type mask) : p(p), mask(mask) { }
    Reference() : p(NULL), mask(0) { }

    operator bool() const {
        return !!(*p & mask);
    }

    Reference& operator=(bool x) {
        if (x)
            *p |= mask;
        else
            *p &= ~mask;
        return *this;
    }

    Reference& operator=(const Reference& x) {
        return *this = bool(x);
    }

    bool operator==(const Reference& rhs) const {
        return bool(*this) == bool(rhs);
    }

    bool operator<(const Reference& rhs) const {
        return !bool(*this) && bool(rhs);
    }

private:
    storage_type* p;
    storage_type mask;
};

template <size_t N>
class BitArray<N>::IteratorBase : public std::iterator<std::random_access_iterator_tag, bool>
{
public:
    typedef std::iterator<std::random_access_iterator_tag, bool> base_type;
    typedef base_type::difference_type difference_type;

    bool operator==(const IteratorBase& rhs) const {
        return (p == rhs.p && offset == rhs.offset);
    }

    bool operator<(const IteratorBase& rhs) const {
        return p < rhs.p || (p == rhs.p && offset < rhs.offset);
    }

    bool operator!=(const IteratorBase& rhs) const {
        return !(*this == rhs);
    }

    bool operator>(const IteratorBase& rhs) const {
        return rhs < *this;
    }

    bool operator<=(const IteratorBase& rhs) const {
        return !(rhs < *this);
    }

    bool operator>=(const IteratorBase& rhs) const {
        return !(*this < rhs);
    }

    ptrdiff_t diff(const IteratorBase& other) {
        return WORD_BITS * (p - other.p) + offset - other.offset;
    }
protected:
    IteratorBase(storage_type* p, storage_type offset) : p(p), offset(offset) { }

    void bumpUp() {
        if (offset++ == WORD_BITS - 1) {
            offset = 0;
            ++p;
        }
    }

    void bumpDown() {
        if (offset-- == 0) {
            offset = WORD_BITS - 1;
            --p;
        }
    }

    void incr(ptrdiff_t i) {
        difference_type n = i + offset;
        p += n / WORD_BITS;
        n = n % WORD_BITS;
        if (n < 0) {
            n += WORD_BITS;
            --p;
        }
        offset = n;
    }

    template <size_t V>
    friend ptrdiff_t operator-(const typename BitArray<V>::IteratorBase& lhs,
        const typename BitArray<V>::IteratorBase& rhs);
    storage_type* p;
    storage_type offset;
};

template <size_t N>
inline ptrdiff_t operator-(const typename BitArray<N>::IteratorBase& lhs,
        const typename BitArray<N>::IteratorBase& rhs) {
    return lhs.diff(rhs);
}

template <size_t N>
class BitArray<N>::ConstIterator : public BitArray<N>::IteratorBase
{
public:
    typedef BitArray<N>::IteratorBase base_type;
    typedef BitArray::reference    reference;
    typedef bool                const_reference;
    typedef const bool*         pointer;
    typedef ConstIterator       const_iterator;
    typedef typename IteratorBase::difference_type difference_type;

    ConstIterator() : IteratorBase(NULL, 0) { }
    ConstIterator(unsigned char* p, unsigned char offset) : IteratorBase(p, offset) { }
    ConstIterator(const ConstIterator& other) : IteratorBase(other.p, other.offset) { }

    const_reference operator*() const {
        return reference(base_type::p, (1 << (WORD_BITS - 1)) >> base_type::offset);
    }

    const_iterator& operator++() {
        base_type::bumpUp();
        return *this;
    }

    const_iterator operator++(int) {
        const_iterator tmp = *this;
        base_type::bumpUp();
        return tmp;
    }

    const_iterator& operator--() {
        base_type::bumpDown();
        return *this;
    }

    const_iterator operator--(int) {
        const_iterator tmp = *this;
        base_type::bumpDown();
        return tmp;
    }

    const_iterator& operator+=(difference_type i) {
        base_type::incr(i);
        return *this;
    }

    const_iterator& operator-=(difference_type i) {
        *this += -i;
        return *this;
    }

    const_iterator operator+(difference_type i) const {
        const_iterator tmp = *this;
        return tmp += i;
    }

    const_iterator operator-(difference_type i) const {
        const_iterator tmp = *this;
        return tmp -= i;
    }

    const_reference operator[](difference_type i) const {
        return *(*this + i);
    }
};

template <size_t N>
class BitArray<N>::Iterator : public BitArray<N>::IteratorBase
{
public:
    typedef BitArray<N>::IteratorBase base_type;
    typedef BitArray::reference reference;
    typedef BitArray::reference* pointer;
    typedef Iterator iterator;
    typedef typename IteratorBase::difference_type difference_type;

    Iterator() : IteratorBase(NULL, 0) { }
    Iterator(storage_type* p, storage_type offset) : IteratorBase(p, offset) { }

    reference operator*() const {
        return reference(base_type::p, (1 << (WORD_BITS - 1)) >> base_type::offset);
    }

    iterator& operator++() {
        base_type::bumpUp();
        return *this;
    }

    iterator operator++(int) {
        iterator tmp = *this;
        base_type::bumpUp();
        return tmp;
    }

    iterator& operator--() {
        base_type::bumpDown();
        return *this;
    }

    iterator operator--(int) {
        iterator tmp = *this;
        base_type::bumpDown();
        return tmp;
    }

    iterator& operator+=(difference_type i) {
        base_type::incr(i);
        return -this;
    }

    iterator& operator-=(difference_type i) {
        *this += -i;
        return *this;
    }

    iterator operator+(difference_type i) const {
        iterator tmp = *this;
        return tmp += i;
    }

    iterator operator-(difference_type i) const {
        iterator tmp = *this;
        return tmp -= i;
    }

    reference operator[](difference_type i) const {
        return *(*this + i);
    }
};

#endif	/* BITARRAY_H */

