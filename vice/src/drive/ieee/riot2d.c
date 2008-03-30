/*
 * riot2d.c - RIOT2 emulation in the SFD1001, 8050 and 8250 disk drive.
 *
 * Written by
 *  Andre' Fachat <fachat@physik.tu-chemnitz.de>
 *  Andreas Boose <viceteam@t-online.de>
 *
 * This file is part of VICE, the Versatile Commodore Emulator.
 * See README for copyright notice.
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA
 *  02111-1307  USA.
 *
 */

#include "vice.h"

#include <stdio.h>

#include "drive.h"
#include "drivecpu.h"
#include "drivetypes.h"
#include "interrupt.h"
#include "lib.h"
#include "parallel.h"
#include "riot.h"
#include "riotd.h"
#include "types.h"


typedef struct driveriot2_context_s {
    unsigned int number;
    struct drive_s *drive_ptr;
    int r_atn_active;     /* init to 0 */
    unsigned int int_num;
} driveriot2_context_t;


void REGPARM3 riot2_store(drive_context_t *ctxptr, WORD addr, BYTE data)
{
    riotcore_store(ctxptr->riot2, addr, data);
}

BYTE REGPARM2 riot2_read(drive_context_t *ctxptr, WORD addr)
{
    return riotcore_read(ctxptr->riot2, addr);
}

static void set_irq(riot_context_t *riot_context, int fl, CLOCK clk)
{
    drive_context_t *drive_context;
    driveriot2_context_t *riot2p;

    drive_context = (drive_context_t *)(riot_context->context);
    riot2p = (driveriot2_context_t *)(riot_context->prv);

    interrupt_set_irq(drive_context->cpu->int_status, (riot2p->int_num),
                      (fl) ? IK_IRQ : 0, clk);
}

static void restore_irq(riot_context_t *riot_context, int fl)
{
    drive_context_t *drive_context;
    driveriot2_context_t *riot2p;

    drive_context = (drive_context_t *)(riot_context->context);
    riot2p = (driveriot2_context_t *)(riot_context->prv);

    interrupt_restore_irq(drive_context->cpu->int_status, riot2p->int_num,
                          (fl) ? IK_IRQ : 0);
}

static void set_handshake(riot_context_t *riot_context, BYTE pa)
{
    drive_context_t *drive_context;
    driveriot2_context_t *riot2p;

    drive_context = (drive_context_t *)(riot_context->context);
    riot2p = (driveriot2_context_t *)(riot_context->prv);

    /* IEEE handshake logic (named as in schematics):
        Inputs: /ATN    = inverted IEEE atn (true = active)
                ATNA    = pa bit 0
                /DACO   = pa bit 1
                RFDO    = pa bit 2
        Output: DACO    = /DACO & (ATN | ATNA) -> to IEEE via MC3446
                RFDO    = (/ATN == ATNA) & RFDO -> to IEEE via MC3446
    */
    /* RFDO = (/ATN == ATNA) & RFDO */
    drive_context->func.parallel_set_nrfd((char)(
        !( ((riot2p->r_atn_active ? 1 : 0) == (pa & 1)) && (pa & 4) )
        ));
    /* DACO = /DACO & (ATNA | ATN) */
    drive_context->func.parallel_set_ndac((char)(
        !( (!(pa & 2)) && ((pa & 1) || (!(riot2p->r_atn_active))) )
        ));
}

void drive_riot_set_atn(riot_context_t *riot_context, int state)
{
    drive_context_t *drive_context;
    driveriot2_context_t *riot2p;

    drive_context = (drive_context_t *)(riot_context->context);
    riot2p = (driveriot2_context_t *)(riot_context->prv);

    if (DRIVE_IS_OLDTYPE(riot2p->drive_ptr->type)) {
        if (riot2p->r_atn_active && !state) {
            riotcore_signal(riot_context, RIOT_SIG_PA7, RIOT_SIG_FALL);
        } else
        if (state && !(riot2p->r_atn_active)) {
            riotcore_signal(riot_context, RIOT_SIG_PA7, RIOT_SIG_RISE);
        }
        riot2p->r_atn_active = state;
        riot1_set_pardata(drive_context->riot1);
        set_handshake(riot_context, riot_context->old_pa);
    }
}

static void undump_pra(riot_context_t *riot_context, BYTE byte)
{
    drive_context_t *drive_context;

    drive_context = (drive_context_t *)(riot_context->context);

    /* bit 0 = atna */
    set_handshake(riot_context, byte);
    drive_context->func.parallel_set_eoi((BYTE)(!(byte & 0x08)));
    drive_context->func.parallel_set_dav((BYTE)(!(byte & 0x10)));
}

static void store_pra(riot_context_t *riot_context, BYTE byte)
{
    drive_context_t *drive_context;

    drive_context = (drive_context_t *)(riot_context->context);

    /* bit 0 = atna */
    /* bit 1 = /daco */
    /* bit 2 = rfdo */
    /* bit 3 = eoio */
    /* bit 4 = davo */
    set_handshake(riot_context, byte);  /* handle atna, nrfd, ndac */
    drive_context->func.parallel_set_eoi((BYTE)(!(byte & 0x08)));
    drive_context->func.parallel_set_dav((BYTE)(!(byte & 0x10)));
}

