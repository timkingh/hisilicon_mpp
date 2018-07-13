/******************************************************************************
  A driver program of ov2710 on Hi3516c
 ******************************************************************************
    Modification:  2013-04  Created
******************************************************************************/

#if !defined(__OV2710_CMOS_H_)
#define __OV2710_CMOS_H_

#include <stdio.h>
#include <string.h>
#include "hi_comm_sns.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/****************************************************************************
 * local variables															*
 ****************************************************************************/
extern const unsigned int sensor_i2c_addr;
extern const unsigned int sensor_addr_byte;
extern const unsigned int sensor_data_byte;

static cmos_inttime_t cmos_inttime;
static cmos_gains_t cmos_gains;
HI_U8 gu8SensorMode = 0;

/*set Frame End Update Mode 2 with HI_MPI_ISP_SetAEAttr and set this value 1 to avoid flicker in antiflicker mode */
/*when use Frame End Update Mode 2, the speed of i2c will affect whole system's performance                       */
/*increase I2C_DFT_RATE in Hii2c.c to 400000 to increase the speed of i2c                                         */
#define CMOS_OV2710_ISP_WRITE_SENSOR_ENABLE (1)

static cmos_isp_default_t st_coms_isp_default =
{
	// color matrix[9]
    {
        4850,
        {   0x0201, 0x80a7, 0x805a,
            0x8040, 0x019d, 0x805d,
            0x8010, 0x8150, 0x0260
        },
        3160,
        {   
		0x18e,0x8018,0x8076,
		0x8045,0x184,0x803e,
		0x8035,0x8134,0x26a
        },
        2470,
        {   
        	//saturation 80%
          0x19d,0x8017,0x8086,
          0x8031,0x17e,0x804c,
          0x8047,0x819b,0x2e2

        //saturation 95%
 		//0X1b6,0x8026,0x8090,
 		//0x803c,0x18f,0x8052,
 		//0x8053,0x81c8,0x31b
 		
        }
    },


    // black level
    {64,64,64,64},

    //calibration reference color temperature
    5000,

    //WB gain at 5000, must keep consistent with calibration color temperature
    {0x0154, 0x100, 0x100, 0x01bd},

    // WB curve parameters, must keep consistent with reference color temperature.
    {156, -71, -170, 198034, 128, -147889},

	// hist_thresh
    {0xd,0x28,0x60,0x80},
    //{0x10,0x40,0xc0,0xf0},

    0x0,	// iridix_balck
    0x3,	// bggr

	/* limit max gain for reducing noise,    */
    0x3e,	0x1,

	// iridix
    0x04,	0x08,	0xa0, 	0x4ff,

    0x1, 	// balance_fe
    0x80,	// ae compensation
    0x15, 	// sinter threshold

    0x0,  0,  0,  //noise profile=0, use the default noise profile lut, don't need to set nr0 and nr1
#if CMOS_OV2710_ISP_WRITE_SENSOR_ENABLE
    2
#else
    1
#endif
};

static cmos_isp_agc_table_t st_isp_agc_table =
{
    //sharpen_alt_d
    {0x8e,0x8b,0x88,0x83,0x7d,0x76,0x76,0x76},

    //sharpen_alt_ud
    {0x8f,0x89,0x7e,0x78,0x6f,0x3c,0x3c,0x3c},

    //snr_thresh
    {0x1e,0x1e,0x1f,0x20,0x22,0x25,0x54,0x54},

    //demosaic_lum_thresh
    {0x40,0x60,0x80,0x80,0x80,0x80,0x80,0x80},

    //demosaic_np_offset
    {0x0,0xa,0x12,0x1a,0x20,0x28,0x30,0x30},

    //ge_strength
    {0x55,0x55,0x55,0x55,0x55,0x55,0x55,0x55},

    /* saturation */
    {0x80,0x80,0x80,0x80,0x68,0x48,0x35,0x30}
};

static cmos_isp_noise_table_t st_isp_noise_table =
{
    //nosie_profile_weight_lut
    {0x0,0x0,0x0,0x6,0x15,0x1b,0x20,0x23,0x25,0x27,0x29,0x2a,0x2b,0x2d,0x2e,0x2f,0x2f,
	0x30,0x31,0x32,0x32,0x33,0x34,0x34,0x35,0x35,0x36,0x36,0x37,0x37,0x38,0x38,0x39,0x39,
	0x39,0x3a,0x3a,0x3a,0x3b,0x3b,0x3b,0x3c,0x3c,0x3c,0x3c,0x3d,0x3d,0x3d,0x3e,0x3e,0x3e,
	0x3e,0x3f,0x3f,0x3f,0x3f,0x3f,0x40,0x40,0x40,0x40,0x40,0x41,0x41,0x41,0x41,0x41,0x42,
	0x42,0x42,0x42,0x42,0x42,0x43,0x43,0x43,0x43,0x43,0x43,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,
	0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
	0x48,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49},

    //demosaic_weight_lut
    {0x0,0x6,0x15,0x1b,0x20,0x23,0x25,0x27,0x29,0x2a,0x2b,0x2d,0x2e,0x2f,0x2f,
	0x30,0x31,0x32,0x32,0x33,0x34,0x34,0x35,0x35,0x36,0x36,0x37,0x37,0x38,0x38,0x39,0x39,
	0x39,0x3a,0x3a,0x3a,0x3b,0x3b,0x3b,0x3c,0x3c,0x3c,0x3c,0x3d,0x3d,0x3d,0x3e,0x3e,0x3e,
	0x3e,0x3f,0x3f,0x3f,0x3f,0x3f,0x40,0x40,0x40,0x40,0x40,0x41,0x41,0x41,0x41,0x41,0x42,
	0x42,0x42,0x42,0x42,0x42,0x43,0x43,0x43,0x43,0x43,0x43,0x44,0x44,0x44,0x44,0x44,0x44,
	0x44,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x45,0x46,0x46,0x46,0x46,0x46,0x46,0x46,0x46,
	0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x47,0x48,0x48,0x48,0x48,0x48,0x48,0x48,0x48,
	0x48,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49,0x49}
};

