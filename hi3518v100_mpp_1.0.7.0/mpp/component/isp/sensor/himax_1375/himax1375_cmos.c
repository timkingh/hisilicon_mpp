#if !defined(__HIMAX_1375_CMOS_H_)
#define __HIMAX_1375_CMOS_H_

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

#define CMOS_HIMAX1375_ISP_WRITE_SENSOR_ENABLE (0)
/****************************************************************************
 * local variables															*
 ****************************************************************************/

static cmos_inttime_t cmos_inttime;
static cmos_gains_t cmos_gains;
HI_U8 gu8SensorMode = 0;

static cmos_isp_default_t st_coms_isp_default =
{
	// color matrix[9]
    {
        4950,
        {   0x1bb, 0x8083, 0x8038,
            0x803c, 0x01a1, 0x8065,
            0x8002, 0x80f2, 0x01f4
        },
        3100,
        {   0x016e, 0x8028, 0x8046,
            0x8064, 0x01a0, 0x803c,
            0x8035, 0x813b, 0x0270
        },
        2480,
        {   0x01a8, 0x8008, 0x80a0,
            0x805f, 0x01b0, 0x8051,
            0x808e, 0x8204, 0x0392
        }
    },


    // black level
    {0,0,0,0},

    //calibration reference color temperature
    4950,

    //WB gain at 5000, must keep consistent with calibration color temperature
    //{0x185, 0x100, 0x100, 0x17d},
    {0x187, 0x100, 0x100, 0x179},
    

    // WB curve parameters, must keep consistent with reference color temperature.   
    //{110,16,-127,138235,128,-87002},//newest
    {123,-15,-144,136180,128,-85414},
	// hist_thresh
    {0xd,0x28,0x60,0x80},
    //{0x10,0x40,0xc0,0xf0},

    0x0,	// iridix_balck
    0x3,	// bggr

	/* limit max gain for reducing noise,    */
    16,  4,

	// iridix
    0x04,	0x08,	0xa0, 	0x4ff,

    0x1, 	// balance_fe
    0x80,	// ae compensation
    0x15, 	// sinter threshold

    0x0,  0,  0,  //noise profile=0, use the default noise profile lut, don't need to set nr0 and nr1
#if CMOS_HIMAX1375_ISP_WRITE_SENSOR_ENABLE 
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
    {0x19,0x1e,0x2d,0x32,0x39,0x3f,0x3f,0x3f},

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
    {0, 27, 31, 33, 35, 36, 37, 38, 39, 40, 40, 41, 41, 42, 42, 43,
    43, 43, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 46, 47, 47,
    47, 47, 47, 48, 48, 48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 49,
    49, 49, 50, 50, 50, 50, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51,
    51, 51, 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 52, 52, 52, 52,
    52, 52, 52, 52, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
    53, 53, 53, 53, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
    54, 54, 54, 54, 54, 54, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55},

    //demosaic_weight_lut
    {0, 27, 31, 33, 35, 36, 37, 38, 39, 40, 40, 41, 41, 42, 42, 43,
    43, 43, 44, 44, 44, 45, 45, 45, 45, 46, 46, 46, 46, 46, 47, 47,
    47, 47, 47, 48, 48, 48, 48, 48, 48, 48, 49, 49, 49, 49, 49, 49,
    49, 49, 50, 50, 50, 50, 50, 50, 50, 50, 50, 51, 51, 51, 51, 51,
    51, 51, 51, 51, 51, 51, 51, 52, 52, 52, 52, 52, 52, 52, 52, 52,
    52, 52, 52, 52, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53, 53,
    53, 53, 53, 53, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54, 54,
    54, 54, 54, 54, 54, 54, 55, 55, 55, 55, 55, 55, 55, 55, 55, 55}
};

static cmos_isp_demosaic_t st_isp_demosaic =
{
    /*vh_slope*/
    0xda,

    /*aa_slope*/
    0xa9,

    /*va_slope*/
    0xec,

    /*uu_slope*/
    0x84,

    /*sat_slope*/
    0x5d,

    /*ac_slope*/
    0xcf,

    /*vh_thresh*/
    0xa9,

    /*aa_thresh*/
    0x23,

    /*va_thresh*/
    0xa6,

    /*uu_thresh*/
    0x2d,

    /*sat_thresh*/
    0x171,

    /*ac_thresh*/
    0x1b3
};

