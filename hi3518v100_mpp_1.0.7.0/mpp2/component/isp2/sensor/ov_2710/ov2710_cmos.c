#if !defined(__OV2710_CMOS_H_)
#define __OV2710_CMOS_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hi_comm_sns.h"
#include "hi_comm_isp.h"
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

#define OV2710_ID 2710

#define CMOS_OV2710_ISP_WRITE_SENSOR_ENABLE (0)
/****************************************************************************
 * local variables                                                            *
 ****************************************************************************/

extern const unsigned int sensor_i2c_addr;
extern unsigned int sensor_addr_byte;
extern unsigned int sensor_data_byte;

HI_U8 gu8SensorMode = 0;

static HI_U32 gu32FullLinesStd = 1104;
static HI_U32 gu32FullLines = 1104;

#if CMOS_OV2710_ISP_WRITE_SENSOR_ENABLE
ISP_SNS_REGS_INFO_S g_stSnsRegsInfo = {0};
#endif

static AWB_CCM_S g_stAwbCcm =
{
    4850,
    {   0x0201, 0x80a7, 0x805a,
        0x8040, 0x019d, 0x805d,
        0x8010, 0x8150, 0x0260
    },
    3160,
    {   
        0x018e, 0x8018, 0x8076,
        0x8045, 0x0184, 0x803e,
        0x8035, 0x8134, 0x026a
    },
    2470,
    {   
        0x019d, 0x8017, 0x8086,
        0x8031, 0x017e, 0x804c,
        0x8047, 0x819b, 0x02e2     
    }
};

static AWB_AGC_TABLE_S g_stAwbAgcTable =
{
    /* bvalid */
    1,

    /* saturation */
    {0x80,0x80,0x80,0x80,0x68,0x48,0x35,0x30}
};


static ISP_CMOS_AGC_TABLE_S g_stIspAgcTable =
{
    /* bvalid */
    1,
    
    /* sharpen_alt_d */
    {0x8e,0x8b,0x88,0x83,0x7d,0x76,0x76,0x76},
        
    /* sharpen_alt_ud */
    {0x8f,0x89,0x7e,0x78,0x6f,0x3c,0x3c,0x3c},
        
    /* snr_thresh */
    {0x1e,0x1e,0x1f,0x20,0x22,0x25,0x54,0x54},
        
    /* demosaic_lum_thresh */
    {0x40,0x60,0x80,0x80,0x80,0x80,0x80,0x80},
        
    /* demosaic_np_offset */
    {0x0,0xa,0x12,0x1a,0x20,0x28,0x30,0x30},
        
    /* ge_strength */
    {0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55}

};

static ISP_CMOS_NOISE_TABLE_S g_stIspNoiseTable =
{
    /* bvalid */
    1,
    
    /* nosie_profile_weight_lut */
    {0x0,0x0,0x0,0x6,0x15,0x1b,0x20,0x23,0x25,0x27,0x29,0x2a,0x2b,0x2d,0x2e,0x2f,0x2f,
    0x30,0x31,0x32,0x32,0x33,0x34,0x34,0x35,0x35,0x36,0x36,0x37,0x37,0x38,0x38,0x39,0x39,
    0x39,0x3a,0x3a,0x3a,0x3b,0x3b,0x3b,0x3c,0x3c,0x3c,0x3c,0x3d,0x3d,0x3d,0x3e,0x3e,0x3e,
    0x3e,0x3f,0x3f,0x3f,0x3f,0x3f,0x40,0x40,0x40,0x40,0x40,0x41,0x41,0x41,0x41,0x41,0x42,
    0x42,0x42,0x42,0x42,0x42,0x43,0x43,0x43,0x43,0x43,0x43,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,
    0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
    0x48,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49},

    /* demosaic_weight_lut */
    {0x0,0x6,0x15,0x1b,0x20,0x23,0x25,0x27,0x29,0x2a,0x2b,0x2d,0x2e,0x2f,0x2f,
    0x30,0x31,0x32,0x32,0x33,0x34,0x34,0x35,0x35,0x36,0x36,0x37,0x37,0x38,0x38,0x39,0x39,
    0x39,0x3a,0x3a,0x3a,0x3b,0x3b,0x3b,0x3c,0x3c,0x3c,0x3c,0x3d,0x3d,0x3d,0x3e,0x3e,0x3e,
    0x3e,0x3f,0x3f,0x3f,0x3f,0x3f,0x40,0x40,0x40,0x40,0x40,0x41,0x41,0x41,0x41,0x41,0x42,
    0x42,0x42,0x42,0x42,0x42,0x43,0x43,0x43,0x43,0x43,0x43,0x44,0x44,0x44,0x44,0x44,0x44,
    0x44,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,
    0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
    0x48,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49}
};

