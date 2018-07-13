#include <stdio.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <unistd.h>

#ifdef HI_GPIO_I2C
#include "gpioi2c_ov.h"
#include "gpio_i2c.h"
#else
#include "hi_i2c.h"
#endif
const unsigned int sensor_i2c_addr	=	0x6c;		/* I2C Address of OV9712 */
const unsigned int sensor_addr_byte	=	2;
const unsigned int sensor_data_byte	=	1;

int sensor_read_register(int addr)
{
#ifdef HI_GPIO_I2C
	int fd = -1;
	int ret, value;
	unsigned char data;
	
	fd = open("/dev/gpioi2c_ov", 0);
    if(fd<0)
    {
    	printf("Open gpioi2c_ov error!\n");
    	return -1;
    }
    printf("GPIO-I2C-OV write !\n")
    value = ((sensor_i2c_addr&0xff)<<24) | ((addr&0xffff)<<8);

    ret = ioctl(fd, GPIO_I2C_READ, &value);
	if (ret)
	{
	    printf("GPIO-I2C-OV write faild!\n");
        close(fd);
		return -1;
	}

	data = value&0xff;

	close(fd);
	return data;
#else
	int fd = -1;
	int ret;
	unsigned int data;
	I2C_DATA_S i2c_data;
	
	fd = open("/dev/hi_i2c", 0);
    if(fd < 0)
    {
    	printf("Open i2c device error!\n");
    	return -1;
    }

    i2c_data.dev_addr  = sensor_i2c_addr;
    i2c_data.reg_addr = addr;
    i2c_data.addr_byte_num= sensor_addr_byte;
    i2c_data.data_byte_num= sensor_data_byte;

    ret = ioctl(fd, CMD_I2C_READ, &i2c_data);
    if(ret)
    {  
        printf("i2c read failed!\n");
        close(fd);
        return -1 ;
    }

	data = i2c_data.data;

	close(fd);
	return data;
#endif		
}


int sensor_write_register(int addr, int data)
{
#ifdef HI_GPIO_I2C
    int fd = -1;
    int ret;
    int value;

    fd = open("/dev/gpioi2c_ov", 0);
    if(fd<0)
    {
        printf("Open gpioi2c_ov error!\n");
        return -1;
    }

    value = ((sensor_i2c_addr&0xff)<<24) | ((addr&0xff)<<16) | (data&0xff);

    ret = ioctl(fd, GPIO_I2C_WRITE, &value);

    if (ret)
    {
        printf("GPIO-I2C write faild!\n");
        close(fd);
        return -1;
    }

    close(fd);
#else
    int fd = -1;
    int ret;
    I2C_DATA_S i2c_data;

    fd = open("/dev/hi_i2c", 0);
    if(fd<0)
    {
        printf("Open hi_i2c error!\n");
        return -1;
    }

    i2c_data.dev_addr = sensor_i2c_addr;
    i2c_data.reg_addr = addr;
    i2c_data.addr_byte_num = sensor_addr_byte;
    i2c_data.data = data;
    i2c_data.data_byte_num = sensor_data_byte;

    ret = ioctl(fd, CMD_I2C_WRITE, &i2c_data);

    if (ret)
    {
        printf("hi_i2c write faild!\n");
        close(fd);
        return -1;
    }

    close(fd);
#endif
	return 0;
}



int sensor_write_register_bit(int addr, int data, int mask)
{
#ifdef HI_GPIO_I2C
    int fd = -1;
    int ret;
    int value;

    fd = open("/dev/gpioi2c_ov", 0);
    if(fd<0)
    {
        printf("Open gpioi2c_ov error!\n");
        return -1;
    }

    value = ((sensor_i2c_addr&0xff)<<24) | ((addr&0xff)<<16);

    ret = ioctl(fd, GPIO_I2C_READ, &value);
    if (ret)
    {
        printf("GPIO-I2C read faild!\n");
        close(fd);
        return -1;
    }

    value &= 0xff;
    value &= ~mask;
    value |= data & mask;

    value = ((sensor_i2c_addr&0xff)<<24) | ((addr&0xff)<<16) | (value&0xff);

    ret = ioctl(fd, GPIO_I2C_WRITE, &value);
    if (ret)
    {
        printf("GPIO-I2C write faild!\n");
        close(fd);
        return -1;
    }

    close(fd);
#else
    int fd = -1;
    int ret;
    int value;
    I2C_DATA_S i2c_data;

    fd = open("/dev/hi_i2c", 0);
    if(fd<0)
    {
        printf("Open hi_i2c error!\n");
        return -1;
    }

    i2c_data.dev_addr = sensor_i2c_addr;
    i2c_data.reg_addr = addr;
    i2c_data.addr_byte_num = sensor_addr_byte;
    i2c_data.data_byte_num = sensor_data_byte;

    ret = ioctl(fd, CMD_I2C_READ, &i2c_data);
    if (ret)
    {
        printf("hi_i2c read faild!\n");
        close(fd);
        return -1;
    }

    value = i2c_data.data;
    value &= ~mask;
    value |= data & mask;

    i2c_data.data = value;

    ret = ioctl(fd, CMD_I2C_WRITE, &i2c_data);
    if (ret)
    {
        printf("hi_i2c write faild!\n");
        close(fd);
        return -1;
    }

    close(fd);
#endif
	return 0;
}


