/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv_drvadp.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2008/06/16
  Description   :
  History       :
  1.Date        : 2008/06/16
    Author      :
    Modification: Created file
  2.Date        : 2010/5/29
    Author      : P00123320
    Modification: Struct PCIV_NOTIFY_PICEND_S have been modify ...
******************************************************************************/
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/init.h>
#include <linux/list.h>
#include <linux/interrupt.h>
#include <linux/delay.h>
#include <linux/device.h>
#include <linux/fs.h>
#include <linux/slab.h>

#include "hi_type.h"
#include "hi_debug.h"
#include "hi_comm_pciv.h"
#include "pci_trans.h"
#include "pciv.h"
#include "hi_mcc_usrdev.h"

struct hi_mcc_handle_attr  g_stMsgHandleAttr = {0, PCIV_MSGPORT_KERNEL, 0};

static HI_S32             g_s32LocalId = -1;
static PCIV_BASEWINDOW_S  g_stBaseWindow[HISI_MAX_MAP_DEV];
static hios_mcc_handle_t *g_pstMsgHandle[HISI_MAX_MAP_DEV] = {NULL};

static struct list_head  g_listDmaTask;
static spinlock_t        g_lockDmaTask;
static spinlock_t        g_lockMccMsg;

#include "sys_ext.h"
HI_U64 g_u64RdoneTime[PCIV_MAX_CHN_NUM] = {0};
HI_U32 g_u32RdoneGap[PCIV_MAX_CHN_NUM] = {0};
HI_U32 g_u32MaxRdoneGap[PCIV_MAX_CHN_NUM] = {0};
HI_U32 g_u32MinRdoneGap[PCIV_MAX_CHN_NUM] = {0};

HI_U64 g_u64WdoneTime[PCIV_MAX_CHN_NUM] = {0};
HI_U32 g_u32WdoneGap[PCIV_MAX_CHN_NUM] = {0};
HI_U32 g_u32MaxWdoneGap[PCIV_MAX_CHN_NUM] = {0};
HI_U32 g_u32MinWdoneGap[PCIV_MAX_CHN_NUM] = {0};

HI_VOID PcivDrvAdpDmaFinish(struct pcit_dma_task *task);

/* The interupter must be lock when call this function */
HI_VOID PcivDrvAdpStartDmaTask(HI_VOID)
{
    PCIV_SENDTASK_S     *pTask;
    struct pcit_dma_task pciTask;
    
    while(!list_empty(&(g_listDmaTask)))
    {
        pTask = list_entry(g_listDmaTask.next, PCIV_SENDTASK_S, list);
        
        pciTask.dir          = pTask->bRead ? HI_PCIT_DMA_READ : HI_PCIT_DMA_WRITE;
        pciTask.src          = pTask->u32SrcPhyAddr;
        pciTask.dest         = pTask->u32DstPhyAddr;
        pciTask.len          = pTask->u32Len;
        pciTask.finish       = PcivDrvAdpDmaFinish;
        pciTask.private_data = pTask;/* point to address of pTask */
        if(HI_SUCCESS == pcit_create_task(&pciTask))
        {
            /* If create task fail, this task will lost */
            list_del(g_listDmaTask.next);
        }
        else
        {
            return;
        }
    }
}

HI_VOID PcivDrvAdpDmaFinish(struct pcit_dma_task *task)
{
    PCIV_SENDTASK_S *stask = (PCIV_SENDTASK_S *)task->private_data;

    if(stask != NULL)
    {
        if(stask->pCallBack != NULL)
        {
            stask->pCallBack(stask);
        }
        kfree(stask);
    }
}


HI_S32 PCIV_DrvAdp_AddDmaTask(PCIV_SENDTASK_S *pTask)
{
    PCIV_SENDTASK_S  *pTaskTmp;
    unsigned long lockFlag;

    pTaskTmp = kmalloc(sizeof(PCIV_SENDTASK_S), GFP_ATOMIC);
    if (! pTaskTmp) {
        printk("alloc memory SEND_TASK_S failed!\n");
        return HI_FAILURE;
    }
    memcpy(pTaskTmp, pTask, sizeof(PCIV_SENDTASK_S));

    spin_lock_irqsave(&(g_lockDmaTask), lockFlag);

    list_add_tail(&(pTaskTmp->list), &(g_listDmaTask));

    PcivDrvAdpStartDmaTask();

    spin_unlock_irqrestore(&(g_lockDmaTask), lockFlag);

    return HI_SUCCESS;
}

