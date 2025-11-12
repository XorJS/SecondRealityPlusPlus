#pragma once

namespace Blob
{
    using Handle = void *;

    Handle open(const char * filename);

    int read(void * buffer, int element_size, int element_count, Handle file);
    void seek(Handle file, int offset, int origin);
    int tell(Handle file);
}