static ISP_CMOS_DEMOSAIC_S g_stIspDemosaic =
{
    /* bvalid */
    1,
    
    /*vh_slope*/
    0xf4,

    /*aa_slope*/
    0xfa,

    /*va_slope*/
    0xef,

    /*uu_slope*/
    0xa0,

    /*sat_slope*/
    0x5d,

    /*ac_slope*/
    0xcf,

    /*vh_thresh*/
    0xa9,

    /*aa_thresh*/
    0x80,

    /*va_thresh*/
    0xa6,

    /*uu_thresh*/
    0xa0,

    /*sat_thresh*/
    0x171,

    /*ac_thresh*/
    0x1b3
};

static ISP_CMOS_SHADING_S g_stIspShading =
{
    /* bvalid */
    1,
    
    /*shading_center_r*/
    0x38b, 0x205,

    /*shading_center_g*/
    0x39e, 0x213,

    /*shading_center_b*/
    0x380, 0x1f9,

    /*shading_table_r*/
    {0x1000,0x1009,0x1015,0x101f,0x102d,0x103a,0x103f,0x1049,0x1051,0x105d,0x1068,0x1075,
    0x1080,0x1090,0x109d,0x10ac,0x10bd,0x10cc,0x10dc,0x10ed,0x10fe,0x1110,0x111f,0x1131,
    0x1142,0x1154,0x1162,0x1170,0x1182,0x1191,0x11a2,0x11ae,0x11bc,0x11ca,0x11db,0x11e8,
    0x11f6,0x1205,0x1215,0x1223,0x1232,0x1242,0x124f,0x125c,0x126b,0x1279,0x128b,0x1297,
    0x12a5,0x12b4,0x12c2,0x12cf,0x12df,0x12ed,0x12fd,0x1309,0x1318,0x1325,0x1334,0x1344,
    0x1351,0x135c,0x136e,0x137d,0x138c,0x139c,0x13aa,0x13bb,0x13ca,0x13d7,0x13e7,0x13f8,
    0x1409,0x141b,0x142c,0x143e,0x144e,0x1461,0x1473,0x148a,0x14a0,0x14b6,0x14cd,0x14df,
    0x14f2,0x1506,0x151d,0x1531,0x1545,0x155c,0x1572,0x1586,0x159e,0x15b1,0x15c7,0x15dc,
    0x15f0,0x1606,0x1622,0x1641,0x1659,0x1673,0x1688,0x16a1,0x16b9,0x16d7,0x16f6,0x170d,
    0x1729,0x1746,0x176e,0x1790,0x17b3,0x17e2,0x180e,0x183e,0x186b,0x1896,0x18bc,0x18f0,
    0x191b,0x194d,0x1991,0x19bf,0x1a02,0x1a3a,0x1a72,0x1abf,0x1b39},

    /*shading_table_g*/
    {0x1000,0x100c,0x101c,0x1029,0x103c,0x1041,0x1050,0x105c,0x106a,0x1077,0x1083,0x1092,
    0x10a3,0x10b2,0x10c3,0x10d4,0x10e7,0x10f7,0x110c,0x111e,0x1132,0x1145,0x1158,0x116b,
    0x117d,0x118f,0x11a3,0x11b5,0x11c7,0x11d8,0x11e7,0x11f1,0x11ff,0x120e,0x121d,0x122a,
    0x1239,0x1248,0x1257,0x1264,0x1275,0x1282,0x1292,0x12a1,0x12af,0x12bd,0x12cb,0x12d8,
    0x12e7,0x12f5,0x1304,0x1312,0x1320,0x132d,0x1339,0x1346,0x1353,0x1362,0x136e,0x137d,
    0x1389,0x1397,0x13a4,0x13b1,0x13be,0x13cd,0x13da,0x13e9,0x13f8,0x1407,0x1415,0x1426,
    0x1435,0x1443,0x1452,0x1460,0x146f,0x1480,0x1492,0x14a1,0x14b2,0x14c1,0x14d3,0x14e4,
    0x14f5,0x150b,0x1526,0x153c,0x1552,0x1567,0x1579,0x1590,0x15a4,0x15bb,0x15cf,0x15e7,
    0x15fb,0x160e,0x1628,0x164c,0x1670,0x1691,0x16ae,0x16ce,0x16eb,0x170b,0x172d,0x174e,
    0x176e,0x1792,0x17b3,0x17d6,0x17f8,0x1816,0x183d,0x185f,0x1888,0x18b2,0x18de,0x190b,
    0x193b,0x1963,0x1991,0x19cc,0x1a19,0x1a5d,0x1acc,0x1b40,0x1b67},

    /*shading_table_b*/
    {0x1000,0x100e,0x1017,0x101b,0x1026,0x102e,0x1030,0x1034,0x103b,0x1043,0x1049,0x1052,
    0x105b,0x1063,0x106e,0x107a,0x1085,0x1090,0x109f,0x10ac,0x10b9,0x10c5,0x10d3,0x10e1,
    0x10ed,0x10f9,0x1104,0x1110,0x111f,0x112e,0x113b,0x1149,0x1156,0x1162,0x116d,0x1177,
    0x1186,0x1192,0x119f,0x11a9,0x11b5,0x11c6,0x11d3,0x11dd,0x11e8,0x11f7,0x1203,0x1210,
    0x121b,0x122a,0x1235,0x1240,0x124d,0x125d,0x1267,0x1271,0x1283,0x1292,0x129d,0x12a9,
    0x12b5,0x12c5,0x12d1,0x12e1,0x12ee,0x12fb,0x1307,0x1319,0x1329,0x1338,0x1349,0x135b,
    0x136a,0x137c,0x138e,0x13a3,0x13b8,0x13cb,0x13df,0x13f0,0x1404,0x1416,0x1427,0x143a,
    0x1453,0x1462,0x1478,0x148b,0x149c,0x14ae,0x14c0,0x14d3,0x14e5,0x14f7,0x1508,0x151b,
    0x152c,0x153f,0x155a,0x1574,0x1590,0x15af,0x15cc,0x15df,0x15fa,0x1618,0x163c,0x165d,
    0x167b,0x169d,0x16cc,0x16ef,0x1718,0x1734,0x1759,0x177f,0x17a0,0x17d2,0x1805,0x1829,
    0x185e,0x187e,0x18aa,0x18ee,0x1900,0x194b,0x199a,0x19da,0x1a06},

    /*shading_off_center_r_g_b*/
    0x63f, 0x681, 0x615,

    /*shading_table_nobe_number*/
    129
};

