#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <sys/poll.h>
#include <sys/time.h>
#include <sys/mman.h>
#include <fcntl.h>
#include <errno.h>
#include <pthread.h>
#include <signal.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>


#include "hi_common.h"
#include "hi_comm_video.h"
#include "hi_comm_sys.h"
#include "hi_comm_vi.h"
#include "hi_comm_vo.h"
#include "mpi_vb.h"
#include "mpi_sys.h"
#include "mpi_vi.h"
#include "mpi_vo.h"

#include "hi_comm_isp.h"
#include "mpi_isp.h"
#include "hi_sns_ctrl.h"

#include "mpi_af.h"
#include "mpi_awb.h"
#include "mpi_ae.h"


#define MAX_FRM_CNT     256



#define ALIGN_BACK(x, a)              ((a) * (((x) / (a))))

static volatile HI_BOOL bQuit = HI_FALSE;   /* bQuit may be set in the signal handler */
#if 1
static HI_S32 s_s32MemDev = -1;

#define MEM_DEV_OPEN() \
    do {\
        if (s_s32MemDev <= 0)\
        {\
            s_s32MemDev = open("/dev/mem", O_CREAT|O_RDWR|O_SYNC);\
            if (s_s32MemDev < 0)\
            {\
                perror("Open dev/mem error");\
                return -1;\
            }\
        }\
    }while(0)

#define MEM_DEV_CLOSE() \
    do {\
        HI_S32 s32Ret;\
        if (s_s32MemDev > 0)\
        {\
            s32Ret = close(s_s32MemDev);\
            if(HI_SUCCESS != s32Ret)\
            {\
                perror("Close mem/dev Fail");\
                return s32Ret;\
            }\
            s_s32MemDev = -1;\
        }\
    }while(0)

HI_VOID usage(HI_VOID)
{
    printf(
        "\n"
        "*************************************************\n"
        "Usage: ./vi_bayerdump [nbit] [FrmCnt] [VCNum]\n"
        "nbit: \n"
        "   The bit num to be dump, 8bit/10bit/12bit/14bit/16bit\n"
        "FrmCnt: \n"
        "   the count of frame to be dump\n"
        "VCNum: \n"
        "   line mode: -1\n"
        "   wdr mode : 0, 1 the frame vc number,long or short frame\n"
        "e.g : ./vi_bayerdump 16 1 -1(line mode)\n"
        "e.g : ./vi_bayerdump 16 1 0(wdr mode:VC0)\n"
        "e.g : ./vi_bayerdump 16 1 1(wdr mode:VC1)\n"
        "*************************************************\n"
        "\n");

}

HI_VOID* COMM_SYS_Mmap(HI_U32 u32PhyAddr, HI_U32 u32Size)
{
    HI_U32 u32Diff;
    HI_U32 u32PagePhy;
    HI_U32 u32PageSize;
    HI_U8* pPageAddr;

    /* The mmap address should align with page */
    u32PagePhy = u32PhyAddr & 0xfffff000;
    u32Diff    = u32PhyAddr - u32PagePhy;

    /* The mmap size shuld be mutliples of 1024 */
    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000) + 0x1000;
    pPageAddr   = mmap ((void*)0, u32PageSize, PROT_READ | PROT_WRITE,
                        MAP_SHARED, s_s32MemDev, u32PagePhy);
    if (MAP_FAILED == pPageAddr )
    {
        perror("mmap error");
        return NULL;
    }
    return (HI_VOID*) (pPageAddr + u32Diff);
}

HI_S32 COMM_SYS_Munmap(HI_VOID* pVirAddr, HI_U32 u32Size)
{
    HI_U32 u32PageAddr;
    HI_U32 u32PageSize;
    HI_U32 u32Diff;

    u32PageAddr = (((HI_U32)pVirAddr) & 0xfffff000);
    u32Diff     = (HI_U32)pVirAddr - u32PageAddr;
    u32PageSize = ((u32Size + u32Diff - 1) & 0xfffff000) + 0x1000;

    return munmap((HI_VOID*)u32PageAddr, u32PageSize);
}
#endif

static HI_VOID SigHandler(HI_S32 signo)
{
    if (HI_TRUE == bQuit)
    {
        return;
    }
    
    if (SIGINT == signo || SIGTERM == signo)
    {
        bQuit = HI_TRUE;
        fclose(stdin);  /* close stdin, so getchar will return EOF */
    }
}

