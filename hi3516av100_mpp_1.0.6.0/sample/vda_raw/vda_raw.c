#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include "sample_comm.h"

#define MAX_VB_BLOCK_NUM  (5)

typedef struct {
    VB_BLK  vb_blk;
    VB_POOL pool_id;
    HI_U32  phyAddr;
    HI_U8   *pVirAddr;
} VB_BLK_INFO;

typedef struct rkVDA_MD_PARAM_S {
    HI_BOOL bThreadStart;
    VDA_CHN VdaChn;
    FILE    *fp_md;
} RK_VDA_MD_PARAM_S;

static pthread_t g_VdaPid[2];
static RK_VDA_MD_PARAM_S g_stMdParam;

/******************************************************************************
* funciton : vda MD mode print -- Md OBJ
******************************************************************************/
static HI_S32 RK_SAMPLE_COMM_VDA_MdPrtObj(FILE* fp, VDA_DATA_S* pstVdaData, HI_U32 frame_cnt)
{
    VDA_OBJ_S* pstVdaObj;
    HI_S32 i;

    //fprintf(fp, "===== %s frame %04d =====\n", __FUNCTION__, frame_cnt);

    if (HI_TRUE != pstVdaData->unData.stMdData.bObjValid) {
        fprintf(fp, "bMbObjValid = FALSE.\n");
        return HI_SUCCESS;
    }

    /*fprintf(fp, "ObjNum=%d, IndexOfMaxObj=%d, SizeOfMaxObj=%d, SizeOfTotalObj=%d\n", \
            pstVdaData->unData.stMdData.stObjData.u32ObjNum, \
            pstVdaData->unData.stMdData.stObjData.u32IndexOfMaxObj, \
            pstVdaData->unData.stMdData.stObjData.u32SizeOfMaxObj, \
            pstVdaData->unData.stMdData.stObjData.u32SizeOfTotalObj);*/
    for (i = 0; i < pstVdaData->unData.stMdData.stObjData.u32ObjNum; i++) {
        pstVdaObj = pstVdaData->unData.stMdData.stObjData.pstAddr + i;
        fprintf(fp, "frame=%d, num=%d, idx=%d, left=%d, top=%d, right=%d, bottom=%d\n", \
                frame_cnt, \
                pstVdaData->unData.stMdData.stObjData.u32ObjNum, i, \
                pstVdaObj->u16Left, pstVdaObj->u16Top, \
                pstVdaObj->u16Right, pstVdaObj->u16Bottom);
    }
    fflush(fp);
    return HI_SUCCESS;
}

/******************************************************************************
* funciton : vda MD mode print -- Alarm Pixel Count
******************************************************************************/
static HI_S32 RK_SAMPLE_COMM_VDA_MdPrtAp(FILE* fp, VDA_DATA_S* pstVdaData, HI_U32 frame_cnt)
{
    fprintf(fp, "===== %s frame %04d =====\n", __FUNCTION__, frame_cnt);

    if (HI_TRUE != pstVdaData->unData.stMdData.bPelsNumValid) {
        fprintf(fp, "bMbObjValid = FALSE.\n");
        return HI_SUCCESS;
    }

    fprintf(fp, "AlarmPixelCount=%d\n", pstVdaData->unData.stMdData.u32AlarmPixCnt);
    fflush(fp);
    return HI_SUCCESS;
}

/******************************************************************************
* funciton : vda MD mode print -- SAD
******************************************************************************/
static HI_S32 RK_SAMPLE_COMM_VDA_MdPrtSad(FILE* fp, VDA_DATA_S* pstVdaData, HI_U32 frame_cnt)
{
    HI_S32 i, j;
    HI_VOID* pAddr;

    fprintf(fp, "===== %s frame %04d =====\n", __FUNCTION__, frame_cnt);
    if (HI_TRUE != pstVdaData->unData.stMdData.bMbSadValid) {
        fprintf(fp, "bMbSadValid = FALSE.\n");
        return HI_SUCCESS;
    }

    for (i = 0; i < pstVdaData->u32MbHeight; i++) {
        pAddr = (HI_VOID*)((HI_U32)pstVdaData->unData.stMdData.stMbSadData.pAddr
                           + i * pstVdaData->unData.stMdData.stMbSadData.u32Stride);

        for (j = 0; j < pstVdaData->u32MbWidth; j++) {
            HI_U8*  pu8Addr;
            HI_U16* pu16Addr;

            if (VDA_MB_SAD_8BIT == pstVdaData->unData.stMdData.stMbSadData.enMbSadBits) {
                pu8Addr = (HI_U8*)pAddr + j;

                fprintf(fp, "%-2d ", *pu8Addr);

            } else {
                pu16Addr = (HI_U16*)pAddr + j;

                fprintf(fp, "%-4d ", *pu16Addr);
            }
        }

        fprintf(fp, "\n");
    }

    fflush(fp);
    return HI_SUCCESS;
}