HI_U32 cmos_get_isp_default(ISP_CMOS_DEFAULT_S *pstDef)
{
    if (HI_NULL == pstDef)
    {
        printf("null pointer when get isp default value!\n");
        return -1;
    }

    memset(pstDef, 0, sizeof(ISP_CMOS_DEFAULT_S));
    
    pstDef->stComm.u8Rggb           = 0x3;      //3: bggr  
    pstDef->stComm.u8BalanceFe      = 0x1;

    pstDef->stDenoise.u8SinterThresh= 0x15;
    pstDef->stDenoise.u8NoiseProfile= 0x0;      //0: use default profile table; 1: use calibrated profile lut, the setting for nr0 and nr1 must be correct.
    pstDef->stDenoise.u16Nr0        = 0x0;
    pstDef->stDenoise.u16Nr1        = 0x0;

    pstDef->stDrc.u8DrcBlack        = 0x0;
    pstDef->stDrc.u8DrcVs           = 0x04;     // variance space
    pstDef->stDrc.u8DrcVi           = 0x08;     // variance intensity
    pstDef->stDrc.u8DrcSm           = 0xa0;     // slope max
    pstDef->stDrc.u16DrcWl          = 0x4ff;    // white level

    memcpy(&pstDef->stAgcTbl, &g_stIspAgcTable, sizeof(ISP_CMOS_AGC_TABLE_S));
    memcpy(&pstDef->stNoiseTbl, &g_stIspNoiseTable, sizeof(ISP_CMOS_NOISE_TABLE_S));
    memcpy(&pstDef->stDemosaic, &g_stIspDemosaic, sizeof(ISP_CMOS_DEMOSAIC_S));
    memcpy(&pstDef->stShading, &g_stIspShading, sizeof(ISP_CMOS_SHADING_S));

    return 0;
}

