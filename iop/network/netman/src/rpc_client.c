#include <errno.h>
#include <intrman.h>
#include <sifcmd.h>
#include <sifman.h>
#include <sysclib.h>
#include <thbase.h>
#include <thevent.h>
#include <thsemap.h>
#include <netman.h>
#include <netman_rpc.h>

#include "internal.h"
#include "rpc_client.h"

static SifRpcClientData_t EEClient;
static union{
	s32 result;
	struct NetManEEInitResult EEInitResult;
	u8 buffer[64];
} SifRpcRxBuffer;
static union{
	s32 state;
	u8 buffer[64];
} SifRpcTxBuffer;

//Data for IOP -> EE transfers
static unsigned short int EEFrameBufferWrPtr;
static u8 *EEFrameBuffer = NULL;	/* On the EE side */

static int NetManRxThreadEvfID = -1, NetManRxDoneEvfID = -1, NetManIOSemaID = -1, RxThreadID = -1;

static struct PacketReqs PacketReqs;

struct NetManPacketBuffer{
	void *handle;
	void *payload;
	u32 length;
};

//Data for SPEED -> IOP transfers
static struct NetManPacketBuffer pbufs[NETMAN_RPC_BLOCK_SIZE];
static int DMATransferID[NETMAN_RPC_BLOCK_SIZE];
static u8 *FrameBuffer = NULL;

static void EERxThread(void *arg);

int NetManInitRPCClient(void)
{
	iop_event_t event;
	iop_sema_t sema;
	iop_thread_t thread;
	int result;

	memset(&PacketReqs, 0, sizeof(PacketReqs));
	memset(DMATransferID, 0, sizeof(DMATransferID));
	if(FrameBuffer == NULL) FrameBuffer = malloc(NETMAN_RPC_BLOCK_SIZE*NETMAN_MAX_FRAME_SIZE);

	if(FrameBuffer != NULL)
	{
		while((result=sceSifBindRpc(&EEClient, NETMAN_RPC_NUMBER, 0))<0 || EEClient.server==NULL) DelayThread(500);

		if((result=sceSifCallRpc(&EEClient, NETMAN_EE_RPC_FUNC_INIT, 0, NULL, 0, &SifRpcRxBuffer, sizeof(struct NetManEEInitResult), NULL, NULL))>=0)
		{
			if((result=SifRpcRxBuffer.EEInitResult.result) == 0)
			{
				EEFrameBuffer=SifRpcRxBuffer.EEInitResult.FrameBuffer;
				EEFrameBufferWrPtr = 0;

				event.attr = EA_SINGLE;
				event.option = 0;
				event.bits = 0;
				NetManRxDoneEvfID = CreateEventFlag(&event);

				sema.attr = 0;
				sema.option = 0;
				sema.initial = 1;
				sema.max = 1;
				NetManIOSemaID = CreateSema(&sema);

				event.attr = EA_SINGLE;
				event.option = 0;
				event.bits = 0;
				NetManRxThreadEvfID = CreateEventFlag(&event);

				thread.attr = TH_C;
				thread.option = 0;
				thread.thread = &EERxThread;
				thread.stacksize = 0x600;
				thread.priority = 0x26; //The receive thread should have a higher priority than transmission threads, so that it won't get interrupted
				RxThreadID = CreateThread(&thread);
				StartThread(RxThreadID, NULL);
			}
		}
	}else result = -ENOMEM;

	return result;
}

void NetManDeinitRPCClient(void)
{
	if(FrameBuffer != NULL)
	{
		free(FrameBuffer);
		FrameBuffer = NULL;
	}
	if(NetManIOSemaID >= 0)
	{
		DeleteSema(NetManIOSemaID);
		NetManIOSemaID = -1;
	}
	if(NetManRxDoneEvfID >= 0)
	{
		DeleteEventFlag(NetManRxDoneEvfID);
		NetManRxDoneEvfID = -1;
	}
	if(RxThreadID >= 0)
	{
		TerminateThread(RxThreadID);
		DeleteThread(RxThreadID);
	}
	if(NetManRxThreadEvfID >= 0)
	{
		DeleteEventFlag(NetManRxThreadEvfID);
		NetManRxThreadEvfID = -1;
	}

	memset(&EEClient, 0, sizeof(EEClient));
}

void NetManRpcToggleGlobalNetIFLinkState(int state)
{
	WaitSema(NetManIOSemaID);
	SifRpcTxBuffer.state=state;
	sceSifCallRpc(&EEClient, NETMAN_EE_RPC_FUNC_HANDLE_LINK_STATUS_CHANGE, 0, &SifRpcTxBuffer, sizeof(s32), NULL, 0, NULL, NULL);
	SignalSema(NetManIOSemaID);
}

