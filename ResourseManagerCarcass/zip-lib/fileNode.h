#ifndef FILE_NODE_H
#define FILE_NODE_H

#include <string>
#include <unistd.h>
#include <sys/stat.h>

#include "types.h"
#include "bigBuffer.h"

class FileNode {
    FileNode(zip *pZip, const char *string, const int64_t i);

    FileNode(zip *zip, const char *fname);

    friend class FuseZipData;
private:
    FileNode (const FileNode &);
    FileNode &operator= (const FileNode &);

    enum nodeState {
        CLOSED,
        OPENED,
        CHANGED,
        NEW,
        NEW_DIR
    };

    BigBuffer *buffer{};
    struct zip *zip;
    int open_count{};
    nodeState state;

    uint64_t m_size{};
    
    void parse_name();

    static const int64_t NEW_NODE_INDEX;




protected:

public:
    FileNode(struct zip *zip, char* name) {
        this->zip = zip;
        this->name = name;
    }


    inline bool isTemporaryDir() const {
        return (state == NEW_DIR) && (id == NEW_NODE_INDEX);
    }

    /**
     * Создать папку
     */
    static FileNode *createDir(struct zip *zip, const char *fname);

    static FileNode *createNodeForZipEntry(struct zip *zip,
            const char *fname);
    ~FileNode();

    void appendChild (FileNode *child);

    void detachChild (FileNode *child);

    void rename (const char *new_name);

    int open();
    int read(char *buf, size_t size, uint64_t offset);
    int write(const char *buf, size_t size, uint64_t offset);
    int close();

    /**
     * @return 0 успешно, != 0 ошибка
     */
    int save();

    int truncate(uint64_t offset);

    inline bool isChanged() const {
        return state == CHANGED || state == NEW;
    }

    inline std::string getParentName () const {
        if (name > full_name.c_str()) {
            return std::string (full_name, 0, name - full_name.c_str() - 1);
        } else {
            return "";
        }
    }

    uint64_t size() const;

    const char *name{};
    std::string full_name;
    bool is_dir{};
    int64_t id;
    nodelist_t childs;
    FileNode *parent{};
};
#endif

