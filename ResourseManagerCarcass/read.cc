#include <stdio.h>
#include <errno.h>
#include <stdlib.h>
#include <sys/neutrino.h>
#include <sys/iofunc.h>

// ���� ������ � �������
char  *data_string = "����������, ���!\n";

int io_read (resmgr_context_t *ctp, io_read_t *msg,
             iofunc_ocb_t *ocb)
{
	printf("Read\n");
    int      sts;            // ������
    int      nbytes;
    int      nleft;
    int      off;            // ������������� �������� ��������, ������� �� ������ ����� ������������ ��� ���������
    int      xtype;          // ���������� xtype, �������������� � �������� ���������
    struct   _xtype_offset *xoffset;

    // 1. ���������, ������� �� ���������� �� ������
    if ((sts = iofunc_read_verify (ctp, msg, ocb, NULL)) != EOK)
        return (sts);

    // 2. ��������� � ���������� ��������������� XTYPE
    xtype = msg -> i.xtype & _IO_XTYPE_MASK;
    if (xtype == _IO_XTYPE_OFFSET)
    {
        xoffset = (struct _xtype_offset *) (&msg -> i + 1);
        off = xoffset -> offset;
    } else if (xtype == _IO_XTYPE_NONE)
    {
        off = ocb -> offset;
    } else {
        return (ENOSYS);
    }

    // 3. ������� ���� ��������?
    nleft = ocb -> attr -> nbytes - off;

    // 4. ������� ���� �� ����� ������ �������?
    nbytes = __min (nleft, msg -> i.nbytes);

    // 5. ���� ���������� ������, ������ �� �������
    if (nbytes)
        MsgReply (ctp -> rcvid, nbytes, data_string + off, nbytes);

    // 6. ���������� �������� "atime" ��� POSIX stat()
    ocb -> attr -> flags |= IOFUNC_ATTR_ATIME | IOFUNC_ATTR_DIRTY_TIME;

    // 7. ���� ������ lseek() �� ����� _IO_XTYPE_OFFSET
    //    ��������� ��� �� ����� ��������� ����
    if (xtype == _IO_XTYPE_NONE)
        ocb -> offset += nbytes;
    else
        // 8. �� ���������� ������, ������ ������������ �������
        MsgReply (ctp -> rcvid, EOK, NULL, 0);

    // 9. ������� ����������, ��� �� ��� �������� ����
    return (_RESMGR_NOREPLY);
}