static void delay_ms(int ms)
{
    usleep(ms*1000);
}

void sensor_prog(int* rom)
{
    int i = 0;
    while (1) {
        int lookup = rom[i++];
        int addr = (lookup >> 16) & 0xFFFF;
        int data = lookup & 0xFFFF;
        if (addr == 0xFFFE) {
            delay_ms(data);
        } else if (addr == 0xFFFF) {
            return;
        } else {
			sensor_write_register(addr, data);
        }
    }   
}

void sensor_init()
{
    printf("-------------ov2710 720p 30fps  init start!\n ----------------");
    sensor_write_register(0x3103,0x93);
    sensor_write_register(0x3008,0x82);
    sensor_write_register(0x3017,0x7f);
    sensor_write_register(0x3018,0xfc);
    sensor_write_register(0x3706,0x61);
    sensor_write_register(0x3712,0x0c);
    sensor_write_register(0x3630,0x6d);
    sensor_write_register(0x3801,0xb4);
    sensor_write_register(0x3621,0x04);
    sensor_write_register(0x3604,0x60);
    sensor_write_register(0x3603,0xa7);
    sensor_write_register(0x3631,0x26);
    sensor_write_register(0x3600,0x04);
    sensor_write_register(0x3620,0x37);
    sensor_write_register(0x3623,0x00);
    sensor_write_register(0x3702,0x9e);
    sensor_write_register(0x3703,0x5c);
    sensor_write_register(0x3704,0x40);
    sensor_write_register(0x370d,0x0f);
    sensor_write_register(0x3713,0x9f);
    sensor_write_register(0x3714,0x4c);
    sensor_write_register(0x3710,0x9e);
    sensor_write_register(0x3801,0xc4);
    sensor_write_register(0x3605,0x05);
    sensor_write_register(0x3606,0x3f);
    sensor_write_register(0x302d,0x90);
    sensor_write_register(0x370b,0x40);
    sensor_write_register(0x3716,0x31);
    sensor_write_register(0x3707,0x52);
    sensor_write_register(0x380d,0x74);
    sensor_write_register(0x5181,0x20);
    sensor_write_register(0x518f,0x00);
    sensor_write_register(0x4301,0xff);
    sensor_write_register(0x4303,0x00);
    sensor_write_register(0x3a00,0x78);
    sensor_write_register(0x300f,0x88);
    sensor_write_register(0x3011,0x28);
    sensor_write_register(0x3a1a,0x06);
    sensor_write_register(0x3a18,0x00);
    sensor_write_register(0x3a19,0x7a);
    sensor_write_register(0x3a13,0x54);
    sensor_write_register(0x382e,0x0f);
    sensor_write_register(0x381a,0x1a);
    sensor_write_register(0x401d,0x02);
    sensor_write_register(0x5688,0x03);
    sensor_write_register(0x5684,0x07);
    sensor_write_register(0x5685,0xa0);
    sensor_write_register(0x5686,0x04);
    sensor_write_register(0x5687,0x43);
    sensor_write_register(0x3a0f,0x40);
    sensor_write_register(0x3a10,0x38);
    sensor_write_register(0x3a1b,0x48);
    sensor_write_register(0x3a13,0x30);
    sensor_write_register(0x3a11,0x90);
    sensor_write_register(0x3a1f,0x10);

    //set frame rate
    sensor_write_register(0x3010,0x00);       //0x20,10fps;0x10,15fps;0x00,30fps

    //set AEC/AGC TO mannual model
    sensor_write_register(0x3503,0x07);

    //To close AE set:
    sensor_write_register(0x3503,0x7);
    sensor_write_register(0x3501,0x2e);
    sensor_write_register(0x3502,0x00);
    sensor_write_register(0x350b,0x10);

    //To Close AWB set:
    sensor_write_register(0x3406,0x01);
    sensor_write_register(0x3400,0x04);
    sensor_write_register(0x3401,0x00);
    sensor_write_register(0x3402,0x04);
    sensor_write_register(0x3403,0x00);
    sensor_write_register(0x3404,0x04);
    sensor_write_register(0x3405,0x00);

    //close shading
    sensor_write_register(0x5000,0x5f);   //df bit[8]=0


   printf("-------------ov2710 720p 30fps  init ok!\n ----------------"); 
}


