#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR
#if INCLUDE_NET_MONITOR

#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <dirent.h>
#include <net/if.h>
#include <errno.h>

#include "mon_get_net_info.h"
#include "mon_lib.h"

typedef struct tagMonTransData
{
    U64 uLInSize;
    U64 uLOutSize;
}MON_TRANS_DATA_S;

extern S8 g_szMonNetworkInfo[MAX_NETCARD_CNT * MAX_BUFF_LENGTH];
extern MON_NET_CARD_PARAM_S * g_pastNet[MAX_NETCARD_CNT];
extern S32 g_lNetCnt;

//������¼����ʱ���
time_t *m_pt1 = NULL, *m_pt2 = NULL;

//������¼֮ǰ�ض�ʱ�̺͵�ǰʱ�̵����ݴ���
MON_TRANS_DATA_S *m_pstTransFormer = NULL;
MON_TRANS_DATA_S *m_pstTransCur = NULL;

/**
 * ����:Ϊ������������ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
S32 mon_netcard_malloc()
{
   S32 lRows = 0;
   MON_NET_CARD_PARAM_S * pastNet;
   
   pastNet = (MON_NET_CARD_PARAM_S *)dos_dmem_alloc(MAX_NETCARD_CNT * sizeof(MON_NET_CARD_PARAM_S));
   if(!pastNet)
   {
      logr_cirt("%s:Line %d:mon_netcard_malloc|pastNet is %p!"
                , dos_get_filename(__FILE__), __LINE__, pastNet);
      goto fail;
   }

   memset(pastNet, 0, MAX_NETCARD_CNT * sizeof(MON_NET_CARD_PARAM_S));
   for(lRows = 0; lRows < MAX_NETCARD_CNT; lRows++)
   {
      g_pastNet[lRows] = &(pastNet[lRows]);
   }

   m_pstTransFormer = (MON_TRANS_DATA_S *)dos_dmem_alloc(sizeof(MON_TRANS_DATA_S));
   if (!m_pstTransFormer)
   {
       logr_error("%s:Line %d: Alloc Memory FAIL.", dos_get_filename(__FILE__), __LINE__);
       goto fail;
   }
   memset(m_pstTransFormer, 0, sizeof(MON_TRANS_DATA_S));

   m_pstTransCur = (MON_TRANS_DATA_S *)dos_dmem_alloc(sizeof(MON_TRANS_DATA_S));
   if (!m_pstTransCur)
   {
       logr_error("%s:Line %d: Alloc Memory FAIL.", dos_get_filename(__FILE__), __LINE__);
       goto fail;
   }
   memset(m_pstTransCur, 0, sizeof(MON_TRANS_DATA_S));

   m_pt1 = dos_dmem_alloc(sizeof(time_t));
   if (!m_pt1)
   {
       logr_error("%s:Line %d: Alloc memory FAIL.");
       goto fail;
   }
   
   m_pt2 = dos_dmem_alloc(sizeof(time_t));
   if (!m_pt2)
   {
       logr_error("%s:Line %d: Alloc memory FAIL.");
       goto fail;
   }
   
   return DOS_SUCC;
fail:
   if (NULL != pastNet)
   {
       dos_dmem_free(pastNet);
       pastNet = NULL;
   }
   if (NULL != g_pastNet[0])
   {
       dos_dmem_free( g_pastNet[0]); 
       g_pastNet[0] = NULL;
   }
   if (NULL != m_pstTransFormer)
   {
       dos_dmem_free(m_pstTransFormer);
       m_pstTransFormer = NULL;
   }
   if (NULL != m_pstTransCur)
   {
       dos_dmem_free(m_pstTransCur);
       m_pstTransCur = NULL;
   }
   if (NULL != m_pt1)
   {
       dos_dmem_free(m_pt1);
       m_pt1 = NULL;
   }
   if (NULL != m_pt2)
   {
       dos_dmem_free(m_pt2);
       m_pt2 = NULL;
   }
   
   return DOS_FAIL;
}

/**
 * ����:�ͷ�Ϊ�������������ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
S32  mon_netcard_free()
{
   S32 lRows = 0;
   MON_NET_CARD_PARAM_S * pastNet = g_pastNet[0];
   if(!pastNet)
   {
      logr_cirt("%s:Line %d:mon_netcard_free|pastNet is %p!"
                , dos_get_filename(__FILE__), __LINE__ , pastNet);
      return DOS_FAIL;
   }
   dos_dmem_free(pastNet);
   
   for(lRows = 0; lRows < MAX_NETCARD_CNT; lRows++)
   {
      g_pastNet[lRows] = NULL;
   }

   if (!m_pstTransCur)
   {
      dos_dmem_free(m_pstTransCur);
      m_pstTransCur = NULL;
   }

   if (!m_pstTransFormer)
   {
      dos_dmem_free(m_pstTransFormer);
      m_pstTransFormer = NULL;
   }

   if (NULL != m_pt1)
   {
       dos_dmem_free(m_pt1);
       m_pt1 = NULL;
   }
   if (NULL != m_pt2)
   {
       dos_dmem_free(m_pt2);
       m_pt2 = NULL;
   }

   return DOS_SUCC;
} 

/** 
 * �ж�ԭ��:
 *   proc�ļ�ϵͳ��������һ���ļ���¼����������״̬��Ϣ
 *   /sys/class/net/$netCardName/carrier
 *   ����ļ�Ϊ�գ��������ǰ������ʧȥ����
 *   �������OK���������������"1"
 *
 *
 * ����:�ж������������Ƿ���
 * ��������
 *   ����1:const S8 * pszNetCard ��������
 * ����ֵ��
 *   �����򷵻�DOS_TRUE�����򷵻�DOS_FALSE
 */
 BOOL mon_is_netcard_connected(const S8 * pszNetCard)
 {
     FILE *pstNetFp = NULL;
     S8  szBuff[512] = {0};
     S8  szCarrierPath[512] = {0};
     BOOL bRet = DOS_FALSE;

     dos_snprintf(szCarrierPath, sizeof(szCarrierPath), "/sys/class/net/%s/carrier", pszNetCard);
     if (NULL != (pstNetFp = fopen(szCarrierPath, "r")))
     {
         if(NULL != fgets(szBuff, sizeof(szBuff), pstNetFp))
         {
            if ('0' == szBuff[0])
            {
                bRet = DOS_FALSE;
            }
            else
            {
                bRet =  DOS_TRUE;
            }
         }
         else
         {
             logr_error("%s:Line %d: File \"%s\" has no content.", dos_get_filename(__FILE__), __LINE__, szCarrierPath);
             bRet = DOS_FALSE;
         }
     }
     else
     {
         logr_error("%s:Line %d:file \"%s\" open FAIL."
                    , dos_get_filename(__FILE__), __LINE__, szCarrierPath);
         bRet = DOS_FALSE;
     }

     fclose(pstNetFp);
     pstNetFp = NULL;
     
     return bRet;
 }

