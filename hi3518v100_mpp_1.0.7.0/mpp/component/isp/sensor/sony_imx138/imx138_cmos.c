/******************************************************************************
  A driver program of panasonic IMX138 on HI3518A 
 ******************************************************************************
    Modification:  2013-05  Created
******************************************************************************/


#if !defined(__IMX138_CMOS_H_)
#define __IMX138_CMOS_H_

#include <stdio.h>
#include <string.h>
#include <assert.h>
#include "hi_comm_sns.h"
#include "hi_sns_ctrl.h"
#include "mpi_isp.h"

#ifdef __cplusplus
#if __cplusplus
extern "C"{
#endif
#endif /* End of #ifdef __cplusplus */

/* Note: format of address is special.
 * chip_id + reg_adddr */
#define EXPOSURE_ADDR (0x220) //2:chip_id, 20: reg addr.
#define LONG_EXPOSURE_ADDR (0x223)
#define PGC_ADDR (0x214)
#define VMAX_ADDR (0x218)

/****************************************************************************
 * local variables															*
 ****************************************************************************/

static cmos_inttime_t cmos_inttime;
static cmos_gains_t cmos_gains;
HI_U8 gu8SensorMode = 0;
static cmos_isp_default_t st_coms_isp_default =
{
    // color correction matrix
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
    },

	// black level for R, Gr, Gb, B channels
	{0xF0,0xF0,0xF0,0xF0},

	//calibration reference color temperature
	5048,

	//WB gain at 5000K, must keep consistent with calibration color temperature
	{0x01ff, 0x100, 0x100, 0x01e4},

	// WB curve parameters, must keep consistent with reference color temperature.
	{40, 106, -110, 184787, 128, -134867},

	// hist_thresh
	{0xd,0x28,0x60,0x80},

	0x00,	// iridix_balck
	0x2,	// 0:rggb; 2: gbrg

	// max again, max dgain
	0x10,	0x10,

	// iridix
	0x04,	0x08,	0xa0, 	0x8ff,

	0x1, 	// balance_fe
	0x80,	// ae compensation
	0x8, 	// sinter threshold

	0x1,  //0: use default profile table; 1: use calibrated profile lut, the setting for nr0 and nr1 must be correct.
	0,    //nr0
	455,  //nr1

    1
};


static cmos_isp_agc_table_t st_isp_agc_table =
{
    //sharpen_alt_d
    //{45,45,45,40,40,35,33,15},
    {80,80,80,71,71,62,59,27},

    //sharpen_alt_ud
    //{30,30,30,28,28,25,22,10},
    {53,53,53,49,49,44,39,18},

    //snr_thresh
    //{33,41,49,57,65,73,81,89},
    {20,25,30,35,39,44,49,0x64},

    //demosaic_lum_thresh
    {0x60,0x60,0x80,0x80,0x80,0x80,0x80,0x80},

    //demosaic_np_offset
    {0,0xa,16,24,32,40,48,56},

    //ge_strength
    {0x55,0x55,0x55,0x55,0x55,0x55,0x37,0x37},

    /* saturation */
    {0x80,0x80,0x6C,0x48,0x44,0x40,0x3C,0x38}
};

static cmos_isp_noise_table_t st_isp_noise_table =
{
  //nosie_profile_weight_lut
    {
        0,0,0,0,0,0,0,15,25,29,31,33,32,35,36,37,37,38,39,39,40,40,40,41,41,41,42,42,42,43,43,43,43,44,44,44,44,44,45,45,45,45,45,45,45,46,46,46,46,46,46,47,47,47,47,47,47,47,47,48,48,48,48,48,48,48,48,48,49,49,49,49,49,49,49,49,49,49,49,49,50,50,50,50,50,50,50,50,50,50,50,50,50,50,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52
    },

  //demosaic_weight_lut
    {
        0,15,25,29,31,33,34,35,36,37,37,38,39,39,40,40,40,41,41,41,42,42,42,43,43,43,43,44,44,44,44,44,45,45,45,45,45,45,46,46,46,46,46,46,46,47,47,47,47,47,47,47,47,48,48,48,48,48,48,48,48,48,49,49,49,49,49,49,49,49,49,49,49,49,49,50,50,50,50,50,50,50,50,50,50,50,50,50,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,51,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52,52
    }
};




