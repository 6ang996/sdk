#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_CPU_MONITOR

#include "mon_lib.h"
#include "mon_get_cpu_info.h"


struct tagMonCPUQueue //cpu����
{
   MON_SYS_CPU_TIME_S *pstHead; //��һ���ڵ�
   MON_SYS_CPU_TIME_S *pstPenultimate; //������2���ڵ�
   MON_SYS_CPU_TIME_S *pstCountdownXII;//��12���ڵ�
   MON_SYS_CPU_TIME_S *pstRear;//���е�β�ڵ�
   U32                 ulQueueLength; //���г���
};

typedef struct tagMonCPUQueue MON_CPU_QUEUE_S;

static S8 m_szMonCPUInfoFile[]            = "/proc/stat";
static MON_CPU_QUEUE_S * m_pstCPUQueue; //CPU����

extern S8 g_szMonCPUInfo[MAX_BUFF_LENGTH];
extern MON_CPU_RSLT_S * g_pstCpuRslt;

static U32  mon_cpu_malloc(MON_SYS_CPU_TIME_S ** ppstCpu);
static U32  mon_cpu_reset_data();
static U32  mon_get_cpu_data(MON_SYS_CPU_TIME_S * pstCpu);
static U32  mon_cpu_en_queue(MON_SYS_CPU_TIME_S * pstCpu);
static U32  mon_cpu_de_queue();
static U32  mon_refresh_cpu_queue();


/**
 * ����:Ϊcpu�ڵ���Ϣ�����ڴ�
 * ��������
 *     ����1:MON_SYS_CPU_TIME_S ** ppstCpu �洢cpu�ڵ���Ϣ�ṹ��ָ��ĵ�ַ
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
static U32  mon_cpu_malloc(MON_SYS_CPU_TIME_S ** ppstCpu)
{
   *ppstCpu = (MON_SYS_CPU_TIME_S *)dos_dmem_alloc(sizeof(MON_SYS_CPU_TIME_S));
   if(DOS_ADDR_INVALID(*ppstCpu))
   {
      logr_error("%s:Line %u:mon_cpu_malloc| alloc memory failure!"
                    , dos_get_filename(__FILE__), __LINE__);
      return DOS_FAIL;
   }
   memset(*ppstCpu, 0, sizeof(MON_SYS_CPU_TIME_S));
   return DOS_SUCC;
}





/**
 * ����:Ϊ�洢cpuռ���ʵĽṹ����Ϣ�����ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_cpu_rslt_malloc()
{
   g_pstCpuRslt = (MON_CPU_RSLT_S *)dos_dmem_alloc(sizeof(MON_CPU_RSLT_S));
   if(DOS_ADDR_INVALID(g_pstCpuRslt))
   {
      logr_cirt("%s:Line %u:mon_cpu_rslt_malloc |alloc memory failure,pstCpuRslt is %p!"
                , dos_get_filename(__FILE__), __LINE__, g_pstCpuRslt);
      return DOS_FAIL;
   }
   
   memset(g_pstCpuRslt, 0, sizeof(MON_CPU_RSLT_S));

   return DOS_SUCC;
}

/**
 * ����:�ͷ�Ϊ�洢cpuռ���ʵĽṹ����Ϣ������ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_cpu_rslt_free()
{
   if(DOS_ADDR_INVALID(g_pstCpuRslt))
   {
      logr_warning("%s:Line %u:mon_cpu_rslt_free|free memory failure,pstCpuRslt is %p!"
                    , dos_get_filename(__FILE__), __LINE__, g_pstCpuRslt);
      return DOS_FAIL;
   }
   
   dos_dmem_free(g_pstCpuRslt);
   g_pstCpuRslt = NULL;

   return DOS_SUCC;
}

/** "/proc/stat"��Ϣ��ʽ(���cpu)
 * cpu  29785 76 47029 36906379 9441 1 694 0 0
 * cpu0 10754 40 16259 18450362 7090 1 689 0 0
 * cpu1 19031 36 30769 18456017 2351 0 5 0 0
 * intr 27609954 125 2 0 0 0 0 0 0 1 0 0 0 4 0 0 0 89 0
 * ......
 * ctxt 51620134
 * btime 1422690146
 * processes 37243
 * ......
 * ����:��ȡCPUʱ�����Ϣ
 * ��������
 *   ����1:MON_SYS_CPU_TIME_S * pstCpu �洢cpuʱ�����Ϣ�Ľڵ�ָ��
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */

static U32  mon_cpu_reset_data()
{
    g_pstCpuRslt->ulCPU10minUsageRate = 0;
    g_pstCpuRslt->ulCPU1minUsageRate = 0;
    g_pstCpuRslt->ulCPU5sUsageRate = 0;
    g_pstCpuRslt->ulCPUUsageRate = 0;

    return DOS_SUCC;
}
 
static U32  mon_get_cpu_data(MON_SYS_CPU_TIME_S * pstCpu)
{
   FILE * fp = NULL;
   S8     szBuff[MAX_BUFF_LENGTH] = {0};
   S32    lRet = 0;
   S8*    pszAnalyseRslt[10] = {0};

   if (DOS_ADDR_INVALID(pstCpu))
   {
      logr_cirt("%s:line %u:mon_cpu_rslt_free|get cpu data failure,pstCpu is %p!"
                , dos_get_filename(__FILE__), __LINE__, pstCpu);
      return DOS_FAIL;
   }

   fp = fopen(m_szMonCPUInfoFile, "r");
   if(DOS_ADDR_INVALID(fp))
   {
      logr_cirt("%s:Line %u:mon_cpu_rslt_free|get cpu data failure,file \'%s\' fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_szMonCPUInfoFile, fp);
      return DOS_FAIL;
   }

   fseek(fp, 0, SEEK_SET);
   if (NULL != (fgets(szBuff, MAX_BUFF_LENGTH, fp)))
   {
      U32 ulRet = 0;
      U32 ulRows = 0;
      U32 ulData = 0;
      
      ulRet = mon_analyse_by_reg_expr(szBuff, " \t\n", pszAnalyseRslt, sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]));
      if(DOS_SUCC != ulRet)
      {
         logr_error("%s:Line %u:mon_cpu_rslt_free|analyse sentence failure!"
                    , dos_get_filename(__FILE__), __LINE__);
         goto failure;
      }

      for(ulRows = 1; ulRows < sizeof(pszAnalyseRslt)/sizeof(pszAnalyseRslt[0]); ulRows++)
      {
         lRet = dos_atoul(pszAnalyseRslt[ulRows], &ulData);
         if(0 != lRet)
         {
            logr_error("%s:Line %u:mon_cpu_rslt_free|dos_atol failure!"
                        , dos_get_filename(__FILE__), __LINE__);
            goto failure;
         }
         
         switch(ulRows)
         {
           /**
            * ����ϸ���ݿɲ鿴:http://blog.sina.com.cn/s/blog_691c5f870100mmqx.html
            */
           case 1:
              pstCpu->ulUser        = ulData;
              break;
           case 2:
              pstCpu->ulNice        = ulData;
              break;
           case 3:
              pstCpu->ulSystem      = ulData;
              break;
           case 4:
              pstCpu->ulIdle        = ulData;
              break;
           case 5:
              pstCpu->ulIowait      = ulData;
              break;
           case 6:
              pstCpu->ulHardirq     = ulData;
              break;
           case 7:
              pstCpu->ulSoftirq     = ulData;
              break;
           case 8:
              pstCpu->ulStealstolen = ulData;
              break;
           case 9:
              pstCpu->ulGuest       = ulData;
              break;
           default:
              break;
         }
      }
   }
   goto success;

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
 * ����:��ʼ��cpuʱ�����Ϣ����
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_init_cpu_queue()
{
   U32 ulRet = 0;
   
   m_pstCPUQueue = (MON_CPU_QUEUE_S *)dos_dmem_alloc(sizeof(MON_CPU_QUEUE_S));
   if (DOS_ADDR_INVALID(m_pstCPUQueue))
   {
       logr_cirt("%s:line %u:mon_init_cpu_queue| init CPU queue failed,pstQueue is %p!"
                    , dos_get_filename(__FILE__),  __LINE__, m_pstCPUQueue);
       return DOS_FAIL;
   }
   
   memset(m_pstCPUQueue, 0, sizeof(MON_CPU_QUEUE_S));
   
   if(DOS_ADDR_INVALID(m_pstCPUQueue->pstHead))
   {
      MON_SYS_CPU_TIME_S * pstCpu = NULL;
      
      ulRet = mon_cpu_malloc(&pstCpu);
      if(DOS_SUCC != ulRet)
      {
         logr_cirt("%s:Line %u:mon_init_cpu_queue|alloc memory failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
         return DOS_FAIL;
      }   
      m_pstCPUQueue->pstHead = pstCpu;
   }
   m_pstCPUQueue->ulQueueLength = 1;

   m_pstCPUQueue->pstHead->next     = NULL;
   m_pstCPUQueue->pstHead->prior    = NULL;
   m_pstCPUQueue->pstPenultimate    = NULL;
   m_pstCPUQueue->pstCountdownXII   = NULL;
   m_pstCPUQueue->pstRear = m_pstCPUQueue->pstHead;

   while ((m_pstCPUQueue->ulQueueLength) < TEN_MINUTE_SAMPLE_ARRAY)
   {    

      MON_SYS_CPU_TIME_S * pstCpu = NULL;
      ulRet = mon_cpu_malloc(&pstCpu);
      if(DOS_SUCC != ulRet)
      {
          logr_cirt("%s:Line %u:mon_init_cpu_queue|alloc memory failure,ulRet is %d!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
          return DOS_FAIL;
      }
      
      ulRet = mon_cpu_en_queue(pstCpu);
      if(DOS_SUCC != ulRet)
      {
          logr_error("%s:Line %u:mon_init_cpu_queue|enter queue failure,ulRet is %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
          return DOS_FAIL;
      }

      if (PENULTIMATE == m_pstCPUQueue->ulQueueLength)
      {/* ���ӽڵ��ͬʱ�������ڶ�ָ��͵�����ʮ��ָ�������ƶ�һ����λ */
         m_pstCPUQueue->pstPenultimate = m_pstCPUQueue->pstRear;
      }
      else if (COUNTDOWN_XII == m_pstCPUQueue->ulQueueLength)
      {
         m_pstCPUQueue->pstCountdownXII = m_pstCPUQueue->pstRear;
      }

      logr_debug("%s:Line %u:mon_init_cpu_queue|m_pstCPUQueue->ulQueueLength = %u!"
                    , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue->ulQueueLength);
   }

   return DOS_SUCC;
}

/**
 * ����:һ���µ�cpuʱ�����Ϣ�������
 * ��������
 *   ����1:MON_SYS_CPU_TIME_S * pstCpu ����ӵĽṹ������ַ
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
static U32  mon_cpu_en_queue(MON_SYS_CPU_TIME_S * pstCpu)
{//�Ӷ�β���

    if (DOS_ADDR_INVALID(m_pstCPUQueue))
    {
       logr_cirt("%s:Line %u: mon_cpu_en_queue|enter queue failure,m_pstCPUQueue is %p!"
                  , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue);
       return DOS_FAIL;
    }
    if (DOS_ADDR_INVALID(pstCpu))
    {
       logr_cirt("%s:Line %u: mon_cpu_en_queue|enter queue failure,pstCpu is %p!"
                  , dos_get_filename(__FILE__), __LINE__, pstCpu);
       return DOS_FAIL;
    }

    pstCpu->next = NULL;
    pstCpu->prior = NULL;

    /**
     *  ��ӵ�ͬʱ�ı��βָ�룬�����Ƶ����ڶ�ָ��͵�����ʮ��ָ��
     */
    m_pstCPUQueue->pstRear->next = pstCpu;
    pstCpu->prior = m_pstCPUQueue->pstRear;
    m_pstCPUQueue->pstRear = pstCpu;
    
    if (m_pstCPUQueue->pstPenultimate && m_pstCPUQueue->ulQueueLength>= TEN_MINUTE_SAMPLE_ARRAY)
    {
       m_pstCPUQueue->pstPenultimate = m_pstCPUQueue->pstPenultimate->next;//��5ָ��������1
    }
    if (m_pstCPUQueue->pstCountdownXII && m_pstCPUQueue->ulQueueLength >= TEN_MINUTE_SAMPLE_ARRAY)  
    {
       m_pstCPUQueue->pstCountdownXII = m_pstCPUQueue->pstCountdownXII->next;//��60ָ��������1
    }
 
    ++(m_pstCPUQueue->ulQueueLength);

    return DOS_SUCC;
}

/**
 * ����:��ͷcpuʱ����ṹ�������
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
static U32  mon_cpu_de_queue()
{//�Ӷ�ͷ����
   MON_SYS_CPU_TIME_S * pTemp = NULL;

   if(DOS_ADDR_INVALID(m_pstCPUQueue))
   {
      logr_cirt("%s:line %u:mon_cpu_de_queue|delete queue failure,pstCPUQueue is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue);
      return DOS_FAIL;
   }
   
   if (DOS_ADDR_INVALID(m_pstCPUQueue->pstHead))
   {
      logr_cirt("%s:line %u:mon_cpu_de_queue|delete queue failure,pstCPUQueue->pstHead is %p!"
                ,dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue->pstHead);
      return DOS_FAIL;
   }
   
   pTemp = m_pstCPUQueue->pstHead;
   m_pstCPUQueue->pstHead = m_pstCPUQueue->pstHead->next;
   m_pstCPUQueue->pstHead->prior = NULL;
   pTemp->next = NULL;
   dos_dmem_free(pTemp);
   
   --(m_pstCPUQueue->ulQueueLength);

   return DOS_SUCC;
}

/**
 * ����:����cpu���������Ϣ�����ĸ�ռ����
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_get_cpu_rslt_data()
{
   U32 ulFirstTemp = 0, ulSecondTemp = 0, ulTwelfthTemp = 0, ulLastTemp = 0;
   U32 ulRet = 0;
   U32 ulLastBusy = 0, ul5sBusy = 0, ul5sAll = 0, ul5sIdle = 0, ul1minBusy = 0, ul1minAll = 0, ul1minIdle = 0, ul10minBusy = 0, ul10minAll = 0, ul10minIdle = 0;

   /*��ͷ���*/
   ulFirstTemp = m_pstCPUQueue->pstHead->ulUser
              + m_pstCPUQueue->pstHead->ulNice
              + m_pstCPUQueue->pstHead->ulSystem
              + m_pstCPUQueue->pstHead->ulIdle
              + m_pstCPUQueue->pstHead->ulIowait
              + m_pstCPUQueue->pstHead->ulSoftirq
              + m_pstCPUQueue->pstHead->ulHardirq
              + m_pstCPUQueue->pstHead->ulStealstolen
              + m_pstCPUQueue->pstHead->ulGuest;

   /*���е����ڶ��ڵ�*/
   ulSecondTemp = m_pstCPUQueue->pstPenultimate->ulUser
               + m_pstCPUQueue->pstPenultimate->ulNice
               + m_pstCPUQueue->pstPenultimate->ulSystem 
               + m_pstCPUQueue->pstPenultimate->ulIdle
               + m_pstCPUQueue->pstPenultimate->ulIowait
               + m_pstCPUQueue->pstPenultimate->ulSoftirq
               + m_pstCPUQueue->pstPenultimate->ulHardirq
               + m_pstCPUQueue->pstPenultimate->ulStealstolen
               + m_pstCPUQueue->pstPenultimate->ulGuest;

   /*���е�����ʮ���ڵ�*/
   ulTwelfthTemp = m_pstCPUQueue->pstCountdownXII->ulUser
                + m_pstCPUQueue->pstCountdownXII->ulNice
                + m_pstCPUQueue->pstCountdownXII->ulSystem
                + m_pstCPUQueue->pstCountdownXII->ulIdle
                + m_pstCPUQueue->pstCountdownXII->ulIowait
                + m_pstCPUQueue->pstCountdownXII->ulSoftirq
                + m_pstCPUQueue->pstCountdownXII->ulHardirq
                + m_pstCPUQueue->pstCountdownXII->ulStealstolen
                + m_pstCPUQueue->pstCountdownXII->ulGuest;

   /*��β�ڵ�*/
   ulLastTemp = m_pstCPUQueue->pstRear->ulUser 
             + m_pstCPUQueue->pstRear->ulNice
             + m_pstCPUQueue->pstRear->ulSystem
             + m_pstCPUQueue->pstRear->ulIdle
             + m_pstCPUQueue->pstRear->ulIowait 
             + m_pstCPUQueue->pstRear->ulSoftirq
             + m_pstCPUQueue->pstRear->ulHardirq
             + m_pstCPUQueue->pstRear->ulStealstolen
             + m_pstCPUQueue->pstRear->ulGuest;
   /**
    *  CPUʱ��ռ����Ϊһ��ƽ��ֵ��ȡ����ʱ������㣬���㷨��:
    *     rate = 1 - (idle2 - idle1)/(total2 - total1)
    *  ����:total = user + nice + system + idle + iowait + softirq + hardirq + stolen + guest
    */

   mon_cpu_reset_data();
    
   /* ����ƽ��ռ���� */
   ulLastBusy = 100 * (ulLastTemp - m_pstCPUQueue->pstRear->ulIdle);
   if (0 == ulLastTemp)
   {
       g_pstCpuRslt->ulCPUUsageRate = 0;
   }
   else
   {
       g_pstCpuRslt->ulCPUUsageRate = (ulLastBusy + ulLastBusy % ulLastTemp) / ulLastTemp;
   }
   

   /* ����5sƽ��ռ���� */
   ul5sAll = ulLastTemp - ulSecondTemp;
   ul5sIdle =  m_pstCPUQueue->pstRear->ulIdle - m_pstCPUQueue->pstPenultimate->ulIdle;
   ul5sBusy = ul5sAll - ul5sIdle;
   ul5sBusy *= 100;
   if (0 == ul5sAll)
   {
       g_pstCpuRslt->ulCPU5sUsageRate = 0;
   }
   else
   {
       g_pstCpuRslt->ulCPU5sUsageRate = (ul5sBusy + ul5sBusy % ul5sAll) / ul5sAll;
   } 
   
   /* ����1min CPUƽ��ռ���� */
   ul1minAll = ulLastTemp - ulTwelfthTemp;
   ul1minIdle = m_pstCPUQueue->pstRear->ulIdle - m_pstCPUQueue->pstCountdownXII->ulIdle;
   ul1minBusy = ul1minAll - ul1minIdle;
   ul1minBusy *= 100;
   if (0 == ul1minAll)
   {
       g_pstCpuRslt->ulCPU1minUsageRate = 0;
   }
   else
   {
       g_pstCpuRslt->ulCPU1minUsageRate = (ul1minBusy + ul1minBusy % ul1minAll) / ul1minAll;
   }

   /* ����10min CPUƽ��ռ���� */
   ul10minAll = ulLastTemp - ulFirstTemp;
   ul10minIdle = m_pstCPUQueue->pstRear->ulIdle - m_pstCPUQueue->pstHead->ulIdle;
   ul10minBusy = ul10minAll - ul10minIdle;
   ul10minBusy *= 100;
   if (0 == ul10minAll)
   {
       g_pstCpuRslt->ulCPU10minUsageRate = 0;
   }
   else
   {
       g_pstCpuRslt->ulCPU10minUsageRate = (ul10minBusy + ul10minBusy % ul10minAll) / ul10minAll;
   }
   
   ulRet = mon_refresh_cpu_queue();
   if (DOS_SUCC != ulRet)
   {
       logr_error("%s:Line %u: Refresh CPU Queue FAIL, lRet is %u"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
       return DOS_FAIL;
   }
   
   return DOS_SUCC;
}

/**
 * ����:����һ�ζ��У�����һ����Ӷ�����һ�����Ӷ���
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
static U32  mon_refresh_cpu_queue()
{
   U32 ulRet = 0;
   
   MON_SYS_CPU_TIME_S * pstCpu = NULL;
   ulRet = mon_cpu_malloc(&pstCpu);
   if(DOS_SUCC != ulRet)
   {
      logr_cirt("%s:Line %u:mon_refresh_cpu_queue|resfresh queue failure,ulRet is %u!"
                , dos_get_filename(__FILE__), __LINE__, ulRet);
      return DOS_FAIL;
   }

   if(DOS_ADDR_INVALID(m_pstCPUQueue))
   {
      logr_cirt("%s:Line %u:mon_refresh_cpu_queue|refresh queue failure,m_pstCPUQueue is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue);
      return DOS_FAIL;
   }

   ulRet = mon_get_cpu_data(pstCpu);
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %u:mon_refresh_cpu_queue|get cpu data failure,ulRet == %u!"
                    , dos_get_filename(__FILE__), __LINE__,ulRet);
      return DOS_FAIL;
   }
   
   ulRet = mon_cpu_en_queue(pstCpu);
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %d:mon_refresh_cpu_queue|lRet == %u!", __FILE__, __LINE__, ulRet);
      return DOS_FAIL;
   }
   
   ulRet = mon_cpu_de_queue();
   if(DOS_SUCC != ulRet)
   {
      logr_error("%s:Line %d:mon_refresh_cpu_queue|delete queue failure,ulRet == %u!"
                    , dos_get_filename(__FILE__), __LINE__, ulRet);
      return DOS_FAIL;
   }

   return DOS_SUCC;
}

/**
 * ����:��ȡcpu��4��ռ���ʵĸ�ʽ����Ϣ�ַ���
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_get_cpu_rslt_formatted_info()
{
   
   memset(g_szMonCPUInfo, 0, MAX_BUFF_LENGTH);
   
   dos_snprintf(g_szMonCPUInfo, MAX_BUFF_LENGTH, 
         "Total CPU Average Usage Rate:%u%%.\n" \
         "5 seconds CPU Average Usage Rate:%u%%.\n" \
         "1min CPU Average Usage Rate:%u%%.\n" \
         "10min CPU Average Usage Rate:%u%%.\n"
         , g_pstCpuRslt->ulCPUUsageRate
         , g_pstCpuRslt->ulCPU5sUsageRate
         , g_pstCpuRslt->ulCPU1minUsageRate
         , g_pstCpuRslt->ulCPU10minUsageRate);
         
   logr_info("%s:Line %u:g_szMonCPUInfo is \n%s"
                , dos_get_filename(__FILE__), __LINE__, g_szMonCPUInfo);
   
   return DOS_SUCC;
}

/**
 * ����:����cpuʱ�������
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32  mon_cpu_queue_destroy()
{
   MON_SYS_CPU_TIME_S * pstCpu = NULL;
   if(DOS_ADDR_INVALID(m_pstCPUQueue))
   {
      logr_cirt("%s:Line %u:mon_cpu_queue_destroy|destory queue failure,m_pstCPUQueue is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue);
      return DOS_FAIL;
   }

   if(DOS_ADDR_INVALID(m_pstCPUQueue->pstHead))
   {
      logr_cirt("%s:Line %u: mon_cpu_queue_destroy|destroy queue failure,m_pstCPUQueue->pstHead is %p!"
                , dos_get_filename(__FILE__), __LINE__, m_pstCPUQueue->pstHead);
                  
      dos_dmem_free(m_pstCPUQueue);
      m_pstCPUQueue = NULL;
      
      return DOS_FAIL;
   }

   pstCpu = m_pstCPUQueue->pstHead;
   while(pstCpu)
   {
      MON_SYS_CPU_TIME_S * pstCpuTemp = pstCpu;
      pstCpu = pstCpu->next;
      pstCpuTemp->next = NULL;
      pstCpu->prior = NULL;

      dos_dmem_free(pstCpuTemp);
      pstCpuTemp = NULL;
   }

   dos_dmem_free(m_pstCPUQueue);
   m_pstCPUQueue = NULL;

   return DOS_SUCC;
}

#endif //end #if INCLUDE_CPU_MONITOR
#endif //#if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif

