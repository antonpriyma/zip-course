
#ifndef FUSE_ZIP_H
#define FUSE_ZIP_H

#include <cstddef>
#include <stdint.h>
#include "qnxZipData.h"

/**
 *  Функции будут вызваны из менеджера ресурсов QNX
 */




class QnxZipCTL{
public:

    FuseZipData *data;

    QnxZipCTL(const char *fileName,
              bool readonly);

    int qnxzip_open(const char *path); //открытие файла


    int qnxzip_read(const char *path, char *buf, size_t size, uint64_t offset); //чтение файла

    int qnxzip_readdir(const char *path, void *buf, uint64_t offset);//чтение папки ?????

    int qnxzip_write(const char *path, const char *buf, size_t size, uint64_t offset); //запись в файл

    int qnxzip_mkdir(const char *path);//создание папки

    int fusezip_unlink(const char *path);

    int qnxzip_ftruncate(const char *path, uint64_t offset, struct qnx_file_info *fi); //удаление файла

    int qnxzip_rmdir(const char *path); //удаление папки


private:
    FileNode *get_file_node(const char *fname);
};



#endif

