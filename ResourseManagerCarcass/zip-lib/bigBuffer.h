
#ifndef BIG_BUFFER_H
#define BIG_BUFFER_H

#include <zip.h>
#include <unistd.h>

#include <vector>

#include "types.h"

class BigBuffer {
private:
    static const unsigned int chunkSize = 4 * 1024; //4 кб

    class ChunkWrapper;

    typedef std::vector<ChunkWrapper> chunks_t;

    struct CallBackStruct {
        size_t pos;
        const BigBuffer *buf;
    };

    chunks_t chunks;


    static zip_int64_t zipUserFunctionCallback(void *state, void *data,
                                            uint64_t len, enum zip_source_cmd cmd);


    inline static unsigned int chunksCount(uint64_t offset) {
        return (offset + chunkSize - 1) / chunkSize;
    }


    inline static unsigned int chunkNumber(uint64_t offset) {
        return offset / chunkSize;
    }


    inline static int chunkOffset(uint64_t offset) {
        return offset % chunkSize;
    }

public:
    uint64_t len;

    BigBuffer();


    BigBuffer(struct zip *z, uint64_t nodeId, uint64_t length);

    ~BigBuffer();

    int read(char *buf, size_t size, uint64_t offset) const;

    int write(const char *buf, size_t size, uint64_t offset);

    int saveToZip(struct zip *z, const char *fname,
                  bool newFile, int64_t &index);

    void truncate(uint64_t offset);
};

#endif