/**
 *  �ļ�/proc/net/dev�г����������������ݴ����������Ӧ�����ݸ�ʽ����:
 *  Inter-|   Receive                                                |  Transmit
 *  face |bytes    packets errs drop fifo frame compressed multicast|bytes    packets errs drop fifo colls carrier compressed
 *     lo:812170463 1868650    0    0    0     0          0         0 812170463 1868650    0    0    0     0       0          0
 *   eth0:2917065308 11065515    0    0    0     0          0         0 4520109097 13962214    0    0    0     0       0          0
 *
 *  �㷨: ����t1ʱ�̣���Ӧ�Ľ����������ֽ���recv1,��Ӧ�ķ����������ֽ���send1
 *        t2ʱ�̣���Ӧ�Ľ����������ֽ���recv2,��Ӧ�ķ����������ֽ���send2
 *        ��t1~t2ʱ�̵����ݴ���������:  (recv2 - recv1 + send2 - send1)/(t2 - t1)
 *
 *
 * ����������mon_is_netcard_connected����
 * ����:��ȡ���ڵ����ݴ������ʣ���λ��kbps
 * ��������
 *   ����1:const S8 * pszDevName ��������
 * ����ֵ��
 *   �ɹ��������ڵ����ݴ������ʣ�ʧ�ܷ���-1
 */
