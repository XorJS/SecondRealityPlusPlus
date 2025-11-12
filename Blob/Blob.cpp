#include "Blob.h"

#include <cstring>
#include <stdint.h>
#include <stdio.h>

#include <unordered_map>

namespace Blob
{
    namespace Data
    {
        extern unsigned char _Blob_343536862[];
        extern unsigned char _Blob_324104981[];
        extern unsigned char _Blob_301510314[];
        extern unsigned char _Blob_301575852[];
        extern unsigned char _Blob_301641390[];
        extern unsigned char _Blob_305442322[];
        extern unsigned char _Blob_307867470[];
        extern unsigned char _Blob_307933000[];
        extern unsigned char _Blob_301772330[];
        extern unsigned char _Blob_301837868[];
        extern unsigned char _Blob_301903406[];
        extern unsigned char _Blob_301968928[];
        extern unsigned char _Blob_302034466[];
        extern unsigned char _Blob_302100004[];
        extern unsigned char _Blob_302165542[];
        extern unsigned char _Blob_302231096[];
        extern unsigned char _Blob_302296634[];
        extern unsigned char _Blob_305704594[];
        extern unsigned char _Blob_301772332[];
        extern unsigned char _Blob_301837870[];
        extern unsigned char _Blob_301903400[];
        extern unsigned char _Blob_301968938[];
        extern unsigned char _Blob_302034468[];
        extern unsigned char _Blob_302100006[];
        extern unsigned char _Blob_302165536[];
        extern unsigned char _Blob_302231074[];
        extern unsigned char _Blob_302296636[];
        extern unsigned char _Blob_302362174[];
        extern unsigned char _Blob_301837856[];
        extern unsigned char _Blob_301903394[];
        extern unsigned char _Blob_301968932[];
        extern unsigned char _Blob_302034470[];
        extern unsigned char _Blob_302100008[];
        extern unsigned char _Blob_302165546[];
        extern unsigned char _Blob_302231084[];
        extern unsigned char _Blob_302296622[];
        extern unsigned char _Blob_302362160[];
        extern unsigned char _Blob_302427698[];
        extern unsigned char _Blob_301903396[];
        extern unsigned char _Blob_301968934[];
        extern unsigned char _Blob_302034464[];
        extern unsigned char _Blob_302100002[];
        extern unsigned char _Blob_302165548[];
        extern unsigned char _Blob_302231086[];
        extern unsigned char _Blob_302296616[];
        extern unsigned char _Blob_302362154[];
        extern unsigned char _Blob_302427700[];
        extern unsigned char _Blob_302493238[];
        extern unsigned char _Blob_301968952[];
        extern unsigned char _Blob_302034490[];
        extern unsigned char _Blob_302100028[];
        extern unsigned char _Blob_308129742[];
        extern unsigned char _Blob_308195272[];
    }

    struct BlobInfo
    {
        const void * ptr;
        unsigned int size;
        unsigned int offset;
    };

