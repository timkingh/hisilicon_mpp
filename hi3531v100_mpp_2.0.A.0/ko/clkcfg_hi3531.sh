#clk_close()
#{
#
#}

clk_cfg()
{
	#himm 0x2003002C 0xFFFF0000;		#3531 VICAP时钟门控、相位、软复位
	#himm 0x20030030 0x002AAAA0;		#3531 VICAP时钟源、总线时钟门控、div时钟选择
	#himm 0x20030034 0x7fe13fd0;		#3531 VOU时钟全开
	#himm 0x20030038 0x00010000;		#3531 VOU撤销复位
	#himm 0x2003003c 0x0000004c;		#3531 HDMI时钟/撤销复位
	#himm 0x20030040 0x2;	#3531 VEDU0撤销复位
	#himm 0x20030044 0x2;	#3531 VEDU1撤销复位
	#himm 0x20030048 0x2;	#3531 VPSS0撤销复位
	#himm 0x2003004c 0x2;	#3531 VPSS1撤销复位
	himm 0x20030050 0x2;	#3531 VDH0撤销复位
	himm 0x20030054 0x2;	#3531 VDH1撤销复位
	#himm 0x20030058 0x2;	#3531 TDE撤销复位
	#himm 0x20030060 0x2;	#3531 JPGE撤销复位
	himm 0x20030064 0x2;	#3531 JPGD撤销复位
	#himm 0x20030068 0x2;	#3531 MDU撤销复位
	#himm 0x20030074 0x2;	#3531 VCMP撤销复位
	himm 0x200300E0 0x2;	#3531 DMA撤销复位
	#himm 0x2003006C 0x2;	#3531 IVE撤销复位
}


#clk_close
clk_cfg
#clk_cfg_hi3531fpga