/******************************************************************************
* funciton : vda MD mode thread process
******************************************************************************/
static HI_VOID* RK_SAMPLE_COMM_VDA_MdGetResult(HI_VOID* pdata)
{
    HI_S32 s32Ret;
    VDA_CHN VdaChn;
    VDA_DATA_S stVdaData;
    RK_VDA_MD_PARAM_S* pgs_stMdParam;
    HI_S32 maxfd = 0;
    FILE* fp = stdout;
    HI_S32 VdaFd;
    fd_set read_fds;
    struct timeval TimeoutVal;
    HI_U32 frame_cnt = 1;

    pgs_stMdParam = (RK_VDA_MD_PARAM_S*)pdata;

    VdaChn = pgs_stMdParam->VdaChn;
    fp = pgs_stMdParam->fp_md;

    /* decide the stream file name, and open file to save stream */
    /* Set Venc Fd. */
    VdaFd = HI_MPI_VDA_GetFd(VdaChn);
    if (VdaFd < 0) {
        SAMPLE_PRT("HI_MPI_VDA_GetFd failed with %#x!\n", VdaFd);
        return NULL;
    }
    if (maxfd <= VdaFd) {
        maxfd = VdaFd;
    }
    system("clear");

    while (HI_TRUE == pgs_stMdParam->bThreadStart) {
        FD_ZERO(&read_fds);
        FD_SET(VdaFd, &read_fds);

        TimeoutVal.tv_sec  = 2;
        TimeoutVal.tv_usec = 0;
        s32Ret = select(maxfd + 1, &read_fds, NULL, NULL, &TimeoutVal);
        if (s32Ret < 0) {
            SAMPLE_PRT("select failed!\n");
            break;
        } else if (s32Ret == 0) {
            SAMPLE_PRT("get vda result time out, exit thread\n");
            break;
        } else {
            if (FD_ISSET(VdaFd, &read_fds)) {
                /*******************************************************
                   step 2.3 : call mpi to get one-frame stream
                   *******************************************************/
                s32Ret = HI_MPI_VDA_GetData(VdaChn, &stVdaData, -1);
                if (s32Ret != HI_SUCCESS) {
                    SAMPLE_PRT("HI_MPI_VDA_GetData failed with %#x!\n", s32Ret);
                    return NULL;
                }

                /*******************************************************
                   *step 2.4 : save frame to file
                   *******************************************************/
                printf("\033[0;0H");/*move cursor*/
                printf("print frame %04d MD info to %p\n", frame_cnt, fp);
                //RK_SAMPLE_COMM_VDA_MdPrtSad(fp, &stVdaData, frame_cnt);
                RK_SAMPLE_COMM_VDA_MdPrtObj(fp, &stVdaData, frame_cnt);
                //RK_SAMPLE_COMM_VDA_MdPrtAp(fp, &stVdaData, frame_cnt);
                frame_cnt++;

                /*******************************************************
                   *step 2.5 : release stream
                   *******************************************************/
                s32Ret = HI_MPI_VDA_ReleaseData(VdaChn, &stVdaData);
                if (s32Ret != HI_SUCCESS) {
                    SAMPLE_PRT("HI_MPI_VDA_ReleaseData failed with %#x!\n", s32Ret);
                    return NULL;
                }
            }
        }
    }

    return HI_NULL;
}

static HI_S32 rk_yuv420to420sp(HI_U8 *src, HI_U8 *dst, HI_U32 width, HI_U32 height)
{
    HI_U32 i, j;
    HI_U32 size = width * height;

    HI_U8 *y = src;
    HI_U8 *u = src + size;
    HI_U8 *v = src + size * 5 / 4;

    HI_U8 *y_tmp  = dst;
    HI_U8 *uv_tmp = dst + size;

    memcpy(y_tmp, y, size);
    for (j = 0, i = 0; j < size / 2; j += 2, i++) {
        uv_tmp[j]   = u[i];
        uv_tmp[j + 1] = v[i];
    }

    return 0;
}