static void undump_prb(riot_context_t *riot_context, BYTE byte)
{
    driveriot2_context_t *riot2p;

    riot2p = (driveriot2_context_t *)(riot_context->prv);

    /* bit 3 Act LED 1 */
    /* bit 4 Act LED 0 */
    /* bit 5 Error LED */

    /* 1001 only needs LED 0 and Error LED */
    riot2p->drive_ptr->led_status = (byte >> 4) & 0x03;

    if ((riot2p->number == 0) && (DRIVE_IS_DUAL(riot2p->drive_ptr->type))) {
        drive[1].led_status = ((byte & 8) ? 1 : 0) | ((byte & 32) ? 2 : 0);
    }
}

static void store_prb(riot_context_t *riot_context, BYTE byte)
{
    driveriot2_context_t *riot2p;

    riot2p = (driveriot2_context_t *)(riot_context->prv);

    /* bit 3 Act LED 1 */
    /* bit 4 Act LED 0 */
    /* bit 5 Error LED */

    /* 1001 only needs LED 0 and Error LED */
    riot2p->drive_ptr->led_status = (byte >> 4) & 0x03;

    if ((riot2p->number == 0) && (DRIVE_IS_DUAL(riot2p->drive_ptr->type))) {
        drive[1].led_status = ((byte & 8) ? 1 : 0) | ((byte & 32) ? 2 : 0);
    }
}

static void reset(riot_context_t *riot_context)
{
    drive_context_t *drive_context;
    driveriot2_context_t *riot2p;

    drive_context = (drive_context_t *)(riot_context->context);
    riot2p = (driveriot2_context_t *)(riot_context->prv);

    riot2p->r_atn_active = 0;

    drive_context->func.parallel_set_dav(0);
    drive_context->func.parallel_set_eoi(0);

    set_handshake(riot_context, riot_context->old_pa);

    /* 1001 only needs LED 0 and Error LED */
    riot2p->drive_ptr->led_status = 3;
}

static BYTE read_pra(riot_context_t *riot_context)
{
    BYTE byte = 0xff;
    if (!parallel_atn)
        byte -= 0x80;
    if (parallel_dav)
        byte -= 0x40;
    if (parallel_eoi)
        byte -= 0x20;
    return (byte & ~(riot_context->riot_io)[1])
        | ((riot_context->riot_io)[0] & (riot_context->riot_io)[1]);
}

static BYTE read_prb(riot_context_t *riot_context)
{
    driveriot2_context_t *riot2p;
    BYTE byte = 0xff;

    riot2p = (driveriot2_context_t *)(riot_context->prv);

    if (parallel_nrfd)
        byte -= 0x80;
    if (parallel_ndac)
        byte -= 0x40;

    if (riot2p->number == 0)
        byte -= 1;        /* device address bit 0 */
    byte -= 2;          /* device address bit 1 */
    byte -= 4;          /* device address bit 2 */

    return (byte & ~(riot_context->riot_io)[3])
        | ((riot_context->riot_io)[2] & (riot_context->riot_io)[3]);
}

static void int_riot2d0(CLOCK c)
{
    riotcore_int_riot(drive0_context.riot2, c);
}

static void int_riot2d1(CLOCK c)
{
    riotcore_int_riot(drive1_context.riot2, c);
}

static riot_initdesc_t riot2_initdesc[2] = {
    { NULL, int_riot2d0 },
    { NULL, int_riot2d1 }
};

void riot2_init(drive_context_t *ctxptr)
{
    riot2_initdesc[0].riot_ptr = drive0_context.riot2;
    riot2_initdesc[1].riot_ptr = drive1_context.riot2;


    riotcore_init(riot2_initdesc, ctxptr->cpu->alarm_context,
                  ctxptr->cpu->clk_guard, ctxptr->mynumber);
}

void riot2_setup_context(drive_context_t *ctxptr)
{
    riot_context_t *riot;
    driveriot2_context_t *riot2p;

    ctxptr->riot2 = lib_malloc(sizeof(riot_context_t));
    riot = ctxptr->riot2;

    riot->prv = lib_malloc(sizeof(driveriot2_context_t));
    riot2p = (driveriot2_context_t *)(riot->prv);
    riot2p->number = ctxptr->mynumber;

    riot->context = (void *)ctxptr;

    riot->rmw_flag = &(ctxptr->cpu->rmw_flag);
    riot->clk_ptr = ctxptr->clk_ptr;

    riotcore_setup_context(riot);

    riot->myname = lib_msprintf("RIOT2D%d", ctxptr->mynumber);

    riot2p->drive_ptr = ctxptr->drive_ptr;
    riot2p->r_atn_active = 0;
    riot2p->int_num = interrupt_cpu_status_int_new(ctxptr->cpu->int_status,
                                                   ctxptr->riot2->myname);
    riot->undump_pra = undump_pra;
    riot->undump_prb = undump_prb;
    riot->store_pra = store_pra;
    riot->store_prb = store_prb;
    riot->read_pra = read_pra;
    riot->read_prb = read_prb;
    riot->reset = reset;
    riot->set_irq = set_irq;
    riot->restore_irq = restore_irq;
}

