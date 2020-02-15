
#define STANDARD_BLOCK_SIZE (512)
#define ERROR_STR_BUF_LEN 0x100


#include <zip.h>
#include <unistd.h>
#include <limits.h>
#include <syslog.h>
#include <sys/types.h>
#include <sys/statvfs.h>

#include <cerrno>
#include <cstring>
#include <cstdlib>
#include <queue>

#include "qnx-zip.h"
#include "types.h"
#include "fileNode.h"
#include "qnxZipData.h"

using namespace std;

int QnxZipCTL::qnxzip_open(const char *path) {
    if (*path == '\0') {
        return -ENOENT;
    }
    FileNode *node = get_file_node(path);
    if (node == NULL) {
        return -ENOENT;
    }
    if (node->is_dir) {
        return -EISDIR;
    }

    return node->open();
}

int QnxZipCTL::qnxzip_read(const char *path, char *buf, size_t size, uint64_t offset) {
    FileNode *f = get_file_node(path);
    return f->read(buf, size, offset);;
}

int QnxZipCTL::qnxzip_readdir(const char *path, void *buf, uint64_t offset) {//???????
    if (*path == '\0') {
        return -ENOENT;
    }
    FileNode *node = this->get_file_node(path);
    if (node == NULL) {
        return -ENOENT;
    }
    filler(buf, ".", NULL, 0);
    filler(buf, "..", NULL, 0);
    for (nodelist_t::const_iterator i = node->childs.begin(); i != node->childs.end(); ++i) {
        filler(buf, (*i)->name, NULL, 0);
    }

    return 0;
}

FileNode *QnxZipCTL::get_file_node(const char *fname) {
    return data->find(fname);
}

int QnxZipCTL::qnxzip_write(const char *path, const char *buf, size_t size, uint64_t offset) {
    FileNode *f = get_file_node(path);
    return f->write(buf, size, offset);
}

int QnxZipCTL::qnxzip_mkdir(const char *path) {
    if (*path == '\0') {
        return -ENOENT;
    }
    string dir_p = path;
    dir_p += "/";
    //syslog(LOG_INFO, "[MKDIR CALLBACK] %s", dir_p.c_str());
    FileNode *node = FileNode::createDir(this->data->zip_file, dir_p.c_str());
    if (node == NULL) {
        return -ENOMEM;
    }
    this->data->insertNode(node);
    return 0;
}

int QnxZipCTL::qnxzip_ftruncate(const char *path, uint64_t offset, struct qnx_file_info *fi) {
    if (*path == '\0') {
        return -EACCES;
    }
    FileNode *node = get_file_node(path);
    if (node == NULL) {
        return -ENOENT;
    }
    if (node->is_dir) {
        return -EISDIR;
    }
    int res;
    if ((res = node->open()) != 0) {
        return res;
    }
    if ((res = node->truncate(offset)) != 0) {
        node->close();
        return -res;
    }
    return node->close();
}

int QnxZipCTL::qnxzip_rmdir(const char *path) {
    if (*path == '\0') {
        return -ENOENT;
    }
    FileNode *node = get_file_node(path);
    if (node == NULL) {
        return -ENOENT;
    }
    if (!node->is_dir) {
        return -ENOTDIR;
    }
    if (!node->childs.empty()) {
        return -ENOTEMPTY;
    }
    return data->removeNode(node);
}



QnxZipCTL::QnxZipCTL(const char *fileName, bool readonly) {

    int err;
    struct zip *zip_file;

    if ((zip_file = zip_open(fileName, 0, &err)) == NULL) {
        printf("ERROR");
        return;
    }


    data = new FuseZipData();
    data->build_tree(zip_file,fileName);
}

int QnxZipCTL::fusezip_unlink(const char *path) {
    if (*path == '\0') {
        return ENOENT;
    }
    FileNode *node = get_file_node(path);
    if (node == NULL) {
        return -ENOENT;
    }
    if (node->is_dir) {
        return -EISDIR;
    }
    return -data->removeNode(node);
}