static inline HI_S32 bitWidth2PixelFormat(HI_U32 u32Nbit, PIXEL_FORMAT_E *penPixelFormat)
{
    PIXEL_FORMAT_E enPixelFormat;
    
    if (8 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_8BPP;
    }
    else if (10 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_10BPP;
    }
    else if (12 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_12BPP;
    }
    else if (14 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER_14BPP;
    }
    else if (16 == u32Nbit)
    {
        enPixelFormat = PIXEL_FORMAT_RGB_BAYER;
    }
    else
    {
        return HI_FAILURE;
    }

    *penPixelFormat = enPixelFormat;
    return HI_SUCCESS;
}

/*
 * Convert the input compact pixel data to 16bit data
 * support input 10bit/12bit/14bit
 * u32DataNum must align to 4
 * pu8Data:     input data, bit width is u32BitWidth
 * u32DataNum:  input pixel number
 * pu16OutData: save result data
 *
 * RETURN:      return the converted pixel number
 * */
HI_S32 convertBitPixel(HI_U8 *pu8Data, HI_U32 u32DataNum, HI_U32 u32BitWidth, HI_U16 *pu16OutData)
{
    HI_S32 i, u32Tmp, s32OutCnt;
    HI_U32 u32Val;
    HI_U64 u64Val;
    HI_U8 *pu8Tmp = pu8Data;
    
    s32OutCnt = 0;
    switch(u32BitWidth) 
    {
    case 10:
        {
            /* 4 pixels consist of 5 bytes  */
            u32Tmp = u32DataNum / 4;

            for (i = 0; i < u32Tmp; i++)
            {
                /* byte4 byte3 byte2 byte1 byte0 */
                pu8Tmp = pu8Data + 5 * i;
                u64Val = pu8Tmp[0] + ((HI_U32)pu8Tmp[1] << 8) + ((HI_U32)pu8Tmp[2] << 16) + 
                         ((HI_U32)pu8Tmp[3] << 24) + ((HI_U64)pu8Tmp[4] << 32);
                
                pu16OutData[s32OutCnt++] = u64Val & 0x3ff;
                pu16OutData[s32OutCnt++] = (u64Val >> 10) & 0x3ff;
                pu16OutData[s32OutCnt++] = (u64Val >> 20) & 0x3ff;
                pu16OutData[s32OutCnt++] = (u64Val >> 30) & 0x3ff;
            }
        }
        break;
    case 12:
        {
            /* 2 pixels consist of 3 bytes  */
            u32Tmp = u32DataNum / 2;

            for (i = 0; i < u32Tmp; i++)
            {
                /* byte2 byte1 byte0 */
                pu8Tmp = pu8Data + 3 * i;
                u32Val = pu8Tmp[0] + (pu8Tmp[1] << 8) + (pu8Tmp[2] << 16);
                pu16OutData[s32OutCnt++] = u32Val & 0xfff;
                pu16OutData[s32OutCnt++] = (u32Val >> 12) & 0xfff;
            }
        }
        break;
    case 14:
        {
            /* 4 pixels consist of 7 bytes  */
            u32Tmp = u32DataNum / 4;

            for (i = 0; i < u32Tmp; i++)
            {
                pu8Tmp = pu8Data + 7 * i;
                u64Val = pu8Tmp[0] + ((HI_U32)pu8Tmp[1] << 8) + ((HI_U32)pu8Tmp[2] << 16) + 
                         ((HI_U32)pu8Tmp[3] << 24) + ((HI_U64)pu8Tmp[4] << 32) + 
                         ((HI_U64)pu8Tmp[5] << 40) + ((HI_U64)pu8Tmp[6] << 48);

                pu16OutData[s32OutCnt++] = u64Val & 0x3fff;
                pu16OutData[s32OutCnt++] = (u64Val >> 14) & 0x3fff;
                pu16OutData[s32OutCnt++] = (u64Val >> 28) & 0x3fff;
                pu16OutData[s32OutCnt++] = (u64Val >> 42) & 0x3fff;
            }
        }
        break;
    default:
        fprintf(stderr, "unsuport bitWidth: %d\n", u32BitWidth);
        return -1;
        break;
    }

    return s32OutCnt;
}