//For this implementation, a packet buffer is only taken if it is enqueued.
void *NetManRpcNetProtStackAllocRxPacket(unsigned int length, void **payload)
{
	struct NetManPacketBuffer *result;

	//Wait for a free spot to appear in the ring buffer.
	while(PacketReqs.count + 1 >= NETMAN_RPC_BLOCK_SIZE)
	{
		SetEventFlag(NetManRxThreadEvfID, 1);
		WaitEventFlag(NetManRxDoneEvfID, 1, WEF_CLEAR|WEF_AND, NULL);
	}

	//Allocation of PBUF descriptors is tied with the allocation of frame slots in the ring buffer by EnQ.
	result = &pbufs[EEFrameBufferWrPtr];
	result->handle = &PacketReqs.length[EEFrameBufferWrPtr];
	result->payload = &FrameBuffer[EEFrameBufferWrPtr * NETMAN_MAX_FRAME_SIZE];
	result->length = length;

	//PacketReqs.count and the write pointer is not incremented here because the interface driver might discard the frame and free this allocated buffer.

	if(DMATransferID[EEFrameBufferWrPtr] != 0)
	{	//If this buffer had a DMA transfer initiated, check that it has completely been transferred out before allowing it to be overwritten.
		while(sceSifDmaStat(DMATransferID[EEFrameBufferWrPtr]) >= 0){};
		DMATransferID[EEFrameBufferWrPtr] = 0;
	}

	*payload = result->payload;

	return result;
}

void NetManRpcNetProtStackFreeRxPacket(void *packet)
{
	((struct NetManPacketBuffer*)packet)->handle = NULL;
}

static void EERxThread(void *arg)
{
	int OldState, sent;

	while(1)
	{
		//Unlike the EE-side implementation, do not use SleepThread because the old IOP kernel still puts the thread to SLEEP within sceSifCallRpc.
		WaitEventFlag(NetManRxThreadEvfID, 1, WEF_CLEAR|WEF_AND, NULL);

		if(PacketReqs.count > 0)
		{
			while(PacketReqs.count > 0)
			{
				WaitSema(NetManIOSemaID);
				//NETMAN_EE_RPC_FUNC_SEND_PACKETS may not accept all frames, if there is insufficient memory.
				if(sceSifCallRpc(&EEClient, NETMAN_EE_RPC_FUNC_HANDLE_PACKETS, 0, &PacketReqs, sizeof(PacketReqs), &SifRpcRxBuffer, sizeof(s32), NULL, NULL) >= 0)
					sent = SifRpcRxBuffer.result;
				else
					sent = 0;
				SignalSema(NetManIOSemaID);

				if(sent > 0)
				{	//Only if frames were accepted successfully.
					CpuSuspendIntr(&OldState);
					PacketReqs.count -= sent;
					CpuResumeIntr(OldState);

					SetEventFlag(NetManRxDoneEvfID, 1);
				} else	//Ideally, this should not happen. But if it does, give the EE some time.
					DelayThread(200);
			}
		} else
			SetEventFlag(NetManRxDoneEvfID, 1);
	}
}

//Only one thread can enter this critical section!
static void EnQFrame(const void *frame, unsigned int length)
{
	SifDmaTransfer_t dmat;
	int dmat_id, OldState;

	//No need to wait for a free spot to appear, as Alloc already took care of that.

	//Transfer the frame over to the EE
	dmat.src = (void*)frame;
	dmat.dest = &EEFrameBuffer[EEFrameBufferWrPtr * NETMAN_MAX_FRAME_SIZE];
	dmat.size = length;
	dmat.attr = 0;
	do{
		CpuSuspendIntr(&OldState);
		dmat_id = sceSifSetDma(&dmat, 1);
		CpuResumeIntr(OldState);
	}while(dmat_id == 0);
	DMATransferID[EEFrameBufferWrPtr] = dmat_id;

	//Record the frame length.
	PacketReqs.length[EEFrameBufferWrPtr] = length;

	CpuSuspendIntr(&OldState);
	//Increase the frame count.
	PacketReqs.count++;
	CpuResumeIntr(OldState);

	//Increase the write (IOP -> EE) pointer by one place.
	EEFrameBufferWrPtr = (EEFrameBufferWrPtr + 1) % NETMAN_RPC_BLOCK_SIZE;

	//Notify the receive thread of the incoming frame.
	SetEventFlag(NetManRxThreadEvfID, 1);
}

//Frames will be enqueued in the order that they were allocated.
void NetManRpcProtStackEnQRxPacket(void *packet)
{
	EnQFrame(((struct NetManPacketBuffer*)packet)->payload, ((struct NetManPacketBuffer*)packet)->length);
}