HI_U32 cmos_get_isp_black_level(ISP_CMOS_BLACK_LEVEL_S *pstBlackLevel)
{
    if (HI_NULL == pstBlackLevel)
    {
        printf("null pointer when get isp black level value!\n");
        return -1;
    }

    /* Don't need to update black level when iso change */
    pstBlackLevel->bUpdate = HI_FALSE;

    pstBlackLevel->au16BlackLevel[0] = 64;
    pstBlackLevel->au16BlackLevel[1] = 64;
    pstBlackLevel->au16BlackLevel[2] = 64;
    pstBlackLevel->au16BlackLevel[3] = 64;

    return 0;    
}

HI_VOID cmos_set_pixel_detect(HI_BOOL bEnable)
{    
    if (bEnable) /* setup for ISP pixel calibration mode */
    {
        //set frame rate
        sensor_write_register(0x380E, (6624>> 8)& 0xff);
        sensor_write_register(0x380F, 6624& 0xff);

        //set exposure time
        sensor_write_register(0x3500, 0x0f);  
        sensor_write_register(0x3501, 0xff);
        sensor_write_register(0x3502, 0xff);

        //set gain
        sensor_write_register(0x350A, 0x00);
        sensor_write_register(0x350B, 0x00);
    }
    else /* setup for ISP 'normal mode' */
    {
        sensor_write_register(0x380E, (1104>> 8)& 0xff);
        sensor_write_register(0x380F, 1104& 0xff);
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
        break;
        //sensor mode 1
        case 1:
            gu8SensorMode = 1;
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
    
    pstAeSnsDft->u32LinesPer500ms = 1104*30/2;
    pstAeSnsDft->u32FlickerFreq = 0;//60*256;//50*256;
    /* 1280 * 736 */
    /* 1296 * 816 */ /* reg: 0x22/23/24/25 */
    gu32FullLinesStd = 1104;
    //gu8Fps = 30;

    pstAeSnsDft->stIntTimeAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stIntTimeAccu.f32Accuracy = 1;
    pstAeSnsDft->u32MaxIntTime = 1096;
    pstAeSnsDft->u32MinIntTime = 2;
    
    pstAeSnsDft->au8HistThresh[0] = 0xd;
    pstAeSnsDft->au8HistThresh[1] = 0x28;
    pstAeSnsDft->au8HistThresh[2] = 0x60;
    pstAeSnsDft->au8HistThresh[3] = 0x80;
    
    pstAeSnsDft->u8AeCompensation = 0x80;
    
    pstAeSnsDft->u32MaxIntTimeTarget = 65535;
    pstAeSnsDft->u32MinIntTimeTarget = 2;

    /* 1(1+1/16), 1(1+2/16), ... , 2(1+1/16), ... , 16(1+15/16) */
    pstAeSnsDft->stAgainAccu.enAccuType = AE_ACCURACY_DB;
    pstAeSnsDft->stAgainAccu.f32Accuracy = 6;
    pstAeSnsDft->u32MaxAgain = 5;  /* 1, 2, 4, 8, 16, 32 (0~24db, unit is 6db) */
    pstAeSnsDft->u32MinAgain = 0;
    pstAeSnsDft->u32MaxAgainTarget = 6;
    pstAeSnsDft->u32MinAgainTarget = 0;
    

    pstAeSnsDft->stDgainAccu.enAccuType = AE_ACCURACY_LINEAR;
    pstAeSnsDft->stDgainAccu.f32Accuracy = 0.0625;
    pstAeSnsDft->u32MaxDgain = 31;  /* 1 ~ 31/16, unit is 1/16 */
    pstAeSnsDft->u32MinDgain = 16;
    pstAeSnsDft->u32MaxDgainTarget = 32;
    pstAeSnsDft->u32MinDgainTarget = 16; 

    pstAeSnsDft->u32ISPDgainShift = 8;
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
            gu32FullLinesStd = 1104;
            pstAeSnsDft->u32MaxIntTime = gu32FullLinesStd;
            pstAeSnsDft->u32LinesPer500ms = gu32FullLinesStd * 30 / 2;
            sensor_write_register(0x380E, (gu32FullLinesStd >> 8) & 0xff);
            sensor_write_register(0x380F, gu32FullLinesStd & 0xff);
        break;
        
        case 25:
            // Change the frame rate via changing the vertical blanking
            gu32FullLinesStd = 1325;
            pstAeSnsDft->u32MaxIntTime = gu32FullLinesStd;
            pstAeSnsDft->u32LinesPer500ms = gu32FullLinesStd * 25 / 2;
            sensor_write_register(0x380E, (gu32FullLinesStd >> 8) & 0xff);
            sensor_write_register(0x380F, gu32FullLinesStd & 0xff);
        break;
        
        default:
        break;
    }

    return;
}