    std::unordered_map<unsigned int, BlobInfo> blobs = { { 343536862, { Data::_Blob_343536862, 6141, 0 } }, { 324104981, { Data::_Blob_324104981, 70718, 0 } },
                                                         { 301510314, { Data::_Blob_301510314, 8928, 0 } }, { 301575852, { Data::_Blob_301575852, 7684, 0 } },
                                                         { 301641390, { Data::_Blob_301641390, 2032, 0 } }, { 305442322, { Data::_Blob_305442322, 796, 0 } },
                                                         { 307867470, { Data::_Blob_307867470, 8, 0 } },    { 307933000, { Data::_Blob_307933000, 16081, 0 } },
                                                         { 301772330, { Data::_Blob_301772330, 832, 0 } },  { 301837868, { Data::_Blob_301837868, 1756, 0 } },
                                                         { 301903406, { Data::_Blob_301903406, 836, 0 } },  { 301968928, { Data::_Blob_301968928, 800, 0 } },
                                                         { 302034466, { Data::_Blob_302034466, 916, 0 } },  { 302100004, { Data::_Blob_302100004, 1292, 0 } },
                                                         { 302165542, { Data::_Blob_302165542, 1024, 0 } }, { 302231096, { Data::_Blob_302231096, 1260, 0 } },
                                                         { 302296634, { Data::_Blob_302296634, 4044, 0 } }, { 305704594, { Data::_Blob_305704594, 900, 0 } },
                                                         { 301772332, { Data::_Blob_301772332, 448, 0 } },  { 301837870, { Data::_Blob_301837870, 532, 0 } },
                                                         { 301903400, { Data::_Blob_301903400, 1404, 0 } }, { 301968938, { Data::_Blob_301968938, 796, 0 } },
                                                         { 302034468, { Data::_Blob_302034468, 564, 0 } },  { 302100006, { Data::_Blob_302100006, 800, 0 } },
                                                         { 302165536, { Data::_Blob_302165536, 448, 0 } },  { 302231074, { Data::_Blob_302231074, 1600, 0 } },
                                                         { 302296636, { Data::_Blob_302296636, 1432, 0 } }, { 302362174, { Data::_Blob_302362174, 796, 0 } },
                                                         { 301837856, { Data::_Blob_301837856, 756, 0 } },  { 301903394, { Data::_Blob_301903394, 1760, 0 } },
                                                         { 301968932, { Data::_Blob_301968932, 448, 0 } },  { 302034470, { Data::_Blob_302034470, 12672, 0 } },
                                                         { 302100008, { Data::_Blob_302100008, 3704, 0 } }, { 302165546, { Data::_Blob_302165546, 796, 0 } },
                                                         { 302231084, { Data::_Blob_302231084, 1252, 0 } }, { 302296622, { Data::_Blob_302296622, 2212, 0 } },
                                                         { 302362160, { Data::_Blob_302362160, 8968, 0 } }, { 302427698, { Data::_Blob_302427698, 796, 0 } },
                                                         { 301903396, { Data::_Blob_301903396, 1756, 0 } }, { 301968934, { Data::_Blob_301968934, 448, 0 } },
                                                         { 302034464, { Data::_Blob_302034464, 1572, 0 } }, { 302100002, { Data::_Blob_302100002, 796, 0 } },
                                                         { 302165548, { Data::_Blob_302165548, 4092, 0 } }, { 302231086, { Data::_Blob_302231086, 1048, 0 } },
                                                         { 302296616, { Data::_Blob_302296616, 3696, 0 } }, { 302362154, { Data::_Blob_302362154, 2716, 0 } },
                                                         { 302427700, { Data::_Blob_302427700, 3356, 0 } }, { 302493238, { Data::_Blob_302493238, 1900, 0 } },
                                                         { 301968952, { Data::_Blob_301968952, 1116, 0 } }, { 302034490, { Data::_Blob_302034490, 796, 0 } },
                                                         { 302100028, { Data::_Blob_302100028, 796, 0 } },  { 308129742, { Data::_Blob_308129742, 8, 0 } },
                                                         { 308195272, { Data::_Blob_308195272, 77069, 0 } } };

    inline uint16_t rotl16(uint16_t x, unsigned r)
    {
        return (uint16_t)((x << (r & 15)) | (x >> ((16 - r) & 15)));
    }

    uint32_t gethash(const char * p)
    {
        uint16_t u1 = 0x1111;
        uint16_t u2 = 0x1111;

        for (const unsigned char * s = (const unsigned char *)p; *s; ++s)
        {
            uint16_t ax = (uint16_t)((uint16_t)*s & ~0x20u);

            u1 ^= ax;
            u1 = rotl16(u1, 1);
            u2 = (uint16_t)(u2 + ax);
        }

        return ((uint32_t)u2 << 16) | (uint32_t)u1;
    }

    Handle open(const char * filename)
    {
        unsigned int hash = gethash(filename);

        if (auto blob = blobs.find(hash); blob != blobs.end())
        {
            auto & currentBlob = blob->second;
            currentBlob.offset = 0;
            return reinterpret_cast<Handle>(&currentBlob);
        }

        return nullptr;
    }

    int read(void * buffer, int element_size, int element_count, Handle file)
    {
        BlobInfo * blob = reinterpret_cast<BlobInfo *>(file);

        size_t count = element_size * element_count;

        if (count > blob->size - blob->offset)
        {
            count = blob->size - blob->offset;
        }

        memcpy(buffer, reinterpret_cast<const unsigned char *>(blob->ptr) + blob->offset, count);

        blob->offset += static_cast<unsigned int>(count);

        return static_cast<int>(count);
    }

    void seek(Handle file, int offset, int origin)
    {
        BlobInfo * blob = reinterpret_cast<BlobInfo *>(file);

        switch (origin)
        {
            case SEEK_SET:
                blob->offset = offset;
                break;
            case SEEK_CUR:
                blob->offset += offset;
                break;
            case SEEK_END:
                blob->offset = blob->size - offset;
                break;
        }
    }

    int tell(Handle file)
    {
        return reinterpret_cast<BlobInfo *>(file)->offset;
    }
}