S32 mon_get_data_trans_speed(const S8 * pszDevName)
{
    U32 ulInterval = 0;
    S8  szNetFile[] = "/proc/net/dev";
    S8  szNetInfo[1024] = {0};
    FILE *fp = NULL;
    S8 *pszInfo = NULL;
    S8 *ppszData[16] = {0};
    U32 ulRet = U32_BUTT;
    S64 LRet = -1;
    U64 uLIn = U64_BUTT, uLOut = U64_BUTT;

    *m_pt1 = *m_pt2;
    *m_pt2 = time(0);
    ulInterval = *m_pt2 - *m_pt1;
    if (0 == ulInterval)
    {
       ulInterval = 5;
       logr_info("%s:Line %d: First Run.");
    }

    m_pstTransFormer->uLInSize = m_pstTransCur->uLInSize;
    m_pstTransFormer->uLOutSize = m_pstTransCur->uLOutSize;

    fp = fopen(szNetFile, "r");
    if (!fp)
    {
        logr_error("%s:Line %d:File \"%s\" open FAIL.", dos_get_filename(__FILE__), __LINE__, szNetFile);
        return DOS_FAIL;
    }

    while(NULL != fgets(szNetInfo, sizeof(szNetInfo), fp))
    {
        pszInfo = dos_strstr(szNetInfo, (S8 *)pszDevName);
        if (NULL != pszInfo)
        {
            break;
        }
    }

    while(*(pszInfo - 1) != ':')
    {
        ++pszInfo;
    }

    ulRet = mon_analyse_by_reg_expr(pszInfo, " \t\n", ppszData, sizeof(ppszData) / sizeof(ppszData[0]));
    if (DOS_SUCC != ulRet)
    {
        logr_error("%s:Line %d: Analysis string FAIL.");
        fclose(fp);
        return DOS_FAIL;
    }

    if (dos_atoull(ppszData[0], &uLIn) < 0 
        || dos_atoull(ppszData[8], &uLOut) < 0)
    {
        logr_error("%s:Line %d: dos_atoull FAIL.");
        fclose(fp);
        return DOS_FAIL;
    }

    m_pstTransCur->uLInSize = uLIn;
    m_pstTransCur->uLOutSize = uLOut;

    LRet = (S64)m_pstTransCur->uLInSize - (S64)m_pstTransFormer->uLInSize 
         + (S64)m_pstTransCur->uLOutSize - (S64)m_pstTransFormer->uLOutSize;

    ulInterval *= 1024;
    
    fclose(fp);
    fp = NULL;

    /*��λ��KB/s*/
    return (LRet + LRet % ulInterval) / ulInterval; 
}


