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
#include "mon_string.h"

extern S8 g_szMonNetworkInfo[MAX_NETCARD_CNT * MAX_BUFF_LENGTH];
extern MON_NET_CARD_PARAM_S * g_pastNet[MAX_NETCARD_CNT];
extern S32 g_lNetCnt;

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
      return DOS_FAIL;
   }

   memset(pastNet, 0, MAX_NETCARD_CNT * sizeof(MON_NET_CARD_PARAM_S));
   for(lRows = 0; lRows < MAX_NETCARD_CNT; lRows++)
   {
      g_pastNet[lRows] = &(pastNet[lRows]);
   }
   
   return DOS_SUCC;
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

   return DOS_SUCC;
} 

/** ethtool eth0
 * Settings for eth0:
 *       Supported ports: [ TP ]
 *       Supported link modes:   10baseT/Half 10baseT/Full 
 *                               100baseT/Half 100baseT/Full 
 *                               1000baseT/Full 
 *       Supported pause frame use: No
 *       Supports auto-negotiation: Yes
 *       Advertised link modes:  10baseT/Half 10baseT/Full 
 *                               100baseT/Half 100baseT/Full 
 *  ......
 *       Wake-on: d
 *       Current message level: 0x00000007 (7)
 *                              drv probe link
 *       Link detected: yes
 * ����:�ж������������Ƿ���
 * ��������
 *   ����1:const S8 * pszNetCard ��������
 * ����ֵ��
 *   �����򷵻�DOS_TRUE�����򷵻�DOS_FALSE
 */
 BOOL mon_is_netcard_connected(const S8 * pszNetCard)
 {
   S8 szEthCmd[MAX_CMD_LENGTH] = {0};
   S8 szNetFile[] = "network";
   S8 szLine[MAX_BUFF_LENGTH] = {0};
   FILE * fp;

   if(!pszNetCard)
   {
      logr_cirt("%s:Line %d:mon_is_netcard_connected|pszNetCard is %p!",
                dos_get_filename(__FILE__), __LINE__, pszNetCard);
      return DOS_FALSE;
   }

   dos_snprintf(szEthCmd, MAX_CMD_LENGTH, "ethtool %s | tail -n 1 > %s", pszNetCard, szNetFile);
   szEthCmd[dos_strlen(szEthCmd)] = '\0';
   system(szEthCmd);

   fp = fopen(szNetFile, "r");
   if (!fp)
   {
      logr_cirt("%s:line %d:mon_is_netcard_connected|file \'%s\' open failed,fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szNetFile, fp);
      return DOS_FALSE;
   }

   fseek(fp, 0, SEEK_SET);
   if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
   {
      /**
       * �ж�����:�ж�Link detected��ߵ��ַ����Ƿ�Ϊyes��������������
       */
      if (mon_is_ended_with(szLine, "yes\n"))
      {
          goto success;
      }
   }
   goto failure;

success:
   fclose(fp);
   fp = NULL;
   unlink(szNetFile);   
   return DOS_TRUE;

failure:
   fclose(fp);
   fp = NULL;
   unlink(szNetFile);        
   return DOS_FALSE;
}

/**
 * ����������mon_is_netcard_connected����
 * ����:��ȡ���ڵ����������
 * ��������
 *   ����1:const S8 * pszDevName ��������
 * ����ֵ��
 *   �ɹ��������ڵ�������ݴ������ʣ�ʧ�ܷ���-1
 */
S32 mon_get_max_trans_speed(const S8 * pszDevName)
{
   S8  szEthCmd[MAX_CMD_LENGTH] = {0};
   S8  szSpeedFile[] = "speedfile";
   S32 lSpeed = 0;
   FILE * fp;

   if(!pszDevName)
   {
      logr_cirt("%s:Line %d:mon_get_max_trans_speed|pszDevName is %p!"
                , dos_get_filename(__FILE__), __LINE__, pszDevName);
      return -1;
   }
   
   dos_snprintf(szEthCmd, MAX_CMD_LENGTH, "ethtool %s > %s"
                , pszDevName, szSpeedFile);
   system(szEthCmd);
   
   fp = fopen(szSpeedFile, "r");
   if(!fp)
   {
      logr_cirt("%s:Line %d:mon_get_max_trans_speed|file \'%s\' open failure,fp is %p!"
                , dos_get_filename(__FILE__), __LINE__, szSpeedFile, fp);
      return -1;
   }
   
   fseek(fp, 0, SEEK_SET);
   while (!feof(fp))
   {
      S8 szLine[MAX_BUFF_LENGTH] = {0};
      if (NULL != (fgets(szLine, MAX_BUFF_LENGTH, fp)))
      {
         if (mon_is_substr(szLine, "Speed"))
         {
            lSpeed = mon_first_int_from_str(szLine);
            if(-1 == lSpeed)
            {
               logr_error("%s:Line %d:mon_get_max_trans_speed|lSpeed is %d!"
                            , dos_get_filename(__FILE__), __LINE__, lSpeed);
               goto failure;
            }  
            logr_debug("%s:Line %d:mon_get_max_trans_speed|lSpeed is %d!"
                        , dos_get_filename(__FILE__), __LINE__, lSpeed);
            goto success;
         }     
      }
   }  
   logr_notice("%s:Line %d:mon_get_max_trans_speed|the device %s is not exist",
               dos_get_filename(__FILE__), __LINE__, pszDevName);
   goto failure;

               
failure:
   fclose(fp);
   fp = NULL;
   unlink(szSpeedFile);
   return -1;
success:
   fclose(fp);
   fp = NULL;
   unlink(szSpeedFile);
   return lSpeed;
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
   S32 lFd;
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

         /* ��ȡ���ݵĴ������� */
         if (0 == (dos_strcmp(g_pastNet[lLength]->szNetDevName, "lo")))
         {  // "lo"�Ĵ�������Ĭ��Ϊ-1
            g_pastNet[lLength]->lRWSpeed = -1;
         }
         else
         {
            S32 lRet = mon_get_max_trans_speed(g_pastNet[lLength]->szNetDevName);
            if(-1 == lRet)
            {
               logr_error("%s:Line %d:mon_get_netcard_data|get max transspeed failure,lRet is %d!"
                            , dos_get_filename(__FILE__), __LINE__, lRet);
               goto failure;
            }
            g_pastNet[lLength]->lRWSpeed = lRet;
         }
         
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