static cmos_isp_demosaic_t st_isp_demosaic =
{
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
/*
 * This function initialises an instance of cmos_inttime_t.
 */
static __inline cmos_inttime_const_ptr_t cmos_inttime_initialize()
{
	cmos_inttime.full_lines_std = 750;
	cmos_inttime.full_lines_std_30fps = 750;
	cmos_inttime.full_lines = 750;
	cmos_inttime.full_lines_limit = 65535;
	cmos_inttime.max_lines_target = 748;
	cmos_inttime.min_lines_target = 2;
	cmos_inttime.vblanking_lines = 0;

	cmos_inttime.exposure_ashort = 0;

	cmos_inttime.lines_per_500ms = 750*30/2; //
	cmos_inttime.flicker_freq = 0;//60*256;//50*256;


	return &cmos_inttime;
}

/*
 * This function applies the new integration time to the ISP registers.
 */
static __inline void cmos_inttime_update(cmos_inttime_ptr_t p_inttime)
{

    HI_U16 exp_time;
    exp_time = p_inttime->full_lines - p_inttime->exposure_ashort - 1;    
    sensor_write_register(EXPOSURE_ADDR, exp_time & 0xFF);
    sensor_write_register(EXPOSURE_ADDR+1, (exp_time & 0xFF00) >> 8);

}

/*
 * This function applies the new vert blanking porch to the ISP registers.
 */
static __inline void cmos_vblanking_update(cmos_inttime_const_ptr_t p_inttime)
{

	HI_U16 vmax = p_inttime->full_lines;
	sensor_write_register(VMAX_ADDR, (vmax&0x00ff));
	sensor_write_register(VMAX_ADDR+1, ((vmax&0xff00) >> 8));
	return;
}

static __inline HI_U16 vblanking_calculate(
		cmos_inttime_ptr_t p_inttime)
{
	if (p_inttime->exposure_ashort >= p_inttime->full_lines - 2)
	{
		p_inttime->exposure_ashort = p_inttime->full_lines - 2;
	}

	p_inttime->vblanking_lines = p_inttime->full_lines - p_inttime->full_lines_std;
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
			// Change the frame rate via changing the vertical blanking
			p_inttime->full_lines_std = 750;
			sensor_write_register(VMAX_ADDR, 0xEE);
			sensor_write_register(VMAX_ADDR+1, 0x02);
			p_inttime->lines_per_500ms = 750 * 30 / 2;
		break;

		case 25:
			// Change the frame rate via changing the vertical blanking
			p_inttime->full_lines_std = 900;
			sensor_write_register(VMAX_ADDR, 0x84);
			sensor_write_register(VMAX_ADDR+1, 0x03);
			p_inttime->lines_per_500ms = 900 * 25 / 2;
		break;

		default:
		break;
	}

}

/*
 * This function initialises an instance of cmos_gains_t.
 */
static __inline cmos_gains_ptr_t cmos_gains_initialize()
{
    cmos_gains.again = 1;
    cmos_gains.dgain = 1;
    cmos_gains.isp_dgain = 1;
    cmos_gains.again_db = 1;
    cmos_gains.dgain_db = 1;
	cmos_gains.again_shift = 10;
	cmos_gains.dgain_shift = 10;
	cmos_gains.dgain_fine_shift = 0;

    cmos_gains.isp_dgain_shift = 4;
    cmos_gains.isp_dgain = 1 << cmos_gains.isp_dgain_shift;
    cmos_gains.max_isp_dgain_target = 1 << cmos_gains.isp_dgain_shift;

    
    cmos_gains.max_again = 16229;
    cmos_gains.max_dgain = 16229;
    cmos_gains.max_again_target = 16229;
    cmos_gains.max_dgain_target = 16229;
    cmos_gains.max_isp_dgain_target = 4 << cmos_gains.isp_dgain_shift;
    cmos_gains.isp_dgain_delay_cfg = HI_TRUE;


	return &cmos_gains;
}