static HI_VOID cmos_slow_framerate_set(HI_U8 u8SlowFrameRate,
    AE_SENSOR_DEFAULT_S *pstAeSnsDft)
{
    HI_U16 u16FullLines = (gu32FullLinesStd * u8SlowFrameRate) >> 4;
    
    sensor_write_register(0x380E, (u16FullLines >> 8) & 0xff);
    sensor_write_register(0x380F, u16FullLines & 0xff);

    pstAeSnsDft->u32MaxIntTime = u16FullLines - 8;
    gu32FullLines = u16FullLines;

    return;
}

static HI_VOID cmos_init_regs_info(HI_VOID)
{
#if CMOS_OV2710_ISP_WRITE_SENSOR_ENABLE
    HI_S32 i;
    static HI_BOOL bInit = HI_FALSE;

    if (HI_FALSE == bInit)
    {
        g_stSnsRegsInfo.enSnsType = ISP_SNS_I2C_TYPE;
        g_stSnsRegsInfo.u32RegNum = 8;
        for (i=0; i<g_stSnsRegsInfo.u32RegNum; i++)
        {
            g_stSnsRegsInfo.astI2cData[i].u8DevAddr = sensor_i2c_addr;
            g_stSnsRegsInfo.astI2cData[i].u32AddrByteNum = sensor_addr_byte;
            g_stSnsRegsInfo.astI2cData[i].u32DataByteNum = sensor_data_byte;
        }
        g_stSnsRegsInfo.astI2cData[0].bDelayCfg = HI_FALSE;
        g_stSnsRegsInfo.astI2cData[0].u32RegAddr = 0x3500;
        g_stSnsRegsInfo.astI2cData[1].bDelayCfg = HI_FALSE;
        g_stSnsRegsInfo.astI2cData[1].u32RegAddr = 0x3501;
        g_stSnsRegsInfo.astI2cData[2].bDelayCfg = HI_FALSE;
        g_stSnsRegsInfo.astI2cData[2].u32RegAddr = 0x3502;
        g_stSnsRegsInfo.astI2cData[3].bDelayCfg = HI_TRUE;
        g_stSnsRegsInfo.astI2cData[3].u32RegAddr = 0x3212;
        g_stSnsRegsInfo.astI2cData[4].bDelayCfg = HI_TRUE;
        g_stSnsRegsInfo.astI2cData[4].u32RegAddr = 0x350A;
        g_stSnsRegsInfo.astI2cData[5].bDelayCfg = HI_TRUE;
        g_stSnsRegsInfo.astI2cData[5].u32RegAddr = 0x350B;
        g_stSnsRegsInfo.astI2cData[6].bDelayCfg = HI_TRUE;
        g_stSnsRegsInfo.astI2cData[6].u32RegAddr = 0x3212;
        g_stSnsRegsInfo.astI2cData[7].bDelayCfg = HI_TRUE;
        g_stSnsRegsInfo.astI2cData[7].u32RegAddr = 0x3212;
        g_stSnsRegsInfo.bDelayCfgIspDgain = HI_TRUE;

        bInit = HI_TRUE;
    }
#endif
    return;
}

