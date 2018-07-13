/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : pciv_firmware.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/07/16
  Description   :
  History       :
  1.Date        : 2009/07/16
    Author      : Z44949
    Modification: Created file
  2.Date        : 2009/10/13
    Author      : P00123320
    Modification: 初始化时启动解码的定时器
  3.Date        : 2009/10/15
    Author      : P00123320
    Modification: 增加VDEC/VO间的同步控制机制
  4.Date        : 2010/2/24
    Author      : P00123320
    Modification: 增加设置和获取前处理属性的接口
  5.Date        : 2010/4/14
    Author      : P00123320
    Modification: 前处理属性中增加新字段 PCIV_PIC_FIELD_E，代替原PCIV_PIC_ATTR_S中u32Field 项
                  用于源图像的丢场处理
  6.Date        : 2010/11/12
    Author      : P00123320
    Modification: 增加 PCIV OSD 功能
******************************************************************************/

#include <linux/kernel.h>
#include <linux/list.h>
#include <linux/sched.h>
#include <linux/slab.h>

#include "hi_common.h"
#include "hi_comm_pciv.h"
#include "hi_comm_region.h"
#include "hi_comm_vo.h"
#include "pciv_firmware.h"
#include "mod_ext.h"
#include "vb_ext.h"
#include "dsu_ext.h"
#include "sys_ext.h"
#include "proc_ext.h"
#include "pciv_fmwext.h"
#include "pciv_pic_queue.h"
#include "vpss_ext.h"
#include "region_ext.h"

#define  PCIVFMW_STATE_STARTED   0
#define  PCIVFMW_STATE_STOPPING  1
#define  PCIVFMW_STATE_STOPED    2

/*get the function entrance*/
#define FUNC_ENTRANCE(type,id) ((type*)(g_astModules[id].pstExportFuncs))


typedef enum hiPCIVFMW_SEND_STATE_E
{
    PCIVFMW_SEND_OK = 0,
    PCIVFMW_SEND_NOK,
    PCIVFMW_SEND_ING,
    PCIVFMW_SEND_BUTT
} PCIVFMW_SEND_STATE_E;

typedef struct hiPCIV_FMWCHANNEL_S
{
    HI_BOOL  bCreate;          /* 是否创建通道 */
    HI_BOOL  bStart;           /* 是否启动传输 */

    HI_U32 u32RgnNum;          /* 通道的region数目 */
    HI_U32 u32TimeRef;         /* 发送方的VI源图像的序号 */
    HI_U32 u32GetCnt;          /* 发送方获取VI图像的次数     或 接收方收到待显示图像的次数 */
    HI_U32 u32SendCnt;         /* 发送方发送缩放后源图像次数 或 接收方送VO显示的次数 */
    HI_U32 u32RespCnt;         /* 发送方发送完图像并释放次数 或 接收方送VO显示完后释放图像次数 */
    HI_U32 u32LostCnt;         /* 发送方未成功发送图像次数   或 接收方未成功送VO显示次数 */
    HI_U32 u32TimerCnt;        /* 发送VDEC解码后图像的定时器运行次数 */

    HI_U32 u32AddJobSucCnt;    /* 成功提交给dsu的job次数 */
    HI_U32 u32AddJobFailCnt;   /* 没有成功提交给dsu的job次数 */
    
    HI_U32 u32MoveTaskSucCnt;  /* 添加dsu move task成功的次数 */
    HI_U32 u32MoveTaskFailCnt; /* 添加dsu move task失败的次数 */

    HI_U32 u32OsdTaskSucCnt;   /* 添加dsu osd task成功的次数 */
    HI_U32 u32OsdTaskFailCnt;  /* 添加dsu osd task失败的次数 */

    HI_U32 u32ZoomTaskSucCnt;  /* 添加dsu zoom task成功的次数 */
    HI_U32 u32ZoomTaskFailCnt; /* 添加dsu zoom task失败的次数 */
    
    HI_U32 u32EndJobSucCnt;    /* 结束dsu job成功的次数 */
    HI_U32 u32EndJobFailCnt;   /* 结束dsu job不成功的次数 */
    
    HI_U32 u32MoveCbCnt;       /* dsu move回调的次数 */
    HI_U32 u32OsdCbCnt;        /* dsu osd回调的次数 */
    HI_U32 u32ZoomCbCnt;       /* dsu zoom回调的次数 */
        
    HI_U32 u32NewDoCnt;
    HI_U32 u32OldUndoCnt;

    PCIVFMW_SEND_STATE_E enSendState;

    PCIV_PIC_QUEUE_S stPicQueue;   /*vdec图像队列*/
    PCIV_PIC_NODE_S *pCurVdecNode; /*当前vdec图像节点*/

    /* 记录缩放后的目标图象属性 */
    PCIV_PIC_ATTR_S stPicAttr;    /* 记录PCI传输的目标图象属性 */
    HI_U32          u32Offset[3]; /* Y/U/V三分量的地址偏移值，初始化时一次算好 */
    HI_U32          u32BlkSize;   /* 每块Buffer的大小 */
    HI_U32          u32Count;     /* The total buffer count */
    HI_U32          au32PhyAddr[PCIV_MAX_BUF_NUM];
    HI_U32          au32PoolId[PCIV_MAX_BUF_NUM];
    VB_BLKHANDLE    vbBlkHdl[PCIV_MAX_BUF_NUM];     /* VB句柄，用于检查是否被VO释放 */
    HI_BOOL         bVoHold[PCIV_MAX_BUF_NUM];      /* Buffer是否正在被VO持有 */
    DSU_ODD_OPT_S   stDsuOpt;
    PCIV_PREPROC_CFG_S stPreProcCfg;

    struct timer_list stBufTimer;
} PCIV_FWMCHANNEL_S;

typedef struct hiPCIV_VBPOOL_S
{
    HI_U32 u32PoolCount;
    HI_U32 u32PoolId[PCIV_MAX_VBCOUNT];
    HI_U32 u32Size[PCIV_MAX_VBCOUNT];
} PCIV_VBPOOL_S;


static PCIV_FWMCHANNEL_S g_stFwmPcivChn[PCIVFMW_MAX_CHN_NUM];
static PCIV_VBPOOL_S     g_stVbPool;
static struct timer_list g_timerVdecSend;

static spinlock_t g_PcivFmwLock;
#define PCIVFMW_SPIN_LOCK   spin_lock_irqsave(&g_PcivFmwLock,flags)
#define PCIVFMW_SPIN_UNLOCK spin_unlock_irqrestore(&g_PcivFmwLock,flags)

#define VDEC_MAX_SEND_CNT 6

static PCIVFMW_CALLBACK_S g_stPcivFmwCallBack;

static HI_U32 s_u32PcivFmwState = PCIVFMW_STATE_STOPED;

static int drop_err_timeref = 1;

#define PCIV_ALPHA_VENC2TDE(alpha) (((alpha) >= 128) ? 255 : (alpha)*2)

HI_VOID PcivFirmWareVoPicFree(unsigned long data);
HI_VOID PcivFmwPutRegion(PCIV_CHN PcivChn, RGN_TYPE_E enType);

HI_S32 PCIV_FirmWareCreate(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr)
{
    HI_S32 s32Ret, i;
    PCIV_FWMCHANNEL_S *pChn = NULL;
    unsigned long flags;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    if (HI_TRUE == pChn->bCreate)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d have already created \n", pcivChn);
        return HI_ERR_PCIV_EXIST;
    }

    s32Ret = PCIV_FirmWareSetAttr(pcivChn, pAttr);
    if (s32Ret != HI_SUCCESS)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "attr of pciv chn%d is invalid \n", pcivChn);
        return s32Ret;
    }

    pChn->bStart             = HI_FALSE;
    pChn->u32SendCnt         = 0;
    pChn->u32GetCnt          = 0;
    pChn->u32RespCnt         = 0;
    pChn->u32LostCnt         = 0;
    pChn->u32NewDoCnt        = 0;
    pChn->u32OldUndoCnt      = 0;
    pChn->enSendState        = PCIVFMW_SEND_OK;
    pChn->u32TimerCnt        = 0;
    pChn->u32RgnNum          = 0;

    pChn->u32AddJobSucCnt    = 0;
    pChn->u32AddJobFailCnt   = 0;
    
    pChn->u32MoveTaskSucCnt  = 0;
    pChn->u32MoveTaskFailCnt = 0;

    pChn->u32OsdTaskSucCnt   = 0;
    pChn->u32OsdTaskFailCnt  = 0;

    pChn->u32ZoomTaskSucCnt  = 0;
    pChn->u32ZoomTaskFailCnt = 0;
    
    pChn->u32EndJobSucCnt    = 0;
    pChn->u32EndJobFailCnt   = 0;
    
    pChn->u32MoveCbCnt       = 0;
    pChn->u32OsdCbCnt        = 0;
    pChn->u32ZoomCbCnt       = 0;

    for (i=0; i<PCIV_MAX_BUF_NUM; i++)
    {
        pChn->bVoHold[i] = HI_FALSE;
    }

    s32Ret = PCIV_CreatPicQueue(&pChn->stPicQueue, VDEC_MAX_SEND_CNT);
    if (s32Ret != HI_SUCCESS)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d create pic queue failed\n", pcivChn);
        return s32Ret;
    }
    pChn->pCurVdecNode = NULL;

    PCIVFMW_SPIN_LOCK;
    pChn->bCreate = HI_TRUE;
    PCIVFMW_SPIN_UNLOCK;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d create ok \n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareDestroy(PCIV_CHN pcivChn)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    unsigned long flags;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    if (HI_FALSE == pChn->bCreate)
    {
        return HI_SUCCESS;
    }
    if (HI_TRUE == pChn->bStart)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d is running,you should stop first \n", pcivChn);
        return HI_ERR_PCIV_NOT_PERM;
    }

    PCIV_DestroyPicQueue(&pChn->stPicQueue);

    PCIVFMW_SPIN_LOCK;
    pChn->bCreate = HI_FALSE;
    PCIVFMW_SPIN_UNLOCK;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d destroy ok \n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PcivFirmWareCheckAttr(PCIV_ATTR_S *pAttr)
{
    PIXEL_FORMAT_E enPixFmt = pAttr->stPicAttr.enPixelFormat;
    HI_U32         u32BlkSize;

    /* 检查图像宽高 */
    if (!pAttr->stPicAttr.u32Height || !pAttr->stPicAttr.u32Width)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pic w:%d, h:%d invalid\n",
            pAttr->stPicAttr.u32Width, pAttr->stPicAttr.u32Height);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }
    if (pAttr->stPicAttr.u32Stride[0] < pAttr->stPicAttr.u32Width)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pic stride0:%d,stride1:%d invalid\n",
            pAttr->stPicAttr.u32Stride[0], pAttr->stPicAttr.u32Stride[1]);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    /* 检查图象格式，并计算所需内存大小及YUV偏移量 */
    u32BlkSize   = pAttr->stPicAttr.u32Height * pAttr->stPicAttr.u32Stride[0];
    switch (enPixFmt)
    {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
        {
            u32BlkSize += (pAttr->stPicAttr.u32Height/2) * pAttr->stPicAttr.u32Stride[1];
            break;
        }
        case PIXEL_FORMAT_YUV_SEMIPLANAR_422:
        {
            u32BlkSize += pAttr->stPicAttr.u32Height * pAttr->stPicAttr.u32Stride[1];
            break;
        }
        default:
        {
            PCIV_FMW_TRACE(HI_DBG_ERR, "Pixel format(%d) unsupported\n", enPixFmt);
            return HI_ERR_PCIV_ILLEGAL_PARAM;
        }
    }

    /* 检查图象属性与缓存大小是否匹配 */
    if (pAttr->u32BlkSize < u32BlkSize)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Buffer block is too smail(%d < %d)\n", pAttr->u32BlkSize, u32BlkSize);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareSetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pAttr)
{
    HI_S32 s32Ret;
    PCIV_FWMCHANNEL_S *pChn = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);
    PCIVFMW_CHECK_PTR(pAttr);

    pChn = &g_stFwmPcivChn[pcivChn];

    /* 通道正在启用过程中，不能更改通道属性 */
    if (HI_TRUE == pChn->bStart)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d is running\n", pcivChn);
        return HI_ERR_PCIV_NOT_PERM;
    }

    /* 检查属性参数有效性 */
    s32Ret = PcivFirmWareCheckAttr(pAttr);
    if (s32Ret != HI_SUCCESS)
    {
        return s32Ret;
    }

    memcpy(&pChn->stPicAttr, &pAttr->stPicAttr, sizeof(PCIV_PIC_ATTR_S));
    memcpy(pChn->au32PhyAddr, pAttr->u32PhyAddr, sizeof(pAttr->u32PhyAddr));
    pChn->u32BlkSize = pAttr->u32BlkSize;
    pChn->u32Count   = pAttr->u32Count;

    /* 设置输出图像的YUV各分量的偏移地址 */
    pChn->u32Offset[0] = 0;
    switch (pAttr->stPicAttr.enPixelFormat)
    {
        case PIXEL_FORMAT_YUV_SEMIPLANAR_420:
        /* fall through */
        case PIXEL_FORMAT_YUV_SEMIPLANAR_422:
        {
            /* Sem-planar格式不需要u32Offset[2](即V分量偏移) */
            pChn->u32Offset[1] = pAttr->stPicAttr.u32Stride[0] * pAttr->stPicAttr.u32Height;
            break;
        }
        
        default:
        {
            PCIV_FMW_TRACE(HI_DBG_ERR, "Pixel format(%d) unsupported\n", pAttr->stPicAttr.enPixelFormat);
            return HI_ERR_PCIV_NOT_SUPPORT;
        }
    }

    return HI_SUCCESS;
}

