/******************************************************************************

  Copyright (C), 2001-2011, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : mpi_pciv.c
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/06/24
  Description   :
  History       :
  1.Date        : 2009/06/24
    Author      : Z44949
    Modification: Created file
  2.Date        : 2009/10/01
    Author      : P00123320
    Modification: Add interface HI_MPI_PCIV_Show()
  3.Date        : 2010/2/24
    Author      : P00123320
    Modification: Add interfaces about set/get pre-process cfg
******************************************************************************/
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include "mpi_pciv.h"
#include "mkp_pciv.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"
{
#endif
#endif /* End of #ifdef __cplusplus */

static HI_S32 g_s32PcivFd[PCIV_MAX_CHN_NUM] = {[ 0 ... PCIV_MAX_CHN_NUM - 1 ] = -1  };

/* 如果ioctl调用因为信号被中断，则重新调用一次
** 如果用户程序中捕获了信号，但没有设置SA_RESTART属性则内核中xx_interruptible
** 系列的调用被中断后，不会再次被系统调度。因此最好在用户态再进行一次封装
*/
#define IOCTL(arg...) \
({\
    HI_S32 s32Ret=0; \
    do{\
        s32Ret = ioctl(arg);\
    }while( (-1==s32Ret) && (errno == EINTR));\
    if(-1 == s32Ret){\
        perror("Ioctl Error");\
        printf("Func:%s,Line:%d,ErrNo:0x%x\n", __FUNCTION__, __LINE__, s32Ret);\
    }\
    s32Ret;\
})

#define PCIV_CHECK_OPEN(id) \
do{ \
    PCIV_CHN chn = (id);\
    if(g_s32PcivFd[chn] < 0)\
    {\
        char name[128];\
        memset(name,0,sizeof(name));\
        sprintf(name,"/dev/pciv");\
        g_s32PcivFd[chn] = open(name, O_RDONLY);\
        if(g_s32PcivFd[chn] < 0)\
        {\
            perror("open PCIV Error");\
            return HI_ERR_PCIV_SYS_NOTREADY;\
        }\
        if(IOCTL(g_s32PcivFd[chn], PCIV_BINDCHN2FD_CTRL, &chn))\
        {\
            close(g_s32PcivFd[chn]);\
            g_s32PcivFd[chn] = -1;\
            return HI_ERR_PCIV_SYS_NOTREADY;\
        }\
    }\
}while(0)

/*****************************************************************************
 Description     : Create and initialize the pciv channel.
 Input           : pcivChn  ** The pciv channel id between [0, PCIV_MAX_CHN_NUM)
 Output          : None
 Return Value    : HI_SUCCESS
                   HI_ERR_PCIV_INVALID_CHNID
                   HI_ERR_PCIV_SYS_NOTREADY
                   HI_ERR_PCIV_EXIST
                   HI_FAILURE

 See Also        : HI_MPI_PCIV_Destroy
*****************************************************************************/
HI_S32 HI_MPI_PCIV_Create(PCIV_CHN pcivChn, PCIV_ATTR_S *pPcivAttr)
{
    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_OPEN(pcivChn);

    return IOCTL(g_s32PcivFd[pcivChn], PCIV_CREATE_CTRL, pPcivAttr);
}

/*****************************************************************************
 Description     : Destroy the pciv channel
 Input           : pcivChn  ** The pciv channel id
 Output          : None
 Return Value    : HI_SUCCESS
                   HI_ERR_PCIV_INVALID_CHNID
                   HI_ERR_PCIV_SYS_NOTREADY
                   HI_FAILURE

 See Also        : HI_MPI_PCIV_Create
*****************************************************************************/
HI_S32 HI_MPI_PCIV_Destroy(PCIV_CHN pcivChn)
{
    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_OPEN(pcivChn);

    return IOCTL(g_s32PcivFd[pcivChn], PCIV_DESTROY_CTRL);
}

/*****************************************************************************
 Description     : Set the attribute of pciv channel
 Input           : pcivChn    ** The pciv channel id
                   pPcivAttr  ** The attribute of pciv channel
 Output          : None
 Return Value    : HI_SUCCESS
                   HI_ERR_PCIV_INVALID_CHNID
                   HI_ERR_PCIV_SYS_NOTREADY
                   HI_ERR_PCIV_NULL_PTR
                   HI_ERR_PCIV_UNEXIST
                   HI_ERR_PCIV_ILLEGAL_PARAM
                   HI_FAILURE

 See Also        : HI_MPI_PCIV_GetAttr
*****************************************************************************/
HI_S32 HI_MPI_PCIV_SetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pPcivAttr)
{
    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_OPEN(pcivChn);
    PCIV_CHECK_PTR(pPcivAttr);

    return IOCTL(g_s32PcivFd[pcivChn], PCIV_SETATTR_CTRL, pPcivAttr);
}

