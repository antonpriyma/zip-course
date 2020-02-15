//
// Created by Anton Priyma on 15.02.2020.
//

#include <stdio.h>
#include <sys/dispatch.h>
#include <string.h>
#include <sys/dtrace.h>
#include <cerrno>
#include "QnxFolderManager.h"



int QnxFolderManager::create_folder_manager() {
    printf("Folder manager created 1\n");
    thread_pool_attr_t pool_attr;
    thread_pool_t *tpp;
    dispatch_t *dpp;
    resmgr_attr_t resmgr_attr;

    //resmgr_context_t    *ctp;
    int id;


    printf("Folder manager created 1\n");
    if ((dpp = dispatch_create()) == NULL) {
        printf("Can not create dispatch:  \n");
        return NULL;
    }

    memset(&pool_attr, 0, sizeof(pool_attr));
    pool_attr.handle = dpp;
    pool_attr.context_alloc = resmgr_context_alloc;
    pool_attr.block_func = resmgr_block;
    pool_attr.handler_func = resmgr_handler;
    pool_attr.context_free = resmgr_context_free;

    pool_attr.lo_water = 2;
    pool_attr.hi_water = 4;
    pool_attr.increment = 1;
    pool_attr.maximum = 50;
    if ((tpp = thread_pool_create(&pool_attr, POOL_FLAG_EXIT_SELF)) == NULL) {
        printf("Can not create thread pool: \n");
        return NULL;
    }

    iofunc_func_init(_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                     _RESMGR_IO_NFUNCS, &io_funcs);

    io_funcs.write = write;
    io_funcs.read = read;
    connect_funcs.open = open;
    connect_funcs.mknod = mkdir;
    connect_funcs.unlink = unlink;


    iofunc_attr_init(&attr, S_IFNAM | 0777, 0, 0);

    memset(&resmgr_attr, 0, sizeof(resmgr_attr));
    resmgr_attr.nparts_max = 1;
    resmgr_attr.msg_max_size = 2048;


    if ((id = resmgr_attach(dpp, &resmgr_attr, path,
                            _FTYPE_ANY, 0, &connect_funcs, &io_funcs, &attr)) == -1) {
        printf("can not create folder res manager: %s \n", path);
        return NULL;
    }

    struct sigaction action;
    memset(&action, 0, sizeof(action));
    action.sa_handler = term;
    sigaction(SIGTERM, &action, NULL);

    printf("Folder manager created: %s \n", path);
    thread_pool_start(tpp);
    printf("Folder manager closed: %s \n", path);
}

int QnxFolderManager::Init_res_manager(char *path) {
    this->path = path;
    zip_manger = QnxZipCTL(path, false);
    return create_folder_manager();
}

int QnxFolderManager::read(resmgr_context_t *ctp, io_read_t *msg, iofunc_ocb_t *ocb) {
    printf("read\n");
    int sts;


    if ((sts = iofunc_read_verify(ctp, msg, ocb, NULL)) != EOK)
        return (sts);

    if (S_ISDIR(ocb->attr->mode)) {
        return (my_read_dir(ctp, msg, ocb));
    } else if (S_ISREG(ocb->attr->mode)) {
        return (my_read_file(ctp, msg, ocb));
    } else {
        return (EBADF);
    }
}

int QnxFolderManager::write(resmgr_context_t *ctp, io_write_t *msg, iofunc_ocb_t *ocb) {
    int max_value = 512 * 1024;
    char value[max_value + 1];
    int len_v = max_value;


    printf("WRITE\n");

    _IO_SET_WRITE_NBYTES(ctp, msg->i.nbytes);
    resmgr_msgread(ctp, value, len_v, sizeof(msg->i));
    value[--len_v] = 0;


    zip_manger.qnxzip_write(cur_path, value, strlen(value), ocb->offset);

    return (_RESMGR_NPARTS(0));
}

int QnxFolderManager::my_read_dir(resmgr_context_t *ctp, io_read_t *msg, iofunc_ocb_t *ocb) {
    int nbytes;
    int nleft;
    struct dirent *dp;
    char *reply_msg;
    char fname[_POSIX_PATH_MAX];

    reply_msg = (char *) calloc(1, msg->i.nbytes);
    if (reply_msg == NULL)
        return (ENOMEM);


    nleft = msg->i.nbytes;
    off64_t NUM_ENTS = 1024;

    while (ocb->offset < NUM_ENTS) {
        zip_manger.qnxzip_readdir(cur_path, fname, ocb->offset);
        sprintf(fname, "%c", fname);

        if (nleft - nbytes >= 0) {

            ocb->offset++;

            nleft -= nbytes;
        } else {

            break;
        }
    }

    MsgReply(ctp->rcvid, nbytes,
             reply_msg, nbytes;

    free(reply_msg);

    return (_RESMGR_NOREPLY);
}

int QnxFolderManager::my_read_file(resmgr_context_t *ctp, io_read_t *msg,
                                   iofunc_ocb_t *ocb) {
    int nbytes;
    int nleft;


    if ((msg->i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
        return (ENOSYS);

    nleft = ocb->attr->nbytes - ocb->offset;
    char *string = (char *) malloc(nleft);
    printf("%d %d", ocb->attr->nbytes, ocb->offset);
    nbytes = __min (nleft, msg->i.nbytes);

    if (nbytes) {
        if (zip_manger.qnxzip_read(cur_path, string, nbytes, ocb->offset) == 0) {
            return (_RESMGR_NOREPLY);
        };

        MsgReply(ctp->rcvid, nbytes, string, nbytes);
        ocb->offset += nbytes;
    } else {
        MsgReply(ctp->rcvid, EOK, NULL, 0);
    }
    return (_RESMGR_NOREPLY);
}

int QnxFolderManager::open(resmgr_context_t *ctp, io_open_t *msg, iofunc_attr_t *attr, void *extra) {
    printf("OPEN\n");

    strncpy(cur_path, msg->connect.path, strlen(msg->connect.path));
    cur_path[strlen(msg->connect.path)] = 0;

    if (S_ISREG(attr->mode)) {
        zip_manger.qnxzip_open(cur_path);
    }

    return (iofunc_open_default(ctp, msg, attr, extra));
}

int QnxFolderManager::mkdir(resmgr_context_t *ctp, io_mknod_t *node, iofunc_attr_t *attr, void *reserved) {
    printf("MKDIR\n");
    strncpy(cur_path, node->connect.path, strlen(node->connect.path));
    zip_manger.qnxzip_mkdir(cur_path);
}

void QnxFolderManager::term(int) {
    zip_manger.data->save();
    delete (zip_manger.data);
}

int QnxFolderManager::unlink(resmgr_context_t *ctp, io_unlink_t *msg,  iofunc_attr_t *attr, void *reserved) {
    printf("Unlink\n");
    strncpy(cur_path, msg->connect.path, strlen(msg->connect.path));
    if (S_ISREG(attr->mode)) {
        if (zip_manger.fusezip_unlink(cur_path)<0){
            return ENFILE;
        }
    }
    if (S_ISDIR(attr->mode)){
        if (zip_manger.qnxzip_rmdir(cur_path)<0){
            return ENFILE;
        }
    }

    return 0;
}