static HI_VOID PCIV_FirmWareInitPreProcCfg(PCIV_CHN pcivChn)
{
    DSU_ODD_OPT_S *pstDsuCfg = &g_stFwmPcivChn[pcivChn].stDsuOpt;
    PCIV_PREPROC_CFG_S *pstPreProcCfg = &g_stFwmPcivChn[pcivChn].stPreProcCfg;

    /* init dsu cfg */
    memset(pstDsuCfg, 0, sizeof(DSU_ODD_OPT_S));
    
    pstDsuCfg->bDeflicker   = HI_FALSE;
    pstDsuCfg->enChoice     = DSU_TASK_SCALE;
    pstDsuCfg->enDnoise     = DSU_DENOISE_ONLYEDAGE;
    pstDsuCfg->enLumaStr    = DSU_LUMA_STR_DISABLE;
    pstDsuCfg->enCE         = DSU_CE_DISABLE;
    pstDsuCfg->bForceHFilt  = HI_FALSE;
    pstDsuCfg->bForceVFilt  = HI_FALSE;
    pstDsuCfg->enFilterType = FILTER_PARAM_TYPE_NORM;
    pstDsuCfg->enHFilter    = DSU_HSCALE_FILTER_DEFAULT;
    pstDsuCfg->enVFilterL   = DSU_VSCALE_FILTER_DEFAULT;
    pstDsuCfg->enVFilterC   = DSU_VSCALE_FILTER_DEFAULT;
	/*配置dsu缩放策略为普通类型
	 (PCI暂不考虑混合输入场景，此场景时需要根据条件
	  对符合某些条件的图像选择2次缩放策略)*/
	pstDsuCfg->enScaleStrategy = DSU_SCALE_STRATEGY_NORM;

    /* init pre-process cfg */
    pstPreProcCfg->enFieldSel   = PCIV_FIELD_BOTH;
    pstPreProcCfg->enFilterType = PCIV_FILTER_TYPE_NORM;
    pstPreProcCfg->enHFilter    = DSU_HSCALE_FILTER_DEFAULT;
    pstPreProcCfg->enVFilterL   = DSU_VSCALE_FILTER_DEFAULT;
    pstPreProcCfg->enVFilterC   = DSU_VSCALE_FILTER_DEFAULT;
}

HI_S32 PCIV_FirmWareSetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);

    if ( (pAttr->enFieldSel < PCIV_FIELD_TOP || pAttr->enFieldSel >= PCIV_FIELD_BUTT)
        || (pAttr->enFilterType < PCIV_FILTER_TYPE_NORM || pAttr->enFilterType >= PCIV_FILTER_TYPE_BUTT)
        || (pAttr->enHFilter < DSU_HSCALE_FILTER_DEFAULT || pAttr->enHFilter >= DSU_HSCALE_FILTER_BUTT)
        || (pAttr->enVFilterC < DSU_VSCALE_FILTER_DEFAULT || pAttr->enHFilter >= DSU_VSCALE_FILTER_BUTT)
        || (pAttr->enVFilterL < DSU_VSCALE_FILTER_DEFAULT || pAttr->enHFilter >= DSU_VSCALE_FILTER_BUTT))
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "args invalid \n");
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    pChn = &g_stFwmPcivChn[pcivChn];

    /* 更新DSU配置选项中的滤波系数部分 */
    pChn->stDsuOpt.enHFilter    = pAttr->enHFilter;
    pChn->stDsuOpt.enVFilterC   = pAttr->enVFilterC;
    pChn->stDsuOpt.enVFilterL   = pAttr->enVFilterL;

    /* 存储用户设置的前处理配置 */
    memcpy(&pChn->stPreProcCfg, pAttr, sizeof(PCIV_PREPROC_CFG_S));

    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d set preproccfg ok\n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareGetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pAttr)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    memcpy(pAttr, &pChn->stPreProcCfg, sizeof(PCIV_PREPROC_CFG_S));

    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareStart(PCIV_CHN pcivChn)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;

    PCIVFMW_CHECK_CHNID(pcivChn);

    pChn = &g_stFwmPcivChn[pcivChn];

    if (pChn->bCreate != HI_TRUE)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn%d not create\n", pcivChn);
        return HI_ERR_PCIV_UNEXIST;
    }

    pChn->bStart = HI_TRUE;
    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn%d start ok \n", pcivChn);
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareStop(PCIV_CHN pcivChn)
{
    unsigned long flags;
    PCIVFMW_CHECK_CHNID(pcivChn);
    
    PCIVFMW_SPIN_LOCK;
    g_stFwmPcivChn[pcivChn].bStart = HI_FALSE;
    PCIVFMW_SPIN_UNLOCK;
            
    if (0 != g_stFwmPcivChn[pcivChn].u32RgnNum)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "Region number of channel %d is %d, now free the region!\n", pcivChn, g_stFwmPcivChn[pcivChn].u32RgnNum);
        PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
        g_stFwmPcivChn[pcivChn].u32RgnNum = 0;
    }
    
    PCIV_FMW_TRACE(HI_DBG_INFO, "pcivfmw chn%d stop ok \n", pcivChn);
    
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareWindowVbCreate(PCIV_WINVBCFG_S *pCfg)
{
    HI_S32 s32Ret, i;
    HI_U32 u32PoolId;

    if( g_stVbPool.u32PoolCount != 0)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Video buffer pool has created\n");
        return HI_ERR_PCIV_BUSY;
    }

    for(i=0; i < pCfg->u32PoolCount; i++)
    {
        s32Ret = VB_CreatePool(&u32PoolId, pCfg->u32BlkCount[i],
                                         pCfg->u32BlkSize[i], RESERVE_MMZ_NAME);
        if(HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "Create pool(Index=%d, Cnt=%d, Size=%d) fail\n",
                                  i, pCfg->u32BlkCount[i], pCfg->u32BlkSize[i]);
            break;
        }
        g_stVbPool.u32PoolCount = i + 1;
        g_stVbPool.u32PoolId[i] = u32PoolId;
        g_stVbPool.u32Size[i]   = pCfg->u32BlkSize[i];
    }

    /* 如果有一个缓存池没有分配成功，则进行回退 */
    if ( g_stVbPool.u32PoolCount != pCfg->u32PoolCount)
    {
        for(i=0; i < g_stVbPool.u32PoolCount; i++)
        {
            (HI_VOID)VB_DestroyPool(g_stVbPool.u32PoolId[i]);
            g_stVbPool.u32PoolId[i] = VB_INVALID_POOLID;
        }

        g_stVbPool.u32PoolCount = 0;

        return HI_ERR_PCIV_NOMEM;
    }

    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareWindowVbDestroy(HI_VOID)
{
    HI_S32 i;

    for(i=0; i < g_stVbPool.u32PoolCount; i++)
    {
        (HI_VOID)VB_DestroyPool(g_stVbPool.u32PoolId[i]);
        g_stVbPool.u32PoolId[i] = VB_INVALID_POOLID;
    }

    g_stVbPool.u32PoolCount = 0;
    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareMalloc(HI_U32 u32Size, HI_S32 s32LocalId, HI_U32 *pPhyAddr)
{
    HI_S32       i;
    VB_BLKHANDLE handle = VB_INVALID_HANDLE;

    HI_CHAR azMmzName[MAX_MMZ_NAME_LEN] = {0};

    if(s32LocalId == 0)
    {
        handle = VB_GetBlkBySize(u32Size, VB_UID_PCIV, azMmzName);
        if(VB_INVALID_HANDLE == handle)
        {
            PCIV_FMW_TRACE(HI_DBG_ERR,"VB_GetBlkBySize fail,size:%d!\n", u32Size);
            return HI_ERR_PCIV_NOBUF;
        }

        *pPhyAddr = VB_Handle2Phys(handle);

        return HI_SUCCESS;
    }

    /* 如果是从片，则从专用VB区域分配内存 */
    for(i=0; i < g_stVbPool.u32PoolCount; i++)
    {
        if(u32Size > g_stVbPool.u32Size[i]) continue;

        handle = VB_GetBlkByPoolId(g_stVbPool.u32PoolId[i], VB_UID_PCIV);

        if(VB_INVALID_HANDLE != handle) break;
    }

    if(VB_INVALID_HANDLE == handle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR,"VB_GetBlkBySize fail,size:%d!\n", u32Size);
        return HI_ERR_PCIV_NOBUF;
    }

    *pPhyAddr = VB_Handle2Phys(handle);
    return HI_SUCCESS;
}

HI_S32 PCIV_FirmWareFree(HI_U32 u32PhyAddr)
{
    VB_BLKHANDLE vbHandle;


    vbHandle = VB_Phy2Handle(u32PhyAddr);
    if(VB_INVALID_HANDLE == vbHandle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Invalid Physical Address 0x%08x\n", u32PhyAddr);
        return HI_ERR_PCIV_ILLEGAL_PARAM;
    }

    return VB_UserSub(VB_Handle2PoolId(vbHandle), u32PhyAddr, VB_UID_PCIV);
}


HI_S32 PCIV_FirmWarePicVoShow(PCIV_CHN pcivChn, PCIV_VOPIC_S *pVoPic,
                              VOU_WHO_SENDPIC_E enWhoSend)
{
    HI_S32               s32Ret;
    VIDEO_FRAME_INFO_S   stVideoFrmInfo;
    VI_FRAME_INFO_S      stViFrmInfo;
    PCIV_FWMCHANNEL_S   *pFmwChn;
    VIDEO_FRAME_S       *pVfrm = NULL;
    HI_S32 s32DevId = 0;
    HI_S32 s32ChnId = pcivChn;
    unsigned long flags;
    
    PCIVFMW_CHECK_CHNID(pcivChn);

    PCIVFMW_SPIN_LOCK;
    pFmwChn = &g_stFwmPcivChn[pcivChn];
    if(HI_TRUE != pFmwChn->bStart)
    {
        PCIVFMW_SPIN_UNLOCK;
        return HI_ERR_PCIV_SYS_NOTREADY;
    }

    pFmwChn->u32GetCnt++;
    s32Ret = HI_FAILURE;

    
    if (PCIV_BIND_VI == pVoPic->enSrcType)
    {
        memset(&stViFrmInfo, 0, sizeof(stViFrmInfo));
        stViFrmInfo.stViFrmInfo.u32PoolId       = VB_Handle2PoolId(VB_Phy2Handle(pFmwChn->au32PhyAddr[pVoPic->u32Index]));
        pVfrm                                   = &stViFrmInfo.stViFrmInfo.stVFrame;
        pVfrm->u32Width                         = pFmwChn->stPicAttr.u32Width;
        pVfrm->u32Height                        = pFmwChn->stPicAttr.u32Height;
        pVfrm->u32Field                         = pVoPic->enFiled;
        pVfrm->enPixelFormat                    = pFmwChn->stPicAttr.enPixelFormat;
        pVfrm->u32PhyAddr[0]                    = pFmwChn->au32PhyAddr[pVoPic->u32Index];
        pVfrm->u32PhyAddr[1]                    = pFmwChn->au32PhyAddr[pVoPic->u32Index] + pFmwChn->u32Offset[1];
        pVfrm->u32PhyAddr[2]                    = pFmwChn->au32PhyAddr[pVoPic->u32Index] + pFmwChn->u32Offset[2];
        pVfrm->u32Stride[0]                     = pFmwChn->stPicAttr.u32Stride[0];
        pVfrm->u32Stride[1]                     = pFmwChn->stPicAttr.u32Stride[1];
        pVfrm->u32Stride[2]                     = pFmwChn->stPicAttr.u32Stride[2];
        pVfrm->u64pts                           = pVoPic->u64Pts;
        pVfrm->u32TimeRef                       = pVoPic->u32TimeRef;
        stViFrmInfo.stMixCapState.bHasDownScale = pVoPic->stMixCapState.bHasDownScale;
        stViFrmInfo.stMixCapState.bMixCapMode   = pVoPic->stMixCapState.bMixCapMode;
        
        s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, s32ChnId, MPP_DATA_VIU_FRAME, &stViFrmInfo);
    }
    else if (PCIV_BIND_VDEC == pVoPic->enSrcType || PCIV_BIND_VO == pVoPic->enSrcType)
    {
        memset(&stVideoFrmInfo, 0, sizeof(stVideoFrmInfo));
        stVideoFrmInfo.u32PoolId = VB_Handle2PoolId(VB_Phy2Handle(pFmwChn->au32PhyAddr[pVoPic->u32Index]));
        pVfrm                    = &stVideoFrmInfo.stVFrame;
        pVfrm->u32Width          = pFmwChn->stPicAttr.u32Width;
        pVfrm->u32Height         = pFmwChn->stPicAttr.u32Height;
        pVfrm->u32Field          = pVoPic->enFiled;
        pVfrm->enPixelFormat     = pFmwChn->stPicAttr.enPixelFormat;
        pVfrm->u32PhyAddr[0]     = pFmwChn->au32PhyAddr[pVoPic->u32Index];
        pVfrm->u32PhyAddr[1]     = pFmwChn->au32PhyAddr[pVoPic->u32Index] + pFmwChn->u32Offset[1];
        pVfrm->u32PhyAddr[2]     = pFmwChn->au32PhyAddr[pVoPic->u32Index] + pFmwChn->u32Offset[2];
        pVfrm->u32Stride[0]      = pFmwChn->stPicAttr.u32Stride[0];
        pVfrm->u32Stride[1]      = pFmwChn->stPicAttr.u32Stride[1];
        pVfrm->u32Stride[2]      = pFmwChn->stPicAttr.u32Stride[2];
        pVfrm->u64pts            = pVoPic->u64Pts;
        pVfrm->u32TimeRef        = pVoPic->u32TimeRef;
        
        if (PCIV_BIND_VDEC == pVoPic->enSrcType)
        {
            s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, s32ChnId, MPP_DATA_VDEC_FRAME, &stVideoFrmInfo);
        }
        else
        {
            s32Ret = CALL_SYS_SendData(HI_ID_PCIV, s32DevId, s32ChnId, MPP_DATA_VOU_FRAME, &stVideoFrmInfo);
        }
        
    }
    else
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv chn %d bind type error, type value: %d.\n", pcivChn, pVoPic->enSrcType);
    }

    if((HI_SUCCESS != s32Ret) && (HI_ERR_VO_CHN_NOT_ENABLE != s32Ret) )
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv chn %d send failed, ret:0x%x\n", pcivChn, s32Ret);
        pFmwChn->u32LostCnt++;
    }
    else
    {
        pFmwChn->u32SendCnt++;
    }
    
    PcivFirmWareVoPicFree(pcivChn);
    PCIVFMW_SPIN_UNLOCK;
    return s32Ret;
}

