#include <stdio.h>
#include <stddef.h>
#include <stdlib.h>
#include <errno.h>
#include <dirent.h>
#include <limits.h>
#include <string.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
 
#define ALIGN(x)  (((x) + 3) & ~3)
#define NUM_ENTS   26
 
static iofunc_attr_t  atoz_attrs [NUM_ENTS];
 
static int my_open (resmgr_context_t *ctp, io_open_t *msg,
                    iofunc_attr_t *attr, void *extra);
static int my_read (resmgr_context_t *ctp, io_read_t *msg,
                    iofunc_ocb_t *ocb);
static int my_write (resmgr_context_t *ctp, io_write_t *msg,
                    iofunc_ocb_t *ocb);
static int my_read_dir (resmgr_context_t *ctp, io_read_t *msg,
                        iofunc_ocb_t *ocb);
static int my_read_file (resmgr_context_t *ctp, io_read_t *msg,
                         iofunc_ocb_t *ocb);
int dirent_size (char *fname);
struct dirent * dirent_fill (struct dirent *dp, int inode, int offset, char *fname);
static void process_data(char *req);
 
int main (int argc, char **argv)
{
    dispatch_t               *dpp;
    resmgr_attr_t            resmgr_attr;
    resmgr_context_t         *ctp;
    resmgr_connect_funcs_t   connect_func;
    resmgr_io_funcs_t        io_func;
    iofunc_attr_t            attr;
    int                      i;
 
    // ������� ��������� ���������������
    if ((dpp = dispatch_create ()) == NULL)
    {
        perror ("������ dispatch_create\n");
        exit (EXIT_FAILURE);
    }
 
    // ���������������� ��������� ������
    memset (&resmgr_attr, 0, sizeof (resmgr_attr));
    resmgr_attr.nparts_max = 1024;
    resmgr_attr.msg_max_size = 2048;
 
    // ��������� ����������� �� ���������
    iofunc_func_init (_RESMGR_CONNECT_NFUNCS, &connect_func,
                      _RESMGR_IO_NFUNCS, &io_func);
 
    // ������� � ���������������� ���������� ������ ��� �������� ...
    iofunc_attr_init (&attr, S_IFDIR | 0555, 0, 0);
    attr.inode = NUM_ENTS + 1; // 1-26 ��������������� ��� ������ �� "a" �� "z"
    attr.nbytes = NUM_ENTS;
 
    // � ��� ���� �� "a" �� "z"
    for (i = 0; i < NUM_ENTS; i++)
    {
        iofunc_attr_init (&atoz_attrs [i], S_IFREG | 0444, 0, 0);
        atoz_attrs[i].inode = i + 1;
        atoz_attrs[i].nbytes = 1;
    }
 
    // �������� ���� �������; ��� ��������� ������ io_open � io_read
    connect_func.open = my_open;
    io_func.read = my_read;
    io_func.write = my_write;
 
    // ���������������� �������
    if (resmgr_attach (dpp, &resmgr_attr, "/dev/atoz",
                       _FTYPE_ANY, _RESMGR_FLAG_DIR, &connect_func, &io_func, &attr) == -1)
    {
        perror ("������ resmgr_attach\n");
        exit (EXIT_FAILURE);
    }
 
    // �������� ��������
    ctp = resmgr_context_alloc (dpp);
 
    // ����� ��������� � ������ �����
    while (1)
    {
        if ((ctp = resmgr_block (ctp)) == NULL)
        {
            perror ("������ resmgr_block\n");
            exit (EXIT_FAILURE);
        }
        resmgr_handler (ctp);
    }
}
 
 
static int my_open (resmgr_context_t *ctp, io_open_t *msg,
                    iofunc_attr_t *attr, void *extra)
{
	printf("OPEN\n %s",msg -> connect.path);
    if (msg -> connect.path [0] == 0)   // ��� ������� /dev/atoz
    {
        return (iofunc_open_default (ctp, msg, attr, extra));
    } else if ( (msg -> connect.path[1]) == 0   &&
                ((msg->connect.path[0] >= 'a') &&
                 msg -> connect.path[0] <= 'z')   )
    {
        return (iofunc_open_default (ctp, msg, atoz_attrs + msg->connect.path[0] - 'a', extra));
    } else
    {
        return (ENOENT);
    }
}
 
