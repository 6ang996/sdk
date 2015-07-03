#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_DISK_MONITOR

#include <fcntl.h>
#include "mon_get_disk_info.h"
#include "mon_lib.h"

extern  S8 g_szMonDiskInfo[MAX_PARTITION_COUNT * MAX_BUFF_LENGTH];
extern  MON_SYS_PART_DATA_S * g_pastPartition[MAX_PARTITION_COUNT];
extern  U32 g_ulPartCnt;

#if 0
static U32  mon_get_disk_temperature();
#endif

static S8 * mon_get_disk_serial_num(S8 * pszPartitionName);
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
      logr_cirt("%s:Line %d: mon_disk_free|alloc memory failure,pstPartition is %p!"
                , dos_get_filename(__FILE__), __LINE__, pstPartition);
      return DOS_FAIL;
   }

   memset(pstPartition, 0, MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));

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
      logr_cirt("%s:Line %u:free memory failure,pstPartition is %p!"
                , dos_get_filename(__FILE__) , __LINE__, pstPartition);
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
        logr_cirt("%s:Line %u:free memory failure,pstPartition is %p!"
                   , dos_get_filename(__FILE__) , __LINE__, pstPartition);
        return DOS_FAIL;
    }

    dos_memzero(pstPartition, MAX_PARTITION_COUNT * sizeof(MON_SYS_PART_DATA_S));

    return DOS_SUCC;
}


/**
 * ����:��ȡ���̵��¶�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
#if 0
static U32  mon_get_disk_temperature()
{
   /* ���������hdparm��װ���⣬���޷����� */
   return DOS_SUCC;
}
#endif

/**
 * ����:��ȡ���̵����к���Ϣ�ַ���
 * ��������
 *   ����1: S8 * pszPartitionName ������
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
static S8 *  mon_get_disk_serial_num(S8 * pszPartitionName)
{   /* Ŀǰû����Ч�ķ��������Ӳ������ */
    if(DOS_ADDR_INVALID(pszPartitionName))
    {
       logr_cirt("%s:Line %u:mon_get_disk_serial_num|pszPartitionName is %p!",
                  dos_get_filename(__FILE__), __LINE__, pszPartitionName);
       return NULL;
    }

    return "IC35L180AVV207-1";
}

/** df�������
 * Filesystem                   1K-blocks     Used Available Use% Mounted on
 * /dev/mapper/VolGroup-lv_root   8780808  6175596   2159160  75% /
 * tmpfs                           699708       72    699636   1% /dev/shm
 * /dev/sda1                       495844    34870    435374   8% /boot
 * .host:/                      209715196 19219176 190496020  10% /mnt/hgfs
 * ����:��ȡ���̵����к���Ϣ�ַ���
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
 U32 mon_get_partition_data()
 {
     S8 szDfCmd[] = "df -l | grep -v \'tmpfs\'| grep /dev/";
     S8 szBuff[MAX_BUFF_LENGTH] = {0};
     FILE *fp = NULL;
     S8 *paszInfo[6] = {0};
     S8 *pszTemp = NULL;
     U32 ulRet = U32_BUTT;
     U32 ulTotalVolume = 0, ulUsedVolume = 0, ulAvailVolume = 0, ulTotalRate = 0;

     g_ulPartCnt = 0;

     fp = popen(szDfCmd, "r");
     if (DOS_ADDR_INVALID(fp))
     {
         logr_error("%s:Line %u:execute df command FAIL.", dos_get_filename(__FILE__), __LINE__);
         return DOS_FAIL;
     }

     ulRet = mon_partition_reset_data();
     if (DOS_SUCC != ulRet)
     {
         logr_error("%s:Line %u:ulRet == %u", dos_get_filename(__FILE__), __LINE__, ulRet);
         return DOS_FAIL;
     }

     while( !feof(fp))
     {
         if (NULL != fgets(szBuff, MAX_BUFF_LENGTH, fp))
         {
             ulRet = mon_analyse_by_reg_expr(szBuff, " \t\n", paszInfo, sizeof(paszInfo) / sizeof(paszInfo[0]));
             if (DOS_SUCC != ulRet)
             {
                 logr_error("%s:Line %u: analyse line FAIL.", dos_get_filename(__FILE__), __LINE__);
                 goto fail;
             }

             dos_snprintf(g_pastPartition[g_ulPartCnt]->szPartitionName, MAX_PARTITION_LENGTH, "%s", paszInfo[0]);

             if (dos_atoul(paszInfo[1], &ulTotalVolume) < 0)
             {
                 logr_error("%s:Line %u: dos_atoul FAIL, paszInfo[1] is %s", dos_get_filename(__FILE__), __LINE__, paszInfo[1]);
                 goto fail;
             }

             if (dos_atoul(paszInfo[2], &ulUsedVolume) < 0)
             {
                 logr_error("%s:Line %u: dos_atoul FAIL, paszInfo[2] is %s", dos_get_filename(__FILE__), __LINE__, paszInfo[2]);
                 goto fail;
             }

             if (dos_atoul(paszInfo[3], &ulAvailVolume) < 0)
             {
                 logr_error("%s:Line %u: dos_atoul FAIL, paszInfo[3] is %s", dos_get_filename(__FILE__), __LINE__, paszInfo[3]);
                 goto fail;
             }

             pszTemp = paszInfo[4];
             while('%' != *pszTemp)
             {
                 ++pszTemp;
             }

             *pszTemp = '\0';

             if (dos_atoul(paszInfo[4], &ulTotalRate) < 0)
             {
                 logr_error("%s:Line %u: dos_atoul FAIL, paszInfo[4] is %s", dos_get_filename(__FILE__), __LINE__, paszInfo[4]);
                 goto fail;
             }

             g_pastPartition[g_ulPartCnt]->ulPartitionTotalBytes = ulTotalVolume;
             g_pastPartition[g_ulPartCnt]->ulPartitionUsedBytes = ulUsedVolume;
             g_pastPartition[g_ulPartCnt]->ulPartitionAvailBytes = ulAvailVolume;
             g_pastPartition[g_ulPartCnt]->ulPartitionUsageRate = ulTotalRate;

             pszTemp = mon_get_disk_serial_num(g_pastPartition[g_ulPartCnt]->szPartitionName);
             dos_snprintf(g_pastPartition[g_ulPartCnt]->szDiskSerialNo, MAX_PARTITION_LENGTH, "%s", pszTemp);

             ++g_ulPartCnt;
         }
     }

     pclose(fp);
     fp = NULL;
     return DOS_SUCC;

fail:
     pclose(fp);
     fp = NULL;
     return DOS_FAIL;
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
    U32 ulTotal = 0, ulUsed = 0;

    for (ulRows = 0; ulRows < g_ulPartCnt; ++ulRows)
    {
        ulTotal += g_pastPartition[ulRows]->ulPartitionAvailBytes + g_pastPartition[ulRows]->ulPartitionUsedBytes ;
        ulUsed += g_pastPartition[ulRows]->ulPartitionUsedBytes;
    }

    ulUsed *= 100;

    return ulUsed / ulTotal + 1;
}

/**
 * ����:��ȡ���̵���ռ����
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ����ش������ֽ���(KBytes)��ʧ�ܷ���DOS_FAIL
 */
U32 mon_get_total_disk_kbytes()
{
    U32  ulTotal = 0, ulRows = 0;

    for (ulRows = 0; ulRows < g_ulPartCnt; ++ulRows)
    {
        ulTotal += g_pastPartition[ulRows]->ulPartitionTotalBytes;
    }

    return ulTotal;
}

#endif //end #if INCLUDE_DISK_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