/* VO显示完成后，调用PCIV或FwmDccs模块注册进来的处理函数 */
HI_VOID PcivFirmWareVoPicFree(unsigned long data)
{
    HI_U32   i, u32Count = 0;
    HI_S32   s32Ret;
    HI_BOOL  bHit    = HI_FALSE;
    PCIV_FWMCHANNEL_S *pFmwChn;
    PCIV_CHN pcivChn = (PCIV_CHN)data;
    PCIV_VOPIC_S stVoPic;

    pFmwChn = &g_stFwmPcivChn[pcivChn];

    if (pFmwChn->bStart != HI_TRUE)
    {
        return;
    }

    for(i=0; i<pFmwChn->u32Count; i++)
    {
        /* 只有当bVoHold 状态为真，且VO占用VB计数为0时， 才执行释放 */
        if(VB_InquireOneUserCnt(VB_Phy2Handle(pFmwChn->au32PhyAddr[i]), VB_UID_VOU) != 0)
        {
            continue;
        }
        if(VB_InquireOneUserCnt(VB_Phy2Handle(pFmwChn->au32PhyAddr[i]), VB_UID_VPSS) != 0)
        {
            continue;
        }
        if(VB_InquireOneUserCnt(VB_Phy2Handle(pFmwChn->au32PhyAddr[i]), VB_UID_GRP) != 0)
        {
            continue;
        }

        /* 调用PCIV注册的回调函数，用于其处理VO显示完毕后的操作 */
        if(g_stPcivFmwCallBack.pfPicFreeFromVo)
        {
            stVoPic.u32Index = i;       /* 可以释放的Buffer序号 */
            stVoPic.u64Pts   = 0;       /* 时间戳 */
            stVoPic.u32Count = u32Count;/* 目前未使用 */
            s32Ret = g_stPcivFmwCallBack.pfPicFreeFromVo(pcivChn, &stVoPic);
            if (s32Ret != HI_SUCCESS)
            {
                PCIV_FMW_TRACE(HI_DBG_ERR, "pcivfmw chn%d pfPicFreeFromVo() failed\n", pcivChn);
                continue;
            }
            bHit = HI_TRUE;
            pFmwChn->bVoHold[i] = HI_FALSE;
            pFmwChn->u32RespCnt++;
        }
    }

    /* 如果没有Buffer被VO释放，则启动定时器任务10ms后再检查 */
    if(bHit != HI_TRUE)
    {
        pFmwChn->stBufTimer.function = PcivFirmWareVoPicFree;
        pFmwChn->stBufTimer.data     = data;
        mod_timer(&(pFmwChn->stBufTimer), jiffies + 1);
    }

    return ;
}


/* PCI传输完成后，释放 DSU 缩放时使用的视频缓冲块 */
HI_S32 PCIV_FirmWareSrcPicFree(PCIV_CHN pcivChn, PCIV_SRCPIC_S *pSrcPic)
{
    HI_S32 s32Ret;

    g_stFwmPcivChn[pcivChn].u32RespCnt++;

    /* 如果MPP 系统已经去初始化，那么Sys模块会将缓存释放*/
    if (PCIVFMW_STATE_STOPED == s_u32PcivFmwState)
    {
        return HI_SUCCESS;
    }

    PCIV_FMW_TRACE(HI_DBG_DEBUG,"- --> addr:0x%x\n", pSrcPic->u32PhyAddr);
    s32Ret = VB_UserSub(pSrcPic->u32PoolId, pSrcPic->u32PhyAddr, VB_UID_PCIV);
    HI_ASSERT(HI_SUCCESS == s32Ret);
    return s32Ret;
}

static HI_S32 PcivFmwSrcPicFilterEx2(PCIV_FWMCHANNEL_S *pFmwChn,
        VIDEO_FRAME_S *pstInFrame, DSU_ODD_OPT_S *pstDsuOpt)
{
    if ((PCIV_FILTER_TYPE_EX2 == pFmwChn->stPreProcCfg.enFilterType)
        && (VIDEO_FIELD_INTERLACED == pstInFrame->u32Field))
    {
        pstDsuOpt->enFilterType = FILTER_PARAM_TYPE_EX2;
        pstDsuOpt->bForceVFilt  = HI_TRUE;
        pstInFrame->u32Field    = VIDEO_FIELD_FRAME;
    }
    return HI_SUCCESS;
}

/*对vi混合输入且vi未水平缩放的图像，选择2级缩放策略*/
static HI_S32 PcivFmwSrcPicFilterEx(const VI_MIXCAP_STAT_S *pstMixStat, DSU_ODD_OPT_S *pstDsuOpt)
{
    if ((NULL != pstMixStat) && (HI_TRUE == pstMixStat->bMixCapMode) && (HI_FALSE == pstMixStat->bHasDownScale))
	{
		pstDsuOpt->enScaleStrategy = DSU_SCALE_STRATEGY_2LEVEL;
	
        PCIV_FMW_TRACE(HI_DBG_INFO, "Scale: %d; mixmode:%d, downscale:%d \n",
        pstDsuOpt->enScaleStrategy, pstMixStat->bMixCapMode, pstMixStat->bHasDownScale);
    }
    
    return HI_SUCCESS;
}

static HI_S32 PcivFmwSrcPicFieldDrop(PCIV_FWMCHANNEL_S *pFmwChn, VIDEO_FRAME_S *pstInFrame)
{
    if (VIDEO_FIELD_INTERLACED == pstInFrame->u32Field
        && pFmwChn->stPicAttr.u32Height <= (pstInFrame->u32Height/2))
    {
        if (PCIV_FIELD_TOP == pFmwChn->stPreProcCfg.enFieldSel
            || PCIV_FIELD_BOTTOM == pFmwChn->stPreProcCfg.enFieldSel)
        {
            if (PCIV_FIELD_BOTTOM == pFmwChn->stPreProcCfg.enFieldSel)
            {
                pstInFrame->u32PhyAddr[0] += pstInFrame->u32Stride[0];
                pstInFrame->u32PhyAddr[1] += pstInFrame->u32Stride[1];
                pstInFrame->u32PhyAddr[2] += pstInFrame->u32Stride[2];
            }

            pstInFrame->u32Stride[0] <<= 1;
            pstInFrame->u32Stride[1] <<= 1;
            pstInFrame->u32Stride[2] <<= 1;
            pstInFrame->u32Height >>= 1;

            pstInFrame->u32Field = VIDEO_FIELD_FRAME;
            return HI_SUCCESS;
        }
    }
    return HI_FAILURE;
}

#if 0
static HI_S32 PcivFmwOsdBmpFieldDrop(DSU_BITMAP_S *pstBitmap, VIDEO_FIELD_E enField)
{
    if (VIDEO_FIELD_TOP == enField || VIDEO_FIELD_BOTTOM == enField)
    {
        pstBitmap->u32Stride <<= 1;
        pstBitmap->u32Height >>= 1;
        if (VIDEO_FIELD_BOTTOM == enField)
        {
            pstBitmap->u32PhyAddr += pstBitmap->u32Stride;
        }
    }
    return HI_SUCCESS;
}
#endif

static HI_S32 PcivFmwSrcPicSend(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pBindObj,
  const VIDEO_FRAME_INFO_S *pstVFrame, const VIDEO_FRAME_INFO_S *pstVdecFrame, VI_MIXCAP_STAT_S *pstMixCapState)
{
    HI_S32 s32Ret;
    PCIV_FWMCHANNEL_S *pChn = &g_stFwmPcivChn[pcivChn];
    PCIV_SRCPIC_S stSrcPic;
    unsigned long flags;

    stSrcPic.u32PoolId  = pstVFrame->u32PoolId;
    stSrcPic.u32PhyAddr = pstVFrame->stVFrame.u32PhyAddr[0];
    stSrcPic.u64Pts     = pstVFrame->stVFrame.u64pts;
    stSrcPic.u32TimeRef = pstVFrame->stVFrame.u32TimeRef;
    stSrcPic.enFiled    = pstVFrame->stVFrame.u32Field;
    stSrcPic.enSrcType  = pBindObj->enType;

    if (pChn->bStart != HI_TRUE)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d have stoped \n", pcivChn);
        return HI_FAILURE;
    }

    /* 调用上层模块注册的回调，发送DSU 缩放后图像 */
    s32Ret = g_stPcivFmwCallBack.pfSrcSendPic(pcivChn, &stSrcPic, pstMixCapState);
    if (s32Ret != HI_SUCCESS)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d pfSrcSendPic failed \n", pcivChn);
        return HI_FAILURE;
    }

    /* 成功发送，占有此VB缓存 (PCIV_FirmWareSrcPicFree接口中最终释放) */
    VB_UserAdd(pstVFrame->u32PoolId, pstVFrame->stVFrame.u32PhyAddr[0], VB_UID_PCIV);

    if (NULL != pstVdecFrame && PCIV_BIND_VDEC == pBindObj->enType)
    {
        /* 发送成功，vdec源图像数据需要在此释放 */
        PCIVFMW_SPIN_LOCK;
        s32Ret = VB_UserSub(pstVdecFrame->u32PoolId, pstVdecFrame->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        HI_ASSERT(s32Ret == HI_SUCCESS);

        HI_ASSERT(pChn->pCurVdecNode != NULL);
        PCIV_PicQueuePutFree(&pChn->stPicQueue, pChn->pCurVdecNode);
        pChn->pCurVdecNode = NULL;
        PCIVFMW_SPIN_UNLOCK;
    }

    pChn->u32SendCnt++;

    return HI_SUCCESS;
}

HI_S32 PcivFmwGetRegion(PCIV_CHN PcivChn, RGN_TYPE_E enType, RGN_INFO_S *pstRgnInfo)
{
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;

    if ((!CKFN_RGN()) || (!CKFN_RGN_GetRegion()))
    {
        return HI_FAILURE;
    }

    stChn.enModId  = HI_ID_PCIV;
    stChn.s32ChnId = PcivChn;
    stChn.s32DevId = 0;         
    s32Ret = CALL_RGN_GetRegion(enType, &stChn, pstRgnInfo);
    HI_ASSERT(HI_SUCCESS == s32Ret);     
    
    return s32Ret;
}

HI_VOID PcivFmwPutRegion(PCIV_CHN PcivChn, RGN_TYPE_E enType)
{
    HI_S32 s32Ret = HI_FAILURE;
    MPP_CHN_S stChn;

    if ((!CKFN_RGN()) || (!CKFN_RGN_PutRegion()))
    {
        return ;
    }
    
    stChn.enModId  = HI_ID_PCIV;
    stChn.s32ChnId = PcivChn;
    stChn.s32DevId = 0;
    
	s32Ret = CALL_RGN_PutRegion(enType, &stChn);    
    HI_ASSERT(HI_SUCCESS == s32Ret);
    return ;
} 

static HI_VOID PcivFmwSrcPicZoomCb(const DSU_TASK_DATA_S *pstDsuTask)
{
    PCIV_CHN           pcivChn;
    PCIV_FWMCHANNEL_S *pChn;
    HI_S32             s32Ret;
    HI_S32             s32ViMixStat = 0;
    PCIV_BIND_OBJ_S stBindObj = {0};
    const VIDEO_FRAME_INFO_S *pstImgIn = &pstDsuTask->stImgIn;
    const VIDEO_FRAME_INFO_S *pstImgOut = &pstDsuTask->stImgOut;
    unsigned long flags;
    VI_MIXCAP_STAT_S stMixCapState;
        
    pcivChn = (HI_S32)(pstDsuTask->privateData);
    HI_ASSERT((pcivChn >= 0) && (pcivChn < PCIVFMW_MAX_CHN_NUM));
    pChn = &g_stFwmPcivChn[pcivChn];

    pChn->u32ZoomCbCnt++;

    VB_UserSub(pstImgIn->u32PoolId, pstImgIn->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
   
    stBindObj.enType = HIGH_WORD(pstDsuTask->reserved);
     
    /* DSU中断内，可能通道已经停止 */
    if (pChn->bStart != HI_TRUE)
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn %d have stoped \n", pcivChn);
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            PCIVFMW_SPIN_LOCK;
            pChn->enSendState = PCIVFMW_SEND_NOK;
            PCIVFMW_SPIN_UNLOCK;
        }
        goto out;
    }

    //stBindObj.enType = HIGH_WORD(pstDsuTask->reserved);
    s32ViMixStat = LOW_WORD(pstDsuTask->reserved);
    stMixCapState.bMixCapMode   = (s32ViMixStat & 0x00ff);
    stMixCapState.bHasDownScale = ((s32ViMixStat >> 8) & 0x00ff);
    
    /* 发送缩放后视频帧 */
    s32Ret = PcivFmwSrcPicSend(pcivChn, &stBindObj, pstImgOut, pstImgIn, &stMixCapState);
    if (s32Ret != HI_SUCCESS)
    {
        if (PCIV_BIND_VDEC == stBindObj.enType)
        {
            PCIVFMW_SPIN_LOCK;
            pChn->enSendState = PCIVFMW_SEND_NOK;
            PCIVFMW_SPIN_UNLOCK;
        }
        goto out;
    }
    if (PCIV_BIND_VDEC == stBindObj.enType)
    {
        PCIVFMW_SPIN_LOCK;
        pChn->enSendState = PCIVFMW_SEND_OK;
        PCIVFMW_SPIN_UNLOCK;
    }

    if (0 != pChn->u32RgnNum)
    {
        pChn->u32RgnNum = 0;
    }
    
out:
    /* 不管是否发送成功，本回调中都释放输出图像的VB缓存 */
    s32Ret = VB_UserSub(pstImgOut->u32PoolId, pstImgOut->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    return;
}


static HI_S32 PcivFmwSrcPicZoom(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pObj, const VIDEO_FRAME_INFO_S *pstSrcFrame, VI_MIXCAP_STAT_S *pstMixCapState)
{
    HI_S32              s32Ret;
    HI_S32              s32ViMixStat = 0;
    PCIV_FWMCHANNEL_S   *pChn = NULL;
    VIDEO_FRAME_S       *pstOutFrame, *pstInFrame;
    VB_BLKHANDLE        VbHandle;
    DSU_TASK_DATA_S     stDsuTask;
    DSU_ODD_OPT_S       stDsuOpt = {0};
    DSU_HANDLE          DsuHandle;
    DSU_EXPORT_FUNC_S   *pDsuExportFunc = (DSU_EXPORT_FUNC_S *)(g_astModules[HI_ID_DSU].pstExportFuncs);

    MPP_CHN_S stChn;
    HI_VOID *pMmzName = HI_NULL;

    pChn = &g_stFwmPcivChn[pcivChn];

    /* 获取用于DSU缩放后输出图像的 Video Buffer */
    stChn.enModId = HI_ID_PCIV;
    stChn.s32DevId = 0;
    stChn.s32ChnId = pcivChn;
    CALL_SYS_GetMmzName(&stChn, &pMmzName);

    VbHandle = VB_GetBlkBySize(pChn->u32BlkSize, VB_UID_PCIV, pMmzName);
    if (VB_INVALID_HANDLE == VbHandle)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Get VB(%dByte) fail\n",pChn->u32BlkSize);
        return HI_FAILURE;
    }
    PCIV_FMW_TRACE(HI_DBG_DEBUG, "+ --> addr:0x%x\n", VB_Handle2Phys(VbHandle));

    /* 配置输入视频帧信息 */
    memcpy(&stDsuTask.stImgIn, pstSrcFrame, sizeof(VIDEO_FRAME_INFO_S));
    pstInFrame = &stDsuTask.stImgIn.stVFrame;

    s32ViMixStat = (pstMixCapState->bMixCapMode & 0x00ff) | (pstMixCapState->bHasDownScale << 8);
    
    /* 配置DSU选项结构体 */
    memcpy(&stDsuOpt, &pChn->stDsuOpt, sizeof(DSU_ODD_OPT_S));

    /* 如果配置为扩展滤波系数2，并且输入图像为两场间插格式 */
    (HI_VOID)PcivFmwSrcPicFilterEx2(pChn, pstInFrame, &stDsuOpt);

    /* 如果前端为D1/CIF混合输入模式，使用二次缩放 */
    (HI_VOID)PcivFmwSrcPicFilterEx(pstMixCapState, &stDsuOpt);
    
    /* 如果输入为两场间插格式，但输出配置为单场时，先进行软件丢场处理 */
    (HI_VOID)PcivFmwSrcPicFieldDrop(pChn, pstInFrame);

    /* 配置输出视频帧信息 */
    stDsuTask.stImgOut.u32PoolId = VB_Handle2PoolId(VbHandle);
    pstOutFrame                 = &stDsuTask.stImgOut.stVFrame;
    pstOutFrame->u32Width       = pChn->stPicAttr.u32Width;
    pstOutFrame->u32Height      = pChn->stPicAttr.u32Height;
    pstOutFrame->u32Field       = pstInFrame->u32Field;
    pstOutFrame->enPixelFormat  = pChn->stPicAttr.enPixelFormat;
    pstOutFrame->u32PhyAddr[0]  = VB_Handle2Phys(VbHandle);
    pstOutFrame->u32PhyAddr[1]  = pstOutFrame->u32PhyAddr[0] + pChn->u32Offset[1];
    pstOutFrame->u32PhyAddr[2]  = pstOutFrame->u32PhyAddr[0] + pChn->u32Offset[2];
    pstOutFrame->pVirAddr[0]    = (HI_VOID*)VB_Handle2Kern(VbHandle);
    pstOutFrame->pVirAddr[1]    = pstOutFrame->pVirAddr[0] + pChn->u32Offset[1];
    pstOutFrame->pVirAddr[2]    = pstOutFrame->pVirAddr[0] + pChn->u32Offset[2];
    pstOutFrame->u32Stride[0]   = pChn->stPicAttr.u32Stride[0];
    pstOutFrame->u32Stride[1]   = pChn->stPicAttr.u32Stride[1];
    pstOutFrame->u32Stride[2]   = pChn->stPicAttr.u32Stride[2];
    pstOutFrame->u64pts         = pstInFrame->u64pts;
    pstOutFrame->u32TimeRef     = pstInFrame->u32TimeRef;

    /* 配置DSU任务信息结构体中的其他项 (DSU回调中执行发送图像操作) */
    stDsuTask.privateData = pcivChn;
    stDsuTask.pCallBack   = PcivFmwSrcPicZoomCb;
    stDsuTask.enCallModId = HI_ID_PCIV;
    stDsuTask.reserved    = MAKE_DWORD(pObj->enType, s32ViMixStat);


    /* 1.Begin DSU job-----------------------------------------------------------------------------------------------------------*/
    s32Ret = pDsuExportFunc->pfnDsuBeginJob(&DsuHandle, DSU_JOB_PRI_NORMAL, HI_ID_PCIV, pcivChn, NULL);
    if (s32Ret != HI_SUCCESS)
    {
        pChn->u32AddJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnDsuBeginJob failed ! pcivChn:%d \n", pcivChn);
        return HI_FAILURE;
    }
    pChn->u32AddJobSucCnt++;

    /* 2.zoom the pic-----------------------------------------------------------------------------------------------------------*/
    
    s32Ret = pDsuExportFunc->pfnDsuAddOddScTask(DsuHandle, &stDsuTask, &stDsuOpt);
    if (HI_SUCCESS != s32Ret)
    {
        pChn->u32ZoomTaskFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "create dsu task failed,errno:%x,will lost this frame\n",s32Ret);
        pDsuExportFunc->pfnDsuCancleJob(DsuHandle);
        VB_UserSub(stDsuTask.stImgOut.u32PoolId, stDsuTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        
        return HI_FAILURE;
    }
    
    VB_UserAdd(stDsuTask.stImgIn.u32PoolId, stDsuTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);
    pChn->u32ZoomTaskSucCnt++;

    /* 3.End DSU job-----------------------------------------------------------------------------------------------------------*/
    
    s32Ret = pDsuExportFunc->pfnDsuEndJob(DsuHandle);
    if (s32Ret != HI_SUCCESS)/* Notes: If EndJob failed, callback will called auto */
    {
        pChn->u32EndJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnDsuEndJob failed ! pcivChn:%d \n", pcivChn);
        pDsuExportFunc->pfnDsuCancleJob(DsuHandle);
        
        return HI_FAILURE;
    }
    
    pChn->u32EndJobSucCnt++;

    return HI_SUCCESS;
}

static HI_VOID PcivFmwSrcPicOsdCb(const DSU_TASK_DATA_S *pstDsuTask)
{
    PCIV_CHN            pcivChn;
    PCIV_FWMCHANNEL_S   *pFmwChn   = NULL;
    const VIDEO_FRAME_INFO_S  *pstImgIn  = NULL;
    const VIDEO_FRAME_INFO_S  *pstImgOut = NULL;

    //PCIVFMW_CHECK_PTR(pstDsuTask);
    if (NULL == pstDsuTask)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "In Function PcivFmwSrcPicOsdCb: pstDsuTask is null, return!\n");
        return;
    }
    
    pstImgIn  = &pstDsuTask->stImgIn;
    pstImgOut = &pstDsuTask->stImgOut;
    pcivChn = (HI_S32)(pstDsuTask->privateData);
    pFmwChn = &g_stFwmPcivChn[pcivChn];        

    pFmwChn->u32OsdCbCnt++;
        
    /* 释放获取的region */
    PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
    pFmwChn->u32RgnNum = 0;
    /* 输入图像已经使用完毕应该释放 */
    VB_UserSub(pstImgIn->u32PoolId, pstImgIn->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
   
    /* DSU回调处于DSU中断中，必须判断通道是否已停止 */
    if (HI_FALSE == pFmwChn->bStart)
    {
        VB_UserSub(pstImgOut->u32PoolId, pstImgOut->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
        return;
    }
    
    if (pstDsuTask->enFnshStat != DSU_TASK_FNSH_STAT_OK)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "PcivFmwSrcPicOsd status is NOK, chn:%d\n", pcivChn);
    }
 
    return;
}


/* DSU callback of PcivFmwSrcPicMove */
static HI_VOID PcivFmwSrcPicMoveCb(const DSU_TASK_DATA_S *pstDsuTask)
{
    
    PCIV_CHN pcivChn;
    PCIV_FWMCHANNEL_S *pFmwChn = NULL;
    const VIDEO_FRAME_INFO_S *pstImgIn = &pstDsuTask->stImgIn;
    pcivChn = (HI_S32)(pstDsuTask->privateData);
    pFmwChn = &g_stFwmPcivChn[pcivChn];

    pFmwChn->u32MoveCbCnt++;

    /* 源输入图像VB缓存使用完毕，可以释放 */
    VB_UserSub(pstImgIn->u32PoolId, pstImgIn->stVFrame.u32PhyAddr[0], VB_UID_PCIV);
    
    return;
}


static HI_S32 PcivFmwSrcPicMove(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pObj,
        const VIDEO_FRAME_INFO_S *pstSrcFrame, VI_MIXCAP_STAT_S *pstMixCapState)
{
    HI_S32              i;
    HI_S32              s32Ret;
    HI_S32              s32Value;
    HI_S32              s32ViMixStat = 0;
    HI_U32              u32PicSize = 0;
    DSU_TASK_DATA_S     stDsuTask;
    DSU_DUAL_OPT_S      stOpt;
    DSU_ODD_OPT_S       stDsuOpt = {0};
    DSU_HANDLE          DsuHandle;
    DSU_BITMAP_S        stOverlayImg;
    VB_BLKHANDLE        VbHandleImage;
    VB_BLKHANDLE        VbHandleZoom;
    VIDEO_FRAME_S       *pstOutFrame = NULL;
    VIDEO_FRAME_S       *pstInFrame = NULL;
    RECT_S              stRect;
    RGN_INFO_S          stRgnInfo;
    MPP_CHN_S           stChn;
    HI_VOID             *pMmzName = HI_NULL;
    PCIV_FWMCHANNEL_S   *pFmwChn = &g_stFwmPcivChn[pcivChn];
    DSU_EXPORT_FUNC_S   *pfnDsuExpFunc = (DSU_EXPORT_FUNC_S*)(g_astModules[HI_ID_DSU].pstExportFuncs);

    stRgnInfo.u32Num = 0;
    s32Ret   = PcivFmwGetRegion(pcivChn, OVERLAYEX_RGN, &stRgnInfo);
    s32Value = s32Ret;
    if (stRgnInfo.u32Num <= 0)
    {
        PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
        s32Ret = PcivFmwSrcPicZoom(pcivChn, pObj, pstSrcFrame, pstMixCapState);
        return s32Ret; 
    }
    
    /* 配置DSU参数选项 */
    memcpy(&stDsuOpt, &pFmwChn->stDsuOpt, sizeof(DSU_ODD_OPT_S));

    /* 输入图像信息拷贝自 pstSrcFrame */
    memcpy(&stDsuTask.stImgIn,  pstSrcFrame, sizeof(VIDEO_FRAME_INFO_S));
    s32ViMixStat = (pstMixCapState->bMixCapMode & 0x00ff) | (pstMixCapState->bHasDownScale << 8);

    /* 扩展滤波系数二的处理 */
    (HI_VOID)PcivFmwSrcPicFilterEx2(pFmwChn, &stDsuTask.stImgIn.stVFrame, &stDsuOpt);

    /* 如果前端为D1/CIF混合输入模式，使用二次缩放 */
    (HI_VOID)PcivFmwSrcPicFilterEx(pstMixCapState, &stDsuOpt);
    
    /* 软件丢场的处理 */
    s32Ret = PcivFmwSrcPicFieldDrop(pFmwChn, &stDsuTask.stImgIn.stVFrame);

    /* 如果成功丢场，那么利用u32PrivateData来标记下级叠OSD时也需要丢场 */
    if (HI_SUCCESS == s32Ret)
    {
        stDsuTask.stImgIn.stVFrame.u32PrivateData =
            (PCIV_FIELD_BOTTOM == pFmwChn->stPreProcCfg.enFieldSel) ? \
            VIDEO_FIELD_BOTTOM : VIDEO_FIELD_TOP;
    }
    else
    {
        stDsuTask.stImgIn.stVFrame.u32PrivateData = 0;
    }

    /* 输出图像基本信息与输入图像一致 */
    memcpy(&stDsuTask.stImgOut, &stDsuTask.stImgIn, sizeof(VIDEO_FRAME_INFO_S));

    /* 计算输出图像大小并申请内存 */
    s32Ret = PcivFmwGetPicSize(&stDsuTask.stImgOut.stVFrame, &u32PicSize);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    stChn.enModId = HI_ID_PCIV;
    stChn.s32DevId = 0;
    stChn.s32ChnId = pcivChn;
    CALL_SYS_GetMmzName(&stChn, &pMmzName);

    VbHandleImage = VB_GetBlkBySize(u32PicSize, VB_UID_PCIV, pMmzName);
    if (VB_INVALID_HANDLE == VbHandleImage)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "======line %d ====Get VB(%dByte) buffer for image out fail,chn: %d.=======\n", __LINE__, u32PicSize, pcivChn);
        return HI_FAILURE;
    }

    /* 根据内存信息配置输出图像的地址信息 */
    stDsuTask.stImgOut.u32PoolId = VB_Handle2PoolId(VbHandleImage);
    pstOutFrame = &stDsuTask.stImgOut.stVFrame;
    pstOutFrame->u32PhyAddr[0]  = VB_Handle2Phys(VbHandleImage);
    pstOutFrame->u32PhyAddr[1]  = pstOutFrame->u32PhyAddr[0] + (pstOutFrame->u32Stride[0]*pstOutFrame->u32Height);
    pstOutFrame->u32PhyAddr[2]  = pstOutFrame->u32PhyAddr[0] + (pstOutFrame->u32Stride[1]*pstOutFrame->u32Height);


    /* 1.Begin DSU job-----------------------------------------------------------------------------------------------------------*/
   
    s32Ret = pfnDsuExpFunc->pfnDsuBeginJob(&DsuHandle, DSU_JOB_PRI_NORMAL, HI_ID_PCIV, pcivChn, NULL);
    if (s32Ret != HI_SUCCESS)
    {
        pFmwChn->u32AddJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnDsuBeginJob failed ! pcivChn:%d \n", pcivChn);
        return HI_FAILURE;
    }
    pFmwChn->u32AddJobSucCnt++;
    
    /* 2.move the picture------------------------------------------------------------------------------------------------------*/
    
    /* 配置DSU任务回调及其他信息 */
    stDsuTask.privateData = pcivChn;
    stDsuTask.pCallBack   = PcivFmwSrcPicMoveCb;
    stDsuTask.enCallModId = HI_ID_PCIV;
    stDsuTask.reserved    = MAKE_DWORD(pObj->enType, pObj->unAttachObj.vdecDevice.vdecChn);
    /* 输入图像被使用中，增加输入图像的VB计数 */
    VB_UserAdd(stDsuTask.stImgIn.u32PoolId, stDsuTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);    
    
    s32Ret = pfnDsuExpFunc->pfnDsuAddOddScTask(DsuHandle, &stDsuTask, &stDsuOpt);
    if (HI_SUCCESS != s32Ret)
    {
        pFmwChn->u32MoveTaskFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "create dsu task move failed,errno:0x%x, will lost this frame\n",s32Ret);
        pfnDsuExpFunc->pfnDsuCancleJob(DsuHandle);
        VB_UserSub(stDsuTask.stImgIn.u32PoolId, stDsuTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);  
        VB_UserSub(stDsuTask.stImgOut.u32PoolId, stDsuTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);  
        return HI_FAILURE;
    }
    pFmwChn->u32MoveTaskSucCnt++;
    
    /* 3.add osd to the picture------------------------------------------------------------------------------------------------*/

    if (HI_FALSE == pFmwChn->bStart)
    {
        VB_UserSub(stDsuTask.stImgOut.u32PoolId, stDsuTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);  
        
        pfnDsuExpFunc->pfnDsuCancleJob(DsuHandle);
        return HI_FAILURE;
    }

    if (s32Value == HI_SUCCESS && stRgnInfo.u32Num > 0)    
    {
        HI_ASSERT(stRgnInfo.u32Num <= OVERLAYEX_MAX_NUM);
	    memcpy(&stDsuTask.stImgIn,  &stDsuTask.stImgOut, sizeof(VIDEO_FRAME_INFO_S));
        pFmwChn->u32RgnNum = stRgnInfo.u32Num;
        
        /* cover process */
        stDsuTask.reserved      = 0;
        stDsuTask.pCallBack     = NULL;
        stDsuTask.privateData   = pcivChn;
        stDsuTask.enCallModId   = HI_ID_PCIV;
        for (i=0; i<stRgnInfo.u32Num; i++)
        {
            stOverlayImg.u32PhyAddr    = stRgnInfo.ppstRgnComm[i]->u32PhyAddr;
            stOverlayImg.enPixelFormat = stRgnInfo.ppstRgnComm[i]->enPixelFmt;
            stOverlayImg.u32Height     = stRgnInfo.ppstRgnComm[i]->stSize.u32Height;
            stOverlayImg.u32Width      = stRgnInfo.ppstRgnComm[i]->stSize.u32Width;
            stOverlayImg.u32Stride     = stRgnInfo.ppstRgnComm[i]->u32Stride;
        
            if (PIXEL_FORMAT_RGB_1555 == stOverlayImg.enPixelFormat)
            {
                stOverlayImg.bAlphaExt1555 = HI_TRUE;
                stOverlayImg.u8Alpha0      = stRgnInfo.ppstRgnComm[i]->u32BgAlpha;
                stOverlayImg.u8Alpha1      = stRgnInfo.ppstRgnComm[i]->u32FgAlpha;
            }

		    stRect.s32X      = stRgnInfo.ppstRgnComm[i]->stPoint.s32X;
            stRect.s32Y      = stRgnInfo.ppstRgnComm[i]->stPoint.s32Y;
            stRect.u32Width  = stRgnInfo.ppstRgnComm[i]->stSize.u32Width;
            stRect.u32Height = stRgnInfo.ppstRgnComm[i]->stSize.u32Height;
            
            stOpt.u8GlobalAlpha     = 255;
            
            if (stRgnInfo.u32Num - 1 == i)
            {
                stDsuTask.pCallBack = PcivFmwSrcPicOsdCb;
            }
            VB_UserAdd(stDsuTask.stImgIn.u32PoolId, stDsuTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV); 
            
            /* Add DSU task */
            s32Ret = pfnDsuExpFunc->pfnDsuAddDualScTask(DsuHandle, &stDsuTask, &stOverlayImg, &stRect, &stOpt);
            if (s32Ret != HI_SUCCESS)
            {
                pFmwChn->u32OsdTaskFailCnt++;
                PCIV_FMW_TRACE(HI_DBG_ERR, "DSU task to add osd failed ! pcivChn:%d \n", pcivChn); 
                /* 释放获取的region */
                PcivFmwPutRegion(pcivChn, OVERLAYEX_RGN);
          
                VB_UserSub(stDsuTask.stImgIn.u32PoolId, stDsuTask.stImgIn.stVFrame.u32PhyAddr[0], VB_UID_PCIV);  
            }
            else
            {
                pFmwChn->u32OsdTaskSucCnt++;
            }
        }
        
    }
    
    /* 4.zoom the picture--------------------------------------------------------------------------------------------------*/
    
    /* 获取用于DSU缩放后输出图像的 Video Buffer */
    stChn.enModId = HI_ID_PCIV;
    stChn.s32DevId = 0;
    stChn.s32ChnId = pcivChn;
    CALL_SYS_GetMmzName(&stChn, &pMmzName);

    VbHandleZoom = VB_GetBlkBySize(pFmwChn->u32BlkSize, VB_UID_PCIV, pMmzName);
    if (VB_INVALID_HANDLE == VbHandleZoom)
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "======line %d ====Get VB(%dByte) fail for image after zoom the picture=======\n", pFmwChn->u32BlkSize, __LINE__);
        pfnDsuExpFunc->pfnDsuCancleJob(DsuHandle);
        VB_UserSub(stDsuTask.stImgOut.u32PoolId, stDsuTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);  
        return HI_FAILURE;
    }
    PCIV_FMW_TRACE(HI_DBG_DEBUG, "+ --> addr:0x%x\n", VB_Handle2Phys(VbHandleZoom));
	
    /* 配置输入视频帧信息 */
    memcpy(&stDsuTask.stImgIn, &stDsuTask.stImgOut, sizeof(VIDEO_FRAME_INFO_S));
    pstInFrame = &stDsuTask.stImgIn.stVFrame;
    
    /* 配置输出视频帧信息 */
    stDsuTask.stImgOut.u32PoolId = VB_Handle2PoolId(VbHandleZoom);
    pstOutFrame                 = &stDsuTask.stImgOut.stVFrame;
    pstOutFrame->u32Width       = pFmwChn->stPicAttr.u32Width;
    pstOutFrame->u32Height      = pFmwChn->stPicAttr.u32Height;
    pstOutFrame->u32Field       = pstInFrame->u32Field;
    pstOutFrame->enPixelFormat  = pFmwChn->stPicAttr.enPixelFormat;
    pstOutFrame->u32PhyAddr[0]  = VB_Handle2Phys(VbHandleZoom);
    pstOutFrame->u32PhyAddr[1]  = pstOutFrame->u32PhyAddr[0] + pFmwChn->u32Offset[1];
    pstOutFrame->u32PhyAddr[2]  = pstOutFrame->u32PhyAddr[0] + pFmwChn->u32Offset[2];
    pstOutFrame->pVirAddr[0]    = (HI_VOID*)VB_Handle2Kern(VbHandleZoom);
    pstOutFrame->pVirAddr[1]    = pstOutFrame->pVirAddr[0] + pFmwChn->u32Offset[1];
    pstOutFrame->pVirAddr[2]    = pstOutFrame->pVirAddr[0] + pFmwChn->u32Offset[2];
    pstOutFrame->u32Stride[0]   = pFmwChn->stPicAttr.u32Stride[0];
    pstOutFrame->u32Stride[1]   = pFmwChn->stPicAttr.u32Stride[1];
    pstOutFrame->u32Stride[2]   = pFmwChn->stPicAttr.u32Stride[2];
    pstOutFrame->u64pts         = pstInFrame->u64pts;

    /* 配置DSU任务信息结构体中的其他项 (DSU回调中执行发送图像操作) */
    stDsuTask.privateData = pcivChn;
    stDsuTask.pCallBack   = PcivFmwSrcPicZoomCb;
    stDsuTask.enCallModId = HI_ID_PCIV;
    stDsuTask.reserved    = MAKE_DWORD(pObj->enType, s32ViMixStat);
     
    s32Ret = pfnDsuExpFunc->pfnDsuAddOddScTask(DsuHandle, &stDsuTask, &stDsuOpt);
    if (HI_SUCCESS != s32Ret)
    {
        pFmwChn->u32ZoomTaskFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "create dsu task failed,errno:%x,will lost this frame\n",s32Ret);
        pfnDsuExpFunc->pfnDsuCancleJob(DsuHandle);
        VB_UserSub(stDsuTask.stImgOut.u32PoolId, stDsuTask.stImgOut.stVFrame.u32PhyAddr[0], VB_UID_PCIV);  
        return HI_FAILURE;
    }
    pFmwChn->u32ZoomTaskSucCnt++;
    
    /* 5.End DSU job------------------------------------------------------------------------------------------------------*/
    
    s32Ret = pfnDsuExpFunc->pfnDsuEndJob(DsuHandle);
    if (s32Ret != HI_SUCCESS)/* Notes: If EndJob failed, callback will called auto */
    {
        pFmwChn->u32EndJobFailCnt++;
        PCIV_FMW_TRACE(HI_DBG_ERR, "pfnDsuEndJob failed ! pcivChn:%d \n", pcivChn);
        pfnDsuExpFunc->pfnDsuCancleJob(DsuHandle);
        
        return HI_FAILURE;
    }

    pFmwChn->u32EndJobSucCnt++;

    return HI_SUCCESS;
    
}

