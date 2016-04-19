#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>
#include <dos/dos_mem.h>

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
#include "mon_warning_msg_queue.h"
#include "mon_lib.h"
#include "mon_def.h"
#include "mon_mail.h"

typedef struct tagMonTransData
{
    U64 uLInSize;
    U64 uLOutSize;
}MON_TRANS_DATA_S;

extern MON_NET_CARD_PARAM_S * g_pastNet[MAX_NETCARD_CNT];
extern U32 g_ulNetCnt;
extern MON_THRESHOLD_S *g_pstCond;
extern MON_WARNING_MSG_S *g_pstWarningMsg;

//������¼����ʱ���
time_t *m_pt1 = NULL, *m_pt2 = NULL;

//������¼֮ǰ�ض�ʱ�̺͵�ǰʱ�̵����ݴ���
MON_TRANS_DATA_S *m_pstTransFormer = NULL;
MON_TRANS_DATA_S *m_pstTransCur = NULL;

static U32 mon_net_reset_data();

/**
 * ����:Ϊ������������ڴ�
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_netcard_malloc()
{
    U32 ulRows = 0;
    MON_NET_CARD_PARAM_S * pastNet;

    pastNet = (MON_NET_CARD_PARAM_S *)dos_dmem_alloc(MAX_NETCARD_CNT * sizeof(MON_NET_CARD_PARAM_S));
    if(DOS_ADDR_INVALID(pastNet))
    {
        DOS_ASSERT(0);
        goto fail;
    }

    dos_memzero(pastNet, MAX_NETCARD_CNT * sizeof(MON_NET_CARD_PARAM_S));
    for(ulRows = 0; ulRows < MAX_NETCARD_CNT; ulRows++)
    {
        g_pastNet[ulRows] = &(pastNet[ulRows]);
    }

    m_pstTransFormer = (MON_TRANS_DATA_S *)dos_dmem_alloc(sizeof(MON_TRANS_DATA_S));
    if (DOS_ADDR_INVALID(m_pstTransFormer))
    {
        DOS_ASSERT(0);
        goto fail;
    }
    dos_memzero(m_pstTransFormer, sizeof(MON_TRANS_DATA_S));

    m_pstTransCur = (MON_TRANS_DATA_S *)dos_dmem_alloc(sizeof(MON_TRANS_DATA_S));
    if (DOS_ADDR_INVALID(m_pstTransCur))
    {
        DOS_ASSERT(0);
        goto fail;
    }
    dos_memzero(m_pstTransCur, sizeof(MON_TRANS_DATA_S));

    m_pt1 = dos_dmem_alloc(sizeof(time_t));
    if (DOS_ADDR_INVALID(m_pt1))
    {
        DOS_ASSERT(0);
        goto fail;
    }

    m_pt2 = dos_dmem_alloc(sizeof(time_t));
    if (DOS_ADDR_INVALID(m_pt2))
    {
        DOS_ASSERT(0);
        goto fail;
    }

    return DOS_SUCC;
fail:
    if (DOS_ADDR_VALID(pastNet))
    {
        dos_dmem_free(pastNet);
        pastNet = NULL;
    }
    if (DOS_ADDR_VALID(g_pastNet[0]))
    {
        dos_dmem_free( g_pastNet[0]);
        g_pastNet[0] = NULL;
    }
    if (DOS_ADDR_VALID(m_pstTransFormer))
    {
        dos_dmem_free(m_pstTransFormer);
        m_pstTransFormer = NULL;
    }
    if (DOS_ADDR_VALID(m_pstTransCur))
    {
        dos_dmem_free(m_pstTransCur);
        m_pstTransCur = NULL;
    }
    if (DOS_ADDR_VALID(m_pt1))
    {
        dos_dmem_free(m_pt1);
        m_pt1 = NULL;
    }
    if (DOS_ADDR_VALID(m_pt2))
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
U32  mon_netcard_free()
{
    U32 ulRows = 0;
    MON_NET_CARD_PARAM_S * pastNet = g_pastNet[0];

    if(DOS_ADDR_INVALID(pastNet))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    dos_dmem_free(pastNet);

    for(ulRows = 0; ulRows < MAX_NETCARD_CNT; ulRows++)
    {
        g_pastNet[ulRows] = NULL;
    }

    if (DOS_ADDR_VALID(m_pstTransCur))
    {
        dos_dmem_free(m_pstTransCur);
        m_pstTransCur = NULL;
    }

    if (DOS_ADDR_VALID(m_pstTransFormer))
    {
        dos_dmem_free(m_pstTransFormer);
        m_pstTransFormer = NULL;
    }

    if (DOS_ADDR_VALID(m_pt1))
    {
        dos_dmem_free(m_pt1);
        m_pt1 = NULL;
    }
    if (DOS_ADDR_VALID(m_pt2))
    {
        dos_dmem_free(m_pt2);
        m_pt2 = NULL;
    }

    return DOS_SUCC;
}

static U32 mon_net_reset_data()
{
    MON_NET_CARD_PARAM_S * pastNet = g_pastNet[0];
    if (DOS_ADDR_INVALID(pastNet))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    dos_memzero(pastNet, MAX_NETCARD_CNT * sizeof(MON_NET_CARD_PARAM_S));

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
            logr_error("%s:Line %u: File \"%s\" has no content.", dos_get_filename(__FILE__), __LINE__, szCarrierPath);
            bRet = DOS_FALSE;
        }
    }
    else
    {
        logr_error("%s:Line %u:file \"%s\" open FAIL."
                    , dos_get_filename(__FILE__), __LINE__, szCarrierPath);
        bRet = DOS_FALSE;
    }

    if (DOS_ADDR_VALID(pstNetFp))
    {
        fclose(pstNetFp);
        pstNetFp = NULL;
    }

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
U32 mon_get_data_trans_speed(const S8 * pszDevName)
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
        mon_trace(MON_TRACE_NET, LOG_LEVEL_WARNING, "First time Running.");
    }

    m_pstTransFormer->uLInSize = m_pstTransCur->uLInSize;
    m_pstTransFormer->uLOutSize = m_pstTransCur->uLOutSize;

    fp = fopen(szNetFile, "r");
    if (DOS_ADDR_INVALID(fp))
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    while(NULL != fgets(szNetInfo, sizeof(szNetInfo), fp))
    {
        pszInfo = dos_strstr(szNetInfo, (S8 *)pszDevName);
        if (DOS_ADDR_VALID(pszInfo))
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
        mon_trace(MON_TRACE_NET, LOG_LEVEL_ERROR, "Analyse buffer by Regular expression FAIL.");
        fclose(fp);
        return DOS_FAIL;
    }

    if (dos_atoull(ppszData[0], &uLIn) < 0
        || dos_atoull(ppszData[8], &uLOut) < 0)
    {
        DOS_ASSERT(0);
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
U32 mon_get_netcard_data()
{
    S32 lFd = 0;
    U32 ulLength = 0, ulRet = 0;
    U32 ulInterfaceNum = 0;
    struct ifreq astReq[16];
    struct ifconf stIfc;
    struct ifreq stIfrcopy;
    S8  szMac[32] = {0};
    S8  szIPv4Addr[32] = {0};
    S8  szBroadAddr[32] = {0};
    S8  szSubnetMask[32] = {0};

    ulRet = mon_net_reset_data();
    if (DOS_SUCC != ulRet)
    {
        mon_trace(MON_TRACE_NET, LOG_LEVEL_ERROR, "Reset netcard Data FAIL.");
        return DOS_FAIL;
    }

    if ((lFd = socket(AF_INET, SOCK_DGRAM, 0)) < 0)
    {
        mon_trace(MON_TRACE_NET, LOG_LEVEL_ERROR, "Socket FAIL, fd:%d, errno:%d,cause:%s.", lFd, errno, strerror(errno));
        goto failure;
    }

    stIfc.ifc_len = sizeof(astReq);
    stIfc.ifc_buf = (caddr_t)astReq;
    if (!ioctl(lFd, SIOCGIFCONF, (S8 *)&stIfc))
    {
        /* ��ȡ�����豸�ĸ��� */
        ulInterfaceNum = g_ulNetCnt = stIfc.ifc_len / sizeof(struct ifreq);
        while (ulInterfaceNum > 0)
        {
            ulInterfaceNum--;
            /*����豸����*/

            if(DOS_ADDR_INVALID(g_pastNet[ulLength]))
            {
                  DOS_ASSERT(0);
                  goto failure;
            }


            dos_strcpy(g_pastNet[ulLength]->szNetDevName, astReq[ulInterfaceNum].ifr_name);
            (g_pastNet[ulLength]->szNetDevName)[dos_strlen(astReq[ulInterfaceNum].ifr_name)] = '\0';
            stIfrcopy = astReq[ulInterfaceNum];

            if (ioctl(lFd, SIOCGIFFLAGS, &stIfrcopy))
            {
                mon_trace(MON_TRACE_NET, LOG_LEVEL_ERROR, "Get netcard data FAIL,errno:%d,cause:%s.", errno, strerror(errno));
                goto failure;
            }

            if (!ioctl(lFd, SIOCGIFHWADDR, (S8 *)(&astReq[ulInterfaceNum])))
            {
                dos_memzero(szMac, sizeof(szMac));
                dos_snprintf(szMac, sizeof(szMac), "%02x:%02x:%02x:%02x:%02x:%02x",
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[0],
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[1],
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[2],
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[3],
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[4],
                       (U8)astReq[ulInterfaceNum].ifr_hwaddr.sa_data[5]);
                /*����豸Mac��ַ*/
                dos_strcpy(g_pastNet[ulLength]->szMacAddress, szMac);
                g_pastNet[ulLength]->szMacAddress[dos_strlen(szMac)] = '\0';
            }
            else
            {
                mon_trace(MON_TRACE_NET, LOG_LEVEL_ERROR, "Get Mac Address FAIL, errno:%d, cause:%s.", errno, strerror(errno));
                goto failure;
            }

            if (!ioctl(lFd, SIOCGIFADDR, (S8 *)&astReq[ulInterfaceNum]))
            {
                dos_snprintf(szIPv4Addr, sizeof(szIPv4Addr), "%s",
                     (S8 *)inet_ntoa(((struct sockaddr_in *)&(astReq[ulInterfaceNum].ifr_addr))->sin_addr));

                /*����豸ip��ַ*/
                dos_strcpy(g_pastNet[ulLength]->szIPAddress, szIPv4Addr);
                g_pastNet[ulLength]->szIPAddress[dos_strlen(szIPv4Addr)] = '\0';
            }
            else
            {
                mon_trace(MON_TRACE_NET, LOG_LEVEL_ERROR, "Get IPv4 Address FAIL, errno:%d, cause:%s.", errno, strerror(errno));
                goto failure;
            }

            if (!ioctl(lFd, SIOCGIFBRDADDR, &astReq[ulInterfaceNum]))
            {
                dos_snprintf(szBroadAddr, sizeof(szBroadAddr), "%s",
                     (S8 *)inet_ntoa(((struct sockaddr_in *)&(astReq[ulInterfaceNum].ifr_broadaddr))->sin_addr));
                /*����豸�㲥ip��ַ*/
                dos_strcpy(g_pastNet[ulLength]->szBroadIPAddress, szBroadAddr);
                g_pastNet[ulLength]->szBroadIPAddress[dos_strlen(szBroadAddr)] = '\0';
            }
            else
            {
                mon_trace(MON_TRACE_NET, LOG_LEVEL_ERROR, "Get Broadcast IP Address FAIL,errno:%d,cause:%s.", errno, strerror(errno));
                goto failure;
            }

            if (!ioctl(lFd, SIOCGIFNETMASK, &astReq[ulInterfaceNum]))
            {
                dos_snprintf(szSubnetMask, sizeof(szSubnetMask), "%s",
                   (S8 *)inet_ntoa(((struct sockaddr_in *)&(astReq[ulInterfaceNum].ifr_netmask))->sin_addr));
                /*����豸��������*/
                dos_strcpy(g_pastNet[ulLength]->szNetMask, szSubnetMask);
                g_pastNet[ulLength]->szNetMask[dos_strlen(szSubnetMask)] = '\0';
            }
            else
            {
                mon_trace(MON_TRACE_NET, LOG_LEVEL_ERROR, "Get Subnet Mask FAIL,errno:%d,cause:%s.", errno, strerror(errno));
                goto failure;
            }

            if (0 == dos_strcmp("lo", g_pastNet[ulLength]->szNetDevName))
            {
                g_pastNet[ulLength]->ulRWSpeed = 0;
                continue;
            }

            ulRet = mon_get_data_trans_speed(g_pastNet[ulLength]->szNetDevName);
            if (DOS_FAIL == ulRet)
            {
                mon_trace(MON_TRACE_NET, LOG_LEVEL_ERROR, "Get Current bandwidth FAIL.");
                goto failure;
            }

            g_pastNet[ulLength]->ulRWSpeed = ulRet;

            ulLength++;
        }
    }
    else
    {
        mon_trace(MON_TRACE_NET, LOG_LEVEL_ERROR, "Get netcard data FAIL,errno:%d,cause:%s.", errno, strerror(errno));
        goto failure;
    }
    goto success;