static cmos_isp_shading_table_t st_isp_shading_table =
{
    /*shading_center_r*/
    0x27a, 0x168,

    /*shading_center_g*/
    0x276, 0x16f,

    /*shading_center_b*/
    0x27a, 0x16c,

    /*shading_table_r*/
    {0x1000,0x1018,0x1028,0x103a,0x104c,0x105c,0x1072,0x1089,0x109e,0x10ba,0x10d5,0x10ef,
	0x110b,0x112b,0x114c,0x116d,0x118b,0x11ae,0x11d0,0x11f5,0x1218,0x123e,0x1260,0x1283,
	0x12aa,0x12cf,0x12f7,0x131b,0x1341,0x1369,0x138f,0x13b5,0x13db,0x1401,0x1423,0x1446,
	0x146d,0x148f,0x14b4,0x14d7,0x14fe,0x151e,0x153e,0x155a,0x1579,0x159a,0x15b7,0x15d3,
	0x15f4,0x1612,0x162e,0x164d,0x1663,0x167f,0x169a,0x16b1,0x16cb,0x16e4,0x16fc,0x170f,
	0x1727,0x173e,0x1753,0x176a,0x1783,0x1793,0x17a5,0x17b8,0x17c9,0x17da,0x17ec,0x17fe,
	0x180d,0x181a,0x182a,0x183b,0x184c,0x185a,0x1865,0x1876,0x1883,0x1890,0x189f,0x18a9,
	0x18b5,0x18c3,0x18cf,0x18d8,0x18e2,0x18e8,0x18ec,0x18f5,0x1901,0x190e,0x191f,0x1934,
	0x1946,0x1955,0x1968,0x197e,0x1993,0x19a4,0x19b5,0x19cc,0x19e1,0x19f5,0x1a06,0x1a16,
	0x1a2a,0x1a3d,0x1a4f,0x1a5d,0x1a6e,0x1a84,0x1a96,0x1aa7,0x1abb,0x1ad2,0x1ae5,0x1af9,
	0x1b0e,0x1b27,0x1b40,0x1b59,0x1b68,0x1b72,0x1b91,0x1bbf,0x1bf6},

    /*shading_table_g*/
    {0x1000,0x1013,0x1022,0x1033,0x1043,0x1054,0x1066,0x107b,0x108e,0x10a5,0x10bc,0x10d7,
	0x10f1,0x110c,0x112a,0x114b,0x116b,0x118c,0x11ae,0x11d0,0x11f3,0x1216,0x1238,0x125c,
	0x1282,0x12a9,0x12ce,0x12f3,0x131b,0x133f,0x1363,0x1386,0x13a9,0x13cc,0x13f0,0x1412,
	0x1434,0x1457,0x1479,0x149c,0x14bc,0x14da,0x14fa,0x1519,0x1537,0x1557,0x1575,0x1590,
	0x15ac,0x15c8,0x15e1,0x15fd,0x1617,0x162f,0x1648,0x165f,0x1673,0x168b,0x16a1,0x16b5,
	0x16c9,0x16db,0x16ee,0x1702,0x1714,0x1726,0x1736,0x1744,0x1752,0x1760,0x176c,0x1778,
	0x1785,0x1794,0x179e,0x17a8,0x17b5,0x17c1,0x17ca,0x17d5,0x17e1,0x17e9,0x17ef,0x17f3,
	0x17f9,0x17ff,0x1800,0x1801,0x1803,0x1806,0x180d,0x181c,0x182f,0x183e,0x184a,0x1854,
	0x185e,0x186a,0x1878,0x188b,0x189b,0x18a8,0x18b5,0x18c2,0x18d0,0x18e0,0x18ef,0x18fc,
	0x1907,0x1910,0x191a,0x1927,0x1936,0x1941,0x194a,0x1958,0x1965,0x1971,0x197e,0x1989,
	0x199c,0x19aa,0x19b7,0x19bd,0x19cc,0x19cc,0x19ce,0x19f6,0x1a0a},

    /*shading_table_b*/
    {0x1000,0x1012,0x101c,0x1025,0x102c,0x1031,0x103b,0x1045,0x104f,0x1059,0x1060,0x1072,
	0x107f,0x108d,0x109f,0x10b5,0x10c9,0x10dc,0x10ee,0x1109,0x111c,0x1137,0x1150,0x1167,
	0x1183,0x119e,0x11bb,0x11d5,0x11f4,0x120c,0x1228,0x1242,0x125a,0x1276,0x1292,0x12ac,
	0x12c7,0x12e0,0x12f9,0x1313,0x132d,0x1346,0x135d,0x1372,0x138a,0x13a4,0x13bb,0x13cf,
	0x13e5,0x13fb,0x1410,0x1427,0x143c,0x1450,0x1462,0x1475,0x1488,0x1496,0x14a5,0x14b7,
	0x14ca,0x14d7,0x14e5,0x14f4,0x1503,0x1511,0x151d,0x1529,0x1532,0x153c,0x1548,0x1552,
	0x155b,0x1566,0x156c,0x1573,0x157d,0x1587,0x158d,0x1593,0x159b,0x15a3,0x15a9,0x15ac,
	0x15b1,0x15b4,0x15b5,0x15b5,0x15b6,0x15b5,0x15b5,0x15ba,0x15c4,0x15d3,0x15e2,0x15ef,
	0x15f9,0x1604,0x1611,0x161e,0x1626,0x1632,0x1641,0x164c,0x165b,0x1667,0x1672,0x1679,
	0x1681,0x168e,0x1699,0x16a0,0x16aa,0x16bc,0x16ce,0x16dc,0x16e9,0x16f1,0x16f9,0x170c,
	0x1722,0x1735,0x173d,0x1739,0x1739,0x173c,0x173c,0x1731,0x1724},

    /*shading_off_center_r_g_b*/
    0xf57, 0xf0e, 0xf42,

    /*shading_table_nobe_number*/
    129
};