static HI_S32 PcivFirmWareSrcPreProc(PCIV_CHN pcivChn,PCIV_BIND_OBJ_S *pObj,
            const VIDEO_FRAME_INFO_S *pstSrcFrame, VI_MIXCAP_STAT_S *pstMixCapState)
{
    HI_S32 s32Ret;
    PCIV_FWMCHANNEL_S *pChn = NULL;
    pChn = &g_stFwmPcivChn[pcivChn];

    if (drop_err_timeref == 1)
    {
        /* 防止发送的VI原始图像乱序，丢弃乱序的视频帧 (u32TimeRef应该是递增的) */
        if ((PCIV_BIND_VI == pObj->enType) || (PCIV_BIND_VO == pObj->enType))
        {
            if (pstSrcFrame->stVFrame.u32TimeRef < pChn->u32TimeRef)
            {
                PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv %d, TimeRef err, (%d,%d)\n",
                pcivChn, pstSrcFrame->stVFrame.u32TimeRef, pChn->u32TimeRef);
                return HI_FAILURE;
            }
            pChn->u32TimeRef = pstSrcFrame->stVFrame.u32TimeRef;
        }
    }

    /* 如果需要在输入源上叠加OSD，流程为: 搬移 -> 叠OSD -> 缩放 */
    if ((PCIV_BIND_VI == pObj->enType) || (PCIV_BIND_VO == pObj->enType))
    {
        PCIV_FMW_TRACE(HI_DBG_INFO, "Pciv channel %d support osd right now\n", pcivChn);
        s32Ret = PcivFmwSrcPicMove(pcivChn, pObj, pstSrcFrame, pstMixCapState);
    }
    else if (PCIV_BIND_VDEC == pObj->enType)
    {
        s32Ret = PcivFmwSrcPicZoom(pcivChn, pObj, pstSrcFrame, pstMixCapState);
    }
    else
    {
        PCIV_FMW_TRACE(HI_DBG_ERR, "Pciv channel %d not support type:%d\n", pcivChn, pObj->enType);
    }

    return s32Ret;
}

/* be called in VIU interrupt handler or VDEC interrupt handler. */
HI_S32 PCIV_FirmWareSrcPicSend(PCIV_CHN pcivChn, PCIV_BIND_OBJ_S *pObj, const VIDEO_FRAME_INFO_S *pPicInfo, VI_MIXCAP_STAT_S *pstMixCapState)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    HI_BOOL bWaitDsu;

    bWaitDsu = HI_FALSE;

    pChn = &g_stFwmPcivChn[pcivChn];

    /* PCIV通道必须已经启动通道 */
    if (pChn->bStart != HI_TRUE)
    {
        return HI_FAILURE;
    }
    pChn->u32GetCnt++;

    /* 执行前处理并发送图像 */
    if (PcivFirmWareSrcPreProc(pcivChn, pObj, pPicInfo, pstMixCapState))
    {
        pChn->u32LostCnt++;
        return HI_FAILURE;
    }

    if (PCIV_BIND_VDEC == pObj->enType)
    {
        pChn->enSendState = PCIVFMW_SEND_ING;
    }
    bWaitDsu = HI_TRUE;

    return (HI_TRUE == bWaitDsu) ? HI_SUCCESS : HI_FAILURE;
}

/* be called in VIU interrupt handler */
HI_S32 PCIV_FirmWareViuSendPic(VI_DEV viDev, VI_CHN viChn,
        const VIDEO_FRAME_INFO_S *pPicInfo, const VI_MIXCAP_STAT_S *pstMixStat)
{
    return HI_SUCCESS;
}

/*
** timer function of Sender
** 1. Get data from vdec and send to another chip through pci
*/
HI_VOID PCIV_VdecTimerFunc(unsigned long data)
{
    HI_S32 s32Ret;
    PCIV_CHN pcivChn;
    PCIV_FWMCHANNEL_S *pChn;
    VIDEO_FRAME_INFO_S *pstVFrameInfo;
    PCIV_BIND_OBJ_S Obj = {0};
    unsigned long flags;
    VI_MIXCAP_STAT_S  stMixCapState;

    /* timer will be restarted after 1 tick */
    g_timerVdecSend.expires  = jiffies + 1;
    g_timerVdecSend.function = PCIV_VdecTimerFunc;
    g_timerVdecSend.data     = 0;
    if (!timer_pending(&g_timerVdecSend))
        add_timer(&g_timerVdecSend);

    for (pcivChn=0; pcivChn<PCIVFMW_MAX_CHN_NUM; pcivChn++)
	{
        PCIVFMW_SPIN_LOCK;
        pChn = &g_stFwmPcivChn[pcivChn];
        
        pChn->u32TimerCnt++;
        
        if (HI_FALSE == pChn->bStart)
        {
            PCIVFMW_SPIN_UNLOCK;
            continue;
        }

        if (PCIVFMW_SEND_ING == pChn->enSendState)
        {
            PCIVFMW_SPIN_UNLOCK;
            continue;
        }
        /* 检查上次是否发送成功(第一次为发送成功状态) */
        else if (PCIVFMW_SEND_OK == pChn->enSendState)
        {
            /* 从队列中获取新的解码图像数据信息  */
            pChn->pCurVdecNode = PCIV_PicQueueGetBusy(&pChn->stPicQueue);
            if (NULL == pChn->pCurVdecNode)
            {
                PCIV_FMW_TRACE(HI_DBG_INFO, "pcivChn:%d no vdec pic\n",pcivChn);
                PCIVFMW_SPIN_UNLOCK;
                continue;
            }
        }
        else if (PCIVFMW_SEND_NOK == pChn->enSendState)
        {
            /* 上次没有发送成功，要使用上次的数据进行重发 */
            if (pChn->pCurVdecNode == NULL)
            {
                PCIVFMW_SPIN_UNLOCK;
                continue;
            }
        }
        else
        {
            printk("pcivChn %d send state error %#x\n",pcivChn,pChn->enSendState);
            PCIVFMW_SPIN_UNLOCK;
            continue;
        }

        pstVFrameInfo = &pChn->pCurVdecNode->stPcivPic.stVideoFrame;
        stMixCapState.bHasDownScale = HI_FALSE;
        stMixCapState.bMixCapMode   = HI_FALSE;
        /* 发送解码图像到PCI对端 */
        Obj.enType = PCIV_BIND_VDEC;
        s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, pstVFrameInfo, &stMixCapState);
        if (s32Ret != HI_SUCCESS)/* 如果发送失败，下一次使用备份数据 */
        {
            pChn->enSendState = PCIVFMW_SEND_NOK;
        }
        PCIVFMW_SPIN_UNLOCK;
	}

	return;
}


/* Called in VIU, virtual VOU or VDEC interrupt handler */
HI_S32 PCIV_FirmWareSendPic(HI_S32 s32DevId, HI_S32 s32ChnId, MPP_DATA_TYPE_E enDataType, void *pPicInfo)

{
    HI_S32 s32Ret;
    PCIV_CHN pcivChn = s32ChnId;
    unsigned long flags;
    PCIV_BIND_OBJ_S Obj = {0};
    VI_FRAME_INFO_S    *pstVifInfo   = NULL;
    VIDEO_FRAME_INFO_S *pstVofInfo   = NULL;
    VIDEO_FRAME_INFO_S *pstVdecfInfo = NULL;
        
    PCIVFMW_CHECK_CHNID(pcivChn);
    PCIVFMW_CHECK_PTR(pPicInfo);
    
    if (MPP_DATA_VIU_FRAME == enDataType)
    {                
        Obj.enType = PCIV_BIND_VI;
        pstVifInfo = (VI_FRAME_INFO_S *)pPicInfo;
        
        PCIV_FWMCHANNEL_S *pChn = &g_stFwmPcivChn[s32ChnId];

        RGN_INFO_S        stRgnInfo;
        PcivFmwGetRegion(pcivChn, OVERLAYEX_RGN, &stRgnInfo);

//        printk("pciv rcv u32TimeRef=%u\n",pstVifInfo->stViFrmInfo.stVFrame.u32TimeRef);
        PCIVFMW_SPIN_LOCK;
#if 0
        s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, &(pstVifInfo->stViFrmInfo), &(pstVifInfo->stMixCapState));
#else
        if (pChn->stPicAttr.u32Width  != pstVifInfo->stViFrmInfo.stVFrame.u32Width
         || pChn->stPicAttr.u32Height != pstVifInfo->stViFrmInfo.stVFrame.u32Height
         || (stRgnInfo.u32Num > 0))
        {
            /* the pic size is not the same or it's needed to add osd */
            s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, &(pstVifInfo->stViFrmInfo), &(pstVifInfo->stMixCapState));
        }
        else
        {
//            printk("send through\n");
            s32Ret = PcivFmwSrcPicSend(pcivChn, &Obj, &pstVifInfo->stViFrmInfo, NULL, &(pstVifInfo->stMixCapState));
        }
#endif
        PCIVFMW_SPIN_UNLOCK;
        if (HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d viu frame pic send failed\n",s32DevId);
            return HI_FAILURE;
        }
    }
    else if (MPP_DATA_VOU_FRAME == enDataType)
    {
        VI_MIXCAP_STAT_S stMixCapState;
        Obj.enType = PCIV_BIND_VO;
        pstVofInfo = (VIDEO_FRAME_INFO_S *)pPicInfo;
        stMixCapState.bHasDownScale    = HI_FALSE;
        stMixCapState.bMixCapMode      = HI_FALSE;
        
        PCIVFMW_SPIN_LOCK;
        s32Ret = PCIV_FirmWareSrcPicSend(pcivChn, &Obj, pstVofInfo, &stMixCapState);
        PCIVFMW_SPIN_UNLOCK;
        if (HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d virtual vou frame pic send failed\n",s32DevId);
            return HI_FAILURE;
        }
    }
    else if (MPP_DATA_VDEC_FRAME == enDataType)
    {
        PCIV_PIC_NODE_S *pNode = NULL;
        PCIV_FWMCHANNEL_S *pChn = NULL;

        pstVdecfInfo = (VIDEO_FRAME_INFO_S *)pPicInfo;
        
        PCIVFMW_SPIN_LOCK;
        pChn = &g_stFwmPcivChn[pcivChn];
        pNode = PCIV_PicQueueGetFree(&pChn->stPicQueue);
        if (NULL == pNode)
        {
            PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d no free node\n",s32DevId);
            PCIVFMW_SPIN_UNLOCK;
            return HI_FAILURE;
        }

        pNode->stPcivPic.enModId = HI_ID_VDEC;
        
        memcpy(&pNode->stPcivPic.stVideoFrame, pstVdecfInfo, sizeof(VIDEO_FRAME_INFO_S));
        VB_UserAdd(pstVdecfInfo->u32PoolId, pstVdecfInfo->stVFrame.u32PhyAddr[0], VB_UID_PCIV);

        PCIV_PicQueuePutBusy(&pChn->stPicQueue, pNode);
        PCIVFMW_SPIN_UNLOCK;
    }
    else
    {
        PCIV_FMW_TRACE(HI_DBG_DEBUG, "pcivChn:%d data type:%d error\n",s32DevId,enDataType);
        return HI_FAILURE;
    }
    return HI_SUCCESS;
}