failure:
    close(lFd);
    lFd = U32_BUTT;
    return DOS_FAIL;
success:
    close(lFd);
    lFd = U32_BUTT;
    return DOS_SUCC;
}

U32  mon_handle_netcard_warning()
{
    BOOL bAddToDB = DOS_FALSE, bNetExcept = DOS_FALSE;
    U32  ulRet = U32_BUTT, ulIndex = 0, ulRows = 0;
    MON_MSG_S  *pstMsg = NULL;

    ulRet = mon_generate_warning_id(NET_RES, 0x00, 0x00);
    if((U32)0xff == ulRet)
    {
        mon_trace(MON_TRACE_MH, LOG_LEVEL_ERROR, "Generate Warning ID FAIL.");
        return DOS_FAIL;
    }
    ulIndex = mon_get_msg_index(ulRet);
    if (U32_BUTT == ulIndex)
    {
        return DOS_FAIL;
    }

    for (ulRows = 0; ulRows < g_ulNetCnt; ++ulRows)
    {
        if(DOS_FALSE == mon_is_netcard_connected(g_pastNet[ulRows]->szNetDevName))
        {
            bNetExcept = DOS_TRUE;
        }
    }

    if (DOS_TRUE == bNetExcept)
    {
        if (DOS_FALSE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                DOS_ASSERT(0);
            }

            GENERATE_WARNING_MSG(pstMsg,ulIndex,ulRet);
            ulRet = mon_send_email(1, "Network connection Warning", (S8 *)pstMsg->msg, MON_WARNING_LEVEL_MAJOR, MON_WARNING_TYPE_MAIL);
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
            }

            bAddToDB = DOS_TRUE;
        }
    }
    else
    {
        if (DOS_TRUE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                DOS_ASSERT(0);
            }

            GENERATE_NORMAL_MSG(pstMsg,ulIndex,ulRet);
            ulRet = mon_send_email(1, "Network connection Warning Recovery", (S8 *)pstMsg->msg, MON_WARNING_LEVEL_MAJOR, MON_WARNING_TYPE_MAIL);
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

U32  mon_handle_bandwidth_warning()
{
    BOOL bAddToDB = DOS_FALSE, bNetBWExcept = DOS_FALSE;
    U32  ulRet = U32_BUTT, ulIndex = 0, ulRows = 0;
    MON_MSG_S *pstMsg = NULL;

    ulRet = mon_generate_warning_id(NET_RES, 0x00, 0x01);
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

    for (ulRows = 0; ulRows < g_ulNetCnt; ++ulRows)
    {
        if (0 != dos_stricmp(g_pastNet[ulRows]->szNetDevName, "lo")
            && g_pastNet[ulRows]->ulRWSpeed >= g_pstCond->ulMaxBandWidth * 1024)
        {
            bNetBWExcept = DOS_TRUE;
        }
    }

    if (DOS_TRUE == bNetBWExcept)
    {
        if (DOS_FALSE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }

            GENERATE_WARNING_MSG(pstMsg, ulIndex, ulRet);
            ulRet = mon_send_email(1, "Network Bandwidth Warning", (S8 *)pstMsg->msg, MON_WARNING_LEVEL_MAJOR, MON_WARNING_TYPE_MAIL);
            if (ulRet != DOS_SUCC)
            {
                DOS_ASSERT(0);
            }

            bAddToDB = DOS_TRUE;
        }
    }
    else
    {
        if (DOS_TRUE == g_pstWarningMsg[ulIndex].bExcep)
        {
            pstMsg  = (MON_MSG_S *)dos_dmem_alloc(sizeof(MON_MSG_S));
            if (DOS_ADDR_INVALID(pstMsg))
            {
                DOS_ASSERT(0);
                return DOS_FAIL;
            }

            GENERATE_NORMAL_MSG(pstMsg,ulIndex,ulRet);
            ulRet = mon_send_email(1, "Network Bandwidth Warning Recovery", (S8 *)pstMsg->msg, MON_WARNING_LEVEL_MAJOR, MON_WARNING_TYPE_MAIL);
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

#endif //end #if INCLUDE_NET_MONITOR
#endif //end #if INCLUDE_RES_MONITOR
#endif //end #if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif