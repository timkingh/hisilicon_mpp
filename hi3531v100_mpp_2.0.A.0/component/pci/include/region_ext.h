/******************************************************************************

                  版权所有 (C), 2001-2011, 华为技术有限公司

 ******************************************************************************
  文 件 名   : region_ext.h
  版 本 号   : 初稿
  作    者   : l64467
  生成日期   : 2010年12月18日
  最近修改   :
  功能描述   : 
  函数列表   :
  修改历史   :
  1.日    期   : 2010年12月18日
    作    者   : l64467
    修改内容   : 创建文件
  
******************************************************************************/
#ifndef __REGION_EXT_H__
#define __REGION_EXT_H__

#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_region.h"

#ifdef __cplusplus
    #if __cplusplus
    extern "C"{
    #endif
#endif /* End of #ifdef __cplusplus */



typedef struct hiRGN_COMM_S
{
    HI_BOOL bShow;           /* 区域是否显示*/
    POINT_S stPoint;         /*区域起始位置*/
    SIZE_S  stSize;          /*区域起始宽高*/
    HI_U32  u32Layer;        /*区域层次*/
    HI_U32  u32BgColor;      /*区域背景色*/
    HI_U32  u32GlobalAlpha;  /*区域全景ALPHA*/
    HI_U32  u32FgAlpha;      /*区域前景ALPHA*/
    HI_U32  u32BgAlpha;      /*区域背景ALPHA*/
    HI_U32  u32PhyAddr;      /*区域所占有内存的物理地址*/
	HI_U32  u32VirtAddr;     /*区域占有内存的虚拟地址*/
	HI_U32  u32Stride;       /*区域内数据的 Stride*/
    PIXEL_FORMAT_E      enPixelFmt;     /*区域像素格式*/
    RGN_ATTACH_FIELD_E  enAttachField;  /* 区域附着的帧场信息 */

    OVERLAY_QP_INFO_S stQpInfo; /*QP信息*/
    
    OVERLAY_INVERT_COLOR_S stInvColInfo; /*反色信息*/
    
}RGN_COMM_S;

typedef struct hiRGN_INFO_S
{
    HI_U32 u32Num;           /* 区域个数*/
    HI_BOOL bModify;         /*是否已经修改*/
    RGN_COMM_S **ppstRgnComm;/* 区域公共信息指针数组的地址*/ 
}RGN_INFO_S;


typedef struct hiRGN_REGISTER_INFO_S
{
    MOD_ID_E    enModId;
    HI_U32      u32MaxDevCnt;   /* If no dev id, should set it 1 */
    HI_U32      u32MaxChnCnt;
} RGN_REGISTER_INFO_S;


typedef struct hiRGN_EXPORT_FUNC_S
{
    HI_S32 (*pfnRgnRegisterMod)(RGN_TYPE_E enType,const RGN_REGISTER_INFO_S *pstRgtInfo);
    HI_S32 (*pfnUnRgnRegisterMod)(RGN_TYPE_E enType,MOD_ID_E enModId);
    
    HI_S32 (*pfnRgnGetRegion)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn, RGN_INFO_S *pstRgnInfo);
    HI_S32 (*pfnRgnPutRegion)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn);
    HI_S32 (*pfnRgnSetModifyFalse)(RGN_TYPE_E enType, const MPP_CHN_S *pstChn);
}RGN_EXPORT_FUNC_S;


#define CKFN_RGN() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN))


#define CKFN_RGN_RegisterMod() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnRegisterMod)
    

#define CALL_RGN_RegisterMod(enType,pstRgtInfo) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnRegisterMod(enType,pstRgtInfo)


#define CKFN_RGN_UnRegisterMod() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnUnRgnRegisterMod)
    

#define CALL_RGN_UnRegisterMod(enType,enModId) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnUnRgnRegisterMod(enType,enModId)
    

#define CKFN_RGN_GetRegion() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnGetRegion)
    

#define CALL_RGN_GetRegion(enType,pstChn,pstRgnInfo) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnGetRegion(enType,pstChn,pstRgnInfo)
    


#define CKFN_RGN_PutRegion() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnPutRegion)
    
#define CALL_RGN_PutRegion(enType,pstChn) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnPutRegion(enType,pstChn)


#define CKFN_RGN_SetModifyFalse() \
    (NULL != FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnSetModifyFalse)
    
#define CALL_RGN_SetModifyFalse(enType,pstChn) \
    FUNC_ENTRY(RGN_EXPORT_FUNC_S, HI_ID_RGN)->pfnRgnSetModifyFalse(enType,pstChn)


#ifdef __cplusplus
    #if __cplusplus
    }
    #endif
#endif /* End of #ifdef __cplusplus */

#endif /* __REGION_EXT_H__ */