HI_S32 PcivFmw_VpssQuery(HI_S32 s32DevId, HI_S32 s32ChnId, VPSS_QUERY_INFO_S *pstQueryInfo, VPSS_INST_INFO_S *pstInstInfo)
{
    HI_U32 u32PicSize;
    VB_BLKHANDLE handle;
    MOD_ID_E enModId;
    VIDEO_FRAME_S *pstVFrame;
    HI_U32   u32PicWidth;
    HI_U32   u32PicHeight;
    HI_U32   u32Stride;
    HI_U32   u32CurTimeRef;
    HI_S32   s32Ret;
    MPP_CHN_S  stChn ={0};
    HI_CHAR *pMmzName = NULL;
    unsigned long flags;

    PCIV_FWMCHANNEL_S   *pChn = NULL;
    PCIV_CHN PcivChn = s32ChnId;

    PCIVFMW_CHECK_CHNID(PcivChn);
    PCIVFMW_CHECK_PTR(pstQueryInfo);
    PCIVFMW_CHECK_PTR(pstInstInfo);

    pChn = &g_stFwmPcivChn[PcivChn];
    if (HI_FALSE == pChn->bCreate)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel doesn't exist, please create it!\n");
        return HI_FAILURE;
	}

	if (HI_FALSE == pChn->bStart)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel has stoped!\n");
        return HI_FAILURE;
	}

    if (NULL == pstQueryInfo->pstSrcPicInfo)
    {
		PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn:%d SrcPicInfo is NULL!\n",PcivChn);
        goto OLD_UNDO;
    }

    enModId = pstQueryInfo->pstSrcPicInfo->enModId;
    u32CurTimeRef = pstQueryInfo->pstSrcPicInfo->stVideoFrame.stVFrame.u32TimeRef;
    
    if (((HI_ID_VIU == enModId) || (HI_ID_VOU == enModId)) && (pChn->u32TimeRef == u32CurTimeRef))
    {
        //重复帧 不接收图像
		PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn:%d duplicated frame!\n",PcivChn);
        goto OLD_UNDO;
    }

    if (HI_ID_VDEC == enModId)
    {
        //重复帧 不接收图像
        s32Ret = g_stPcivFmwCallBack.pfQueryPcivChnShareBufState(PcivChn);
        if (s32Ret != HI_SUCCESS)
        {
		    PCIV_FMW_TRACE(HI_DBG_INFO, "pciv chn:%d has no free share buf to receive pic!\n",PcivChn);
            goto OLD_UNDO;
        }
    }

    if (HI_TRUE == pstQueryInfo->bScaleCap)
    {
        u32PicWidth       = pChn->stPicAttr.u32Width;
        u32PicHeight      = pChn->stPicAttr.u32Height;

    }
    else
    {
        u32PicWidth  = pstQueryInfo->pstSrcPicInfo->stVideoFrame.stVFrame.u32Width;
        u32PicHeight = pstQueryInfo->pstSrcPicInfo->stVideoFrame.stVFrame.u32Height;

    }

	if (HI_FALSE == pstQueryInfo->bMalloc)
	{
		/* bypass */
        /* if picture from VIU or USER, send to PCIV Firmware directly */
        if ((HI_ID_VIU == enModId) || (HI_ID_USR == enModId))
        {
		    pstInstInfo->bNew = HI_TRUE;
		    pstInstInfo->bVpss = HI_TRUE;
        }
        /* if picture from VEDC, send to PCIV queue and then deal with DSU, check the queue is full or not  */
        else if (HI_ID_VDEC == enModId)
        {
            PCIVFMW_SPIN_LOCK;
            if (0 == PCIV_PicQueueGetFreeNum(&(pChn->stPicQueue)))
            {
                /* if PCIV queue is full, old undo */
                PCIV_FMW_TRACE(HI_DBG_ERR, "pciv chn:%d queue is full!\n",PcivChn);
                PCIVFMW_SPIN_UNLOCK;
                goto OLD_UNDO;
            }
            PCIVFMW_SPIN_UNLOCK;
            
            pstInstInfo->bNew = HI_TRUE;
		    pstInstInfo->bVpss = HI_TRUE;
        }
	}
	else
	{
        u32Stride = pChn->stPicAttr.u32Stride[0];

        if (PIXEL_FORMAT_YUV_SEMIPLANAR_420 == pChn->stPicAttr.enPixelFormat)
        {
            u32PicSize = u32Stride * u32PicHeight * 3 / 2;
        }
        else if (PIXEL_FORMAT_YUV_SEMIPLANAR_422 == pChn->stPicAttr.enPixelFormat)
        {
            u32PicSize = u32Stride * u32PicHeight * 2;
        }
        else
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "[PcivChn:%d]enPixelFormat(%d) not support\n",PcivChn,pChn->stPicAttr.enPixelFormat);
            goto OLD_UNDO;
        }

        stChn.enModId = HI_ID_PCIV;
        stChn.s32DevId = s32DevId;
        stChn.s32ChnId = s32ChnId;
        s32Ret = CALL_SYS_GetMmzName(&stChn, (void**)&pMmzName);
        HI_ASSERT(HI_SUCCESS == s32Ret);

        handle = VB_GetBlkBySize(u32PicSize, VB_UID_VPSS, pMmzName);
        if (VB_INVALID_HANDLE == handle)
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "get VB for u32PicSize: %d from ddr:%s fail,for grp %d VPSS Query\n",u32PicSize,pMmzName,s32DevId);
            goto OLD_UNDO;
        }

        pstInstInfo->astDestPicInfo[0].stVideoFrame.u32PoolId = VB_Handle2PoolId(handle);

        pstVFrame = &pstInstInfo->astDestPicInfo[0].stVideoFrame.stVFrame;
        pstVFrame->u32Width      = u32PicWidth;
        pstVFrame->u32Height     = u32PicHeight;
        pstVFrame->u32PhyAddr[0] = VB_Handle2Phys(handle);
        pstVFrame->u32PhyAddr[1] = pstVFrame->u32PhyAddr[0] + u32PicHeight*u32Stride;
        pstVFrame->pVirAddr[0] 	 = (void *)VB_Handle2Kern(handle);
        pstVFrame->pVirAddr[1] 	 = (void *)((HI_U32)pstVFrame->pVirAddr[0] + u32PicHeight*u32Stride);
        pstVFrame->u32Stride[0]  = u32Stride;
        pstVFrame->u32Stride[1]  = u32Stride;
        pstVFrame->enPixelFormat = pChn->stPicAttr.enPixelFormat;

        pstInstInfo->bVpss = HI_TRUE;
        pstInstInfo->bNew  = HI_TRUE;
	}

    pChn->u32NewDoCnt++;
	return HI_SUCCESS;

OLD_UNDO:
    pstInstInfo->bVpss   = HI_FALSE;
    pstInstInfo->bNew    = HI_FALSE;
    pstInstInfo->bDouble = HI_FALSE;
    pstInstInfo->bUpdate = HI_FALSE;
    pChn->u32OldUndoCnt++;
	return HI_SUCCESS;
}


HI_S32 PcivFmw_VpssSend(HI_S32 s32DevId, HI_S32 s32ChnId, VPSS_SEND_INFO_S *pstSendInfo)
{
    PCIV_FWMCHANNEL_S *pChn = NULL;
    MOD_ID_E          enModId;
    HI_S32            s32Ret;
    PCIV_BIND_OBJ_S   BindObj;
    unsigned long     flags;
    RGN_INFO_S        stRgnInfo;
    VI_MIXCAP_STAT_S  stMixCapState;
    PCIV_CHN PcivChn  = s32ChnId;

    PCIVFMW_CHECK_CHNID(PcivChn);
    PCIVFMW_CHECK_PTR(pstSendInfo);
    PCIVFMW_CHECK_PTR(pstSendInfo->pstDestPicInfo[0]);

    pChn = &g_stFwmPcivChn[PcivChn];
    if (HI_FALSE == pChn->bCreate)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel doesn't exist, please create it!\n");
        return HI_FAILURE;
	}

	if (HI_FALSE == pChn->bStart)
	{
		PCIV_FMW_TRACE(HI_DBG_ALERT, "pciv channel has stoped!\n");
        return HI_FAILURE;
	}

    if (HI_FALSE == pstSendInfo->bSuc)
    {
        printk("==send====line %d.=====bsuc is failure.==\n", __LINE__);
        return HI_FAILURE;
    }

    enModId = pstSendInfo->pstDestPicInfo[0]->enModId;

    if (HI_ID_VDEC == enModId)
    {
        BindObj.enType = PCIV_BIND_VDEC;
	}
    else if (HI_ID_VOU == enModId)
    {
        BindObj.enType = PCIV_BIND_VO;
    }
    else
    {
        BindObj.enType = PCIV_BIND_VI;
    }

    stMixCapState.bHasDownScale = HI_FALSE;
    stMixCapState.bMixCapMode   = HI_FALSE;

    s32Ret = PcivFmwGetRegion(PcivChn, OVERLAYEX_RGN, &stRgnInfo);
    
    /* use which flag to know VPSS is bypass or not */
    if (pChn->stPicAttr.u32Width  != pstSendInfo->pstDestPicInfo[0]->stVideoFrame.stVFrame.u32Width
     || pChn->stPicAttr.u32Height != pstSendInfo->pstDestPicInfo[0]->stVideoFrame.stVFrame.u32Height
     || (stRgnInfo.u32Num > 0))
	{
		/* bypass */

        PcivFmwPutRegion(PcivChn, OVERLAYEX_RGN);
        PCIVFMW_SPIN_LOCK;
        s32Ret = PCIV_FirmWareSrcPicSend(PcivChn, &BindObj, &(pstSendInfo->pstDestPicInfo[0]->stVideoFrame), &stMixCapState);
        if (HI_SUCCESS != s32Ret)
        {
            PCIV_FMW_TRACE(HI_DBG_ALERT, "PCIV_FirmWareSrcPicSend failed! pciv chn %d\n", PcivChn);
            PCIVFMW_SPIN_UNLOCK;
            return HI_FAILURE;
        }
        PCIVFMW_SPIN_UNLOCK;
	}
	else
	{
        pChn->u32GetCnt++;
        PCIVFMW_SPIN_LOCK;
        s32Ret = PcivFmwSrcPicSend(PcivChn, &BindObj, &(pstSendInfo->pstDestPicInfo[0]->stVideoFrame), NULL, &stMixCapState);
        if (HI_SUCCESS != s32Ret)
        {
            pChn->u32LostCnt++;
            PCIV_FMW_TRACE(HI_DBG_ALERT, "PcivFmwSrcPicSend failed! pciv chn %d.\n", PcivChn);
            PCIVFMW_SPIN_UNLOCK;
            return HI_FAILURE;
        }
        PCIVFMW_SPIN_UNLOCK;
	}
 
	return HI_SUCCESS;
}

HI_S32 PcivFmw_VpssResetCallBack(HI_S32 s32DevId, HI_S32 s32ChnId, HI_VOID *pvData)
{
    PCIV_CHN PcivChn = s32ChnId;
    PCIV_FWMCHANNEL_S *pChn = NULL;
    VPSS_SEND_INFO_S *pstSendInfo = NULL;

    pChn = &g_stFwmPcivChn[PcivChn];

    pstSendInfo = (VPSS_SEND_INFO_S*)pvData;
    if (HI_ID_VDEC == pstSendInfo->pstDestPicInfo[0]->enModId)
    {
	    pChn->stPicQueue.u32FreeNum = VDEC_MAX_SEND_CNT;
    	pChn->stPicQueue.u32Max     = VDEC_MAX_SEND_CNT;
    	pChn->stPicQueue.u32BusyNum = 0;
    }

    return HI_SUCCESS;
}


HI_S32 PCIV_FirmWareRegisterFunc(PCIVFMW_CALLBACK_S *pstCallBack)
{
    PCIVFMW_CHECK_PTR(pstCallBack);
    PCIVFMW_CHECK_PTR(pstCallBack->pfSrcSendPic);
    PCIVFMW_CHECK_PTR(pstCallBack->pfPicFreeFromVo);

    memcpy(&g_stPcivFmwCallBack, pstCallBack, sizeof(PCIVFMW_CALLBACK_S));

    return HI_SUCCESS;
}