/*****************************************************************************
 Description     : Get the attribute of pciv channel
 Input           : pcivChn    ** The pciv channel id
 Output          : pPcivAttr  ** The attribute of pciv channel
 Return Value    : HI_SUCCESS
                   HI_ERR_PCIV_INVALID_CHNID
                   HI_ERR_PCIV_SYS_NOTREADY
                   HI_ERR_PCIV_NULL_PTR
                   HI_ERR_PCIV_UNEXIST
                   HI_FAILURE

 See Also        : HI_MPI_PCIV_SetAttr
*****************************************************************************/
HI_S32 HI_MPI_PCIV_GetAttr(PCIV_CHN pcivChn, PCIV_ATTR_S *pPcivAttr)
{
    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_OPEN(pcivChn);
    PCIV_CHECK_PTR(pPcivAttr);


    return IOCTL(g_s32PcivFd[pcivChn], PCIV_GETATTR_CTRL, pPcivAttr);;
}


/*****************************************************************************
 Description     : Start to send or receive video frame
 Input           : pcivChn    ** The pciv channel id
 Output          : None
 Return Value    : HI_SUCCESS
                   HI_ERR_PCIV_INVALID_CHNID
                   HI_ERR_PCIV_SYS_NOTREADY
                   HI_ERR_PCIV_UNEXIST
                   HI_ERR_PCIV_NOT_CONFIG
                   HI_FAILURE

 See Also        : HI_MPI_PCIV_Stop
*****************************************************************************/
HI_S32 HI_MPI_PCIV_Start(PCIV_CHN pcivChn)
{
    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_OPEN(pcivChn);

    return IOCTL(g_s32PcivFd[pcivChn], PCIV_START_CTRL);;
}

/*****************************************************************************
 Description     : Stop send or receive video frame
 Input           : pcivChn    ** The pciv channel id
 Output          : None
 Return Value    : HI_SUCCESS
                   HI_ERR_PCIV_INVALID_CHNID
                   HI_ERR_PCIV_SYS_NOTREADY
                   HI_ERR_PCIV_UNEXIST
                   HI_FAILURE

 See Also        : HI_MPI_PCIV_Start
*****************************************************************************/
HI_S32 HI_MPI_PCIV_Stop(PCIV_CHN pcivChn)
{
    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_OPEN(pcivChn);

    return IOCTL(g_s32PcivFd[pcivChn], PCIV_STOP_CTRL);;
}

/*****************************************************************************
 Description     : Create a series of dma task
 Input           : pTask    ** The task list to create
 Output          : None
 Return Value    : HI_SUCCESS
                   HI_ERR_PCIV_SYS_NOTREADY
                   HI_ERR_PCIV_NULL_PTR
                   HI_ERR_PCIV_ILLEGAL_PARAM
                   HI_ERR_PCIV_NOBUF
                   HI_FAILURE

 See Also        : None
*****************************************************************************/
HI_S32  HI_MPI_PCIV_DmaTask(PCIV_DMA_TASK_S *pTask)
{
    PCIV_CHECK_OPEN(0);
    PCIV_CHECK_PTR(pTask);

    if(pTask->u32Count >= PCIV_MAX_DMABLK)
    {
        return HI_ERR_PCIV_NOBUF;
    }

    return IOCTL(g_s32PcivFd[0], PCIV_DMATASK_CTRL, pTask);
}

/*****************************************************************************
 Description     : Alloc 'u32BlkSize' bytes memory and give the physical address
                   The memory used by PCI must be located within the PCI window,
                   So you should call this function to alloc it.
 Input           : u32BlkSize    ** The size of each memory block
                   u32BlkCnt     ** The count of memory block
 Output          : u32PhyAddr    ** The physical address of the memory
 Return Value    : HI_SUCCESS
                   HI_ERR_PCIV_SYS_NOTREADY
                   HI_ERR_PCIV_NOBUF
                   HI_FAILURE

 See Also        : HI_MPI_PCIV_Free
*****************************************************************************/
HI_U32  HI_MPI_PCIV_Malloc(HI_U32 u32BlkSize, HI_U32 u32BlkCnt, HI_U32 u32PhyAddr[])
{
    HI_S32 s32Ret=HI_SUCCESS, s32Cnt=0;
    PCIV_IOCTL_MALLOC_S stMalloc;

    PCIV_CHECK_OPEN(0);

    for(s32Cnt=0; s32Cnt<u32BlkCnt; s32Cnt++)
    {
        stMalloc.u32Size    = u32BlkSize;
        stMalloc.u32PhyAddr = 0;
        s32Ret = IOCTL(g_s32PcivFd[0], PCIV_MALLOC_CTRL, &stMalloc);
        if(HI_SUCCESS != s32Ret)
        {
            break;
        }

        u32PhyAddr[s32Cnt] = stMalloc.u32PhyAddr;
    }

    /* If one block malloc failure then free all the memory */
    if(HI_SUCCESS != s32Ret)
    {
        s32Cnt--;
        for( ; s32Cnt>=0; s32Cnt--)
        {
            (HI_VOID)IOCTL(g_s32PcivFd[0], PCIV_FREE_CTRL, &u32PhyAddr[s32Cnt]);
        }
    }

    return s32Ret;
}

/*****************************************************************************
 Description     : Free the memory
 Input           : u32BlkCnt     ** The count of memory block
 Output          : u32PhyAddr    ** The physical address of the memory
 Return Value    : HI_SUCCESS
                   HI_ERR_PCIV_SYS_NOTREADY

 See Also        : HI_MPI_PCIV_Free
*****************************************************************************/
HI_S32  HI_MPI_PCIV_Free(HI_U32 u32BlkCnt, HI_U32 u32PhyAddr[])
{
    HI_U32 i;
    PCIV_CHECK_OPEN(0);

    for(i=0 ; i < u32BlkCnt; i++)
    {
        (HI_VOID)IOCTL(g_s32PcivFd[0], PCIV_FREE_CTRL, &u32PhyAddr[i]);
    }
    return HI_SUCCESS;
}


/*****************************************************************************
 Description     : Get the ID of this chip
 Input           : None
 Output          : None
 Return Value    : The chip ID if success
                   HI_FAILURE or HI_ERR_PCIV_SYS_NOTREADY  if failure

 See Also        : HI_MPI_PCIV_GetBaseWindow
*****************************************************************************/
HI_S32  HI_MPI_PCIV_GetLocalId(HI_VOID)
{
    HI_U32 s32ChipId;

    PCIV_CHECK_OPEN(0);

    if(IOCTL(g_s32PcivFd[0], PCIV_GETLOCALID_CTRL, &s32ChipId) != HI_SUCCESS)
    {
        return HI_FAILURE;
    }

    return s32ChipId;
}

/*****************************************************************************
 Description     : Enum all the connected chip.Call this function as follow.
                   {
                       HI_S32 s32ChipID[PCIV_MAX_CHIPNUM], i, s32Ret;

                       s32Ret = HI_MPI_PCIV_EnumChip(s32ChipID);
                       HI_ASSERT(HI_SUCCESS == s32Ret);

                       for(i=0; i<PCIV_MAX_CHIPNUM; i++)
                       {
                           if(s32ChipID[i] == -1) break;
                           printf("The chip%d is connected\n", s32ChipID[i]);
                       }
                       printf("Total %d chips are connected\n", i);
                   }
 Input           : s32ChipID  ** The chip id array
 Output          : None
 Return Value    : HI_SUCCESS if success.
                   HI_FAILURE if failure

 See Also        : HI_MPI_PCIV_GetLocalId
                   HI_MPI_PCIV_GetBaseWindow
*****************************************************************************/
HI_S32 HI_MPI_PCIV_EnumChip(HI_S32 s32ChipID[PCIV_MAX_CHIPNUM])
{
    HI_S32 s32Ret;
    PCIV_IOCTL_ENUMCHIP_S stChip;

    PCIV_CHECK_OPEN(0);

    s32Ret = IOCTL(g_s32PcivFd[0], PCIV_ENUMCHIPID_CTRL, &stChip);
    if(HI_SUCCESS != s32Ret)
    {
        return HI_FAILURE;
    }

    memcpy(s32ChipID, stChip.s32ChipArray, sizeof(stChip.s32ChipArray));

    return HI_SUCCESS;
}


/*****************************************************************************
 Description  : On the host, you can get all the slave chip's NP,PF and CFG window
                On the slave, you can only get the PF AHB Addres of itself.
 Input        : s32ChipId     ** The chip Id which you want to access
 Output       : pBase         ** On host  pBase->u32NpWinBase,
                                          pBase->u32PfWinBase,
                                          pBase->u32CfgWinBase
                                 On Slave pBase->u32PfAHBAddr
 Return Value : HI_SUCCESS if success.
                HI_ERR_PCIV_SYS_NOTREADY
                HI_ERR_PCIV_NULL_PTR
                HI_FAILURE

 See Also     : HI_MPI_PCIV_GetLocalId
*****************************************************************************/
HI_S32  HI_MPI_PCIV_GetBaseWindow(HI_S32 s32ChipId, PCIV_BASEWINDOW_S *pBase)
{
    PCIV_CHECK_OPEN(0);
    PCIV_CHECK_PTR(pBase);

    pBase->s32ChipId = s32ChipId;

    return IOCTL(g_s32PcivFd[0], PCIV_GETBASEWINDOW_CTRL, pBase);
}

