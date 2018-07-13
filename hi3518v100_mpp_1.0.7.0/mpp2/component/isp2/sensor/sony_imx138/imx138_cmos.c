#if !defined(__IMX138_CMOS_H_)
#define __IMX138_CMOS_H_

#include <stdio.h>
#include <string.h>
#include "hi_comm_sns.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"
#include "mpi_ae.h"
#include "mpi_awb.h"
#include "mpi_af.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

#define EXPOSURE_ADDR (0x220) //2:chip_id, 0C: reg addr.

#define PGC_ADDR (0x214)
#define VMAX_ADDR (0x218)

#define IMX138_ID 138

/****************************************************************************
 * local variables                                                            *
 ****************************************************************************/

HI_U32 gu32FullLinesStd = 750;
HI_U32 gu32FullLines = 750;
HI_U8 gu8SensorMode = 0;

static AWB_CCM_S g_stAwbCcm =
{
    5048,
    {
        0x01f2, 0x80bd, 0x8035,
        0x8042, 0x01a3, 0x8061,
        0x0026, 0x80e0, 0x1b9,
    },
    3200,
    {
        0x01dd, 0x807b, 0x8062,
        0x807b, 0x01d2, 0x8057,
        0x0036, 0x8110, 0x01d9,
    },
    2480,
    {
        0x01b1, 0x8066, 0x804b,
        0x8074, 0x01b2, 0x803e,
        0x001e, 0x81d9, 0x02ba,
    }
};

static AWB_AGC_TABLE_S g_stAwbAgcTable =
{
    /* bvalid */
    1,

    /* saturation */
    {0x80,0x80,0x6C,0x48,0x44,0x40,0x3C,0x38}
};

static ISP_CMOS_AGC_TABLE_S g_stIspAgcTable =
{
    /* bvalid */
    1,

    /* sharpen_alt_d */
    {80,80,80,71,71,62,59,27},
        
    /* sharpen_alt_ud */
    {53,53,53,49,49,44,39,18},
        
    /* snr_thresh */
    {20,25,30,35,39,44,49,0x64},
        
    /* demosaic_lum_thresh */
    {0x60,0x60,0x80,0x80,0x80,0x80,0x80,0x80},
        
    /* demosaic_np_offset */
    {0,0xa,16,24,32,40,48,56},
        
    /* ge_strength */
    {0x55,0x55,0x55,0x55,0x55,0x55,0x37,0x37},
};

static ISP_CMOS_NOISE_TABLE_S g_stIspNoiseTable =
{
    /* bvalid */
    1,
    
    /* nosie_profile_weight_lut */
    {
    0,0,0,0,0,0,0,15,25,29,31,33,32,35,36,37,
    37,38,39,39,40,40,40,41,41,41,42,42,42,43,43,43,
    43,44,44,44,44,44,45,45,45,45,45,45,45,46,46,46,
    46,46,46,47,47,47,47,47,47,47,47,48,48,48,48,48,
    48,48,48,48,49,49,49,49,49,49,49,49,49,49,49,49,
    50,50,50,50,50,50,50,50,50,50,50,50,50,50,51,51,
    51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,52,
    52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52
    },

    /* demosaic_weight_lut */
    {
    0,15,25,29,31,33,34,35,36,37,37,38,39,39,40,40,
    40,41,41,41,42,42,42,43,43,43,43,44,44,44,44,44,
    45,45,45,45,45,45,46,46,46,46,46,46,46,47,47,47,
    47,47,47,47,47,48,48,48,48,48,48,48,48,48,49,49,
    49,49,49,49,49,49,49,49,49,49,49,50,50,50,50,50,
    50,50,50,50,50,50,50,50,51,51,51,51,51,51,51,51,
    51,51,51,51,51,51,51,51,52,52,52,52,52,52,52,52,
    52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52
    }
};

static ISP_CMOS_DEMOSAIC_S g_stIspDemosaic =
{
    /* bvalid */
    1,
    
    /*vh_slope*/
    0xfa,

    /*aa_slope*/
    0xfa,

    /*va_slope*/
    0xfa,

    /*uu_slope*/
    0xfa,

    /*sat_slope*/
    0x5d,

    /*ac_slope*/
    0xcf,

    /*vh_thresh*/
    0x32,

    /*aa_thresh*/
    0x3c,

    /*va_thresh*/
    0x3c,

    /*uu_thresh*/
    0x3c,

    /*sat_thresh*/
    0x171,

    /*ac_thresh*/
    0x1b3,
};