static __inline HI_U16 digital_gain_lut_get_value(HI_U8 index)
{
    static HI_U16 gain_lut[] =
    {
        0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
        0x60, 0x61, 0x62, 0x63, 0x64, 0x65, 0x66, 0x67, 0x68, 0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
        0x70, 0x71, 0x72, 0x73, 0x74, 0x75, 0x76, 0x77, 0x78, 0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
        0x80, 0x81, 0x82, 0x83, 0x84, 0x85, 0x86, 0x87, 0x88, 0x89,0x8A,0x8B,0x8C
    };
	return gain_lut[index];
}

static  HI_U32   digital_gain_table[80]={

 1060,   1097,   1136,   1176,   1217,   1260,   1304,   1350,   1397,   1446,   1497,   1550,   1604,   1661,   1719,   1780,
 1842,   1907,   1974,   2043,   2115,   2189,   2266,   2346,   2428,   2514,   2602,   2693,   2788,   2886,   2987,   3092,
 3201,   3314,   3430,   3551,   3675,   3805,   3938,   4077,   4220,   4368,   4522,   4681,   4845,   5015,   5192,   5374,
 5563,   5758,   5961,   6170,   6387,   6611,   6844,   7084,   7333,   7591,   7858,   8134,   8420,   8716,   9022,   9339,
 9667,  10007,  10359,  10723,  11099,  11489,  11893,  12311,  12744,  13192,  13655,  14135,  14632,  15146,  15678,   16229

};

/*
 * This function applies the new gains to the ISP registers.
 */
static __inline void cmos_gains_update(cmos_gains_const_ptr_t p_gains)
{

    //HI_U16 lut_val;
    /* analog gain = APGC * 0.3 db, APGC = [0x0,0x50], ag_db=[0db, 24db] */
    /* digital gain = DPGC * 0.3 db, DPCG = [0x50,0xA0], dg_db=[0db, 24db] */
    if((p_gains->again_db + p_gains->dgain_db) <= 0xA0)
    {
        sensor_write_register(PGC_ADDR, p_gains->again_db + p_gains->dgain_db);
    }
    else
    {
        sensor_write_register(PGC_ADDR, 0xA0);
    }

}

static __inline HI_U32 analog_gain_from_exposure_calculate(
		cmos_gains_ptr_t p_gains,
		HI_U32 exposure,
		HI_U32 exposure_max,
		HI_U32 exposure_shift)
{
    int _i;
    HI_U32 _again = 0;
    HI_U32 exposure0;
    int shft = 0;

    HI_U32 u32MaxStep = 0;
    HI_U32 times;

    /*linear mode, again: 0 to 24dB(step 0.3dB)*/
        u32MaxStep = 80;
    // normalize

    while (exposure > (1<<22))
    {
        exposure >>= 1;
        exposure_max >>= 1;
        ++shft;
    }
  
	exposure0 = exposure;

    times = (exposure0 * 1024)/exposure_max;

      
      if(times <=1024)
      {
        exposure0 = exposure0;
      }
      
      else
      {
        if(times > p_gains->max_again_target)
        {
            times = p_gains->max_again_target;
        }
        for(_i = 0x0; _i < 0x50; _i++)
        {
          
          if(times <=digital_gain_table[_i])
              break;
          _again++;
          
        }

        if(times >= digital_gain_table[79])
        {
           times = digital_gain_table[79];
        }
          
        exposure0 = exposure0*1024/times;
    
      }
      
    p_gains->again = ((HI_U64)exposure << p_gains->again_shift) / exposure0; 
    p_gains->again = p_gains->again < (1 << p_gains->again_shift) ? (1 << p_gains->again_shift) : p_gains->again;

    p_gains->again_db = _again;
    return exposure0 << shft;

}