/*****************************************************************************
 Description  : Only on the slave chip, you need to create some VB Pool.
                Those pool will bee created on the PCI Window Zone.
 Input        : pCfg.u32PoolCount ** The total number of pool want to create
                pCfg.u32BlkSize[] ** The size of each VB block
                pCfg.u32BlkCount[]** The number of each VB block

 Output       : None
 Return Value : HI_SUCCESS if success.
                HI_ERR_PCIV_SYS_NOTREADY
                HI_ERR_PCIV_NULL_PTR
                HI_ERR_PCIV_NOMEM
                HI_ERR_PCIV_BUSY
                HI_ERR_PCIV_NOT_SUPPORT
                HI_FAILURE

 See Also     : HI_MPI_PCIV_GetLocalId
*****************************************************************************/
HI_S32 HI_MPI_PCIV_WinVbCreate(PCIV_WINVBCFG_S *pCfg)
{
    PCIV_CHECK_OPEN(0);
    PCIV_CHECK_PTR(pCfg);

    return IOCTL(g_s32PcivFd[0], PCIV_WINVBCREATE_CTRL, pCfg);
}

/*****************************************************************************
 Description  : Destroy the pools which's size is equal to the pCfg.u32BlkSize[]
 Input        : pCfg.u32PoolCount ** The total number of pool want to destroy
                pCfg.u32BlkSize[] ** The size of each VB block
                pCfg.u32BlkCount[]** Don't care this parament

 Output       : None
 Return Value : HI_SUCCESS if success.
                HI_ERR_PCIV_SYS_NOTREADY
                HI_ERR_PCIV_NOT_SUPPORT
                HI_FAILURE

 See Also     : HI_MPI_PCIV_GetLocalId
*****************************************************************************/
HI_S32 HI_MPI_PCIV_WinVbDestroy(HI_VOID)
{
    PCIV_CHECK_OPEN(0);

    return IOCTL(g_s32PcivFd[0], PCIV_WINVBDESTROY_CTRL);
}

/*****************************************************************************
 Description  :
 Input        : pCfg.u32PoolCount ** The total number of pool want to destroy
                pCfg.u32BlkSize[] ** The size of each VB block
                pCfg.u32BlkCount[]** Don't care this parament

 Output       : None
 Return Value : HI_SUCCESS if success.
                HI_ERR_PCIV_SYS_NOTREADY
                HI_ERR_PCIV_NOT_SUPPORT
                HI_FAILURE
*****************************************************************************/
HI_S32 HI_MPI_PCIV_Show(PCIV_CHN pcivChn)
{
    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_OPEN(pcivChn);

    return IOCTL(g_s32PcivFd[pcivChn], PCIV_SHOW_CTRL);
}

/*****************************************************************************
 Description  :
 Input        : pCfg.u32PoolCount ** The total number of pool want to destroy
                pCfg.u32BlkSize[] ** The size of each VB block
                pCfg.u32BlkCount[]** Don't care this parament

 Output       : None
 Return Value : HI_SUCCESS if success.
                HI_ERR_PCIV_SYS_NOTREADY
                HI_ERR_PCIV_NOT_SUPPORT
                HI_FAILURE
*****************************************************************************/
HI_S32 HI_MPI_PCIV_Hide(PCIV_CHN pcivChn)
{
    PCIV_CHECK_CHNID(pcivChn);
    PCIV_CHECK_OPEN(pcivChn);

    return IOCTL(g_s32PcivFd[pcivChn], PCIV_HIDE_CTRL);
}

HI_S32 HI_MPI_PCIV_SetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pstCfg)
{
    PCIV_CHECK_OPEN(pcivChn);
    PCIV_CHECK_PTR(pstCfg);

    return IOCTL(g_s32PcivFd[pcivChn], PCIV_SETVPPCFG_CTRL, pstCfg);
}

HI_S32 HI_MPI_PCIV_GetPreProcCfg(PCIV_CHN pcivChn, PCIV_PREPROC_CFG_S *pstCfg)
{
    PCIV_CHECK_OPEN(pcivChn);
    PCIV_CHECK_PTR(pstCfg);

    return IOCTL(g_s32PcivFd[pcivChn], PCIV_GETVPPCFG_CTRL, pstCfg);
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

