
#include <cerrno>
#include <cstdlib>
#include <cstring>
#include <string>
#include <stdexcept>
#include <syslog.h>
#include <zip.h>

#include "bigBuffer.h"

//Данные файла
class BigBuffer::ChunkWrapper {
private:
    char *m_ptr; //Указатель на данные

public:

    ChunkWrapper(): m_ptr(NULL) {
    }

    ChunkWrapper(const ChunkWrapper &other) {
        m_ptr = other.m_ptr;
        const_cast<ChunkWrapper*>(&other)->m_ptr = NULL;
    }

    ~ChunkWrapper() {
        if (m_ptr != NULL) {
            free(m_ptr);
        }
    }


    ChunkWrapper &operator=(const ChunkWrapper &other) {
        if (&other != this) {
            m_ptr = other.m_ptr;
            const_cast<ChunkWrapper*>(&other)->m_ptr = NULL;
        }
        return *this;
    }

    char *ptr(bool init = false) {
        if (init && m_ptr == NULL) {
            m_ptr = (char *)malloc(chunkSize);
            if (m_ptr == NULL) {
                throw std::bad_alloc();
            }
        }
        return m_ptr;
    }

    /**
     * @param dest      Буффер назначения>.
     * @param offset    Смещение в буфере, откуда начатаь читать.
     * @param count     Кол-во байт.
     */
    size_t read(char *dest, __uint64_t offset, size_t count) const {
        if (offset + count > chunkSize) {
            count = chunkSize - offset;
        }
        if (m_ptr != NULL) {
            memcpy(dest, m_ptr + offset, count);
        } else {
            memset(dest, 0, count);
        }
        return count;
    }

    /**
     * Заполнить внутренний буфер байтами из 'src'.
     *
     * @param src       Откуда читаем.
     * @param offset    Смещение, куда пишем.
     * @param count     Кол-во байт.
     *
     * @return  Количество записанных байт, может отличаться от
     *      'count' если offset+count>chunkSize.
     */
    size_t write(const char *src, uint64_t offset, size_t count) {
        if (offset + count > chunkSize) {
            count = chunkSize - offset;
        }
        if (m_ptr == NULL) {
            m_ptr = (char *)malloc(chunkSize);
            if (offset > 0) {
                memset(m_ptr, 0, offset);
            }
        }
        memcpy(m_ptr + offset, src, count);
        return count;
    }

    /**
     * Отчистить буффер 'offset'.
     */
    void clearTail(__uint64_t offset) {
        if (m_ptr != NULL && offset < chunkSize) {
            memset(m_ptr + offset, 0, chunkSize - offset);
        }
    }

};



BigBuffer::BigBuffer(struct zip *z, uint64_t nodeId, uint64_t length):
        len(length) {
    struct zip_file *zf = zip_fopen_index(z, nodeId, 0);
    unsigned int ccount = chunksCount(length);
    chunks.resize(ccount, ChunkWrapper());
    unsigned int chunk = 0;
    int nr;
    while (length > 0) {
        __uint64_t readSize = chunkSize;
        if (readSize > length) {
            readSize = length;
        }
        nr = zip_fread(zf, chunks[chunk].ptr(true), readSize);
        if (nr < 0) {
            std::string err = zip_file_strerror(zf);
            zip_fclose(zf);
            throw std::runtime_error(err);
        }
        ++chunk;
        length -= nr;
        if ((nr == 0 || chunk == ccount) && length != 0) {
            zip_fclose(zf);
            throw std::runtime_error("data length differ");
        }
    }
    if (zip_fclose(zf)) {
        throw std::runtime_error(zip_strerror(z));
    }
}

BigBuffer::~BigBuffer() {
}

int BigBuffer::read(char *buf, size_t size, uint64_t offset) const {
    if (offset > len) {
        return 0;
    }
    uint64_t chunk = chunkNumber(offset);
    int pos = chunkOffset(offset);
    if (size > unsigned(len - offset)) {
        size = len - offset;
    }
    int nread = size;
    while (size > 0) {
        size_t r = chunks[chunk].read(buf, pos, size);

        size -= r;
        buf += r;
        ++chunk;
        pos = 0;
    }

    return nread;
}

int BigBuffer::write(const char *buf, size_t size, uint64_t offset) {
    int chunk = chunkNumber(offset);
    int pos = chunkOffset(offset);
    int nwritten = size;

    if (offset > len) {
        if (len > 0) {
            chunks[chunkNumber(len)].clearTail(chunkOffset(len));
        }
        len = size + offset;
    } else if (size > unsigned(len - offset)) {
        len = size + offset;
    }
    chunks.resize(chunksCount(len));
    while (size > 0) {
        size_t w = chunks[chunk].write(buf, pos, size);

        size -= w;
        buf += w;
        ++ chunk;
        pos = 0;
    }
    return nwritten;
}

void BigBuffer::truncate(uint64_t offset) {
    chunks.resize(chunksCount(offset));

    if (offset > len && len > 0) {
        chunks[chunkNumber(len)].clearTail(chunkOffset(len));
    }

    len = offset;
}

uint64_t BigBuffer::zipUserFunctionCallback(void *state, void *data,
                                            uint64_t len, enum zip_source_cmd cmd) {
    CallBackStruct *b = (CallBackStruct*)state;
    switch (cmd) {
        case ZIP_SOURCE_OPEN: {
            b->pos = 0;
            return 0;
        }
        case ZIP_SOURCE_READ: {
            int r = b->buf->read((char*)data, len, b->pos);
            b->pos += r;
            return r;
        }
        case ZIP_SOURCE_STAT: {
            struct zip_stat *st = (struct zip_stat*)data;
            zip_stat_init(st);
            st->valid = ZIP_STAT_SIZE | ZIP_STAT_MTIME;
            st->size = b->buf->len;
            return sizeof(struct zip_stat);
        }
        case ZIP_SOURCE_FREE: {
            delete b;
            return 0;
        }
        case ZIP_SOURCE_CLOSE:
            return 0;
    }
}

int BigBuffer::saveToZip(zip_t *z, const char *fname,
        bool newFile, int64_t &index) {
    struct zip_source *s;
    struct CallBackStruct *cbs = new CallBackStruct();
    cbs->buf = this;
    if ((s=zip_source_function(z, zipUserFunctionCallback, cbs)) == NULL) {
        delete cbs;
        return -ENOMEM;
    }
    uint64_t nid;
    if ((newFile && (nid = zip_file_add(z, fname, s, ZIP_FL_ENC_UTF_8)) < 0)
            || (!newFile && zip_file_replace(z, index, s, ZIP_FL_ENC_UTF_8) < 0)) {
        delete cbs;
        zip_source_free(s);
        return -ENOMEM;
    }
    if (newFile) {
        index = nid;
    }
    return 0;
}
