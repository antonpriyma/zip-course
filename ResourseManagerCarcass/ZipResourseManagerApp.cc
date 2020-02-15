#include <stdio.h>
#include <stdlib.h>
#include <stddef.h>
#include <string.h>
#include <sys/iofunc.h>
#include <sys/dispatch.h>
#include "write.cc"

//Чтение
//Запись
//Создание папки
//удаление???
//сжатие обратно


int main (int argc, char **argv)
{
    resmgr_connect_funcs_t  connect_funcs;
    resmgr_io_funcs_t       io_funcs;
    iofunc_attr_t           attr;

    thread_pool_attr_t  pool_attr;
    thread_pool_t       *tpp;
    dispatch_t          *dpp;
    resmgr_attr_t       resmgr_attr;
    //resmgr_context_t    *ctp;
    int                 id;

    if ((dpp = dispatch_create ()) == NULL)
    {
        fprintf (stderr, "%s: Can not create dispatch\n", argv[0]);
        return (EXIT_FAILURE);
    }

    memset (&pool_attr, 0, sizeof (pool_attr));
    pool_attr.handle = dpp;
    pool_attr.context_alloc = resmgr_context_alloc;
    pool_attr.block_func = resmgr_block;
    pool_attr.handler_func = resmgr_handler;
    pool_attr.context_free = resmgr_context_free;

    pool_attr.lo_water = 2;
    pool_attr.hi_water = 4;
    pool_attr.increment = 1;
    pool_attr.maximum = 50;
    if ((tpp = thread_pool_create (&pool_attr, POOL_FLAG_EXIT_SELF)) == NULL)
    {
        fprintf (stderr, "%s: Can not create thread pool\n", argv[0]);
        return (EXIT_FAILURE);
    }

    iofunc_func_init (_RESMGR_CONNECT_NFUNCS, &connect_funcs,
                      _RESMGR_IO_NFUNCS, &io_funcs);

    io_funcs.write = io_write;
  

    iofunc_attr_init (&attr, S_IFNAM | 0777, 0, 0);


    memset (&resmgr_attr, 0, sizeof (resmgr_attr));
    resmgr_attr.nparts_max = 1;
    resmgr_attr.msg_max_size = 2048;

    if ((id = resmgr_attach (dpp, &resmgr_attr, "/dev/zipctl",
                             _FTYPE_ANY, 0, &connect_funcs, &io_funcs, &attr)) == -1)
    {
        fprintf (stderr, "%s: Can not create resourse manager\n", argv [0]);
        return (EXIT_FAILURE);
    }

    thread_pool_start (tpp);
}