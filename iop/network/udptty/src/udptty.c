/*
# _____     ___ ____     ___ ____
#  ____|   |    ____|   |        | |____|
# |     ___|   |____ ___|    ____| |    \    PS2DEV Open Source Project.
#-----------------------------------------------------------------------
# Copyright (c) 2003  Marcus R. Brown <mrbrown@0xd6.org>
# Licenced under Academic Free License version 2.0
# Review ps2sdk README & LICENSE files for further details.
#
# $Id: tty.c 629 2004-10-11 00:45:00Z mrbrown $
# TTY filesystem for UDPTTY.

  modified by jimmikaelkael <jimmikaelkael@wanadoo.fr>

*/

#include <tamtypes.h>
#include <loadcore.h>
#include <stdio.h>
#include <thsemap.h>
#include <ioman.h>
#include <intrman.h>
#include <sysclib.h>
#include <sysmem.h>
#include <thbase.h>
#include <thevent.h>
#include <ps2ip.h>
#include <errno.h>

#define MODNAME "udptty"
IRX_ID(MODNAME, 2, 1);

extern struct irx_export_table _exp_udptty;

#define DEVNAME "tty"

static int udp_socket;
static int tty_sema = -1;

static int tty_init(iop_device_t *device);
static int tty_deinit(iop_device_t *device);
static int tty_stdout_fd(void);
static int tty_write(iop_file_t *file, void *buf, size_t size);

IOMAN_RETURN_VALUE_IMPL(EIO);

/* device ops */
static iop_device_ops_t tty_ops = {
    &tty_init, // init
    &tty_deinit, // deinit
    IOMAN_RETURN_VALUE(EIO), // format
    (void *)&tty_stdout_fd, // open
    (void *)&tty_stdout_fd, // close
    IOMAN_RETURN_VALUE(EIO), // read
    (void *)&tty_write, // write
    IOMAN_RETURN_VALUE(EIO), // lseek
    IOMAN_RETURN_VALUE(EIO), // ioctl
    IOMAN_RETURN_VALUE(EIO), // remove
    IOMAN_RETURN_VALUE(EIO), // mkdir
    IOMAN_RETURN_VALUE(EIO), // rmdir
    IOMAN_RETURN_VALUE(EIO), // dopen
    IOMAN_RETURN_VALUE(EIO), // dclose
    IOMAN_RETURN_VALUE(EIO), // dread
    IOMAN_RETURN_VALUE(EIO), // getstat
    IOMAN_RETURN_VALUE(EIO), // chstat
};

/* device descriptor */
static iop_device_t tty_device = {
    DEVNAME,
    IOP_DT_CHAR | IOP_DT_CONS,
    1,
    "TTY via SMAP UDP",
    &tty_ops,
};


/* KPRTTY */
#ifdef KPRTTY
#define PRNT_IO_BEGIN 0x200
#define PRNT_IO_END   0x201

typedef struct _KprArg
{
    int eflag;
    int bsize;
    char *kpbuf;
    int prpos;
    int calls;
} KprArg;

static KprArg g_kprarg;

#define KPR_BUFFER_SIZE 0x1000
static char kprbuffer[KPR_BUFFER_SIZE];

static void PrntFunc(void *context, int chr)
{
    KprArg *kpa = (KprArg *)context;

    switch (chr) {
        case 0:
            break;
        case PRNT_IO_BEGIN:
            kpa->calls++;
            break;
        case PRNT_IO_END:
            break;
        case '\n':
            PrntFunc(context, '\r');
        default:
            if (kpa->prpos < kpa->bsize)
                kpa->kpbuf[kpa->prpos++] = chr;
            break;
    }
}

static int Kprnt(void *context, const char *format, void *arg)
{
    if (format)
        prnt(PrntFunc, context, format, arg);

    return 0;
}

static int Kprintf_Handler(void *context, const char *format, va_list ap)
{
    KprArg *kpa = (KprArg *)context;
    int res;

    res = CpuInvokeInKmode(Kprnt, kpa, format, ap);

    if (QueryIntrContext())
        iSetEventFlag(kpa->eflag, 1);
    else
        SetEventFlag(kpa->eflag, 1);

    return res;
}

static void KPRTTY_Thread(void *args)
{
    u32 flags;
    KprArg *kpa = (KprArg *)args;

    while (1) {
        WaitEventFlag(kpa->eflag, 1, WEF_AND | WEF_CLEAR, &flags);

        if (kpa->prpos) {
            write(1, kpa->kpbuf, kpa->prpos);
            kpa->prpos = 0;
        }
    }
}

static void kprtty_init(void)
{
    iop_event_t efp;
    iop_thread_t thp;
    KprArg *kpa;
    int thid;

    kpa = &g_kprarg;

    efp.attr   = EA_SINGLE;
    efp.option = 0;
    efp.bits   = 0;

    thp.attr      = TH_C;
    thp.option    = 0;
    thp.thread    = &KPRTTY_Thread;
    thp.stacksize = 0x800;
    thp.priority  = 8;

    kpa->eflag = CreateEventFlag(&efp);
    kpa->bsize = KPR_BUFFER_SIZE;
    kpa->kpbuf = kprbuffer;
    kpa->prpos = 0;
    kpa->calls = 0;

    thid = CreateThread(&thp);
    StartThread(thid, (void *)kpa);

    KprintfSet(&Kprintf_Handler, kpa);
}
#endif

int _start(int argc, char *argv[])
{
    (void)argc;
    (void)argv;

    // register exports
    RegisterLibraryEntries(&_exp_udptty);

    // create the socket
    udp_socket = lwip_socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (udp_socket < 0)
        return MODULE_NO_RESIDENT_END;

    DelDrv(tty_device.name);

    if (AddDrv(&tty_device) < 0)
        return MODULE_NO_RESIDENT_END;

    close(0);
    open(DEVNAME "00:", 0x1000 | O_RDWR);
    
    close(1);
    open(DEVNAME "00:", O_WRONLY);

    close(2);
    open(DEVNAME "00:", O_WRONLY);

    printf("UDPTTY loaded!\n");

#ifdef KPRTTY
    kprtty_init();
    printf("KPRTTY enabled!\n");
#endif

    return MODULE_RESIDENT_END;
}

int _shutdown()
{
    lwip_close(udp_socket);

    return 0;
}

/* Copy the data into place, calculate the various checksums, and send the
   final packet.  */
static int udp_send(void *buf, size_t size)
{
    struct sockaddr_in peer;

    peer.sin_family      = AF_INET;
    peer.sin_port        = htons(18194);
    peer.sin_addr.s_addr = inet_addr("255.255.255.255");

    lwip_sendto(udp_socket, buf, size, 0, (struct sockaddr *)&peer, sizeof(peer));

    return 0;
}

/* TTY driver.  */

static int tty_init(iop_device_t *device)
{
    (void)device;

    if ((tty_sema = CreateMutex(IOP_MUTEX_UNLOCKED)) < 0)
        return -1;

    return 0;
}

static int tty_deinit(iop_device_t *device)
{
    (void)device;

    DeleteSema(tty_sema);
    return 0;
}

static int tty_stdout_fd(void)
{
    return 1;
}

static int tty_write(iop_file_t *file, void *buf, size_t size)
{
    int res = 0;

    (void)file;

    WaitSema(tty_sema);
    res = udp_send(buf, size);
    SignalSema(tty_sema);

    return res;
}