/* while isp notify ae to update sensor regs, ae call these funcs. */
static HI_VOID cmos_inttime_update(HI_U32 u32IntTime)
{
    HI_U32 u32Tmp = u32IntTime << 4;
#if CMOS_OV2710_ISP_WRITE_SENSOR_ENABLE
    cmos_init_regs_info();
    g_stSnsRegsInfo.astI2cData[0].u32Data = (u32Tmp >> 16) & 0x0F;
    g_stSnsRegsInfo.astI2cData[1].u32Data = (u32Tmp >> 8) & 0xFF;
    g_stSnsRegsInfo.astI2cData[2].u32Data = u32Tmp & 0xFF;
#else 
    sensor_write_register(0x3500, (u32Tmp >> 16) & 0x0F); //bit [16--24] ,bit[20-24 not used]
    sensor_write_register(0x3501, (u32Tmp >> 8) & 0xFF); //bit [8--15]
    sensor_write_register(0x3502, u32Tmp & 0xFF);//bit[0--7]
#endif
    return;
}

static HI_VOID cmos_gains_update(HI_U32 u32Again, HI_U32 u32Dgain)
{
    HI_U8 u8High, u8Low, u8Tmp;
    switch (u32Again)
    {
        case 0 :    /* 0db, 1 multiplies */
            u8High = 0x00;
            u8Tmp  = 0x00;
            break;
        case 1 :    /* 6db, 2 multiplies */
            u8High = 0x10;
            u8Tmp  = 0x00;
            break;
        case 2 :    /* 12db, 4 multiplies */
            u8High = 0x30;
            u8Tmp  = 0x00;
            break;
        case 3 :    /* 18db, 8 multiplies */
            u8High = 0x70;
            u8Tmp  = 0x00;
            break;
        case 4 :    /* 24db, 16 multiplies */
            u8High = 0xf0;
            u8Tmp  = 0x00;
            break;
        case 5 :    /* 30db, 32 multiplies */
            u8High = 0x00;
            u8Tmp  = 0x01;
            break;
        default:
            u8High = 0x00;
            u8Tmp  = 0x00;
            break;
    }

    u8Low = (u32Dgain - 16) & 0xf;

#if CMOS_OV2710_ISP_WRITE_SENSOR_ENABLE
    cmos_init_regs_info();
    g_stSnsRegsInfo.astI2cData[3].u32Data = 0x00;
    g_stSnsRegsInfo.astI2cData[4].u32Data = u8Tmp;
    g_stSnsRegsInfo.astI2cData[5].u32Data = (u8High | u8Low);
    g_stSnsRegsInfo.astI2cData[6].u32Data = 0x10;
    g_stSnsRegsInfo.astI2cData[7].u32Data = 0xA0;
    HI_MPI_ISP_SnsRegsCfg(&g_stSnsRegsInfo);
#else
    sensor_write_register(0x3212, 0x00);
    sensor_write_register(0x350A, u8Tmp);
    sensor_write_register(0x350B, (u8High | u8Low));
    sensor_write_register(0x3212, 0x10);
    sensor_write_register(0x3212, 0xA0);
#endif

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
    
    pstAwbSnsDft->u16WbRefTemp = 5000;

    pstAwbSnsDft->au16GainOffset[0] = 0x154;
    pstAwbSnsDft->au16GainOffset[1] = 0x100;
    pstAwbSnsDft->au16GainOffset[2] = 0x100;
    pstAwbSnsDft->au16GainOffset[3] = 0X1bd;

    pstAwbSnsDft->as32WbPara[0] = 156;
    pstAwbSnsDft->as32WbPara[1] = -71;
    pstAwbSnsDft->as32WbPara[2] = -170;
    pstAwbSnsDft->as32WbPara[3] = 198034;
    pstAwbSnsDft->as32WbPara[4] = 128;
    pstAwbSnsDft->as32WbPara[5] = -147889;

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
    s32Ret = HI_MPI_ISP_SensorRegCallBack(OV2710_ID, &stIspRegister);
    if (s32Ret)
    {
        printf("sensor register callback function failed!\n");
        return s32Ret;
    }
    
    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AE_LIB_NAME);
    cmos_init_ae_exp_function(&stAeRegister.stSnsExp);
    s32Ret = HI_MPI_AE_SensorRegCallBack(&stLib, OV2710_ID, &stAeRegister);
    if (s32Ret)
    {
        printf("sensor register callback function to ae lib failed!\n");
        return s32Ret;
    }

    stLib.s32Id = 0;
    strcpy(stLib.acLibName, HI_AWB_LIB_NAME);
    cmos_init_awb_exp_function(&stAwbRegister.stSnsExp);
    s32Ret = HI_MPI_AWB_SensorRegCallBack(&stLib, OV2710_ID, &stAwbRegister);
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


#endif 