HI_S32 PCIV_FmwInit(void *p)
{
    HI_S32 i, s32Ret;
    BIND_SENDER_INFO_S stSenderInfo;
    BIND_RECEIVER_INFO_S stReceiver;
    VPSS_REGISTER_INFO_S stVpssRgstInfo;
    RGN_REGISTER_INFO_S stRgnInfo;
    
    if (PCIVFMW_STATE_STARTED == s_u32PcivFmwState)
    {
        return HI_SUCCESS;
    }

    spin_lock_init(&g_PcivFmwLock);

    stSenderInfo.enModId      = HI_ID_PCIV;
    stSenderInfo.u32MaxDevCnt = 1;
    stSenderInfo.u32MaxChnCnt = PCIV_MAX_CHN_NUM;
    s32Ret = CALL_SYS_RegisterSender(&stSenderInfo);
    HI_ASSERT(HI_SUCCESS == s32Ret);

    stReceiver.enModId      = HI_ID_PCIV;
    stReceiver.u32MaxDevCnt = 1;
    stReceiver.u32MaxChnCnt = PCIV_MAX_CHN_NUM;
    stReceiver.pCallBack = PCIV_FirmWareSendPic;
    s32Ret = CALL_SYS_RegisterReceiver(&stReceiver);
    HI_ASSERT(HI_SUCCESS == s32Ret);

	init_timer(&g_timerVdecSend);
    g_timerVdecSend.expires  = jiffies + 3;
    g_timerVdecSend.function = PCIV_VdecTimerFunc;
    g_timerVdecSend.data     = 0;
    add_timer(&g_timerVdecSend);

    g_stVbPool.u32PoolCount = 0;

    memset(g_stFwmPcivChn, 0, sizeof(g_stFwmPcivChn));
    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
    {
        g_stFwmPcivChn[i].bStart     = HI_FALSE;
        g_stFwmPcivChn[i].u32SendCnt = 0;
        g_stFwmPcivChn[i].u32GetCnt  = 0;
        g_stFwmPcivChn[i].u32RespCnt = 0;
        g_stFwmPcivChn[i].u32Count   = 0;
        g_stFwmPcivChn[i].u32RgnNum  = 0;
        init_timer(&g_stFwmPcivChn[i].stBufTimer);
        PCIV_FirmWareInitPreProcCfg(i);
    }

    if ((CKFN_VPSS_ENTRY()) && (CKFN_VPSS_Register()))
    {
        stVpssRgstInfo.enModId        = HI_ID_PCIV;
        stVpssRgstInfo.pVpssQuery     = PcivFmw_VpssQuery;
        stVpssRgstInfo.pVpssSend      = PcivFmw_VpssSend;
        stVpssRgstInfo.pResetCallBack = PcivFmw_VpssResetCallBack;
        s32Ret = CALL_VPSS_Register(&stVpssRgstInfo);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    /* 向region模块注册OVERLAY */
    if ((CKFN_RGN()) && (CKFN_RGN_RegisterMod()))
    {
        stRgnInfo.enModId      = HI_ID_PCIV;
        stRgnInfo.u32MaxDevCnt = 1;
        stRgnInfo.u32MaxChnCnt = PCIV_MAX_CHN_NUM;
        
        /* 注册OVERLAY */
        s32Ret = CALL_RGN_RegisterMod(OVERLAYEX_RGN, &stRgnInfo);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }
    
    s_u32PcivFmwState = PCIVFMW_STATE_STARTED;
	return HI_SUCCESS;
}


HI_VOID PCIV_FmwExit(HI_VOID)
{
    HI_S32 i, s32Ret;

    if (PCIVFMW_STATE_STOPED == s_u32PcivFmwState)
    {
        return;
    }

    CALL_SYS_UnRegisterReceiver(HI_ID_PCIV);
    CALL_SYS_UnRegisterSender(HI_ID_PCIV);

    for (i=0; i<PCIVFMW_MAX_CHN_NUM; i++)
    {
        /* 停止通道 */
        PCIV_FirmWareStop(i);

        /* 销毁通道 */
        PCIV_FirmWareDestroy(i);

        del_timer_sync(&g_stFwmPcivChn[i].stBufTimer);
    }

	del_timer_sync(&g_timerVdecSend);

    if ((CKFN_VPSS_ENTRY()) && (CKFN_VPSS_UnRegister()))
    {
        HI_ASSERT(HI_SUCCESS == CALL_VPSS_UnRegister(HI_ID_PCIV));
    }

    if ((CKFN_RGN()) && (CKFN_RGN_UnRegisterMod()))
    {
        /* 反注册OVERLAY */
        s32Ret = CALL_RGN_UnRegisterMod(OVERLAYEX_RGN, HI_ID_PCIV);
        HI_ASSERT(HI_SUCCESS == s32Ret);
    }

    s_u32PcivFmwState = PCIVFMW_STATE_STOPED;
    return;
}

static HI_VOID PCIV_FmwNotify(MOD_NOTICE_ID_E enNotice)
{
    return;
}

static HI_VOID PCIV_FmwQueryState(MOD_STATE_E *pstState)
{
    *pstState = MOD_STATE_FREE;
    return;
}

HI_S32 PCIV_FmwProcShow(struct seq_file *s, HI_VOID *pArg)
{
    PCIV_FWMCHANNEL_S  *pstChnCtx;
    PCIV_CHN         pcivChn;
    HI_CHAR *pszPcivFieldStr[] = {"top","bottom","both"}; /* PCIV_PIC_FIELD_E */

    seq_printf(s, "\n[PCIVF] Version: ["MPP_VERSION"], Build Time:["__DATE__", "__TIME__"]\n");

    seq_printf(s, "\n-----PCIV FMW CHN INFO----------------------------------------------------------\n");
    seq_printf(s, "%6s"     "%8s"   "%8s"    "%8s"     "%8s"    "%8s"     "%8s"     "%8s"     "%8s"   "%8s"     "%8s\n" ,
                  "PciChn", "Width","Height","Stride0","GetCnt","SendCnt","RespCnt","LostCnt","NewDo","OldUndo","PoolId0");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        seq_printf(s, "%6d" "%8d" "%8d" "%8d" "%8d" "%8d" "%8d" "%8d" "%8d" "%8d" "%8d\n",
            pcivChn,
            pstChnCtx->stPicAttr.u32Width,
            pstChnCtx->stPicAttr.u32Height,
            pstChnCtx->stPicAttr.u32Stride[0],
            pstChnCtx->u32GetCnt,
            pstChnCtx->u32SendCnt,
            pstChnCtx->u32RespCnt,
            pstChnCtx->u32LostCnt,
            pstChnCtx->u32NewDoCnt,
            pstChnCtx->u32OldUndoCnt,
            pstChnCtx->au32PoolId[0]);
    }

    seq_printf(s, "\n-----PCIV FMW CHN PREPROC INFO--------------------------------------------------\n");
    seq_printf(s, "%6s"     "%8s"   "%8s"   "%8s"    "%8s"    "%8s"  "%8s\n"
                 ,"PciChn", "FiltT","HFilt","VFiltC","VFiltL","bDFL","Field");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        seq_printf(s, "%6d" "%8d" "%8d" "%8d" "%8d" "%8d" "%8s\n",
            pcivChn,
            pstChnCtx->stPreProcCfg.enFilterType,
            pstChnCtx->stPreProcCfg.enHFilter,
            pstChnCtx->stPreProcCfg.enVFilterC,
            pstChnCtx->stPreProcCfg.enVFilterL,
            pstChnCtx->stDsuOpt.bDeflicker,
            pszPcivFieldStr[pstChnCtx->stPreProcCfg.enFieldSel]);
    }

    seq_printf(s, "\n-----PCIV CHN QUEUE INFO----------------------------------------------------------\n");
    seq_printf(s, "%6s"     "%8s"     "%8s"     "%8s"    "%8s\n",
                  "PciChn", "busynum","freenum","state", "Timer");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        seq_printf(s, "%6d" "%8d" "%8d" "%8d" "%8d\n",
            pcivChn,
            pstChnCtx->stPicQueue.u32BusyNum,
            pstChnCtx->stPicQueue.u32FreeNum,
            pstChnCtx->enSendState,
            pstChnCtx->u32TimerCnt);
    }

    seq_printf(s, "\n-----PCIV CHN CALL DSU INFO----------------------------------------------------------\n");
    seq_printf(s, "%6s"     "%8s"     "%10s"     "%8s"    "%8s"      "%8s"      "%10s"       "%8s"     "%8s"      "%8s"      "%10s"       "%8s"     "%8s"   "%8s\n",
                  "PciChn", "JobSuc","JobFail","EndSuc", "EndFail", "MoveSuc", "MoveFail", "OsdSuc", "OsdFail", "ZoomSuc", "ZoomFail", "MoveCb", "OsdCb","ZoomCb");
    for (pcivChn = 0; pcivChn < PCIVFMW_MAX_CHN_NUM; pcivChn++)
    {
        pstChnCtx = &g_stFwmPcivChn[pcivChn];
        if (HI_FALSE == pstChnCtx->bCreate) continue;
        seq_printf(s, "%6d" "%8d" "%8d" "%8d" "%8d" "%8d" "%10d" "%8d" "%8d" "%8d" "%8d" "%10d" "%8d" "%8d\n",
            pcivChn,
            pstChnCtx->u32AddJobSucCnt,
            pstChnCtx->u32AddJobFailCnt,
            pstChnCtx->u32EndJobSucCnt,
            pstChnCtx->u32EndJobFailCnt,
            pstChnCtx->u32MoveTaskSucCnt,
            pstChnCtx->u32MoveTaskFailCnt,
            pstChnCtx->u32OsdTaskSucCnt,
            pstChnCtx->u32OsdTaskFailCnt,
            pstChnCtx->u32ZoomTaskSucCnt,
            pstChnCtx->u32ZoomTaskFailCnt,
            pstChnCtx->u32MoveCbCnt,
            pstChnCtx->u32OsdCbCnt,
            pstChnCtx->u32ZoomCbCnt);
    }

    return HI_SUCCESS;
}

static PCIV_FMWEXPORT_FUNC_S s_stExportFuncs = {
    .pfnPcivSendPic = PCIV_FirmWareViuSendPic,
};

static HI_U32 PCIV_GetVerMagic(HI_VOID)
{
	return VERSION_MAGIC;
}

static UMAP_MODULE_S s_stPcivFmwModule = {
    .pstOwner = THIS_MODULE,
    .enModId  = HI_ID_PCIVFMW,

    .pfnInit        = PCIV_FmwInit,
    .pfnExit        = PCIV_FmwExit,
    .pfnQueryState  = PCIV_FmwQueryState,
    .pfnNotify      = PCIV_FmwNotify,
    .pfnVerChecker  = PCIV_GetVerMagic,
    
    .pstExportFuncs = &s_stExportFuncs,
    .pData          = HI_NULL,
};

static int __init PCIV_FmwModInit(void)
{
    CMPI_PROC_ITEM_S *item;

    item = CMPI_CreateProc(PROC_ENTRY_PCIVFMW, PCIV_FmwProcShow, NULL);
    if (! item)
    {
        printk("PCIV create proc error\n");
        return HI_FAILURE;
    }

	if (CMPI_RegisterMod(&s_stPcivFmwModule))
    {
        printk("PCIV register module error\n");
        CMPI_RemoveProc(PROC_ENTRY_PCIVFMW);
        return HI_FAILURE;
    }

    return HI_SUCCESS;
}

static void __exit PCIV_FmwModExit(void)
{
	CMPI_RemoveProc(PROC_ENTRY_PCIVFMW);
    CMPI_UnRegisterMod(HI_ID_PCIVFMW);
    return ;
}

EXPORT_SYMBOL(PCIV_FirmWareCreate);
EXPORT_SYMBOL(PCIV_FirmWareDestroy);
EXPORT_SYMBOL(PCIV_FirmWareSetAttr);
EXPORT_SYMBOL(PCIV_FirmWareStart);
EXPORT_SYMBOL(PCIV_FirmWareStop);

EXPORT_SYMBOL(PCIV_FirmWareWindowVbCreate);
EXPORT_SYMBOL(PCIV_FirmWareWindowVbDestroy);
EXPORT_SYMBOL(PCIV_FirmWareMalloc);
EXPORT_SYMBOL(PCIV_FirmWareFree);
EXPORT_SYMBOL(PCIV_FirmWarePicVoShow);
EXPORT_SYMBOL(PCIV_FirmWareSrcPicFree);
EXPORT_SYMBOL(PCIV_FirmWareRegisterFunc);

EXPORT_SYMBOL(PCIV_FirmWareSetPreProcCfg);
EXPORT_SYMBOL(PCIV_FirmWareGetPreProcCfg);

module_param(drop_err_timeref, uint, S_IRUGO);
MODULE_PARM_DESC(drop_err_timeref, "whether drop err timeref video frame. 1:yes,0:no.");

module_init(PCIV_FmwModInit);
module_exit(PCIV_FmwModExit);

MODULE_AUTHOR("Hi3531 MPP GRP");
MODULE_LICENSE("Proprietary");
MODULE_VERSION(MPP_VERSION);