HI_S32 sample_bayer_dump(VIDEO_FRAME_S* pVBuf, HI_U32 u32Nbit, FILE* pfd)
{
    HI_U16 *pu16Data = NULL;
    HI_U32 phy_addr, size;
    HI_U8* pUserPageAddr[2];
    HI_U8  *pu8Data;
    PIXEL_FORMAT_E enPixelFormat = PIXEL_FORMAT_BUTT;

    bitWidth2PixelFormat(u32Nbit, &enPixelFormat);
    if (enPixelFormat != pVBuf->enPixelFormat)
    {
        fprintf(stderr, "invalid pixel format:%d, u32Nbit: %d\n", pVBuf->enPixelFormat, u32Nbit);
        return HI_FAILURE;
    }

    size = (pVBuf->u32Stride[0]) * (pVBuf->u32Height);
    phy_addr = pVBuf->u32PhyAddr[0];

    MEM_DEV_OPEN();

    pUserPageAddr[0] = (HI_U8*) COMM_SYS_Mmap(phy_addr, size);
    if (NULL == pUserPageAddr[0])
    {
        MEM_DEV_CLOSE();
        return -1;
    }
    
    pu8Data = pUserPageAddr[0];
    if ((8 != u32Nbit) && (16 != u32Nbit))
    {
        pu16Data = (HI_U16*)malloc(pVBuf->u32Width * 2);
        if (NULL == pu16Data)
        {
            fprintf(stderr, "alloc memory failed\n");
            COMM_SYS_Munmap(pUserPageAddr[0], size);            
            MEM_DEV_CLOSE();
            return HI_FAILURE;
        }
    }

    /* save Y ----------------------------------------------------------------*/
    fprintf(stderr, "saving......dump data......u32Stride[0]: %d, width: %d\n", pVBuf->u32Stride[0], pVBuf->u32Width);
    fflush(stderr);

    if (8 == u32Nbit || 16 == u32Nbit)
    {
        HI_U32 u32H;
        // process unit: line
        for (u32H = 0; u32H < pVBuf->u32Height; u32H++)
        {
            fwrite(pu8Data, pVBuf->u32Width, u32Nbit / 8, pfd);
            pu8Data += (pVBuf->u32Width * (u32Nbit / 8));
        }
    }
    else
    {
        HI_U32 u32ProcessedNum = 0;
        HI_U32 u32PixNum, u32TotalPixNum;
        // process unit: pixel
        // as width and height must align to 2, so u32TotalPixNum must align to 4
        u32TotalPixNum = pVBuf->u32Height * pVBuf->u32Width;  
        do
        {
            u32PixNum = MIN2((pVBuf->u32Width / 4) * 4, u32TotalPixNum - u32ProcessedNum);
            convertBitPixel(pu8Data, u32PixNum, u32Nbit, pu16Data);
            fwrite(pu16Data, u32PixNum, 2, pfd);
            u32ProcessedNum += u32PixNum;
            if (10 == u32Nbit)
            {
                pu8Data += (u32PixNum / 4) * 5;
            }
            else if (12 == u32Nbit)
            {
                pu8Data += (u32PixNum / 2) * 3;
            }
            else if (14 == u32Nbit)
            {
                pu8Data += (u32PixNum / 4) * 7;
            }            
        }while(u32ProcessedNum < u32TotalPixNum);
    }

    fflush(pfd);

    fprintf(stderr, "done u32TimeRef: %d!\n", pVBuf->u32TimeRef);
    fflush(stderr);

    if (NULL != pu16Data)
    {
        free(pu16Data);
    }
    COMM_SYS_Munmap(pUserPageAddr[0], size);

    MEM_DEV_CLOSE();

    return HI_SUCCESS;
}