/*
 * This function initialises an instance of cmos_inttime_t.
 */
static __inline cmos_inttime_const_ptr_t cmos_inttime_initialize()
{
    cmos_inttime.full_lines_std = 832;
    cmos_inttime.full_lines_std_30fps = 832;
    cmos_inttime.full_lines_std_25fps = 998;
    cmos_inttime.full_lines = 832;
    cmos_inttime.full_lines_limit = 65535;
    cmos_inttime.max_lines = 832;
    cmos_inttime.min_lines = 2;
    cmos_inttime.max_lines_target = cmos_inttime.max_lines;
    cmos_inttime.min_lines_target = cmos_inttime.min_lines;

    cmos_inttime.vblanking_lines = 0;

    cmos_inttime.exposure_ashort = 0;
    cmos_inttime.exposure_shift = 0;

    cmos_inttime.lines_per_500ms = cmos_inttime.full_lines_std_30fps * 30 / 2; 
    cmos_inttime.flicker_freq = 0;//60*256;//50*256;

    return &cmos_inttime;
}

/*
 * This function applies the new integration time to the ISP registers.
 */
static __inline void cmos_inttime_update(cmos_inttime_ptr_t p_inttime)
{

    HI_U32 _curr = p_inttime->exposure_ashort;

    //refresh the sensor setting every frame to avoid defect pixel error
    sensor_write_register(0x0016, _curr&0xFF);
    sensor_write_register(0x0015, (_curr>>8)&0xFF);

    //refresh  exposure register
    sensor_write_register(0x0100,0x01);
    sensor_write_register(0x0101,0x01);
    sensor_write_register(0x0000,0x01);
    //printf("ashort %d \n",p_inttime->exposure_ashort);
    return;
}

/*
 * This function applies the new vert blanking porch to the ISP registers.
 */
static __inline void cmos_vblanking_update(cmos_inttime_const_ptr_t p_inttime)
{
    
    sensor_write_register(0x0011, p_inttime->vblanking_lines & 0xff);
    sensor_write_register(0x0010, (p_inttime->vblanking_lines & 0xff00) >> 8);
    
     //refresh   register
    sensor_write_register(0x0100,0x01);
    sensor_write_register(0x0101,0x01);
    sensor_write_register(0x0000,0x01);
  //  printf("full lines: %d\n" ,p_inttime->full_lines);
    return;
}

