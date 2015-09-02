#ifdef __cplusplus
extern "C"{
#endif


#include <dos.h>
#include <dos/dos_mem.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_MEM_MONITOR

#include "mon_get_mem_info.h"
#include "mon_lib.h"
#include "mon_def.h"

static S8  m_szMemInfoFile[] = "/proc/meminfo";
extern S8  g_szMonMemInfo[MAX_BUFF_LENGTH];
extern MON_SYS_MEM_DATA_S * g_pstMem;

static U32  mon_mem_reset_data();


/**
 * ����:Ϊg_pstMem�����ڴ�
 * ��������
 *     �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_mem_malloc()
{
    g_pstMem = (MON_SYS_MEM_DATA_S *)dos_dmem_alloc(sizeof(MON_SYS_MEM_DATA_S));
    if (DOS_ADDR_INVALID(g_pstMem))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(g_pstMem, sizeof(MON_SYS_MEM_DATA_S));

    return DOS_SUCC;
}


/**
 * ����:�ͷ�Ϊg_pstMem�����ڴ�
 * ��������
 *     �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_mem_free()
{
    if (DOS_ADDR_INVALID(g_pstMem))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_dmem_free(g_pstMem);
    g_pstMem = NULL;

    return DOS_SUCC;
}


static U32  mon_mem_reset_data()
{
    g_pstMem->ulBuffers = 0;
    g_pstMem->ulCached = 0;
    g_pstMem->ulPhysicalMemFreeBytes = 0;
    g_pstMem->ulPhysicalMemTotalBytes = 0;
    g_pstMem->ulPhysicalMemUsageRate = 0;
    g_pstMem->ulSwapFreeBytes = 0;
    g_pstMem->ulSwapTotalBytes = 0;
    g_pstMem->ulSwapUsageRate = 0;

    return DOS_SUCC;
}

/** "/proc/meminfo"����
 * MemTotal:        1688544 kB
 * MemFree:          649060 kB
 * Buffers:          103812 kB
 * Cached:           203864 kB
 * SwapCached:            0 kB
 * Active:           640116 kB
 * ......
 * Mlocked:               0 kB
 * SwapTotal:       3125240 kB
 * SwapFree:        3125240 kB
 * Dirty:                 4 kB
 * ......
 * ����:��proc�ļ�ϵͳ��ȡ�ڴ�ռ����Ϣ���������
 * ��������
 *     �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_read_mem_file()
{
    U32 ulCount = 0;
    S8  szLine[MAX_BUFF_LENGTH] = {0};
    S8* pszAnalyseRslt[3] = {0};
    U32 ulRet = 0;

    FILE * fp = fopen(m_szMemInfoFile, "r");
    if (DOS_ADDR_INVALID(fp))
    {
        DOS_ASSERT(0);
        goto failure;
    }

    fseek(fp, 0, SEEK_SET);
    mon_mem_reset_data();

    while (!feof(fp))
    {
        if ( NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
        {
            S32 lRet = 0;
            U32 ulData = 0;
            ulRet = mon_analyse_by_reg_expr(szLine, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
            if(DOS_SUCC != ulRet)
            {
                DOS_ASSERT(0);
                goto failure;
            }

            lRet = dos_atoul(pszAnalyseRslt[1], &ulData);
            if(0 > lRet)
            {
                DOS_ASSERT(0);
                goto failure;
            }
            /* ��һ�������ݣ��ڶ���Ϊ��ֵ���Դ����� */
            if (0 == dos_strnicmp(pszAnalyseRslt[0], "MemTotal", dos_strlen("MemTotal")))
            {
                g_pstMem->ulPhysicalMemTotalBytes = (ulData + ulData % 1024) / 1024;
                ++ulCount;
                continue;
            }
            else if(0 == dos_strnicmp(pszAnalyseRslt[0], "MemFree", dos_strlen("MemFree")))
            {
                g_pstMem->ulPhysicalMemFreeBytes = (ulData + ulData % 1024) / 1024;
                ++ulCount;
                continue;
            }

            else if(0 == dos_strnicmp(pszAnalyseRslt[0], "SwapTotal", dos_strlen("SwapTotal")))
            {
                g_pstMem->ulSwapTotalBytes = (ulData + ulData % 1024) / 1024;
                ++ulCount;
                continue;
            }
            else if(0 == dos_strnicmp(pszAnalyseRslt[0], "SwapFree", dos_strlen("SwapFree")))
            {
                g_pstMem->ulSwapFreeBytes = (ulData + ulData % 1024) / 1024;
                ++ulCount;
                continue;
            }
            else if(0 == dos_strnicmp(pszAnalyseRslt[0], "Buffers", dos_strlen("Buffers")))
            {
                g_pstMem->ulBuffers = (ulData + ulData % 1024) / 1024;
                ++ulCount;
                continue;
            }
            else if (0 == dos_strnicmp(pszAnalyseRslt[0], "Cached", dos_strlen("Cached")))
            {
                g_pstMem->ulCached = (ulData + ulData % 1024) / 1024;
                ++ulCount;
                continue;
            }

            if (MEMBER_COUNT == ulCount)
            {
                ulRet = mon_get_mem_data();
                if(DOS_SUCC != ulRet)
                {
                    mon_trace(MON_TRACE_MEM, LOG_LEVEL_ERROR, "Get Memory Info FAIL.");
                }
                goto success;
            }
        }
    }

success:
    fclose(fp);
    fp = NULL;
    return DOS_SUCC;
failure:
    fclose(fp);
    fp = NULL;
    return DOS_FAIL;
}

/**
 * ����:��ȡ������g_pstMem���������
 * ��������
 *     �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_get_mem_data()
{
    U32 ulPhyMemTotal = g_pstMem->ulPhysicalMemTotalBytes;
    U32 ulPhyMemFree  = g_pstMem->ulPhysicalMemFreeBytes;

    U32 ulBusyMem = ulPhyMemTotal - ulPhyMemFree;

    U32 ulSwapTotal   = g_pstMem->ulSwapTotalBytes;
    U32 ulSwapFree    = g_pstMem->ulSwapFreeBytes;

    U32 ulBusySwap = ulSwapTotal - ulSwapFree;

    /**
    * �ڴ�ռ�����㷨:
    *     ������ٻ���Ĵ�С����RAM��С����ô��ռ�õľ���: busy = total -free
    *     ����ռ�õ�Ϊ: busy = total - free - buffers - cached
    */
    if (0 == ulPhyMemTotal)
    {
        g_pstMem->ulPhysicalMemUsageRate = 0;
    }
    else
    {
        ulBusyMem *= 100;
        g_pstMem->ulPhysicalMemUsageRate = (ulBusyMem + ulBusyMem % ulPhyMemTotal) / ulPhyMemTotal;
    }

    if (0 == ulSwapTotal)
    {
        g_pstMem->ulSwapUsageRate = 0;
    }
    else
    {
        ulSwapFree *= 100;
        g_pstMem->ulSwapUsageRate = (ulBusySwap + ulBusySwap % ulSwapTotal) / ulSwapTotal;
    }

    return DOS_SUCC;
}

#endif //end #if INCLUDE_MEM_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_HB_SERVER)

#ifdef __cplusplus
}
#endif


