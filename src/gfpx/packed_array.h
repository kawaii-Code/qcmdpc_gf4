#ifndef _PACKED_ARRAY_H
#define _PACKED_ARRAY_H

struct PackedArray {
    explicit PackedArray(std::uint64_t _size) {
        memory = new std::uint8_t[_size];
        memset(memory, 0, _size * sizeof(*memory));
        size = _size;
    }
    ~PackedArray() {
        delete[] memory;
    }

    PackedArray(const PackedArray &other) = delete;
    PackedArray(PackedArray &&other) = delete;
    PackedArray &operator=(const PackedArray &other) = delete;
    PackedArray &operator=(PackedArray &&other) = delete;

    Slice slice(std::uint64_t start, std::uint64_t len) {
        assert(memory + start + len <= memory + size);
        return Slice {
            memory + start,
            len,
        };
    }

    Slice full_slice() {
        return Slice {
            memory,
            size,
        };
    }

    std::uint8_t *memory;
    std::uint64_t size;
};

#endif // _PACKED_ARRAY_H
