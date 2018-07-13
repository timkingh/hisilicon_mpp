/******************************************************************************

  Copyright (C), 2001-2055, Hisilicon Tech. Co., Ltd.

 ******************************************************************************
  File Name     : dsu_ext.h
  Version       : Initial Draft
  Author        : Hisilicon multimedia software group
  Created       : 2009/02/01
  Description   :
  History       :
  1.Date        : 2009/02/01
    Author      : w54723
    Modification: Created file
******************************************************************************/

#ifndef __DSU_EXT_H__
#define __DSU_EXT_H__

#include "hi_comm_video.h"
#include "hi_errno.h"

#define HI_TRACE_DSU(level, fmt...)	\
do{\
	HI_TRACE(level, HI_ID_DSU, "[%s]: %d,", __FUNCTION__, __LINE__);\
	HI_TRACE(level, HI_ID_DSU, ##fmt);\
} while(0);

#define DSU_INVALD_JOB_HDL          (-1UL) /*invalid job handle*/

#define DSU_MAX_JOB_NUM             128  /*max job number*/
/*choose scale filter, 
frequency more less, picture may be more obscurer,
Notes:225M means 2.25M*/
typedef enum hiDSU_SCALE_FILTER_E
{
    DSU_SCALE_FILTER_DEFAULT,
    DSU_SCALE_FILTER_2M,
    DSU_SCALE_FILTER_225M,
    DSU_SCALE_FILTER_25M,
    DSU_SCALE_FILTER_275M,
    DSU_SCALE_FILTER_3M,
    DSU_SCALE_FILTER_325M,
    DSU_SCALE_FILTER_35M,
    DSU_SCALE_FILTER_375M,
    DSU_SCALE_FILTER_4M,
    DSU_SCALE_FILTER_5M,
    DSU_SCALE_FILTER_6M,
    DSU_SCALE_FILTER_65M,
    
    DSU_SCALE_FILTER_DEFAULT_EX,
    DSU_SCALE_FILTER_3M_EX,
    DSU_SCALE_FILTER_325M_EX,
    DSU_SCALE_FILTER_5M_EX,
    DSU_SCALE_FILTER_BUTT,
}DSU_SCALE_FILTER_E;

/*choose denoise arg*/
typedef enum hiDSU_DENOISE_E
{
    DSU_DENOISE_ONLYEDAGE,      /* only edge improve, no denoise*/
    DSU_DENOISE_LOWNOISE,       /* low denoise*/
    DSU_DENOISE_MIDNOISE,       /* mid denoise*/
    DSU_DENOISE_HIGHNOISE,      /* high denoise*/
    DSU_DENOISE_VERYHIGHNOISE,  /* very high denoise*/
    DSU_DENOISE_BUTT,
}DSU_DENOISE_E;

/*choose color entrans arg*/
typedef enum hiDSU_CE_E
{
    DSU_CE_DISABLE,
    DSU_CE_ENABLE,
    DSU_CE_BUTT,
}DSU_CE_E;

/*choose luma strenth arg*/
typedef enum hiDSU_LUMA_STR_E
{
    DSU_LUMA_STR_DISABLE,
    DSU_LUMA_STR_ENABLE,
    DSU_LUMA_STR_BUTT,
}DSU_LUMA_STR_E;

/*define DSU struct here*/
/*choose using scale or denoise function*/
typedef enum hiDSU_TASK_CHOICE_E
{
    DSU_TASK_SCALE = 0,    /*普通缩放；如果输入输出图像大小相同，仅搬移*/
    DSU_TASK_DENOISE,  /*空域去噪,hi3520不支持*/
    DSU_TASK_BUTT,
}DSU_TASK_CHOICE_E;

/*task完成状态*/
typedef enum hiDSU_TASK_FNSH_STAT_E
{
    DSU_TASK_FNSH_STAT_OK = 1,   /*task已正确完成*/
    DSU_TASK_FNSH_STAT_FAIL = 2, /*task执行出现异常或未完成*/
    DSU_TASK_FNSH_STAT_BUTT
}DSU_TASK_FNSH_STAT_E;

/*DSU操作数据信息结构，包括输入、输出图像信息及调用者，回调函数信息。
  DSU任务完成后，每个操作以回调函数形式通知用户，该结构作为回调函数参数
  返回该操作对应的数据信息
*/
typedef struct hiDSU_TASK_DATA_S
{
    VIDEO_FRAME_INFO_S      stImgIn;        /* input picture */
    VIDEO_FRAME_INFO_S      stImgOut;       /* output picture */
    HI_U32                  privateData;    /* task's private data */
    void             (*pCallBack)(const struct hiDSU_TASK_DATA_S *pTask); /* callback */
    MOD_ID_E	            enCallModId;    /* module ID */
    HI_U32                  u32DsuChnId;    /*chnnal ID */
    HI_U32 		            u32InField;     /* field flag */
    HI_U32                  reserved;       /* save current picture's state while debug */
    HI_U64                  u64Pts;         /* save current picture's pts while debug */
    DSU_TASK_FNSH_STAT_E    enFnshStat;     /* output param:task finish status(ok or nok)*/
}DSU_TASK_DATA_S;

/*DSU任务的优先级级别*/
typedef enum hiDSU_JOB_PRI_E
{
    DSU_JOB_PRI_HIGH = 0,      /*高优先级*/
    DSU_JOB_PRI_NORMAL =1,     /*中优先级*/
    DSU_JOB_PRI_LOW =2,        /*低优先级*/
    DSU_JOB_PRI_BUTT
}DSU_JOB_PRI_E;

/*job完成状态*/
typedef enum hiDSU_JOB_FNSH_STAT_E
{
    DSU_JOB_FNSH_STAT_OK = 1,   /*JOB已正确完成(表示其下的所有task已正常完成)*/
    DSU_JOB_FNSH_STAT_FAIL = 2, /*JOB执行出现异常或未完成*/
    DSU_JOB_FNSH_STAT_BUTT
}DSU_JOB_FNSH_STAT_E;

/*DSU任务信息结构，一个dsu job完成后通知用户时返回给用户的信息*/
typedef struct hiDSU_JOB_DATA_S
{
    HI_U32                jobPrivateData;    /* job级的用户私有数据 */
    DSU_JOB_FNSH_STAT_E   enJobFnshStat;     /* output param:job finish status(ok or nok)*/
    void             (*pJobCallBack)(struct hiDSU_JOB_DATA_S *pJobData); /* callback */
}DSU_JOB_DATA_S;

/*指定双源叠加操作的前景位图的信息 */
typedef struct hiDSU_BITMAP_S
{
    /* 位图首地址 */
    HI_U32 u32PhyAddr;

    /* 颜色格式 */
    PIXEL_FORMAT_E enPixelFormat;

    /* 位图高度 */
    HI_U32 u32Height;

    /* 位图宽度 */
    HI_U32 u32Width;

    /* 位图跨度 */
    HI_U32 u32Stride;

    /*如果是ARGB1555格式 可指定alpha通道的是否扩展及Alpha0、Alpha1值 */
    HI_BOOL bAlphaExt1555; /* 是否使能1555的Alpha扩展 */
    HI_U8 u8Alpha0;        /* 扩展alpha通道值为0的像素的alpha值为u8Alpha0*/
    HI_U8 u8Alpha1;        /* 扩展alpha通道值为1的像素的alpha值为u8Alpha1*/
} DSU_BITMAP_S;

/*指定双源叠加操作的输出图像的镜像操作方式*/
typedef enum hiDSU_MIRROR_E
{
    DSU_MIRROR_NONE = 0,        /*不进行镜像操作*/
    DSU_MIRROR_HORIZONTAL,      /*水平镜像：即图像左右翻转*/
    DSU_MIRROR_VERTICAL,        /*垂直镜像：即图像左右翻转*/
    DSU_MIRROR_BOTH,            /*同时水平和垂直镜像*/
    DSU_MIRROR_BUTT
} DSU_MIRROR_E;

/*指定双源叠加操作的输出图像的Clip操作方式*/
typedef enum hiDSU_CLIPMODE_E
{
    DSU_CLIPMODE_NONE = 0,     /*不进行clip操作*/
    DSU_CLIPMODE_INSIDE,       /*区域内作clip：仅更新指定区域内的像素*/
    DSU_CLIPMODE_OUTSIDE,      /*区域内作clip：仅更新指定区域外的像素*/
    DSU_CLIPMODE_BUTT
} DSU_CLIPMODE_E;

/*双源运算的操作结构*/
typedef struct hiDSU_DUAL_OPT_S
{
    /*是否进行alpha叠加*/
    //HI_BOOL bAlphaBlend;

    /*全局Alpha值,在bAlphaBlend==true时有效*/
    HI_U8   u8GlobalAlpha;

    /*输出clip模式*/
    DSU_CLIPMODE_E enClipMode;

    /*clip区域定义,在enClipMode==INSIDE
     或enClipMode==OUTSIDE时有效*/
    RECT_S stClipRect;

    /*镜像类型*/
    //DSU_MIRROR_E enMirror;
} DSU_DUAL_OPT_S;

/*dsu 缩放策略*/
typedef enum hiDSU_SCALE_STRATEGY_E
{
	DSU_SCALE_STRATEGY_NORM = 0,
	DSU_SCALE_STRATEGY_2LEVEL,
	DSU_SCALE_STRATEGY_WBC,
	DSU_SCALE_STRATEGY_BUTT
}DSU_SCALE_STRATEGY_E;

/*单源运算的操作结构*/
typedef struct hiDSU_ODD_OPT_S
{
    DSU_TASK_CHOICE_E       enChoice;        /* scale or denoise switch */
    HI_BOOL                 bForceHFilt;     /* 是否强制水平滤波，当输入输出图像
                                                水平方向大小相同时可设置强制水平滤波*/
    HI_BOOL                 bForceVFilt;     /* 是否强制垂直滤波，当输入输出图像
                                                垂直方向大小相同时可设置强制垂直滤波*/												
    DSU_FILTER_PARAM_TYPE    enFilterType;   /* filter param type*/
    DSU_HSCALE_FILTER_E      enHFilter;      /* horizontal filter */
    DSU_VSCALE_FILTER_E      enVFilterL;     /* vertical filter of luminance */
    DSU_VSCALE_FILTER_E      enVFilterC;     /* vertical filter of chroma */
    HI_BOOL                 bDeflicker      /*是否抗闪烁*/;
    DSU_DENOISE_E           enDnoise;       /* denoise switch,hi3520不支持 */
    DSU_CE_E                enCE;           /* chroma enhancement，hi3520不支持 */
    DSU_LUMA_STR_E          enLumaStr;      /* contrast stretch，hi3520不支持 */
    DSU_SCALE_STRATEGY_E 	enScaleStrategy; /* 缩放策略*/
}DSU_ODD_OPT_S;  

typedef HI_S32  DSU_HANDLE;
typedef HI_S32 	FN_DSU_Init(HI_U32 tasknum);
typedef HI_VOID  FN_DSU_Exit(HI_VOID);
typedef HI_S32   FN_DSU_CreateTask(DSU_TASK_DATA_S *pTask,DSU_ODD_OPT_S *pstOpt,
                                       DSU_JOB_PRI_E enPriLevel);
typedef HI_S32   FN_DSU_CreateBypassTask(DSU_TASK_DATA_S *pTask,
                                       DSU_JOB_PRI_E enPriLevel);

typedef HI_S32   FN_DSU_BeginJob(DSU_HANDLE *phHandle,
                                     DSU_JOB_PRI_E enPriLevel,
                                     MOD_ID_E	enCallModId,
                                     HI_U32 u32DsuChnId,
                                     DSU_JOB_DATA_S *pJobData);
typedef HI_S32   FN_DSU_AddODDScTask(DSU_HANDLE hHandle,
                                               DSU_TASK_DATA_S *pTask,
                                               DSU_ODD_OPT_S *pstOpt);
typedef HI_S32   FN_DSU_AddDualScTask(DSU_HANDLE hHandle,
                                            DSU_TASK_DATA_S *pTask,
                                            DSU_BITMAP_S* pstForeGround,
                                            RECT_S  *pstDstRect,
                                            DSU_DUAL_OPT_S* pstOpt);
typedef HI_S32   FN_DSU_AddFillTask(DSU_HANDLE hHandle,
                                               DSU_TASK_DATA_S *pTask,
                                               RECT_S *pstDstRect,
                                               HI_U32 u32FillData);
typedef HI_S32   FN_DSU_AddBypassTask(DSU_HANDLE hHandle,
                                               DSU_TASK_DATA_S *pTask);

typedef HI_S32 FN_DSU_CancleJob(DSU_HANDLE hHandle);

typedef HI_S32 FN_DSU_EndJob(DSU_HANDLE hHandle);

typedef HI_S32 FN_DSU_SetFilterParam(DSU_FILTER_PARAM_S *pstDsuFiltParam); 

typedef HI_S32 FN_DSU_GetFilterParam(DSU_FILTER_PARAM_S *pstDsuFiltParam); 

typedef HI_S32 FN_DSU_AddRotateTask(DSU_HANDLE hHandle, DSU_TASK_DATA_S *pTask, ROTATE_E enRotateAngle); 

typedef HI_S32 FN_DSU_AddLDCTask(DSU_HANDLE hHandle, DSU_TASK_DATA_S *pTask, LDC_ATTR_S *pstAttr);

typedef struct hiDSU_EXPORT_FUNC_S
{
    FN_DSU_Init *pfnInit;
    FN_DSU_Exit *pfnExit;
    FN_DSU_CreateTask *pfnDsuCreateTask;
    FN_DSU_CreateBypassTask *pfnDsuCreateBypassTask;
    FN_DSU_BeginJob   *pfnDsuBeginJob;
    FN_DSU_CancleJob  *pfnDsuCancleJob;
    FN_DSU_EndJob     *pfnDsuEndJob;
    FN_DSU_AddODDScTask *pfnDsuAddOddScTask;
    FN_DSU_AddDualScTask *pfnDsuAddDualScTask;
    FN_DSU_AddFillTask *pfnDsuFillTask;
    FN_DSU_AddBypassTask *pfnDsuBypassTask;
    FN_DSU_SetFilterParam *pfnDsuSetFilterParam;
    FN_DSU_GetFilterParam *pfnDsuGetFilterParam;
    FN_DSU_AddRotateTask *pfnDsuAddRotateTask;
    FN_DSU_AddLDCTask *pfnDsuAddLDCTask;
}DSU_EXPORT_FUNC_S;

/* failure caused by malloc buffer */
#define HI_ERR_DSU_NOBUF             HI_DEF_ERR(HI_ID_DSU, EN_ERR_LEVEL_ERROR, EN_ERR_NOBUF)

#define HI_ERR_DSU_BUF_EMPTY       HI_DEF_ERR(HI_ID_DSU, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_EMPTY)
#define HI_ERR_DSU_NULL_PTR        HI_DEF_ERR(HI_ID_DSU, EN_ERR_LEVEL_ERROR, EN_ERR_NULL_PTR)
#define HI_ERR_DSU_ILLEGAL_PARAM   HI_DEF_ERR(HI_ID_DSU, EN_ERR_LEVEL_ERROR, EN_ERR_ILLEGAL_PARAM)
#define HI_ERR_DSU_BUF_FULL        HI_DEF_ERR(HI_ID_DSU, EN_ERR_LEVEL_ERROR, EN_ERR_BUF_FULL)
#define HI_ERR_DSU_SYS_NOTREADY    HI_DEF_ERR(HI_ID_DSU, EN_ERR_LEVEL_ERROR, EN_ERR_SYS_NOTREADY)
#define HI_ERR_DSU_NOT_SUPPORT     HI_DEF_ERR(HI_ID_DSU, EN_ERR_LEVEL_ERROR, EN_ERR_NOT_SUPPORT)

#endif /* __DSU_H__ */