/* It's a half work */
HI_S32 PcivDrvAdpSendMsg(PCIV_REMOTE_OBJ_S *pRemoteObj,
                         PCIV_MSGTYPE_E enType, PCIV_VOPIC_S *pVoPic)
{
    static PCIV_MSG_S stMsg;
    PCIV_NOTIFY_PICEND_S *pNotify = (PCIV_NOTIFY_PICEND_S *)stMsg.cMsgBody;
    unsigned long lockFlag;
    HI_S32 s32Ret = HI_FAILURE;

    spin_lock_irqsave(&(g_lockMccMsg), lockFlag); // lock handle
    stMsg.u32Target = pRemoteObj->s32ChipId;
    stMsg.enMsgType = enType;
    stMsg.u32MsgLen = sizeof(PCIV_NOTIFY_PICEND_S) + PCIV_MSG_HEADLEN;

    memcpy(&pNotify->stPicInfo, pVoPic, sizeof(PCIV_VOPIC_S));
    pNotify->pcivChn    = pRemoteObj->pcivChn;

    if(g_s32LocalId == 0)
    {
        if(g_pstMsgHandle[stMsg.u32Target] != NULL)
        {
            /* Danger!! But I don't known how to deal with it if send fail */
            s32Ret = hios_mcc_sendto(g_pstMsgHandle[stMsg.u32Target], &stMsg, stMsg.u32MsgLen);
        }
    }
    else
    {
        if(g_pstMsgHandle[0] != NULL)
        {
            /* Danger!! But I don't known how to deal with it if send fail */
            s32Ret = hios_mcc_sendto(g_pstMsgHandle[0], &stMsg, stMsg.u32MsgLen);
        }
    }

    if(stMsg.u32MsgLen == s32Ret)
    {
        s32Ret = HI_SUCCESS;
    }
    else
    {
        printk("Send Msg Error tar:%d, handle:%p, type:%d, len:%d, ret:%d\n",
            stMsg.u32Target, g_pstMsgHandle[stMsg.u32Target], enType, stMsg.u32MsgLen, s32Ret);
        panic("-------------------Msg Error---------------------------\n");
    }

    spin_unlock_irqrestore(&(g_lockMccMsg), lockFlag);

    return s32Ret;
}

HI_S32 PCIV_DrvAdp_DmaEndNotify(PCIV_REMOTE_OBJ_S *pRemoteObj, PCIV_VOPIC_S *pVoPic)
{
    return PcivDrvAdpSendMsg(pRemoteObj, PCIV_MSGTYPE_WRITEDONE, pVoPic);
}

HI_S32 PCIV_DrvAdp_BufFreeNotify(PCIV_REMOTE_OBJ_S *pRemoteObj, PCIV_VOPIC_S *pVoPic)
{
    return PcivDrvAdpSendMsg(pRemoteObj, PCIV_MSGTYPE_READDONE, pVoPic);
}