/**
 * ����:��ȡ��������������������MAC��ַ��IPv4��ַ���㲥IP��ַ���������롢����������������ݴ�������
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
S32 mon_get_netcard_data()
{
   S32 lFd = 0, lRet = 0;
   S32 lInterfaceNum = 0;
   struct ifreq astReq[16];
   struct ifconf stIfc;
   struct ifreq stIfrcopy;
   S8  szMac[32] = {0};
   S8  szIPv4Addr[32] = {0};
   S8  szBroadAddr[32] = {0};
   S8  szSubnetMask[32] = {0};
   S32 lLength = 0;

   if ((lFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
   {
      logr_error("%s:Line %d: socket failed!", dos_get_filename(__FILE__)
                 , __LINE__);
      goto failure;
   }

   stIfc.ifc_len = sizeof(astReq);
   stIfc.ifc_buf = (caddr_t)astReq;
   if (!ioctl(lFd, SIOCGIFCONF, (S8 *)&stIfc))
   {
      /* ��ȡ�����豸�ĸ��� */
      lInterfaceNum = g_lNetCnt= stIfc.ifc_len / sizeof(struct ifreq);
      while (lInterfaceNum > 0)
      {
         lInterfaceNum--;
         /*����豸����*/

         if(!g_pastNet[lLength])
         {
            logr_cirt("%s:Line %d:mon_get_netcard_data|get netcard data failure,m_pastNet[%d] is %p!"
                        , dos_get_filename(__FILE__), __LINE__ ,lLength ,g_pastNet[lLength]);
            goto failure;
         }
         
         dos_strcpy(g_pastNet[lLength]->szNetDevName, astReq[lInterfaceNum].ifr_name);
         (g_pastNet[lLength]->szNetDevName)[dos_strlen(astReq[lInterfaceNum].ifr_name)] = '\0';
         stIfrcopy = astReq[lInterfaceNum];

         if (ioctl(lFd, SIOCGIFFLAGS, &stIfrcopy))
         {
            logr_error("%s:Line %d:mon_get_netcard_data|get netcard data failure,error no is %s"
                        , dos_get_filename(__FILE__), __LINE__, strerror(errno));
            goto failure;
         }

         if (!ioctl(lFd, SIOCGIFHWADDR, (S8 *)(&astReq[lInterfaceNum])))
         {
            memset(szMac, 0, sizeof(szMac));
            dos_snprintf(szMac, sizeof(szMac), "%02x:%02x:%02x:%02x:%02x:%02x",
                   (unsigned char)astReq[lInterfaceNum].ifr_hwaddr.sa_data[0],
                   (unsigned char)astReq[lInterfaceNum].ifr_hwaddr.sa_data[1],
                   (unsigned char)astReq[lInterfaceNum].ifr_hwaddr.sa_data[2],
                   (unsigned char)astReq[lInterfaceNum].ifr_hwaddr.sa_data[3],
                   (unsigned char)astReq[lInterfaceNum].ifr_hwaddr.sa_data[4],
                   (unsigned char)astReq[lInterfaceNum].ifr_hwaddr.sa_data[5]);
            /*����豸Mac��ַ*/
            dos_strcpy(g_pastNet[lLength]->szMacAddress, szMac);
            g_pastNet[lLength]->szMacAddress[dos_strlen(szMac)] = '\0';
         }
         else
         {
            logr_error("%s:Line %d:mon_get_netcard_data|get netcard data failure,error no is %s"
                        , dos_get_filename(__FILE__), __LINE__, strerror(errno));
            goto failure;
         }
             
         if (!ioctl(lFd, SIOCGIFADDR, (S8 *)&astReq[lInterfaceNum]))
         {     
            dos_snprintf(szIPv4Addr, sizeof(szIPv4Addr), "%s",
               (S8 *)inet_ntoa(((struct sockaddr_in *)&(astReq[lInterfaceNum].ifr_addr))->sin_addr));

            /*����豸ip��ַ*/
            dos_strcpy(g_pastNet[lLength]->szIPAddress, szIPv4Addr);
            g_pastNet[lLength]->szIPAddress[dos_strlen(szIPv4Addr)] = '\0';
         }
         else
         {
           logr_error("%s:Line %d:mon_get_netcard_data|get netcard data failure,error no is %s"
                        , dos_get_filename(__FILE__), __LINE__, strerror(errno));
           goto failure;
         }

         if (!ioctl(lFd, SIOCGIFBRDADDR, &astReq[lInterfaceNum]))
         {
            dos_snprintf(szBroadAddr, sizeof(szBroadAddr), "%s",
               (S8 *)inet_ntoa(((struct sockaddr_in *)&(astReq[lInterfaceNum].ifr_broadaddr))->sin_addr));
            /*����豸�㲥ip��ַ*/
            dos_strcpy(g_pastNet[lLength]->szBroadIPAddress, szBroadAddr);
            g_pastNet[lLength]->szBroadIPAddress[dos_strlen(szBroadAddr)] = '\0';
         }
         else
         {
            logr_error("%s:Line %d:mon_get_netcard_data|get netcard data failure,error no is %s"
                        , dos_get_filename(__FILE__), __LINE__, strerror(errno));
            goto failure;
         }

         if (!ioctl(lFd, SIOCGIFNETMASK, &astReq[lInterfaceNum]))
         {
            dos_snprintf(szSubnetMask, sizeof(szSubnetMask), "%s",
               (S8 *)inet_ntoa(((struct sockaddr_in *)&(astReq[lInterfaceNum].ifr_netmask))->sin_addr));
            /*����豸��������*/
            dos_strcpy(g_pastNet[lLength]->szNetMask, szSubnetMask);
            g_pastNet[lLength]->szNetMask[dos_strlen(szSubnetMask)] = '\0';
         }
         else
         {
             logr_error("%s:Line %d:mon_get_netcard_data|get netcard data failure,error no is %s"
                        , dos_get_filename(__FILE__), __LINE__, strerror(errno));
             goto failure;
         }

         if (0 == dos_strcmp("lo", g_pastNet[lLength]->szNetDevName))
         {
             g_pastNet[lLength]->lRWSpeed = -1;
             continue;
         }
         
         lRet = mon_get_data_trans_speed(g_pastNet[lLength]->szNetDevName);
         if (0 > lRet)
         {
             logr_error("%s:Line %d: Get data transport speed FAIL."
                        , dos_get_filename(__FILE__), __LINE__);
             goto failure;
         }
         
         g_pastNet[lLength]->lRWSpeed = lRet;
         
         lLength++;
      }
   }
   else
   {
     logr_error("%s:Line %d:mon_get_netcard_data|get netcard data failure,error no is %s"
                , dos_get_filename(__FILE__), __LINE__, strerror(errno));
     goto failure;
   }
   goto success;