static cmos_isp_demosaic_t st_isp_demosaic =
{
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


static cmos_isp_shading_table_t st_isp_shading_table =
{
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



/*
 * This function initialises an instance of cmos_inttime_t.
 */
static __inline cmos_inttime_const_ptr_t cmos_inttime_initialize()
{
    cmos_inttime.full_lines_std = 1104;
    cmos_inttime.full_lines_std_30fps = 1104;
    cmos_inttime.full_lines_std_25fps=1325;
    cmos_inttime.full_lines = 1104;
    cmos_inttime.full_lines_limit = 65535;
    cmos_inttime.exposure_shift = 0;

    cmos_inttime.lines_per_500ms = cmos_inttime.full_lines_std_30fps * 30 / 2; // 500ms / 39.17us
    cmos_inttime.flicker_freq = 0;//60*256;//50*256;
    return &cmos_inttime;
}

/*
 * This function applies the new integration time to the ISP registers.
 */
static __inline void cmos_inttime_update(cmos_inttime_ptr_t p_inttime)
{
     HI_U32 _curr = p_inttime->exposure_ashort *16;
#if CMOS_OV2710_ISP_WRITE_SENSOR_ENABLE
    ISP_I2C_DATA_S stI2cData;

    stI2cData.bDelayCfg = HI_FALSE;
    stI2cData.u8DevAddr = sensor_i2c_addr;
    stI2cData.u32AddrByteNum = sensor_addr_byte;
    stI2cData.u32DataByteNum = sensor_data_byte;
    
    stI2cData.u32RegAddr = 0x3502;
    stI2cData.u32Data =(_curr&0xFF);//bit[0--7]
    HI_MPI_ISP_I2cWrite(&stI2cData);
    
    stI2cData.u32RegAddr = 0x3501;
    stI2cData.u32Data = (_curr>>8)&0xFF; //bit [8--15]
    HI_MPI_ISP_I2cWrite(&stI2cData);

    stI2cData.u32RegAddr = 0x3500;
    stI2cData.u32Data = (_curr>>16)&0x0F; //bit [16--24] ,bit[20-24 not used]
    HI_MPI_ISP_I2cWrite(&stI2cData);
    
#else
    sensor_write_register(0x3502, _curr&0xFF);//bit[0--7]
    sensor_write_register(0x3501, (_curr>>8)&0xFF); //bit [8--15]
    sensor_write_register(0x3500, (_curr>>16)&0x0F); //bit [16--24] ,bit[20-24 not used]
#endif
    
	
    return;
}

/*
 * This function applies the new vert blanking porch to the ISP registers.
 */
static __inline void cmos_vblanking_update(cmos_inttime_const_ptr_t p_inttime)
{
    sensor_write_register(0x380E, p_inttime->full_lines >> 8& 0xff);
    sensor_write_register(0x380F, p_inttime->full_lines& 0xff);
    return;
}

static __inline HI_U16 vblanking_calculate(
		cmos_inttime_ptr_t p_inttime)
{
    if(p_inttime->exposure_ashort >= p_inttime->full_lines - 8)
    {
    	p_inttime->exposure_ashort = p_inttime->full_lines - 8;
    }

    p_inttime->vblanking_lines = p_inttime->full_lines - p_inttime->full_lines_std_30fps;
    return p_inttime->exposure_ashort;
}

/* Set fps base */
static __inline void cmos_fps_set(
		cmos_inttime_ptr_t p_inttime,
		const HI_U8 fps
		)
{
    switch(fps)
    {
        default:
        case 30:
	    p_inttime->full_lines_std = p_inttime->full_lines_std_30fps;
	    sensor_write_register(0x380E, (p_inttime->full_lines_std >> 8)& 0xff);
	    sensor_write_register(0x380F, p_inttime->full_lines_std& 0xff);
	    p_inttime->lines_per_500ms =  p_inttime->full_lines_std_30fps * 30 / 2;
	    break;

        case 25:
        p_inttime->full_lines_std = p_inttime->full_lines_std_25fps ;
        sensor_write_register(0x380E, (p_inttime->full_lines_std >> 8)& 0xff);
	    sensor_write_register(0x380F, p_inttime->full_lines_std& 0xff);
	    p_inttime->lines_per_500ms =  p_inttime->full_lines_std_25fps * 25/ 2;
        break;

    }
    return;

}

/*
 * This function initialises an instance of cmos_gains_t.
 */
static __inline cmos_gains_ptr_t cmos_gains_initialize()
{
    cmos_gains.max_again = 992;
    cmos_gains.max_dgain = 1;
    cmos_gains.again_shift = 4;
    cmos_gains.dgain_shift = 0;
    cmos_gains.dgain_fine_shift = 0;
    cmos_gains.isp_dgain_delay_cfg = HI_TRUE;
    cmos_gains.isp_dgain_shift = 8;
    cmos_gains.isp_dgain = 1 << cmos_gains.isp_dgain_shift;
    cmos_gains.max_isp_dgain_target = 4 << cmos_gains.isp_dgain_shift;
    

    return &cmos_gains;
}

static __inline HI_U32 cmos_get_ISO(cmos_gains_ptr_t p_gains)
{
    HI_U32 _again = p_gains->again == 0 ? 1 : p_gains->again;
    HI_U32 _dgain = p_gains->dgain == 0 ? 1 : p_gains->dgain;
    HI_U32 _isp_dgain = p_gains->isp_dgain== 0 ? 1 : p_gains->isp_dgain;

    p_gains->iso =  (((HI_U64)_again * _dgain *_isp_dgain* 100) >> (p_gains->again_shift + p_gains->dgain_shift+p_gains->isp_dgain_shift)); 
    
    return p_gains->iso;
}

static __inline HI_U32 analog_gain_from_exposure_calculate(
		cmos_gains_ptr_t p_gains,
		HI_U32 exposure,
		HI_U32 exposure_max,
		HI_U32 exposure_shift)
{
    HI_U32 _again = 1 << p_gains->again_shift;
    int shift = 0;
    HI_U8 i = 0;

    while (exposure > (1<<26))
    {
        exposure >>= 1;
        exposure_max >>= 1;
        ++shift;
    }

    if(exposure > exposure_max)
    {
        _again = (exposure << p_gains->again_shift) / exposure_max;
        _again = _again < 16? 16: _again;
    	_again = _again > p_gains->max_again_target ? p_gains->max_again_target : _again;
        
        for(i = 0; i < 6; i++)
        {
            if(_again < 32)
            {
                break;
            }
            _again >>= 1;
        }
    }
    else
    {
    }

    p_gains->again_db = (1 << (i + 4)) + _again - 32;
    p_gains->again = _again << i;

    return (exposure << (shift + 4)) / p_gains->again;

}

static __inline HI_U32 digital_gain_from_exposure_calculate(
		cmos_gains_ptr_t p_gains,
		HI_U32 exposure,
		HI_U32 exposure_max,
		HI_U32 exposure_shift)
{
    HI_U32 isp_dgain = (1 << p_gains->isp_dgain_shift);
    int shft = 0;

    while (exposure > (1 << 22))
    {
    	exposure >>= 1;
    	exposure_max >>= 1;
    	++shft;
    }
    exposure_max = (0 == exposure_max)? 1: exposure_max;

    if(exposure > exposure_max)
    {
        isp_dgain = ((exposure << p_gains->isp_dgain_shift) + (exposure_max >> 1)) / exposure_max;
        exposure = exposure_max;
        isp_dgain = (isp_dgain > p_gains->max_isp_dgain_target) ? (p_gains->max_isp_dgain_target) : isp_dgain;        
    }
    else
    {
    }
    p_gains->isp_dgain = isp_dgain;
    p_gains->dgain = 1;

    return exposure << shft;
}

/*
 * This function applies the new gains to the ISP registers.
 */
static __inline void cmos_gains_update(cmos_gains_const_ptr_t p_gains)
{


#if CMOS_OV2710_ISP_WRITE_SENSOR_ENABLE
    ISP_I2C_DATA_S stI2cData;

    stI2cData.bDelayCfg = HI_TRUE;
    stI2cData.u8DevAddr = sensor_i2c_addr;
    stI2cData.u32AddrByteNum = sensor_addr_byte;
    stI2cData.u32DataByteNum = sensor_data_byte;
    
    stI2cData.u32RegAddr = 0x3212;
    stI2cData.u32Data = 0x00;
    HI_MPI_ISP_I2cWrite(&stI2cData);
    
    stI2cData.u32RegAddr = 0x350A;
    stI2cData.u32Data = (p_gains->again_db&0x100)>>8;
    HI_MPI_ISP_I2cWrite(&stI2cData);
    
    stI2cData.u32RegAddr = 0x350B;
    stI2cData.u32Data = (p_gains->again_db&0xFF);
    HI_MPI_ISP_I2cWrite(&stI2cData);

    stI2cData.u32RegAddr = 0x3212;
    stI2cData.u32Data = 0x10;
    HI_MPI_ISP_I2cWrite(&stI2cData);
    stI2cData.u32Data = 0xA0;
    HI_MPI_ISP_I2cWrite(&stI2cData);
    
#else
    unsigned int regval=p_gains->again_db;
    sensor_write_register(0x3212, 0x00);
    sensor_write_register(0x350A, ((regval&0x100)>>8));
    sensor_write_register(0x350B, (regval&0xFF));
    sensor_write_register(0x3212, 0x10);
    sensor_write_register(0x3212, 0xA0);
#endif
 
	return;
}

static HI_U32 cmos_get_isp_agc_table(cmos_isp_agc_table_ptr_t p_cmos_isp_agc_table)
{
    if (NULL == p_cmos_isp_agc_table)
    {
        printf("null pointer when get isp agc table value!\n");
        return -1;
    }
    memcpy(p_cmos_isp_agc_table, &st_isp_agc_table, sizeof(cmos_isp_agc_table_t));
    return 0;
}

static HI_U32 cmos_get_isp_noise_table(cmos_isp_noise_table_ptr_t p_cmos_isp_noise_table)
{
    if (NULL == p_cmos_isp_noise_table)
    {
        printf("null pointer when get isp noise table value!\n");
        return -1;
    }
    memcpy(p_cmos_isp_noise_table, &st_isp_noise_table, sizeof(cmos_isp_noise_table_t));
    return 0;
}

static HI_U32 cmos_get_isp_demosaic(cmos_isp_demosaic_ptr_t p_cmos_isp_demosaic)
{
    if (NULL == p_cmos_isp_demosaic)
    {
        printf("null pointer when get isp demosaic value!\n");
        return -1;
    }
    memcpy(p_cmos_isp_demosaic, &st_isp_demosaic,sizeof(cmos_isp_demosaic_t));
    return 0;

}

static HI_U32 cmos_get_isp_shading_table(cmos_isp_shading_table_ptr_t p_cmos_isp_shading_table)
{
    if (NULL == p_cmos_isp_shading_table)
    {
        printf("null pointer when get isp shading table value!\n");
        return -1;
    }
    memcpy(p_cmos_isp_shading_table, &st_isp_shading_table, sizeof(cmos_isp_shading_table_t));
    return 0;
}


static void setup_sensor(int isp_mode)
{
    if(0 == isp_mode) /* ISP 'normal' isp_mode */
    {
        sensor_write_register(0x380E, (1104>> 8)& 0xff);
        sensor_write_register(0x380F, 1104& 0xff);
    }
    else if(1 == isp_mode) /* ISP pixel calibration isp_mode */
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
}


static HI_U8 cmos_get_analog_gain(cmos_gains_ptr_t p_gains)
{
    return 0;
}

static HI_U8 cmos_get_digital_gain(cmos_gains_ptr_t p_gains)
{
    return 0;
}

static HI_U32 cmos_get_isp_default(cmos_isp_default_ptr_t p_coms_isp_default)
{
    if (NULL == p_coms_isp_default)
    {
        printf("null pointer when get isp default value!\n");
        return -1;
    }
    memcpy(p_coms_isp_default, &st_coms_isp_default, sizeof(cmos_isp_default_t));
    return 0;
}


/****************************************************************************
 * callback structure                                                       *
 ****************************************************************************/

SENSOR_EXP_FUNC_S stSensorExpFuncs =
{
    .pfn_cmos_inttime_initialize = cmos_inttime_initialize,
    .pfn_cmos_inttime_update = cmos_inttime_update,

    .pfn_cmos_gains_initialize = cmos_gains_initialize,
    .pfn_cmos_gains_update = cmos_gains_update,
    .pfn_cmos_gains_update2 = NULL,
    .pfn_analog_gain_from_exposure_calculate = analog_gain_from_exposure_calculate,
    .pfn_digital_gain_from_exposure_calculate = digital_gain_from_exposure_calculate,

    .pfn_cmos_fps_set = cmos_fps_set,
    .pfn_vblanking_calculate = vblanking_calculate,
    .pfn_cmos_vblanking_front_update = cmos_vblanking_update,

    .pfn_setup_sensor = setup_sensor,

    .pfn_cmos_get_analog_gain = cmos_get_analog_gain,
    .pfn_cmos_get_digital_gain = cmos_get_digital_gain,
    .pfn_cmos_get_digital_fine_gain = NULL,
    .pfn_cmos_get_iso = cmos_get_ISO,

    .pfn_cmos_get_isp_default = cmos_get_isp_default,
    .pfn_cmos_get_isp_special_alg = NULL,
    .pfn_cmos_get_isp_agc_table = cmos_get_isp_agc_table,
    .pfn_cmos_get_isp_noise_table = cmos_get_isp_noise_table,
    .pfn_cmos_get_isp_demosaic = cmos_get_isp_demosaic,
    .pfn_cmos_get_isp_shading_table = cmos_get_isp_shading_table,
    .pfn_cmos_get_isp_gamma_table = NULL,
};

int sensor_register_callback(void)
{
    int ret;
    ret = HI_MPI_ISP_SensorRegCallBack(&stSensorExpFuncs);
    if (ret)
    {
        printf("sensor register callback function failed!\n");
        return ret;
    }

    return 0;
}

//chang sensor mode
int sensor_mode_set(HI_U8 u8Mode)
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
            return -1;
        break;
    }
    return 0;
}


#ifdef __cplusplus
#if __cplusplus
}
#endif
#endif /* End of #ifdef __cplusplus */


#endif // __OV2710_CMOS_H_
