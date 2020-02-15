
#ifndef FUSEZIP_TYPES_H
#define FUSEZIP_TYPES_H

#include <unistd.h>

#include <cstring>
#include <cstdlib>
#include <list>
#include <map>

class FileNode;
class FuseZipData;

struct ltstr {
    bool operator() (const char* s1, const char* s2) const {
        const char *e1, *e2;
        char cmp1, cmp2;

        for (e1 = s1 + strlen(s1) - 1; e1 > s1 && *e1 == '/'; --e1);
        for (e2 = s2 + strlen(s2) - 1; e2 > s2 && *e2 == '/'; --e2);

        for (;s1 <= e1 && s2 <= e2 && *s1 == *s2; s1++, s2++);
        cmp1 = (s1 <= e1) ? *s1 : '\0';
        cmp2 = (s2 <= e2) ? *s2 : '\0';
        return cmp1 < cmp2;
    }
};

typedef std::list <FileNode*> nodelist_t;
typedef std::map <const char*, FileNode*, ltstr> filemap_t;

#endif