static __inline HI_U16 vblanking_calculate(
		cmos_inttime_ptr_t p_inttime)
{
    if(p_inttime->exposure_ashort >= p_inttime->full_lines - 4)
    {
    	p_inttime->exposure_ashort = p_inttime->full_lines - 4;
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
		case 30:
            p_inttime->lines_per_500ms = cmos_inttime.full_lines_std_30fps * 30 / 2; 
            p_inttime->full_lines_std= p_inttime->full_lines_std_30fps;
		break;

		case 25:
            /* do not change full_lines_std */
            p_inttime->lines_per_500ms = cmos_inttime.full_lines_std_25fps * 25 / 2;
            p_inttime->full_lines_std= p_inttime->full_lines_std_25fps;
            
        break;

		default:
		break;
	}
    
	return;
}

/*
 * This function initialises an instance of cmos_gains_t.
 */
static __inline cmos_gains_ptr_t cmos_gains_initialize()
{
    cmos_gains.max_again = 16;
    cmos_gains.max_dgain = 4<<6;
    cmos_gains.isp_dgain_delay_cfg = HI_TRUE;

    cmos_gains.max_again_target = cmos_gains.max_again; 
    cmos_gains.max_dgain_target = 169;//when the dgain > 169,the image quality is bad
    cmos_gains.again_shift = 0;
    cmos_gains.dgain_shift = 6;
    cmos_gains.dgain_fine_shift = 0;

    cmos_gains.isp_dgain_shift = 8;
    cmos_gains.isp_dgain = 1<< cmos_gains.isp_dgain_shift;
    cmos_gains.max_isp_dgain_target = 4 << cmos_gains.isp_dgain_shift;

    return &cmos_gains;
}

static __inline HI_U32 cmos_get_ISO(cmos_gains_ptr_t p_gains)
{ 
    HI_U32 _again = p_gains->again == 0 ? 1 : p_gains->again;
    HI_U32 _dgain = p_gains->dgain == 0 ? 1 : p_gains->dgain;
    HI_U32 _isp_dgain = p_gains->isp_dgain== 0 ? 1 : p_gains->isp_dgain;

    p_gains->iso =  (((HI_U64)_again * _dgain *_isp_dgain* 100) >>(p_gains->again_shift + p_gains->dgain_shift+p_gains->isp_dgain_shift)); 
    
    return p_gains->iso;
}

/*
 * This function applies the new gains to the ISP registers.
 */
static __inline void cmos_gains_update(cmos_gains_const_ptr_t p_gains)
{
    
    switch(p_gains->again)
    {
        case (16):
            sensor_write_register(0x0018, 0x4);
        break;
             
        case (8):
            sensor_write_register(0x0018, 0x3);    
        break;
        
        case (4):
            sensor_write_register(0x0018, 0x2);
        break;
        
        case (2):
            sensor_write_register(0x0018, 0x1);
        break;
        default:
            sensor_write_register(0x0018, 0x0);
           
 
    }

    //set dgain:
    sensor_write_register(0x001D, p_gains->dgain);
     //printf("dgain wirte register %#X \n",p_gains->dgain_db);
     
     //refresh   register
    sensor_write_register(0x0100,0x01);
    sensor_write_register(0x0101,0x01);
    sensor_write_register(0x0000,0x01);
    
    return;
}

static HI_U32 cmos_gains_lin_to_db_convert(HI_U32 data, HI_U32 shift_in)
{
    #define PRECISION 8
    HI_U32 _res = 0;
    if(0 == data)
    return _res;

    data = data << PRECISION; // to ensure precision.
    for(;;)
    {
        /* Note to avoid endless loop here. */
        data = (data * 913) >> 10;
        // data = (data*913 + (1<<9)) >> 10; // endless loop when shift_in is 0. */
        if(data <= ((1<<shift_in) << PRECISION))
        {
            break;
        }
    _res++;
    }
    return _res;
}

static __inline HI_U32 analog_gain_from_exposure_calculate(
		cmos_gains_ptr_t p_gains,
		HI_U32 exposure,
		HI_U32 exposure_max,
		HI_U32 exposure_shift)
{
    HI_U32 _again = 1 << p_gains->again_shift;
    int shift = 0;

    //prevent overlow of exposure
    while (exposure > (1<<22))
    {
        exposure >>= 1;
        exposure_max >>= 1;
        ++shift;
    }
    
    _again = (exposure<<p_gains->again_shift) /exposure_max;
    _again = (_again < (1<<p_gains->again_shift)) ? (1<<p_gains->again_shift) : _again;        
    _again = (_again > p_gains->max_again_target) ? p_gains->max_again_target : _again;
    
    if (_again >= 16) { _again = 16; }
    else if (_again >= 8) { _again = 8; }
    else if (_again >= 4) { _again = 4; }
    else if (_again >= 2) { _again = 2; }
    else{_again = 1;}
    
    p_gains->again = _again;
    exposure = (exposure<<p_gains->again_shift) / p_gains->again;
    p_gains->again_db = cmos_gains_lin_to_db_convert(p_gains->again, p_gains->again_shift);
    return exposure<<shift;
    
}

static __inline HI_U32 digital_gain_from_exposure_calculate(
		cmos_gains_ptr_t p_gains,
		HI_U32 exposure,
		HI_U32 exposure_max,
		HI_U32 exposure_shift)
{
    HI_U32 exposure0;
    int shift = 0;	
    HI_U32 _dgain=1<<p_gains->dgain_shift; 
    HI_U32 isp_dgain = (1 << p_gains->isp_dgain_shift);
	//prevent overlow of exposure
    while (exposure > (1<<22))
    {
    	exposure >>= 1;
    	exposure_max >>= 1;
    	++shift;
    }
    exposure_max = (0 == exposure_max)? 1: exposure_max;
    exposure0=exposure;
    
    //calculate digital gain
    if(exposure > exposure_max)
    {
       _dgain = (exposure << p_gains->dgain_shift)/exposure_max;
    }
    
    _dgain = (_dgain>(p_gains->max_dgain_target - 1))?(p_gains->max_dgain_target - 1):_dgain;//max digital gain is a little less than 4
    _dgain = (_dgain>169)?169:_dgain;//the sensor dgain_max is 4,we make it to be 169>>6 for image quality
    _dgain = (_dgain < (1<<p_gains->dgain_shift)) ? (1<<p_gains->dgain_shift) : _dgain;
    
    p_gains->dgain= _dgain;
    p_gains->dgain_db = cmos_gains_lin_to_db_convert(p_gains->dgain, p_gains->dgain_shift);
    exposure=(exposure << p_gains->dgain_shift) / p_gains->dgain;
   
    //calculate ISP gain
    if (exposure > exposure_max)
    {
        isp_dgain = (exposure  * isp_dgain) / exposure_max;
        exposure = exposure_max;
    }
    isp_dgain = (isp_dgain > p_gains->max_isp_dgain_target) ? (p_gains->max_isp_dgain_target) : isp_dgain;
    //isp_dgain = isp_dgain < (1<< p_gains->isp_dgain_shift) ? (1<< p_gains->isp_dgain_shift):isp_dgain;
    p_gains->isp_dgain = isp_dgain;

    return exposure<<shift;
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
    //return;
    if(0 == isp_mode) /* ISP 'normal' isp_mode */
    {
        sensor_write_register(0x0010, 0x0);
        sensor_write_register(0x0011, 0x0);
    }
    else if(1 == isp_mode) /* ISP pixel calibration isp_mode */
    {
        /* 5 fps */
        sensor_write_register(0x0010, 0x13); 
        sensor_write_register(0x0011, 0x80); 
        
        /* min gain */
        sensor_write_register(0x0018, 0x0);               
        sensor_write_register(0x001d, 0x40); 
        /* max exposure time*/
        sensor_write_register(0x0016, 0x7e);
        sensor_write_register(0x0015, 0x13);
    }
     //refresh  exposure register
    sensor_write_register(0x0100,0x01);
    sensor_write_register(0x0101,0x01);
    sensor_write_register(0x0000,0x01);
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
    .pfn_cmos_get_isp_shading_table = NULL,
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


#endif // __HIMAX1375_CMOS_H_
