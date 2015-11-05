#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>
#include <dos/dos_mem.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_DISK_MONITOR

#include <fcntl.h>
#include <mntent.h>
#include <sys/vfs.h>
#include "mon_get_disk_info.h"
#include "mon_warning_msg_queue.h"
#include "mon_lib.h"
#include "mon_def.h"

extern  MON_SYS_PART_DATA_S * g_pastPartition[MAX_PARTITION_COUNT];
extern  U32 g_ulPartCnt;
extern  MON_THRESHOLD_S *g_pstCond;
extern  MON_WARNING_MSG_S *g_pstWarningMsg;

U32  mon_get_disk_serial_num(S8 *pszPartitionName, S8 *pszSerialNum, U32 ulLen);
static U32  mon_partition_reset_data();

/**
 * ����:Ϊ������Ϣ��������ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_disk_malloc()
{
    U32 ulRows = 0;
    MON_SYS_PART_DATA_S * pstPartition = NULL;

    pstPartition = (MON_SYS_PART_DATA_S *)dos_dmem_alloc(MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));
    if(!pstPartition)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(pstPartition, MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));

    for(ulRows = 0; ulRows < MAX_PARTITION_COUNT; ulRows++)
    {
        g_pastPartition[ulRows] = &(pstPartition[ulRows]);
    }

    return DOS_SUCC;
}

/**
 * ����:�ͷ�Ϊ������Ϣ���������ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_disk_free()
{
    U32 ulRows = 0;
    MON_SYS_PART_DATA_S * pstPartition = g_pastPartition[0];
    if(DOS_ADDR_INVALID(pstPartition))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_dmem_free(pstPartition);

    for(ulRows = 0 ; ulRows < MAX_PARTITION_COUNT; ulRows++)
    {
        g_pastPartition[ulRows] = NULL;
    }

    pstPartition = NULL;

    return DOS_SUCC;
}

static U32  mon_partition_reset_data()
{
    MON_SYS_PART_DATA_S * pstPartition = g_pastPartition[0];

    if(DOS_ADDR_INVALID(pstPartition))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_memzero(pstPartition, MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));
    return DOS_SUCC;
}

/**
 * ����:��ȡ���̵����к���Ϣ�ַ���
 * ��������
 *   ����1: S8 * pszPartitionName ������
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_get_disk_serial_num(S8 *pszPartitionName, S8 *pszSerialNum, U32 ulLen)
{
    if (DOS_ADDR_INVALID(pszPartitionName)
        || DOS_ADDR_INVALID(pszSerialNum)
        || 0 == ulLen)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_snprintf(pszSerialNum, ulLen, "%s", "IC35L180AVV207-1");

    return DOS_SUCC;
}

/* ������鿴statfs�������÷� */
U32 mon_get_partition_data()
{
    FILE  *fpMountTable = NULL;
    struct mntent *pstMountEntry = NULL;
    struct statfs stFileSys;
    const S8  *pszName = NULL, *pszMountPoint = NULL;
    U32  ulRet = U32_BUTT;
    U64  LUsed = 0, LTotal = 0;

    fpMountTable = setmntent("/etc/mtab", "r");
    if (DOS_ADDR_INVALID(fpMountTable))
    {
        DOS_ASSERT(0);
    }

    mon_partition_reset_data();

    g_ulPartCnt = 0;
    while (1)
    {
        pstMountEntry = getmntent(fpMountTable);
        if (!pstMountEntry)
        {
            endmntent(fpMountTable);
            fpMountTable = NULL;
            break;
        }
        pszName = pstMountEntry->mnt_fsname;
        /* ���˵�����'/'��ͷ��������������'/'��ͷ�� */
        if ('/' != *pszName || ('/' == *pszName && '/' == *(pszName + 1)))
        {
            continue;
        }
        pszMountPoint = pstMountEntry->mnt_dir;
        if (statfs(pszMountPoint, &stFileSys) < 0)
        {
            DOS_ASSERT(0);
            mon_trace(MON_TRACE_DISK, LOG_LEVEL_ERROR, "statfs FAIL. MountPoint:%s, errno:%d, cause:%s.", pszMountPoint, errno, strerror(errno));
            continue;
        }

        /* �������� */
        dos_snprintf(g_pastPartition[g_ulPartCnt]->szPartitionName, MAX_PARTITION_LENGTH, "%s", pstMountEntry->mnt_fsname);

        g_pastPartition[g_ulPartCnt]->uLPartitionTotalBytes = stFileSys.f_blocks * stFileSys.f_bsize;
        g_pastPartition[g_ulPartCnt]->uLPartitionAvailBytes = stFileSys.f_bavail * stFileSys.f_bsize;
        g_pastPartition[g_ulPartCnt]->uLPartitionUsedBytes = (stFileSys.f_blocks - stFileSys.f_bavail) * stFileSys.f_bsize;

        LUsed = g_pastPartition[g_ulPartCnt]->uLPartitionUsedBytes * 100;
        LTotal = g_pastPartition[g_ulPartCnt]->uLPartitionTotalBytes;
        g_pastPartition[g_ulPartCnt]->uLPartitionUsageRate = (LUsed + LUsed % LTotal) / LTotal;

        ulRet = mon_get_disk_serial_num(g_pastPartition[g_ulPartCnt]->szPartitionName, g_pastPartition[g_ulPartCnt]->szDiskSerialNo, MAX_PARTITION_LENGTH);
        if (DOS_SUCC != ulRet)
        {
            DOS_ASSERT(0);
            endmntent(fpMountTable);
            fpMountTable = NULL;
            return DOS_FAIL;
        }
        ++g_ulPartCnt;
    }

    endmntent(fpMountTable);
    fpMountTable = NULL;
    return DOS_SUCC;
 }