HI_U32 cmos_get_isp_default(ISP_CMOS_DEFAULT_S *pstDef)
{
    if (HI_NULL == pstDef)
    {
        printf("null pointer when get isp default value!\n");
        return -1;
    }

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));

    pstDef->stComm.u8Rggb           = 0x2;      //2: gbrg 
    pstDef->stComm.u8BalanceFe      = 0x1;

    pstDef->stDenoise.u8SinterThresh= 0x8;
    pstDef->stDenoise.u8NoiseProfile= 0x1;      //0: use default profile table; 1: use calibrated profile lut, the setting for nr0 and nr1 must be correct.
    pstDef->stDenoise.u16Nr0        = 0x0;
    pstDef->stDenoise.u16Nr1        = 455;

    pstDef->stDrc.u8DrcBlack        = 0x00;
    pstDef->stDrc.u8DrcVs           = 0x04;     // variance space
    pstDef->stDrc.u8DrcVi           = 0x08;     // variance intensity
    pstDef->stDrc.u8DrcSm           = 0xa0;     // slope max
    pstDef->stDrc.u16DrcWl          = 0x8ff;    // white level

    memcpy(&pstDef->stNoiseTbl, &g_stIspNoiseTable, sizeof(ISP_CMOS_NOISE_TABLE_S));            
    memcpy(&pstDef->stAgcTbl, &g_stIspAgcTable, sizeof(ISP_CMOS_AGC_TABLE_S));
    memcpy(&pstDef->stDemosaic, &g_stIspDemosaic, sizeof(ISP_CMOS_DEMOSAIC_S));

    return 0;
}

HI_U32 cmos_get_isp_black_level(ISP_CMOS_BLACK_LEVEL_S *pstBlackLevel)
{
    HI_S32  i;
    
    if (HI_NULL == pstBlackLevel)
    {
        printf("null pointer when get isp black level value!\n");
        return -1;
    }

    /* Don't need to update black level when iso change */
    pstBlackLevel->bUpdate = HI_FALSE;
            
    for (i=0; i<4; i++)
    {
        pstBlackLevel->au16BlackLevel[i] = 0xF0;
    }

    return 0;    
}

HI_VOID cmos_set_pixel_detect(HI_BOOL bEnable)
{
    if (bEnable) /* setup for ISP pixel calibration mode */
    {
        /* Sensor must be programmed for slow frame rate (5 fps and below)*/
        /* change frame rate to 5 fps by setting 1 frame length = 750 * 30 / 5 */
        sensor_write_register(VMAX_ADDR, 0x94);
        sensor_write_register(VMAX_ADDR + 1, 0x11);

        /* max Exposure time */
		sensor_write_register(EXPOSURE_ADDR, 0x00);
		sensor_write_register(EXPOSURE_ADDR + 1, 0x00);

        /* Analog and Digital gains both must be programmed for their minimum values */
		sensor_write_register(PGC_ADDR, 0x00);
    }
    else /* setup for ISP 'normal mode' */
    {
        sensor_write_register(VMAX_ADDR, 0xEE);
        sensor_write_register(VMAX_ADDR + 1, 0x02);
    }
    
    return;
}

HI_VOID cmos_set_wdr_mode(HI_U8 u8Mode)
{
    switch(u8Mode)
    {
        //sensor mode 0
        case 0:
            gu8SensorMode = 0;
            // TODO:
        break;
        //sensor mode 1
        case 1:
            gu8SensorMode = 1;
             // TODO:
        break;

        default:
            printf("NOT support this mode!\n");
            return;
        break;
    }
    
    return;
}

static HI_S32 cmos_get_ae_default(AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    if (HI_NULL == pstAeSnsDft)
    {
        printf("null pointer when get ae default value!\n");
        return -1;
    }
    
    pstAeSnsDft->au8HistThresh[0] = 0xd;
    pstAeSnsDft->au8HistThresh[1] = 0x28;
    pstAeSnsDft->au8HistThresh[2] = 0x60;
    pstAeSnsDft->au8HistThresh[3] = 0x80;
    
    pstAeSnsDft->u8AeCompensation = 0x80;
    
    pstAeSnsDft->u32LinesPer500ms = 750*30/2;
    pstAeSnsDft->u32FlickerFreq = 0;//60*256;//50*256;

    gu32FullLinesStd = 750;

    pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
    pstAeSnsDft->u32MaxIntTime = 748;
    pstAeSnsDft->u32MinIntTime = 2;    
    pstAeSnsDft->u32MaxIntTimeTarget = 65535;
    pstAeSnsDft->u32MinIntTimeTarget = 2;

    pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_DB;
    pstAeSnsDft->stAgainAccu.f32Accuracy = 0.3;    
    pstAeSnsDft->u32MaxAgain = 80;  /* 24db / 0.3db = 80 */
    pstAeSnsDft->u32MinAgain = 0;
    pstAeSnsDft->u32MaxAgainTarget = 80;
    pstAeSnsDft->u32MinAgainTarget = 0;

    pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_DB;
    pstAeSnsDft->stDgainAccu.f32Accuracy = 0.3;    
    pstAeSnsDft->u32MaxDgain = 80;  /* 24db / 0.3db = 80 */
    pstAeSnsDft->u32MinDgain = 0;
    pstAeSnsDft->u32MaxDgainTarget = 80;
    pstAeSnsDft->u32MinDgainTarget = 0;

    pstAeSnsDft->u32ISPDgainShift = 4;
    pstAeSnsDft->u32MaxISPDgainTarget = 4 << pstAeSnsDft->u32ISPDgainShift;
    pstAeSnsDft->u32MinISPDgainTarget = 1 << pstAeSnsDft->u32ISPDgainShift;

    return 0;
}

/* the function of sensor set fps */
static HI_VOID cmos_fps_set(HI_U8 u8Fps, AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    switch(u8Fps)
    {
        case 30:
            // Change the frame rate via changing the vertical blanking
            gu32FullLinesStd = 750;
			pstAeSnsDft->u32MaxIntTime = 747;
            pstAeSnsDft->u32LinesPer500ms = 750 * 30 / 2;
			sensor_write_register(VMAX_ADDR, 0xEE);
			sensor_write_register(VMAX_ADDR+1, 0x02);
        break;
        
        case 25:
            // Change the frame rate via changing the vertical blanking
            gu32FullLinesStd = 900;
            pstAeSnsDft->u32MaxIntTime = 897;
            pstAeSnsDft->u32LinesPer500ms = 900 * 25 / 2;
			sensor_write_register(VMAX_ADDR, 0x84);
			sensor_write_register(VMAX_ADDR+1, 0x03);
        break;
        
        default:
        break;
    }

    return;
}

