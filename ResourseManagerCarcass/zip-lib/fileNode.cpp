#define __STDC_LIMIT_MACROS

#include <cerrno>
#include <climits>
#include <ctime>
#include <cstdlib>
#include <stdint.h>
#include <cstring>
#include <stdexcept>
#include <syslog.h>
#include <cassert>

#include "fileNode.h"


const int64_t FileNode::NEW_NODE_INDEX = -2;


FileNode::FileNode(struct zip *zip, const char *fname) {
    this->zip = zip;
    full_name = fname;
}

FileNode *FileNode::createDir(struct zip *zip, const char *fname) {
    FileNode *n = createNodeForZipEntry(zip, fname);
    if (n == NULL) {
        return NULL;
    }
    n->state = CLOSED;
    n->is_dir = true;

    return n;
}


FileNode *FileNode::createNodeForZipEntry(struct zip *zip,
                                          const char *fname) {
    FileNode *n = new FileNode(zip, fname);
    n->is_dir = false;
    n->open_count = 0;
    n->state = CLOSED;


    n->parse_name();

    return n;
}

FileNode::~FileNode() {
    if (state == OPENED || state == CHANGED || state == NEW) {
        delete buffer;
    }
}

/**
 * Если последний '/' -> это директория
 */
void FileNode::parse_name() {
    assert(!full_name.empty());

    const char *lsl = full_name.c_str();
    while (*lsl++) {}
    lsl--;
    while (lsl > full_name.c_str() && *lsl != '/') {
        lsl--;
    }
    if (*lsl == '/' && *(lsl + 1) == '\0') {
        full_name[full_name.size() - 1] = 0;
        this->is_dir = true;
        while (lsl > full_name.c_str() && *lsl != '/') {
            lsl--;
        }
    }
    if (*lsl == '/') {
        lsl++;
    }
    this->name = lsl;
}

void FileNode::appendChild(FileNode *child) {
    childs.push_back(child);
}

void FileNode::detachChild(FileNode *child) {
    childs.remove(child);
}

void FileNode::rename(const char *new_name) {
    full_name = new_name;
    parse_name();
}

int FileNode::open() {
    if (state == NEW) {
        return 0;
    }
    if (state == OPENED) {
        if (open_count == INT_MAX) {
            return -EMFILE;
        } else {
            ++open_count;
        }
    }
    if (state == CLOSED) {
        open_count = 1;
        assert (zip != NULL);
        buffer = new BigBuffer(zip, id, m_size);
        state = OPENED;

    }
    return 0;
}

int FileNode::read(char *buf, size_t sz, uint64_t offset) {
    return buffer->read(buf, sz, offset);
}

int FileNode::write(const char *buf, size_t sz, uint64_t offset) {
    if (state == OPENED) {
        state = CHANGED;
    }
    return buffer->write(buf, sz, offset);
}

int FileNode::close() {
    m_size = buffer->len;
    if (state == OPENED && --open_count == 0) {
        delete buffer;
        state = CLOSED;
    }
    return 0;
}

int FileNode::save() {
    assert (!is_dir);
    // index is modified if state == NEW
    assert (zip != NULL);
    return buffer->saveToZip(zip, full_name.c_str(),
                             state == NEW, id);
}


int FileNode::truncate(uint64_t offset) {
    if (state != CLOSED) {
        if (state != NEW) {
            state = CHANGED;
        }
        buffer->truncate(offset);
        return 0;

    } else {
        return EBADF;
    }
}

uint64_t FileNode::size() const {
    if (state == NEW || state == OPENED || state == CHANGED) {
        return buffer->len;
    } else {
        return m_size;
    }
}