static HI_S32 rk_yuv420spto420(HI_U8 *src, HI_U8 *dst, HI_U32 width, HI_U32 height)
{
    HI_U32 i, j;
    HI_U32 size = width * height;

    HI_U8 *y = src;
    HI_U8 *u = src + size;
    HI_U8 *v = src + size;

    HI_U8 *y_tmp = dst;
    HI_U8 *u_tmp = dst + size;
    HI_U8 *v_tmp = dst + size * 5 / 4;

    memcpy(y_tmp, y, size);
    for (j = 0, i = 0; i < size / 2; i += 2, j++) {
        u_tmp[j] = u[i];
        v_tmp[j] = v[i + 1];
    }

    return 0;
}

int main(int argc, char **argv)
{
    if (argc < 6) {
        SAMPLE_PRT("Usage: ./vda_raw in.yuv width height out.md frame_num\n");
        return -1;
    }

    HI_S32 ret;
    HI_U32 width = atoi(argv[2]);
    HI_U32 height = atoi(argv[3]);
    HI_U32 size = width * height * 3 / 2;
    HI_U32 frames = atoi(argv[5]);
    SAMPLE_PRT("file %s width %d height %d frames %d\n", argv[1], width, height, frames);

    FILE *fp_yuv = NULL;
    fp_yuv = fopen(argv[1], "r");
    if (fp_yuv == NULL) {
        SAMPLE_PRT("failed to open file %s.\n", argv[1]);
        return -1;
    }

    FILE *fp_md = NULL;
    fp_md = fopen(argv[4], "w");
    if (fp_md == NULL) {
        SAMPLE_PRT("failed to open file %s.\n", argv[4]);
        return -1;
    }

    VB_CONF_S vb_s;
    memset(&vb_s, 0, sizeof(VB_CONF_S));
    vb_s.u32MaxPoolCnt = 15;
    vb_s.astCommPool[0].u32BlkSize = 1920 * 1088 * 2;
    vb_s.astCommPool[0].u32BlkCnt = 15;

    memset(vb_s.astCommPool[0].acMmzName, 0, sizeof(vb_s.astCommPool[0].acMmzName));

    ret = HI_MPI_VB_SetConf(&vb_s);
    if (HI_SUCCESS != ret) {
        SAMPLE_PRT("failed to set conf.\n");
        return -1;
    }

    ret = HI_MPI_VB_Init();
    if (HI_SUCCESS != ret) {
        SAMPLE_PRT("failed to init VB ret:%x.\n", ret);
        goto end;
    }

    MPP_SYS_CONF_S sys_s;
    sys_s.u32AlignWidth = 16;
    ret = HI_MPI_SYS_SetConf(&sys_s);
    if (HI_SUCCESS != ret) {
        SAMPLE_PRT("failed to set sys ret:%x.\n", ret);
        goto end;
    }

    ret = HI_MPI_SYS_Init();
    if (HI_SUCCESS != ret) {
        SAMPLE_PRT("failed to init sys ret:%x.\n", ret);
        goto end;
    }

    VDA_CHN_ATTR_S chn_s;
    chn_s.enWorkMode = VDA_WORK_MODE_MD;
    chn_s.u32Width  = width;
    chn_s.u32Height = height;
    chn_s.unAttr.stMdAttr.enVdaAlg = VDA_ALG_BG;
    chn_s.unAttr.stMdAttr.enMbSize = VDA_MB_16PIXEL;
    chn_s.unAttr.stMdAttr.enMbSadBits = VDA_MB_SAD_8BIT;
    chn_s.unAttr.stMdAttr.enRefMode = VDA_REF_MODE_DYNAMIC;
    chn_s.unAttr.stMdAttr.u32VdaIntvl = 0;
    chn_s.unAttr.stMdAttr.u32BgUpSrcWgt = 128;
    chn_s.unAttr.stMdAttr.u32MdBufNum = 8;
    chn_s.unAttr.stMdAttr.u32ObjNumMax = 128;
    chn_s.unAttr.stMdAttr.u32SadTh = 100;

    VDA_CHN chn = 0;
    ret = HI_MPI_VDA_CreateChn(chn, &chn_s);
    if (HI_SUCCESS != ret) {
        SAMPLE_PRT("failed to create vda chn ret:%x.\n", ret);
        goto end;
    }

    ret = HI_MPI_VDA_StartRecvPic(chn);
    if (HI_SUCCESS != ret) {
        SAMPLE_PRT("failed to start recv pic ret:%x.\n", ret);
        goto end;
    }
    SAMPLE_PRT("system init ok\n");

    g_stMdParam.bThreadStart = HI_TRUE;
    g_stMdParam.VdaChn   = 0;
    g_stMdParam.fp_md = fp_md;

    /* create thread to get result */
    pthread_create(&g_VdaPid[0], 0, RK_SAMPLE_COMM_VDA_MdGetResult, (HI_VOID*)&g_stMdParam);

    HI_S32 cnt = 0;
    VB_BLK vb_blk = VB_INVALID_HANDLE;
    HI_U32 phyAddr;
    HI_U8  *buf = NULL;

    buf = (HI_U8 *)malloc(size);
    if (buf == NULL) {
        SAMPLE_PRT("failed to malloc buf.\n");
        goto end;
    }

    HI_U8 idx = 0;
    VB_BLK_INFO blk_info[MAX_VB_BLOCK_NUM];
    VB_BLK_INFO *info = NULL;

    for (idx = 0; idx < MAX_VB_BLOCK_NUM; idx++) {
        info = &blk_info[idx];

        info->vb_blk = HI_MPI_VB_GetBlock(VB_INVALID_POOLID, size, NULL);
        if (info->vb_blk == VB_INVALID_HANDLE) {
            SAMPLE_PRT("failed to get vb block %d\n", info->vb_blk);
            goto end;
        }

        SAMPLE_PRT("get vb_blk %d success\n", info->vb_blk);
        info->pool_id = HI_MPI_VB_Handle2PoolId(info->vb_blk);
        if (info->pool_id == VB_INVALID_POOLID) {
            SAMPLE_PRT("failed to get vb pool id %d\n", info->pool_id);
        }

        info->phyAddr = HI_MPI_VB_Handle2PhysAddr(info->vb_blk);
        if (info->phyAddr == 0) {
            SAMPLE_PRT("failed to get phys addr.\n");
            goto end;
        }

        info->pVirAddr = (HI_U8 *)HI_MPI_SYS_Mmap(info->phyAddr, size);
        if (info->pVirAddr == NULL) {
            SAMPLE_PRT("failed to get virtual addr\n");
            goto end;
        }
    }

    VIDEO_FRAME_INFO_S video_info;

    do {
        memset(&video_info.stVFrame, 0x00, sizeof(VIDEO_FRAME_S));
        idx = cnt % MAX_VB_BLOCK_NUM;
        info = &blk_info[idx];

        video_info.stVFrame.u32Width  = width;
        video_info.stVFrame.u32Height = height;
        video_info.stVFrame.enPixelFormat = PIXEL_FORMAT_YUV_SEMIPLANAR_420;
        video_info.u32PoolId = info->pool_id;
        video_info.stVFrame.u32PhyAddr[0] = info->phyAddr;
        video_info.stVFrame.u32PhyAddr[1] = info->phyAddr + width * height;
        video_info.stVFrame.pVirAddr[0] = info->pVirAddr;
        video_info.stVFrame.pVirAddr[1] = info->pVirAddr + width * height;
        video_info.stVFrame.u32Stride[0] = width;
        video_info.stVFrame.u32Stride[1] = width;
        video_info.stVFrame.u32Field = VIDEO_FIELD_FRAME;
        video_info.stVFrame.enCompressMode = COMPRESS_MODE_NONE;
        video_info.stVFrame.enVideoFormat = VIDEO_FORMAT_LINEAR;
        video_info.stVFrame.u64pts = cnt * 40;
        video_info.stVFrame.u32TimeRef = cnt;

        ret = fread(buf, size, 1, fp_yuv);
        if (ret < 0) {
            SAMPLE_PRT("failed to read yuv %d\n", ret);
            break;
        } else if (ret == 0) {
            SAMPLE_PRT("failed to read yuv %d\n", ret);
            break;
        }

        rk_yuv420to420sp(buf, info->pVirAddr, width, height);

        ret = HI_MPI_VDA_SendPic(chn, &video_info, -1);
        if (HI_SUCCESS != ret) {
            SAMPLE_PRT("failed to send pic ret:%x.\n", ret);
            goto end;
        } else {
            SAMPLE_PRT("chn %d send frame %d success\n", chn, cnt++);
        }
    } while (cnt < frames);

    SAMPLE_PRT("thread id %lu, current thread id %lu\n", g_VdaPid[0], pthread_self());

    void *dummy;
    pthread_join(g_VdaPid[0], &dummy);

end:
    for (idx = 0; idx < MAX_VB_BLOCK_NUM; idx++) {
        info = &blk_info[idx];

        if (info->pVirAddr)
            HI_MPI_SYS_Munmap(info->pVirAddr, size);

        (void)HI_MPI_VB_ReleaseBlock(info->vb_blk);
    }

    (void)HI_MPI_SYS_Exit();

    (void)HI_MPI_VB_Exit();

    if (buf)
        free(buf);

    return 0;
}