static HI_VOID cmos_slow_framerate_set(HI_U8 u8SlowFrameRate,
    AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    gu32FullLines = (gu32FullLinesStd * u8SlowFrameRate) >> 4;

	sensor_write_register(VMAX_ADDR, (gu32FullLines & 0x00ff));
	sensor_write_register(VMAX_ADDR+1, ((gu32FullLines & 0xff00) >> 8));
    
    pstAeSnsDft->u32MaxIntTime = gu32FullLines - 3;
    
    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static HI_VOID cmos_inttime_update(HI_U32 u32IntTime)
{
    HI_U32 u32Value = gu32FullLines - u32IntTime;
    
    sensor_write_register(EXPOSURE_ADDR, u32Value & 0xFF);
    sensor_write_register(EXPOSURE_ADDR + 1, (u32Value & 0xFF00) >> 8);
    
    return;
}

static HI_VOID cmos_gains_update(HI_U32 u32Again, HI_U32 u32Dgain)
{
    HI_U32 u32Tmp = u32Again + u32Dgain;

    u32Tmp = u32Tmp > 0xA0 ? 0xA0 : u32Tmp;
    
    sensor_write_register(PGC_ADDR, u32Tmp);

    return;
}

static HI_S32 cmos_get_awb_default(AWB_SENSOR_DEFAULT_S *pstAwbSnsDft)
{
    if (HI_NULL == pstAwbSnsDft)
    {
        printf("null pointer when get awb default value!\n");
        return -1;
    }

    memset(pstAwbSnsDft, 0, sizeof(AWB_SENSOR_DEFAULT_S));

    pstAwbSnsDft->u16WbRefTemp = 5048;

    pstAwbSnsDft->au16GainOffset[0] = 0x1ff;
    pstAwbSnsDft->au16GainOffset[1] = 0x100;
    pstAwbSnsDft->au16GainOffset[2] = 0x100;
    pstAwbSnsDft->au16GainOffset[3] = 0x1e4;

    pstAwbSnsDft->as32WbPara[0] = 40;
    pstAwbSnsDft->as32WbPara[1] = 106;
    pstAwbSnsDft->as32WbPara[2] = -110;
    pstAwbSnsDft->as32WbPara[3] = 184787;
    pstAwbSnsDft->as32WbPara[4] = 128;
    pstAwbSnsDft->as32WbPara[5] = -134867;

    memcpy(&pstAwbSnsDft->stCcm, &g_stAwbCcm, sizeof(AWB_CCM_S));
    memcpy(&pstAwbSnsDft->stAgcTbl, &g_stAwbAgcTable, sizeof(AWB_AGC_TABLE_S));
    
    return 0;
}

/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/
HI_S32 cmos_init_sensor_exp_function(ISP_SENSOR_EXP_FUNC_S *pstSensorExpFunc)
{
    memset(pstSensorExpFunc, 0, sizeof(ISP_SENSOR_EXP_FUNC_S));

    pstSensorExpFunc->pfn_cmos_sensor_init = sensor_init;
    pstSensorExpFunc->pfn_cmos_get_isp_default = cmos_get_isp_default;
    pstSensorExpFunc->pfn_cmos_get_isp_black_level = cmos_get_isp_black_level;
    pstSensorExpFunc->pfn_cmos_set_pixel_detect = cmos_set_pixel_detect;
    pstSensorExpFunc->pfn_cmos_set_wdr_mode = cmos_set_wdr_mode;

    return 0;
}

HI_S32 cmos_init_ae_exp_function(AE_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    memset(pstExpFuncs, 0, sizeof(AE_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_ae_default    = cmos_get_ae_default;
    pstExpFuncs->pfn_cmos_fps_set           = cmos_fps_set;
    pstExpFuncs->pfn_cmos_slow_framerate_set= cmos_slow_framerate_set;    
    pstExpFuncs->pfn_cmos_inttime_update    = cmos_inttime_update;
    pstExpFuncs->pfn_cmos_gains_update      = cmos_gains_update;

    return 0;
}

HI_S32 cmos_init_awb_exp_function(AWB_SENSOR_EXP_FUNC_S *pstExpFuncs)
{
    memset(pstExpFuncs, 0, sizeof(AWB_SENSOR_EXP_FUNC_S));

    pstExpFuncs->pfn_cmos_get_awb_default = cmos_get_awb_default;

    return 0;
}

int sensor_register_callback(void)
{
    HI_S32 s32Ret;
    ALG_LIB_S stLib;
    ISP_SENSOR_REGISTER_S stIspRegister;
    AE_SENSOR_REGISTER_S  stAeRegister;
    AWB_SENSOR_REGISTER_S stAwbRegister;

    cmos_init_sensor_exp_function(&stIspRegister.stSnsExp);
    s32Ret = HI_MPI_ISP_SensorRegCallBack(IMX138_ID, &stIspRegister);
    if (s32Ret)
    {
        printf("sensor register callback function failed!\n");
        return s32Ret;
    }
    
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AE_LIB_NAME);
    cmos_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret = HI_MPI_AE_SensorRegCallBack(&stLib, IMX138_ID, &stAeRegister);
    if (s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AWB_LIB_NAME);
    cmos_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret = HI_MPI_AWB_SensorRegCallBack(&stLib, IMX138_ID, &stAwbRegister);
    if (s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }
    
    return 0;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */

#endif // __IMX104_CMOS_H_