static __inline HI_U32 digital_gain_from_exposure_calculate(
		cmos_gains_ptr_t p_gains,
		HI_U32 exposure,
		HI_U32 exposure_max,
		HI_U32 exposure_shift)
{
	int _i;
	HI_U32 _dgain = 0;
	HI_U32 exposure0;
    HI_U32 times;
	int shft = 0;
    HI_U32 isp_dgain = (1 << p_gains->isp_dgain_shift);

    HI_U32 u32MaxStep = 0;

    /*linear mode, dgain: 0.3 to 24dB(step 0.3dB)*/
    u32MaxStep = 80;


	// normalize
	while (exposure > (1<<20)) /* analog use (1<<22) for analog exposure is bigger. */
	{
		exposure >>= 1;
		exposure_max >>= 1;
		++shft;
	}

	exposure0 = exposure;
    
    times = (exposure0 * 1024)/exposure_max;
    
   
        if(times <=1024)
        {
          exposure0 = exposure0;
        }
        
  /* unit: 0.3db */      
        else
        {
            if(times > p_gains->max_dgain_target)
            {
                times = p_gains->max_dgain_target;
            }
            for(_i = 0x0; _i < 80; _i++)
            {

                if(times <=digital_gain_table[_i])
                    break;
                _dgain++;

            }

            if(times >= digital_gain_table[79])
            {
                times = digital_gain_table[79];
            }

            exposure0 = exposure0*1024/times;
        }


	p_gains->dgain = (exposure << p_gains->dgain_shift) / exposure0;
    p_gains->dgain =( p_gains->dgain < (1 << p_gains->dgain_shift))? (1 << p_gains->dgain_shift): p_gains->dgain;
    p_gains->dgain_db = _dgain;
    exposure_max = (0 == exposure_max)? 1: exposure_max;

    
    if(exposure0 > exposure_max )   
    {     
       isp_dgain = ((exposure0 << p_gains->isp_dgain_shift) + (exposure_max >> 1)) / exposure_max; 
       
       
        exposure0 = exposure_max;  
        
        isp_dgain = (isp_dgain > p_gains->max_isp_dgain_target) ? (p_gains->max_isp_dgain_target) : isp_dgain;
        
     }  

    p_gains->isp_dgain = isp_dgain;


    return exposure0 << shft;
}

static void setup_sensor(int isp_mode)
{
    printf("defect bad pixiel  \n");
	if(0 == isp_mode) /* setup for ISP 'normal mode' */
	{
        sensor_write_register(VMAX_ADDR, 0xEE);
        sensor_write_register(VMAX_ADDR + 1, 0x02);
	}
	else if(1 == isp_mode) /* setup for ISP pixel calibration mode */
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
}

static __inline HI_U32 cmos_get_ISO(cmos_gains_ptr_t p_gains)
{


    HI_U32 _again = p_gains->again == 0 ? 1 : p_gains->again;
	HI_U32 _dgain = p_gains->dgain == 0 ? 1 : p_gains->dgain;
    HI_U32 _ispdgain = p_gains->isp_dgain == 0 ? 1 : p_gains->isp_dgain;

		p_gains->iso =  ((_again * _dgain * _ispdgain * 100) 
        >> (p_gains->again_shift + p_gains->dgain_shift 
        + p_gains->isp_dgain_shift));

	return p_gains->iso;
}

/* Note: The unit of return value is 1db.  */
static HI_U8 cmos_get_analog_gain(cmos_gains_ptr_t cmos_gains)
{
    return (cmos_gains->again_db *  3 / 10);
}

/* Note: The unit of return value is 1db.  */
static HI_U8 cmos_get_digital_gain(cmos_gains_ptr_t cmos_gains)
{
    return  (cmos_gains->dgain_db *  3 / 10);
}

/*
static HI_U8 cmos_get_digital_fine_gain(cmos_gains_ptr_t cmos_gains)
{
    return cmos_gains->dgain_fine;
}
*/

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

#endif // __IMX138_CMOS_H_
