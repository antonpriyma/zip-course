#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/neutrino.h>
#include <sys/dispatch.h>
#include <folder_manager/QnxFolderManager.h>
#include <sys/types.h>
#include <sys/iofunc.h>
#include <pthread.h>
#include <process.h>
#include <unistd.h>

#include <map>

#include "myresmgr.h"


std::map<char *, pid_t> pathProcess;

typedef struct Mount Mount;
struct Mount {
    char *from;
    char *to;
};

typedef struct Unmount unmount;
struct Unmount {
    char *from;
};


Unmount parse_unmount_command(char *data);

Mount parse_mount_command(char *data);

void process_data(int offset, void *buffer, int nbytes) {
    char *data = (char *) buffer;
    if (strcmp(data, "MOUNT") == 0) {
        Mount mount;
        mount = parse_mount_command(data);
        pid_t pid;

        if ((pid = fork()) == -1) {
            perror("fork");
        }


        if (pid == 0) {
            printf("mount command: %s\n", data);
            QnxFolderManager manager;
            if (manager.Init_res_manager(mount.to) == -1) {
                printf("Can not create folder manager: %s", mount.to);
            }
            printf("Folder manager created 1, process id: %d\n", getpid());
        } else {
            printf("back to main process");
            pathProcess.insert(std::pair<char *, pid_t>(mount.to, pid));
            return;
        }
    } else {
        //Unmount resourse manager
        Unmount unmount;
        unmount = parse_unmount_command(data);
        pid_t pid = pathProcess.at(unmount.from);
        if (pid) {
            kill(pid, 0);
        }
    }
}

Mount parse_mount_command(char *data) {
    char *token = strtok(data, " ");
    int i = 0;

    Mount res = Mount{};

    while (token != NULL) {
        i++;
        if (i == 1) {
            res.from = token;
        }
        if (i == 2) {
            res.to = token;
        }
        printf("%s\n", token);
        token = strtok(NULL, " ");
    }
    return res;
}

Unmount parse_unmount_command(char *data) {
    char *token = strtok(data, " ");
    int i = 0;

    Unmount res = Unmount{};

    while (token != NULL) {
        i++;
        if (i == 1) {
            res.from = token;
        }
        printf("%s\n", token);
        token = strtok(NULL, " ");
    }
    return res;
}

int io_write(resmgr_context_t *ctp, io_write_t *msg,
             iofunc_ocb_t *ocb) {
    printf("Write\n");
    int sts;
    int nbytes;
    int off;
    int start_data_offset;
    int xtype;
    char *buffer;
    struct _xtype_offset *xoffset;


    if ((sts = iofunc_write_verify(ctp, msg, ocb, NULL)) != EOK)
        return (sts);


    xtype = msg->i.xtype & _IO_XTYPE_MASK;
    if (xtype == _IO_XTYPE_OFFSET) {
        xoffset = (struct _xtype_offset *) (&msg->i + 1);
        start_data_offset = sizeof(msg->i) + sizeof(*xoffset);
        off = xoffset->offset;
    } else if (xtype == _IO_XTYPE_NONE) {
        off = ocb->offset;
        start_data_offset = sizeof(msg->i);
    } else
        return (ENOSYS);


    nbytes = msg->i.nbytes;
    if ((buffer = (char *) malloc(nbytes)) == NULL)
        return (ENOMEM);


    if (resmgr_msgread(ctp, buffer, nbytes, start_data_offset) == -1) {
        free(buffer);
        return (errno);
    }


    process_data(off, buffer, nbytes);


    free(buffer);

    _IO_SET_WRITE_NBYTES (ctp, nbytes);


    if (nbytes) {
        ocb->attr->flags |= IOFUNC_ATTR_MTIME | IOFUNC_ATTR_DIRTY_TIME;
        if (xtype == _IO_XTYPE_NONE)
            ocb->offset += nbytes;
    }


    return (EOK);
}