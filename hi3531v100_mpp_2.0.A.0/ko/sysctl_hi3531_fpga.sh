#!/bin/sh

# mddrc0 pri&timeout setting
himm 0x20110150  0x03ff6         #DMA1 DMA2
himm 0x20110154  0x03ff6         #ETH/GMAC0 GMAC1 GMAC
himm 0x20110158  0x03ff6         #SCD0 SCD1 CIPHER 
himm 0x2011015c  0x03ff6         #IVE
himm 0x20110160  0x03ff6         #PCIE0 PCIE1
himm 0x20110164  0x03ff6         #SATA SDIO NANDC USB1
himm 0x20110168  0x03ff6         #A9/(Hi3532涓篈RMI) 
himm 0x2011016c  0x03ff6         #ARMD JPGD JPGE
himm 0x20110170  0x03ff6         #MD 
himm 0x20110174  0x03ff6         #VOIE
himm 0x20110178  0x03ff3         #VDH0 BPD
himm 0x2011017c  0x10c02         #VENC0 VENC1
himm 0x20110180  0x03ff4         #VCMP TDE0 TDE1
himm 0x20110184  0x03ff2         #VPSS0 VPSS1
himm 0x20110188  0x10101         #VICAP
himm 0x2011018c  0x10200         #VDP

#mddrc order control idmap_mode
himm 0x20110100 0xe7      #mddrc order enable mddrc idmap mode select
himm 0x20110020 0x784     #双ddr操作挂死问题规避

himm 0x200500d8 0x3             #DDR0只使能VICAP和VDP乱序


# mddrc1 pri&timeout setting
himm 0x20120150  0x03ff6         #DMA1 DMA2
himm 0x20120154  0x03ff6         #ETH/GMAC0 GMAC1 GMAC
himm 0x20120158  0x03ff6         #SCD0 SCD1 CIPHER 
himm 0x2012015c  0x03ff6         #IVE
himm 0x20120160  0x03ff6         #PCIE0 PCIE1
himm 0x20120164  0x03ff6         #SATA SDIO NANDC USB1
himm 0x20120168  0x03ff6         #A9/(Hi3532涓篈RMI) 
himm 0x2012016c  0x03ff6         #ARMD JPGD JPGE
himm 0x20120170  0x03ff6         #MD 
himm 0x20120174  0x03ff6         #VOIE
himm 0x20120178  0x03ff3         #VDH0 BPD
himm 0x2012017c  0x10c02         #VENC0 VENC1
himm 0x20120180  0x03ff4         #VCMP TDE0 TDE1
himm 0x20120184  0x03ff2         #VPSS0 VPSS1
himm 0x20120188  0x10101         #VICAP
himm 0x2012018c  0x10200         #VDP

#mddrc order control idmap_mode
himm 0x20120100 0xe7      #mddrc order enable mddrc idmap mode select
himm 0x20120020 0x784     #双ddr操作挂死问题规避

himm 0x200500dc 0x3              #DDR1只使能VICAP和VDP乱序


#outstanding
#vdp, 2
himm 0x205CCE00 0x80021220
#vicap, 2
himm 0x20580004 0x00020002
#vpss0, 2 - 1 = 1
himm 0x20600314 0x00640011
#venc0, 4 - 1 = 3
himm 0x206200A4 0x3
#tde0, 2
himm 0x20610844 0x80220000
#tde1, 2
himm 0x20611844 0x80220000
#vdh0 6 - 1 = 5
himm 0x20630030 0x00015500
#bpd 
himm 0x206d0030 0x00000330
#jpgd,3532没有这个地址，注释掉。 
#himm 0x10170040 0x00000070