static int my_read (resmgr_context_t *ctp, io_read_t *msg,
                    iofunc_ocb_t *ocb)
{
	printf("read\n");
    int sts;
 
    // ������������ ��������������� ������� ��� �������� ������������
    if ((sts = iofunc_read_verify (ctp, msg, ocb, NULL)) != EOK)
        return (sts);
    // ������, ���� �� ������ ���� ��� �������
    if (S_ISDIR (ocb ->attr -> mode))
    {
        return (my_read_dir (ctp, msg, ocb));
    } else if (S_ISREG (ocb -> attr -> mode))
    {
        return (my_read_file (ctp, msg, ocb));
    } else
    {
        return (EBADF);
    }
}
 
 
static int my_write(resmgr_context_t *ctp, io_write_t *msg, iofunc_ocb_t *ocb) {
    int max_value = 512 * 1024;
    char value[max_value + 1];
    int len_v = max_value;
   
 
    printf("WRITE\n");
 
    _IO_SET_WRITE_NBYTES(ctp, msg->i.nbytes);
    resmgr_msgread(ctp, value, len_v, sizeof(msg->i));
    value[--len_v] = 0;
 
 
    process_data(value);
 
    return (_RESMGR_NPARTS(0));
}
 
void process_data(char *req) {
    char *command = strtok(req," ");
    if (strcmp(command,"MOUNT")){
        //mount(NULL,dir,_MOUNT_READONLY,'qnx6',?,?)
    }
    if (strcmp(command,"UNMOUNT")){
        //umount(dir,NULL)
    }
}
 
static int my_read_dir (resmgr_context_t *ctp, io_read_t *msg,
                        iofunc_ocb_t *ocb)
{
    int nbytes;
    int nleft;
    struct dirent *dp;
    char *reply_msg;
    char fname [_POSIX_PATH_MAX];
 
    // �������� ����� ��� ������
    reply_msg = (char *) calloc (1, msg -> i.nbytes);
    if (reply_msg == NULL)
        return (ENOMEM);
 
    // ��������� �������� �����
    dp = (struct dirent *) reply_msg;
 
    // �������� nleft ����
    nleft = msg -> i.nbytes;
    while (ocb -> offset < NUM_ENTS)
    {
        // ������� ��� �����
        sprintf (fname, "%c", ocb -> offset + 'a');
 
        // ���������, ��������� ����� ���������
        nbytes = dirent_size (fname);
 
        // ���� �����?
        if (nleft - nbytes >= 0)
        {
            // ��������� ������� �������� � ��������� ���������
            dp = dirent_fill (dp, ocb -> offset + 1, ocb -> offset, fname);
            // ��������� �������� OCB
            ocb -> offset++;
            // ������, ������� ���� �� ������������
            nleft -= nbytes;
        } else
        {
            // ����� ������ ���, ������������
            break;
        }
    }
 
    // ������������ ������� � �������
    MsgReply (ctp -> rcvid, (char *) dp - reply_msg,
              reply_msg, (char *) dp - reply_msg);
    // ���������� �����
    free (reply_msg);
 
    // ������� ����������, ��� �� ��� �������� ����
    return (_RESMGR_NOREPLY);
}
 
static int my_read_file (resmgr_context_t *ctp, io_read_t *msg,
                         iofunc_ocb_t *ocb)
{
    int nbytes;
    int nleft;
    char *string;
 
    // ��� ��� ������� xtype...
    if ((msg -> i.xtype & _IO_XTYPE_MASK) != _IO_XTYPE_NONE)
        return (ENOSYS);
 
    // ��������, ������� ���� ��������...
    nleft = ocb->attr->nbytes - ocb->offset;
    printf("%d %d",ocb->attr->nbytes,ocb->offset);
    // ... � ������� �� ����� ���������� �������
    nbytes = __min (nleft, msg->i.nbytes);
 
    if (nbytes)
    {
        // ������� �������� ������
        //string = ocb->attr->inode -1 + 'A';
        string = "kek";
        // ���������� �� �������
        MsgReply (ctp->rcvid, nbytes, string, nbytes);
        // �������� ����� � ��������
        ocb->attr->flags |= IOFUNC_ATTR_ATIME | IOFUNC_ATTR_DIRTY_TIME;
        ocb->offset += nbytes;
    } else
    {
        // ���������� ������, ������������ ����� �����
        MsgReply (ctp->rcvid, EOK, NULL, 0);
    }
    // ��� �������� ����
    return (_RESMGR_NOREPLY);
}
 
int dirent_size (char *fname)
{
    return (ALIGN(sizeof(struct dirent) - 4 + strlen(fname)));
}
 
struct dirent * dirent_fill (struct dirent *dp, int inode, int offset, char *fname)
{
    dp -> d_ino = inode;
    dp -> d_offset = offset;
    strcpy (dp -> d_name, fname);
    dp->d_namelen = strlen (dp->d_name);
    dp->d_reclen = ALIGN (sizeof(struct dirent) - 4 + dp->d_namelen);
    return ((struct dirent *) ((char *) dp + dp->d_reclen));
}