HI_S32 PcivDrvAdpMsgISR(void *handle ,void *buf, unsigned int data_len)
{
    HI_S32 s32Ret;
    PCIV_MSG_S *pMsg = (PCIV_MSG_S *)buf;

    if(pMsg->u32Target > HISI_MAX_MAP_DEV)
    {
        printk("No this target %d\n", pMsg->u32Target);
        return -1;
    }

    if( pMsg->u32Target != g_s32LocalId )
    {
        if((0 != g_s32LocalId))
        {
            /* 在从片上，只能接收本片的消息 */
            printk("Who are you? Target=%d\n", pMsg->u32Target);
            return 0;
        }

        /* 主片上，如果是其他从片的消息则进行转发 */
        if(g_pstMsgHandle[pMsg->u32Target] != NULL)
        {
            unsigned long lockFlag;

            /* Danger!! But I don't known how to deal with it if send fail */
            spin_lock_irqsave(&(g_lockMccMsg), lockFlag);

            (void)hios_mcc_sendto(g_pstMsgHandle[pMsg->u32Target], buf, data_len);

            spin_unlock_irqrestore(&(g_lockMccMsg), lockFlag);
        }
        return 0;
    }

    s32Ret = HI_FAILURE;
    switch(pMsg->enMsgType)
    {
        /*
          * 图像发送端调用DMA任务接口后，即会发送WRITEDONE消息给接收端，
          * 本地接收端收到消息后，取出消息中的图像信息，送VO显示
          */
        case PCIV_MSGTYPE_WRITEDONE:
        {
            PCIV_NOTIFY_PICEND_S *pNotify = (PCIV_NOTIFY_PICEND_S *)pMsg->cMsgBody;
            PCIV_VOPIC_S stVoPic;
#if 1
            HI_U64 u64Time = CALL_SYS_GetTimeStamp();

            if (0 == g_u64WdoneTime[pNotify->pcivChn])
            {
                /* initial */
                g_u64WdoneTime[pNotify->pcivChn] = u64Time;
                g_u32MinWdoneGap[pNotify->pcivChn] = 0xFFFF;
            }

            g_u32WdoneGap[pNotify->pcivChn] = u64Time - g_u64WdoneTime[pNotify->pcivChn];

            g_u32MaxWdoneGap[pNotify->pcivChn] = (g_u32WdoneGap[pNotify->pcivChn] > g_u32MaxWdoneGap[pNotify->pcivChn]) ? g_u32WdoneGap[pNotify->pcivChn] : g_u32MaxWdoneGap[pNotify->pcivChn];

            if (g_u32WdoneGap[pNotify->pcivChn] != 0)
                g_u32MinWdoneGap[pNotify->pcivChn] = (g_u32WdoneGap[pNotify->pcivChn] < g_u32MinWdoneGap[pNotify->pcivChn]) ? g_u32WdoneGap[pNotify->pcivChn] : g_u32MinWdoneGap[pNotify->pcivChn];

            g_u64WdoneTime[pNotify->pcivChn] = u64Time;
#endif
            memcpy(&stVoPic, &pNotify->stPicInfo, sizeof(PCIV_VOPIC_S));
            PCIV_PicVoShow(pNotify->pcivChn, &stVoPic);
            s32Ret = HI_SUCCESS;
            break;
        }
        /*
          * 图像接收端完成图像的VO显示后，即会发送READDONE消息给发送端，
          * 本地发送端收到消息后，取出消息中的Buffer信息，更新空闲Buffer状态
          */
        case PCIV_MSGTYPE_READDONE:
        {
            PCIV_NOTIFY_PICEND_S *pNotify = (PCIV_NOTIFY_PICEND_S *)pMsg->cMsgBody;
#if 1
            HI_U64 u64Time = CALL_SYS_GetTimeStamp();

            if (0 == g_u64RdoneTime[pNotify->pcivChn])
            {
                /* initial */
                g_u64RdoneTime[pNotify->pcivChn] = u64Time;
                g_u32MinRdoneGap[pNotify->pcivChn] = 0xFFFF;
            }

            g_u32RdoneGap[pNotify->pcivChn] = u64Time - g_u64RdoneTime[pNotify->pcivChn];

            g_u32MaxRdoneGap[pNotify->pcivChn] = (g_u32RdoneGap[pNotify->pcivChn] > g_u32MaxRdoneGap[pNotify->pcivChn]) ? g_u32RdoneGap[pNotify->pcivChn] : g_u32MaxRdoneGap[pNotify->pcivChn];

            if (g_u32RdoneGap[pNotify->pcivChn] != 0)
                g_u32MinRdoneGap[pNotify->pcivChn] = (g_u32RdoneGap[pNotify->pcivChn] < g_u32MinRdoneGap[pNotify->pcivChn]) ? g_u32RdoneGap[pNotify->pcivChn] : g_u32MinRdoneGap[pNotify->pcivChn];

            g_u64RdoneTime[pNotify->pcivChn] = u64Time;
#endif
            PCIV_FreeShareBuf(pNotify->pcivChn, pNotify->stPicInfo.u32Index, pNotify->stPicInfo.u32Count);
            s32Ret = HI_SUCCESS;
            break;
        }
        default:
            printk("Unknown message:%d\n",pMsg->enMsgType);
            break;
    }

    if(HI_SUCCESS != s32Ret)
    {
        printk("Unknown how to process\n");
    }
    return s32Ret;
}

HI_S32 PCIV_DrvAdp_GetBaseWindow(PCIV_BASEWINDOW_S *pBaseWin)
{
    HI_S32 i, s32Ret;

    s32Ret = HI_FAILURE;
    for(i=0; i<HISI_MAX_MAP_DEV; i++)
    {
        if(g_stBaseWindow[i].s32ChipId == pBaseWin->s32ChipId)
        {
            memcpy(pBaseWin, &g_stBaseWindow[i], sizeof(PCIV_BASEWINDOW_S));
            s32Ret = HI_SUCCESS;
            break;
        }
    }

    return s32Ret;
}