failure:
   close(lFd);
   lFd = -1;
   return DOS_FAIL;
success:
   close(lFd);
   lFd = -1;
   return DOS_SUCC;
}

/**
 * ����:��ȡ����������Ϣ�ĸ�ʽ����Ϣ�ַ���
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
S32  mon_netcard_formatted_info()
{
    S32 lRows = 0;

    memset(g_szMonNetworkInfo, 0, MAX_NETCARD_CNT * MAX_BUFF_LENGTH);
    
    for (lRows = 0; lRows < g_lNetCnt; lRows++)
    {
       S8 szTempInfo[MAX_BUFF_LENGTH] = {0};

       if(!g_pastNet[lRows])
       {
          logr_cirt("%s:Line %d:mon_netcard_formatted_info|get netcard formatted information failure,m_pastNet[%d] is %p!"
                    , dos_get_filename(__FILE__), __LINE__, lRows, g_pastNet[lRows]);
          return DOS_FAIL;
       }
       
       dos_snprintf(szTempInfo, MAX_BUFF_LENGTH
                    , "Netcard:%s\nMacAddress:%s\nIPAddress:%s\nBroadIPAddress:%s\nSubNetMask:%s\nData Transmission speed:%dMb/s\n"
                    , g_pastNet[lRows]->szNetDevName
                    , g_pastNet[lRows]->szMacAddress
                    , g_pastNet[lRows]->szIPAddress
                    , g_pastNet[lRows]->szBroadIPAddress
                    , g_pastNet[lRows]->szNetMask
                    , g_pastNet[lRows]->lRWSpeed);
        dos_strcat(g_szMonNetworkInfo, szTempInfo);
        dos_strcat(g_szMonNetworkInfo, "\n");
    }

    return DOS_SUCC;
}

#endif //end #if INCLUDE_NET_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif