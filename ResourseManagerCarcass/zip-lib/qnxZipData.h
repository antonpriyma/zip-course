
#ifndef FUSEZIP_DATA
#define FUSEZIP_DATA

#include <string>
#include <vector>

#include "types.h"
#include "fileNode.h"

class FuseZipData {
private:

    FileNode *findParent(const FileNode *node) const;//ok


    FileNode *m_root;
    filemap_t files;

public:
    zip *zip_file;

    void build_tree(struct zip *zip_file,const char *fileName);


    int removeNode(FileNode *node);

    void insertNode(FileNode *node);

    ~FuseZipData();


    FileNode *find(const char *fname) const;;

    void save();

    void renameNode(FileNode *node, const char *newName, bool reparent);
};

#endif

