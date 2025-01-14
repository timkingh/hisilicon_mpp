
#ifndef __PCIV_PIC_QUEUE_H__
#define __PCIV_PIC_QUEUE_H__ 

typedef struct HI_PCIV_PIC_INFO_S
{   
    VIDEO_FRAME_INFO_S stVideoFrame;
    MOD_ID_E        enModId;
}PCIV_PIC_INFO_S;

/*PCIV队列节点*/
typedef struct HI_PCIV_PIC_NODE_S
{
    PCIV_PIC_INFO_S  stPcivPic;      /*图像信息*/
    struct list_head stList;        
    
}PCIV_PIC_NODE_S;

/*PCIV 队列信息*/

typedef struct 
{
    PCIV_PIC_NODE_S *pstNodeBuf;    /*base address of node */
    
    struct list_head stFreeList;    
    struct list_head stBusyList;

    HI_U32           u32FreeNum;
    HI_U32           u32BusyNum;
    HI_U32           u32Max;
    
}PCIV_PIC_QUEUE_S;

HI_S32 PCIV_CreatPicQueue(PCIV_PIC_QUEUE_S *pstPicQueue, HI_U32 u32MaxNum);
HI_VOID PCIV_DestroyPicQueue(PCIV_PIC_QUEUE_S *pstNodeQueue);
HI_VOID  PCIV_PicQueuePutBusy(PCIV_PIC_QUEUE_S *pstNodeQueue, PCIV_PIC_NODE_S  *pstPicNode);
PCIV_PIC_NODE_S *PCIV_PicQueueGetBusy(PCIV_PIC_QUEUE_S *pstNodeQueue);
PCIV_PIC_NODE_S *PCIV_PicQueueQueryBusy(PCIV_PIC_QUEUE_S *pstNodeQueue);
PCIV_PIC_NODE_S *PCIV_PicQueueGetFree(PCIV_PIC_QUEUE_S *pstNodeQueue);
HI_VOID PCIV_PicQueuePutFree(PCIV_PIC_QUEUE_S *pstNodeQueue,PCIV_PIC_NODE_S  *pstPicNode);
HI_U32 PCIV_PicQueueGetFreeNum(PCIV_PIC_QUEUE_S *pstNodeQueue);
HI_U32 PCIV_PicQueueGetBusyNum(PCIV_PIC_QUEUE_S *pstNodeQueue);



#endif/*__PCIV_PIC_QUEUE_H__*/
