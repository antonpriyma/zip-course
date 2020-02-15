//
// Created by Anton Priyma on 15.02.2020.
//

#include <sys/resmgr.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include <sys/iofunc.h>
#include "zip-lib/qnx-zip.h"


#ifndef RESOURSEMANAGERCARCASS_QNXFOLDERMANAGER_H
#define RESOURSEMANAGERCARCASS_QNXFOLDERMANAGER_H

#endif //RESOURSEMANAGERCARCASS_QNXFOLDERMANAGER_H


class QnxFolderManager {
public:
    int Init_res_manager(char *path);


private:
    char *path;
    static char *cur_path;
    static QnxZipCTL zip_manger;

    resmgr_connect_funcs_t connect_funcs;
    resmgr_io_funcs_t io_funcs;
    iofunc_attr_t attr;

    static int read(resmgr_context_t *ctp, io_read_t *msg,
                    iofunc_ocb_t *ocb);

    static int write(resmgr_context_t *ctp, io_write_t *msg,
                     iofunc_ocb_t *ocb);

    static int open(resmgr_context_t *ctp, io_open_t *msg, iofunc_attr_t *attr, void *extra);

    static int my_read_dir(resmgr_context_t *pContext,
                           io_read_t *msg,
                           iofunc_ocb_t *pOcb);

    static int my_read_file(resmgr_context_t *ctp, io_read_t *msg,
                            iofunc_ocb_t *ocb);


    int create_folder_manager();

    static int mkdir(resmgr_context_t *, io_mknod_t *,iofunc_attr_t *attr, void *reserved);

    static int unlink(resmgr_context_t *ctp, io_unlink_t *msg,  iofunc_attr_t *attr, void *extra);

    static void term(int);
};