HI_S32 VI_DumpBayer(VI_DEV ViDev, VI_DUMP_ATTR_S* pstViDumpAttr, HI_U32 u32Nbit, HI_U32 u32Cnt, HI_S32 s32VcNum)
{
    HI_S32 j;
    VI_FRAME_INFO_S stFrame;
    VI_FRAME_INFO_S astFrame[MAX_FRM_CNT];
    HI_CHAR szYuvName[128] = {0};
    FILE* pfd = NULL;
    HI_S32 s32MilliSec = 2000;
    VI_CHN ViChn;
    HI_S32 s32CapCnt = 0;
    HI_U32 u32Width, u32Height;
    HI_BOOL bGetFrame = HI_FALSE;

    VIU_GET_RAW_CHN(ViDev, ViChn);

    if (HI_MPI_VI_SetFrameDepth(ViChn, 1))
    {
        printf("HI_MPI_VI_SetFrameDepth err, vi chn %d \n", ViChn);
        return HI_FAILURE;
    }

    usleep(5000);
    
    {
        /* waiting for the matching pixel format */
        HI_S32 s32Tries = 10;
        while(s32Tries > 0)
        {
            s32Tries--;
            if (HI_TRUE == bQuit)
            {
                break;
            }
            if (HI_MPI_VI_GetFrame(ViChn, &stFrame.stViFrmInfo, s32MilliSec))
            {
                printf("HI_MPI_VI_GetFrame err, vi chn %d \n", ViChn);
                break;
            }
            if (pstViDumpAttr->enPixelFormat == stFrame.stViFrmInfo.stVFrame.enPixelFormat)
            {
                bGetFrame = HI_TRUE;
                break;
            }
            HI_MPI_VI_ReleaseFrame(ViChn, &stFrame);
            usleep(30000);
        }
        if (HI_TRUE != bGetFrame)
        {
            fprintf(stderr, "get raw data frame timeout!\n");
            return HI_FAILURE;
        }
    }
    u32Width  = stFrame.stViFrmInfo.stVFrame.u32Width;
    u32Height = stFrame.stViFrmInfo.stVFrame.u32Height;

    /* get VI frame  */
    for (j = 0; j < u32Cnt; j++)
    {
        if (HI_TRUE == bQuit)
        {
            break;
        }
        
        if (HI_MPI_VI_GetFrame(ViChn, &astFrame[j].stViFrmInfo, s32MilliSec) < 0)
        {
            printf("get vi chn %d frame err\n", ViChn);
            printf("only get %d frame\n", j);
            break;
        }
    }
    HI_MPI_VI_ReleaseFrame(ViChn, &stFrame.stViFrmInfo);
    s32CapCnt = j;
    if ((HI_TRUE == bQuit) || (0 >= s32CapCnt))
    {
        goto exit;
    }

    /* make file name */
    if (-1 == s32VcNum)
    {
        sprintf(szYuvName, "./vi_dev_%d_%d_%d_%d_%dbits.raw", ViDev, u32Width, u32Height, 
                    s32CapCnt, u32Nbit);
    }
    else
    {
        sprintf(szYuvName, "./vi_dev_%d_%d_%d_%d_%dbits_vc%d.raw", ViDev, u32Width, u32Height, 
                    s32CapCnt, u32Nbit, s32VcNum);
    }
    printf("Dump raw frame of vi chn %d to file: \"%s\"\n", ViChn, szYuvName);

    /* open file */
    pfd = fopen(szYuvName, "wb");
    if (NULL == pfd)
    {
        printf("open file failed:%s!\n", strerror(errno));
        goto exit;
    }

    for (j = 0; j < s32CapCnt; j++)
    {
        if (HI_TRUE == bQuit)
        {
            break;
        }
        /* save VI frame to file */
        sample_bayer_dump(&astFrame[j].stViFrmInfo.stVFrame, u32Nbit, pfd);
    }

exit:
    for (j = 0; j < s32CapCnt; j++)
    {
        /* release frame after using */
        HI_MPI_VI_ReleaseFrame(ViChn, &astFrame[j].stViFrmInfo);
    }
    if (NULL != pfd)
    {
        fclose(pfd);
    }

    return 0;
}

