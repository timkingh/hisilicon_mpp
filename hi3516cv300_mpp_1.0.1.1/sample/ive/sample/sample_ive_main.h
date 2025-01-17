#ifndef __SAMPLE_IVE_MAIN_H__
#define __SAMPLE_IVE_MAIN_H__
#include "hi_type.h"
/******************************************************************************
* function : show Canny sample
******************************************************************************/
HI_VOID SAMPLE_IVE_Canny(HI_CHAR chComplete);
/******************************************************************************
* function : show BgModel sample
******************************************************************************/
HI_VOID SAMPLE_IVE_BgModel(HI_CHAR chEncode, HI_CHAR chVo);
/******************************************************************************
* function : show Gmm sample
******************************************************************************/
HI_VOID SAMPLE_IVE_Gmm(HI_CHAR chEncode, HI_CHAR chVo);
/******************************************************************************
* function : show Gmm2 sample
******************************************************************************/
HI_VOID SAMPLE_IVE_Gmm2(HI_VOID);

/******************************************************************************
* function : show Occlusion detected sample
******************************************************************************/
HI_VOID SAMPLE_IVE_Od(HI_VOID);
/******************************************************************************
* function : show Md sample
******************************************************************************/
HI_VOID SAMPLE_IVE_Md(HI_VOID);
/******************************************************************************
* function : show Test Memory sample
******************************************************************************/
HI_VOID SAMPLE_IVE_TestMemory(HI_VOID);
/******************************************************************************
* function : show Sobel sample
******************************************************************************/
HI_VOID SAMPLE_IVE_Sobel(HI_VOID);

/******************************************************************************
* function :Canny sample signal handle
******************************************************************************/
HI_VOID SAMPLE_IVE_Canny_HandleSig(HI_VOID);
/******************************************************************************
* function : BgModel sample signal handle
******************************************************************************/
HI_VOID SAMPLE_IVE_BgModel_HandleSig(HI_VOID);
/******************************************************************************
* function : Gmm sample signal handle
******************************************************************************/
HI_VOID SAMPLE_IVE_Gmm_HandleSig(HI_VOID);
/******************************************************************************
* function : Gmm2 sample signal handle
******************************************************************************/
HI_VOID SAMPLE_IVE_Gmm2_HandleSig(HI_VOID);
/******************************************************************************
* function : Od sample signal handle
******************************************************************************/
HI_VOID SAMPLE_IVE_Od_HandleSig(HI_VOID);
/******************************************************************************
* function : Md sample signal handle
******************************************************************************/
HI_VOID SAMPLE_IVE_Md_HandleSig(HI_VOID);
/******************************************************************************
* function : TestMemory sample signal handle
******************************************************************************/
HI_VOID SAMPLE_IVE_TestMemory_HandleSig(HI_VOID);
/******************************************************************************
* function : Sobel sample signal handle
******************************************************************************/
HI_VOID SAMPLE_IVE_Sobel_HandleSig(HI_VOID);

#endif