HI_S32 PCIV_DrvAdp_GetLocalId(HI_VOID)
{
    return g_s32LocalId;
}

HI_S32 PCIV_DrvAdp_EunmChip(HI_S32 s32ChipArray[PCIV_MAX_CHIPNUM])
{
    HI_S32 i;
    for(i=0; i<HISI_MAX_MAP_DEV; i++)
    {
        s32ChipArray[i] = -1;

        if( g_stBaseWindow[i].s32ChipId == -1) break;
        if( i >= PCIV_MAX_CHIPNUM) break;

        s32ChipArray[i] = g_stBaseWindow[i].s32ChipId;
    }

    return HI_SUCCESS;
}

HI_S32 PCIV_DrvAdp_CheckRemote(HI_S32 s32RemoteId)
{
    HI_S32 s32Ret;

    if (!g_pstMsgHandle[s32RemoteId])
    {
        printk("%s g_pstMsgHandle is NULL \n", __FUNCTION__);
        return HI_ERR_PCIV_NOT_PERM;
    }

    s32Ret = hios_mcc_check_remote(s32RemoteId, g_pstMsgHandle[s32RemoteId]);
    if (s32Ret != HI_SUCCESS)
    {
        printk("Err : pci target id:%d not checked !!! \n", s32RemoteId);
        return s32Ret;
    }
    return HI_SUCCESS;
}

HI_S32 PCIV_DrvAdp_Init(void)
{
    hios_mcc_handle_opt_t stopt;
    int i, nRemotId[HISI_MAX_MAP_DEV];
    PCIV_BASEWINDOW_S *pBaseWin = &g_stBaseWindow[0];

    INIT_LIST_HEAD(&(g_listDmaTask));
    spin_lock_init(&(g_lockDmaTask));
    spin_lock_init(&g_lockMccMsg);

    for(i=0; i<HISI_MAX_MAP_DEV; i++)
    {
        nRemotId[i] = -1;
        g_stBaseWindow[i].s32ChipId = -1;
    }

    g_s32LocalId = hios_mcc_getlocalid(NULL);

    /* pci host ---------------------------------------------------------------------------*/
    if(g_s32LocalId == 0)
    {
        hios_mcc_getremoteids(nRemotId, NULL);
        for(i=0; i<HISI_MAX_MAP_DEV; i++)
        {
            if(nRemotId[i] != -1)
            {
                g_stMsgHandleAttr.target_id = nRemotId[i];
                g_pstMsgHandle[nRemotId[i]] = hios_mcc_open(&g_stMsgHandleAttr);
                if(g_pstMsgHandle[nRemotId[i]] == NULL)
                {
                    printk("hios_mcc_open err, id:%d\n", nRemotId[i]);
                    continue;
                }

                stopt.recvfrom_notify = PcivDrvAdpMsgISR;
                hios_mcc_setopt(g_pstMsgHandle[nRemotId[i]], &stopt);

                pBaseWin->s32ChipId     = nRemotId[i];
                pBaseWin->u32PfWinBase  = get_pf_window_base(nRemotId[i]);
                pBaseWin->u32PfAHBAddr  = 0;

                pBaseWin++;
            }
        }
    }
    /* pci device--------------------------------------------------------------------------*/
    else
    {
        g_pstMsgHandle[0] = hios_mcc_open(&g_stMsgHandleAttr);
        if(g_pstMsgHandle[0] == NULL)
        {
            printk("Can't open mcc device 0!\n");
            return -1;
        }

        stopt.recvfrom_notify = PcivDrvAdpMsgISR;
        stopt.data = g_s32LocalId;
        hios_mcc_setopt(g_pstMsgHandle[0], &stopt);

        pBaseWin->s32ChipId     = 0;
        pBaseWin->u32NpWinBase  = 0;
        pBaseWin->u32PfWinBase  = 0;
        pBaseWin->u32CfgWinBase = 0;
        pBaseWin->u32PfAHBAddr  = get_pf_window_base(0);
    }

    return 0;
}

HI_VOID PCIV_DrvAdp_Exit(void)
{
    int i;
    for(i=0; i<HISI_MAX_MAP_DEV; i++)
    {
        if(g_pstMsgHandle[i] != NULL)
        {
            hios_mcc_close(g_pstMsgHandle[i]);
        }
    }

    return ;
}


