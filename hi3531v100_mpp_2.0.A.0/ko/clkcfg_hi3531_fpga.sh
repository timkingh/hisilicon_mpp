
clk_cfg_hi3531fpga()
{
	#himm 0x20030044 0x2;	#3532IVE撤销复位
	#himm 0x2003006c 0x2;	#3531IVE撤销复位
	himm 0x20030040 0x0;	#3531 VEDU0撤销复位
	himm 0x20030060 0x0;	#3531 JPGE撤销复位
	himm 0x20030068 0x0;	#3531 MDU撤销复位
	himm 0x20030074 0x0;	#3531 VCMP撤销复位
	himm 0x20030058 0x0;	#3531 TDE撤销复位
	himm 0x20030048 0x0;	#3531 VPSS0撤销复位
	himm 0x20030050 0x0;	#3531 VDH0撤销复位
	himm 0x20030064 0x0;	#3531 JPGD0撤销复位
#	himm 0x2003002c 0xf000; #3531 VICAP的BT1120端口时钟反相

	himm 0x20030038 0x00040010  #VDP BT1120时钟反向
	himm 0x20030034 0xC0;   #VDP时钟反向
}


clk_cfg_hi3531fpga
