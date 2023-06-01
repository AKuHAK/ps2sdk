/*
 * Copyright (c) 2007 Lukasz Bruun <mail@lukasz.dk>
 *
 * See the file LICENSE included with this distribution for licensing terms.
 */

/**
 * @file
 * IOP pad driver
 */

#ifndef __FREEPAD_SIO2CMDS_H__
#define __FREEPAD_SIO2CMDS_H__

#define SIO2_CMD_MAX 16

#define PAD_ID_FINDPADS  0x01
#define PAD_ID_MOUSE     0x12
#define PAD_ID_NEGICON   0x23
#define PAD_ID_KONAMIGUN 0x31
#define PAD_ID_DIGITAL   0x41
#define PAD_ID_JOYSTICK  0x53
#define PAD_ID_NAMCOGUN  0x63
#define PAD_ID_ANALOG    0x73
#define PAD_ID_ANALOG2   0x79
#define PAD_ID_MULTITAP  0x80
#define PAD_ID_JOGCON    0xE3
#define PAD_ID_JOGCON2   0xE5
#define PAD_ID_CONFIG    0xF3

#define PAD_ID_HI(id) ((id) >> 4)
#define PAD_ID_LO(id) ((id)&0xF)

void sio2cmdReset(void);
void sio2cmdInitFindPads(void);
void sio2cmdInitMouse(void);
void sio2cmdInitNegicon(void);
void sio2cmdInitKonamiGun(void);
void sio2cmdInitDigital(void);
void sio2cmdInitJoystick(void);
void sio2cmdInitNamcoGun(void);
void sio2cmdInitAnalog(void);
void sio2cmdInitJogcon(void);
void sio2cmdInitConfig(void);

u32 sio2cmdCheckId(u8 id);

void sio2CmdSetReadData(u32 id, u8 *buf);
u32 sio2CmdSetEnterConfigMode(u32 id, u8 *buf);
u32 sio2CmdSetExitConfigMode(u32 id, u8 *buf);
u32 sio2CmdSetQueryModel(u32 id, u8 *buf);
u32 sio2CmdSetQueryAct(u32 id, u8 *buf);
u32 sio2CmdSetQueryComb(u32 id, u8 *buf);
u32 sio2CmdSetQueryMode(u32 id, u8 *buf);
u32 sio2CmdSetQueryButtonMask(u32 id, u8 *buf);
u32 sio2CmdSetSetButtonInfo(u32 id, u8 *buf);
u32 sio2CmdSetSetVrefParam(u32 id, u8 *buf);
u32 sio2CmdSetSetMainMode(u32 id, u8 *buf);
u32 sio2CmdSetSetActAlign(u32 id, u8 *buf);

u32 sio2CmdGetPortCtrl1(u8 id, u32 b, u8 c);
u32 sio2CmdGetPortCtrl2(u32 id, u32 b);


#endif