HI_S32 main(int argc, char* argv[])
{
    VI_DEV ViDev = 0;
    HI_S32 s32Ret = 0;
    HI_U32 u32Nbit = 8;
    HI_U32 u32FrmCnt = 1;
    HI_S32 s32VcNum = -1;
    HI_U32 u32DesIdAddr = 0x20580210;
    HI_U32 u32BackupValue;
    VI_DUMP_ATTR_S stDumpBkAttr = {0}, stDumpAttr = {0};
    PIXEL_FORMAT_E enPixelFormat;
	
    printf("\nNOTICE: This tool only can be used for TESTING !!!\n");
    if (argc < 2)
    {
        usage();
        return HI_FAILURE;
    }

    s32Ret = HI_MPI_SYS_GetReg(u32DesIdAddr, &u32BackupValue);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_SYS_GetReg failed, s32Ret = %d!\n", s32Ret);
        return s32Ret;
    }

    if (0 == u32BackupValue)
    {
        s32VcNum = -1;
    }
    else if (1 == u32BackupValue)
    {
        s32VcNum = 0;
    }
    else if (2 == u32BackupValue)
    {
        s32VcNum = 1;
    }
    else
    {
        s32VcNum = -1;
    }

    if (argc > 1)
    {
        u32Nbit = atoi(argv[1]);    /* nbit of Raw data:8bit;10bit;12bit */
    }
    if (argc > 2)
    {
        u32FrmCnt = atoi(argv[2]);  /* the frame number */
    }
    if (argc > 3)
    {
        s32VcNum = atoi(argv[3]);   /* long or short frame */
    }

    s32Ret = bitWidth2PixelFormat(u32Nbit, &enPixelFormat);
    if (HI_SUCCESS != s32Ret)
    {
        printf("can't not support %d bits, only support 8bits,10bits,12bits,14bits,16bits\n", u32Nbit);
        exit(HI_FAILURE);
    }
    
    if (1 > u32FrmCnt || MAX_FRM_CNT < u32FrmCnt)
    {
        printf("invalid FrmCnt %d, FrmCnt range from 1 to %d\n", u32FrmCnt, MAX_FRM_CNT);
        exit(HI_FAILURE);
    }
    
    /* backup and set dump attr */
    s32Ret = HI_MPI_VI_GetDevDumpAttr(ViDev, &stDumpBkAttr);
    if (HI_SUCCESS != s32Ret)
    {
        fprintf(stderr, "HI_MPI_VI_GetDevDumpAttr failed(0x%x)\n", s32Ret);
        return s32Ret;
    }
    stDumpAttr.enDumpType = VI_DUMP_TYPE_RAW;
    stDumpAttr.enPixelFormat = enPixelFormat;
    s32Ret = HI_MPI_VI_SetDevDumpAttr(ViDev, &stDumpAttr);
    if (HI_SUCCESS != s32Ret)
    {
        fprintf(stderr, "HI_MPI_VI_SetDevDumpAttr failed(0x%x)\n", s32Ret);
        return s32Ret;
    }

    s32Ret = HI_MPI_VI_EnableBayerDump(ViDev);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_EnableBayerDump failed(0x%x)!\n", s32Ret);
        HI_MPI_VI_SetDevDumpAttr(ViDev, &stDumpBkAttr);
        goto EXIT;
    }

    if (-1 == s32VcNum)
    {
        s32Ret = HI_MPI_SYS_SetReg(u32DesIdAddr, 0x1);
    }
    else if (0 == s32VcNum)
    {
        s32Ret = HI_MPI_SYS_SetReg(u32DesIdAddr, 0x1);
    }
    else if (1 == s32VcNum)
    {
        s32Ret = HI_MPI_SYS_SetReg(u32DesIdAddr, 0x2);
    }
    else
    {
        printf("Invalid VC num!\n");
        goto EXIT;
    }

    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_SYS_SetReg failed, s32Ret = 0x%x!\n", s32Ret);
        goto EXIT;
    }
    
    /* register signal handler */
    signal(SIGINT, SigHandler);
    signal(SIGTERM, SigHandler);

    printf("===press the ENTER key to continue.\n");
    getchar();
    if (HI_TRUE == bQuit)
    {
        goto EXIT;
    }

    s32Ret = VI_DumpBayer(ViDev, &stDumpAttr, u32Nbit, u32FrmCnt, s32VcNum);
    if (HI_SUCCESS != s32Ret)
    {
        printf("VI_StartBayerData failed!\n");
        goto EXIT;
    }

EXIT:
    s32Ret = HI_MPI_VI_DisableBayerDump(ViDev);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_VI_DisableBayerDump failed!\n");
    }
    HI_MPI_VI_SetDevDumpAttr(ViDev, &stDumpBkAttr);

    s32Ret = HI_MPI_SYS_SetReg(u32DesIdAddr, u32BackupValue);
    if (HI_SUCCESS != s32Ret)
    {
        printf("HI_MPI_SYS_SetReg failed, s32Ret = 0x%x!\n", s32Ret);
    }

    return s32Ret;
}