/**
 * ����:��ȡ���̵���ռ����
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����ռ���ʴ�С��ʧ�ܷ���DOS_FAIL
 */
U32  mon_get_total_disk_usage_rate()
{
    U32 ulRows = 0;
    U64 LTotal = 0, LUsed = 0;

    for (ulRows = 0; ulRows < g_ulPartCnt; ++ulRows)
    {
        LTotal += g_pastPartition[ulRows]->uLPartitionTotalBytes;
        LUsed += g_pastPartition[ulRows]->uLPartitionUsedBytes;
    }

    LUsed *= 100;

    return (LUsed + LUsed % LTotal) / LTotal;
}

/**
 * ����:��ȡ���̵����ֽ���
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ����ش������ֽ���(KBytes)��ʧ�ܷ���DOS_FAIL
 */
U64 mon_get_total_disk_bytes()
{
    U64  uLTotal = 0;
    U32  ulRows = 0;

    for (ulRows = 0; ulRows < g_ulPartCnt; ++ulRows)
    {
        uLTotal += g_pastPartition[ulRows]->uLPartitionTotalBytes;
    }

    return uLTotal;
}

U32  mon_handle_disk_warning()
{
    BOOL bAddToDB = DOS_FALSE, bDiskExcept = DOS_FALSE;
    U32  ulRet = U32_BUTT, ulIndex = U32_BUTT, ulRows = 0;
    U32  ulTotalDiskRate = U32_BUTT;
    MON_MSG_S  *pstMsg = NULL;

    ulRet = mon_generate_warning_id(DISK_RES, 0x00, RES_LACK);
    if ((U32)0xff == ulRet)
    {
        mon_trace(MON_TRACE_MH, LOG_LEVEL_ERROR, "Generate Warning ID FAIL.");
        return DOS_FAIL;
    }

    ulIndex = mon_get_msg_index(ulRet);
    if (U32_BUTT == ulIndex)
    {
        return DOS_FAIL;
    }

    for (ulRows = 0; ulRows < g_ulPartCnt; ++ulRows)
    {
        if(g_pastPartition[ulRows]->uLPartitionUsageRate >= g_pstCond->ulPartitionThreshold)
        {
            bDiskExcept = DOS_TRUE;
        }
    }
    ulTotalDiskRate = mon_get_total_disk_usage_rate();
    if(ulTotalDiskRate >= g_pstCond->ulDiskThreshold)
    {
        bDiskExcept = DOS_TRUE;
    }

    /* ��������澯 */
    if (DOS_TRUE == bDiskExcept)
    {
        /* �����һ�β����澯���򽫸澯����澯���� */
        if (DOS_FALSE == g_pstWarningMsg[ulIndex].bExcep)
        {
            /* ����澯��Ϣ */
            pstMsg = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }

            GENERATE_WARNING_MSG(pstMsg, ulIndex, ulRet);
            ulRet = mon_send_email((S8 *)pstMsg->msg, "System disk Warning", "bubble@dipcc.com");
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
            }
            bAddToDB = DOS_TRUE;
        }
    }
    else
    {
        /* ����澯��������δ�ָ�����ָ�֮ */
        if (g_pstWarningMsg[ulIndex].bExcep == DOS_TRUE)
        {
            /* ����澯��Ϣ */
            pstMsg = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }

            GENERATE_NORMAL_MSG(pstMsg,ulIndex,ulRet);
            ulRet = mon_send_email((S8 *)pstMsg->msg, "System Disk Warning Recovery", "bubble@dipcc.com");
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
            }
            bAddToDB = DOS_TRUE;
        }
    }
    if (DOS_TRUE == bAddToDB)
    {
        /* ����Ϣ������Ϣ���� */
        ulRet = mon_warning_msg_en_queue(pstMsg);
        if(DOS_SUCC != ulRet)
        {
            mon_trace(MON_TRACE_MH, LOG_LEVEL_ERROR, "Warning Msg EnQueue FAIL.");
            return DOS_FAIL;
        }
    }
    return DOS_SUCC;
}

#endif //end #if INCLUDE_DISK_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

