

#include <zip.h>
#include <syslog.h>
#include <cerrno>
#include <cassert>
#include <stdexcept>
#include <include/libzip/lib/zipint.h>

#include "qnxZipData.h"


FuseZipData::~FuseZipData() {
    int res = zip_close(zip);
    if (res != 0) {
        syslog(LOG_ERR, "Error while closing archive: %s", zip_strerror(zip));
    }
    for (auto i = files.begin(); i != files.end(); ++i) {
        delete i->second;
    }
}

void FuseZipData::build_tree(struct zip *zip_file, const char *fileName) {
    //Просто в цикле строим дерево
    m_root = new FileNode(zip_file,fileName);
    m_root->name = "";
    int files_total = zip_get_num_files(zip_file);
    printf("%d\n", files_total);
    for (int i = 0; i < files_total; i++) {
        insertNode(new FileNode(zip,fileName));
    };
}

int FuseZipData::removeNode(FileNode *node) {
    assert(node != NULL);
    assert(node->parent != NULL);
    node->parent->detachChild(node);
    files.erase(node->full_name.c_str());

    int64_t id = node->id;
    delete node;
    if (id >= 0) {
        return (zip_delete(zip, id) == 0) ? 0 : ENOENT;
    } else {
        return 0;
    }
}

FileNode *FuseZipData::findParent(const FileNode *node) const {
    std::string name = node->getParentName();
    return find(name.c_str());
}

void FuseZipData::insertNode(FileNode *node) {
    FileNode *parent = findParent(node);
    assert (parent != NULL);
    parent->appendChild(node);
    node->parent = parent;
    assert (files.find(node->full_name.c_str()) == files.end());
    files[node->full_name.c_str()] = node;
}

void FuseZipData::renameNode(FileNode *node, const char *newName, bool
reparent) {
    assert(node != NULL);
    assert(newName != NULL);
    FileNode *parent1 = node->parent, *parent2;
    assert (parent1 != NULL);
    if (reparent) {
        parent1->detachChild(node);
    }

    files.erase(node->full_name.c_str());
    node->rename(newName);
    files[node->full_name.c_str()] = node;

    if (reparent) {
        parent2 = findParent(node);
        assert (parent2 != NULL);
        parent2->appendChild(node);
        node->parent = parent2;
    }
}

FileNode *FuseZipData::find(const char *fname) const {
    filemap_t::const_iterator i = files.find(fname);
    if (i == files.end()) {
        return NULL;
    } else {
        return i->second;
    }
}

void FuseZipData::save() {
    for (filemap_t::const_iterator i = files.begin(); i != files.end(); ++i) {
        FileNode *node = i->second;
        if (node == m_root) {
            continue;
        }
        assert(node != NULL);
        bool save = true;
        if (node->isChanged() && !node->is_dir) {
            save = true;
            int res = node->save();
            if (res != 0) {
                save = false;
                printf("Error while saving file %s in ZIP archive: %d", res);
            }
        }
        if (save) {
            if (node->isTemporaryDir()) {
                int64_t idx = zip_dir_add(zip,
                                              node->full_name.c_str(), ZIP_FL_ENC_UTF_8);
                if (idx < 0) {
                    printf("Unable to save directory %s in ZIP archive");
                    continue;
                }
                node->id = idx;
            }
        }
    }
}



