#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* include system header file */
#include <pthread.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
#include <fcntl.h>
#include <sys/select.h>
#include <limits.h>
#include <netdb.h>
#include <ctype.h>

/* include the dos header files */
#include <dos.h>
#include <pt/dos_sqlite3.h>
#include <pt/pt.h>
#include <pt/pts.h>
/* include provate header file */
#include "pts_telnet.h"
#include "../telnetd/telnetd.h"
#include "pts_msg.h"

#define PTS_TELNET_HTLP "\rlist    : list [����ip]\r\n\
\rlook    : look id\r\n\
\rconnect : connect id/sn [ip][port]\r\n"

PTS_CMD_CLIENT_CB_ST g_astCmdClient[PTS_MAX_CLIENT_NUMBER];

U32 pts_get_port_from_string(S8 *szParam, U16 *pusPort)
{
    if (NULL == szParam || NULL == pusPort)
    {
        return DOS_FAIL;
    }

    S8 *szSub = szParam;
    U32 ulPort = 0;

    while (*szSub)
    {
        if (*szSub < '0' || *szSub > '9')
        {
            return DOS_FAIL;
        }
        szSub++;
    }

    ulPort = atoi(szParam);
    if (ulPort > PTS_MAX_PORT)
    {
        return DOS_FAIL;
    }

    *pusPort = ulPort;

    return DOS_SUCC;
}

/**
 * ������S32 pts_cmd_client_list_add(S32 lSocket, U32 ulStreamID, U8 *pucPtcID)
 * ���ܣ�
 *      1.���cmd�ͻ���������
 * ����
 * ����ֵ:
 */
S32 pts_cmd_client_list_add(S32 lSocket, U32 ulStreamID, U8 *pucPtcID)
{
    S32 i = 0;

    if (NULL == pucPtcID)
    {
        return DOS_FAIL;
    }

    for (i=0; i<PTS_MAX_CLIENT_NUMBER; i++)
    {
        if (!g_astCmdClient[i].bIsValid)
        {
            g_astCmdClient[i].ulStreamID = ulStreamID;
            g_astCmdClient[i].lSocket = lSocket;
            g_astCmdClient[i].bIsValid = DOS_TRUE;
            dos_memcpy(g_astCmdClient[i].aucID, pucPtcID, PTC_ID_LEN);

            return DOS_SUCC;
        }
    }

    return DOS_FAIL;
}

/**
 * ������S32 pts_cmd_client_search_by_socket(S32 lSocket)
 * ���ܣ�������socket�������в��ҽڵ�
 * ����
 * ����ֵ:
 */
S32 pts_cmd_client_search_by_socket(S32 lSocket)
{

    S32 i = 0;

    for (i=0; i<PTS_MAX_CLIENT_NUMBER; i++)
    {
        if (g_astCmdClient[i].bIsValid && g_astCmdClient[i].lSocket == lSocket)
        {
            return i;
        }
    }

    return DOS_FAIL;
}

/**
 * ������S32 pts_cmd_client_search_by_stream(U32 ulStream)
 * ���ܣ�����stream�������в��ҽڵ�
 * ����
 * ����ֵ:
 */
S32 pts_cmd_client_search_by_stream(U32 ulStream)
{

    S32 i = 0;

    for (i=0; i<PTS_MAX_CLIENT_NUMBER; i++)
    {
        if (g_astCmdClient[i].bIsValid && g_astCmdClient[i].ulStreamID == ulStream)
        {
            return i;
        }
    }

    return DOS_FAIL;
}

/**
 * ������S32 pts_cmd_client_delete(U32 lSub)
 * ���ܣ�ɾ���ڵ�
 * ����
 * ����ֵ:
 */
VOID pts_cmd_client_delete(U32 ulClientIndex)
{
    S32 i = 0;
    PT_CC_CB_ST *pstPtcSendNode = NULL;
    PT_MSG_TAG stMsgDes;

    for (i=0; i< PTS_MAX_CLIENT_NUMBER; i++)
    {
        if (g_astCmdClient[i].lSocket == ulClientIndex)
        {
            g_astCmdClient[i].bIsValid = DOS_FALSE;
            /* �ͷ�pts�շ�����, ֪ͨptc�ͷ���Դ */
            dos_memcpy(stMsgDes.aucID, g_astCmdClient[i].aucID, PTC_ID_LEN);
            stMsgDes.enDataType = PT_DATA_CMD;
            stMsgDes.ulStreamID = g_astCmdClient[i].ulStreamID;
#if PT_MUTEX_DEBUG
            pts_send_pthread_mutex_lock(__FILE__, __LINE__);
#else
            pthread_mutex_lock(&g_pts_mutex_send);
#endif
            pstPtcSendNode = pt_ptc_list_search(g_pstPtcListSend, g_astCmdClient[i].aucID);
            if (pstPtcSendNode != NULL)
            {
                pts_send_exit_notify_to_ptc(&stMsgDes, pstPtcSendNode);
                pts_delete_send_stream_node(&stMsgDes, pstPtcSendNode, DOS_FALSE);
            }
#if PT_MUTEX_DEBUG
            pts_send_pthread_mutex_unlock(__FILE__, __LINE__);
#else
            pthread_mutex_unlock(&g_pts_mutex_send);
#endif
            pts_delete_recv_stream_node(&stMsgDes, NULL, DOS_FALSE);
            return;
        }

    }
    pt_logr_debug("telnet not found Client, ClientIndex is %d", ulClientIndex);
}

/**
 * ������S32 pts_send_ptc_list2cmd_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
 * ���ܣ��������У���ȡptc�б�Ļص�����
 * ����
 * ����ֵ:
 */
S32 pts_send_ptc_list2cmd_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    U32 ulClientIndex = *(U32 *)para;

    S8 szPtcListNode[PT_DATA_BUFF_128] = {0};
    snprintf(szPtcListNode, sizeof(szPtcListNode), "\r%-20s%-20s%-20s%-20s\n", column_value[0], column_value[1], column_value[2], column_value[3]);

    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListNode, dos_strlen(szPtcListNode));
    return DOS_SUCC;
}

/**
 * ������VOID pts_send_ptc_list2cmd(U32 ulClientIndex)
 * ���ܣ�����ptc����cmd
 * ����
 * ����ֵ:
 */
VOID pts_send_ptc_list2cmd(U32 ulClientIndex)
{
    S8  achSql[PTS_SQL_STR_SIZE] = {0};
    S8 szPtcListHead[PT_DATA_BUFF_128] = {0};
    snprintf(szPtcListHead, sizeof(szPtcListHead), "%-20s%-20s%-20s%-20s\n", "ID", "SN", "name", "alias");

    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListHead, dos_strlen(szPtcListHead));
    sprintf(achSql, "select * from ipcc_alias where register = 1");
    dos_sqlite3_exec_callback(g_stMySqlite, achSql, pts_send_ptc_list2cmd_callback, &ulClientIndex);
}

VOID pts_send_ptc_list2cmd_by_internetIP(U32 ulClientIndex, S8 *szInternetIP)
{
    S8  achSql[PTS_SQL_STR_SIZE] = {0};
    S8 szPtcListHead[PT_DATA_BUFF_128] = {0};
    snprintf(szPtcListHead, sizeof(szPtcListHead), "%-20s%-20s%-20s%-20s\n", "ID", "SN", "name", "alias");

    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListHead, dos_strlen(szPtcListHead));
    sprintf(achSql, "select * from ipcc_alias where register = 1 and internetIP = '%s'", szInternetIP);
    dos_sqlite3_exec_callback(g_stMySqlite, achSql, pts_send_ptc_list2cmd_callback, &ulClientIndex);
}

/**
 * ������S32 pts_get_password(S8 *szUserName, S8 *pcPassWord, S32 lPasswLen)
 * ���ܣ���ȡ�û���½����
 * ����
 * ����ֵ:
 */
S32 pts_get_password(S8 *szUserName, S8 *pcPassWord, S32 lPasswLen)
{
    S8 achSql[PTS_SQL_STR_SIZE] = {0};

    dos_memzero(pcPassWord, lPasswLen);
    sprintf(achSql, "select password from pts_user where name='%s';", szUserName);
    dos_sqlite3_exec_callback(g_stMySqlite, achSql, pts_get_password_callback, (void *)pcPassWord);
    if (dos_strlen(pcPassWord) > 0)
    {
        return PTS_GET_PASSW_SUCC;
    }
    else
    {
        return PTS_USER_NO_EXITED;
    }

}

S32 pts_get_sn_by_id(S8 *szID, S8 *szSN, S32 lLen)
{
    S8 achSql[PTS_SQL_STR_SIZE] = {0};

    dos_memzero(szSN, lLen);
    sprintf(achSql, "select sn from ipcc_alias where id=%s and register = 1;", szID);
    dos_sqlite3_exec_callback(g_stMySqlite, achSql, pts_get_password_callback, (void *)szSN);
    if (dos_strlen(szSN) > 0)
    {
        return DOS_SUCC;
    }
    else
    {
        return DOS_FAIL;
    }

}

S32 pts_look_ptc_detail_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name)
{
    U32 ulClientIndex = *(U32 *)para;

    S8 szPtcListNode[PT_DATA_BUFF_128] = {0};
    snprintf(szPtcListNode, sizeof(szPtcListNode), "%-20s%-20s%-20s%-20s\n", "ID", "SN", "name", "alias");
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListNode, dos_strlen(szPtcListNode));
    snprintf(szPtcListNode, sizeof(szPtcListNode), "\r%-20s%-20s%-20s%-20s\n", column_value[0], column_value[1], column_value[2], column_value[3]);
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListNode, dos_strlen(szPtcListNode));
    snprintf(szPtcListNode, sizeof(szPtcListNode), "\r%-20s%-20s%-20s%-20s\n", "version", "LastLoginTime", "IntranetIP", "IntranetPort");
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListNode, dos_strlen(szPtcListNode));
    snprintf(szPtcListNode, sizeof(szPtcListNode), "\r%-20s%-20s%-20s%-20s\n", column_value[4], column_value[6], column_value[8], column_value[10]);
    telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szPtcListNode, dos_strlen(szPtcListNode));

    return DOS_SUCC;
}

VOID pts_look_ptc_detail_msg(U32 ulClientIndex, S8 *szPtcID)
{
    if (NULL == szPtcID)
    {
        return;
    }

    S8  achSql[PTS_SQL_STR_SIZE] = {0};
    sprintf(achSql, "select * from ipcc_alias where id = %s;", szPtcID);
    dos_sqlite3_exec_callback(g_stMySqlite, achSql, pts_look_ptc_detail_callback, &ulClientIndex);
}

/**
 * ������S32 cli_server_analyse_cmd(U32 ulClientIndex, U32 ulMode, S8 *szBuffer, U32 ulLength)
 * ���ܣ�����telnet���͹���������
 * ������
 *      U32 ulClientIndex��telnet�ͻ��˱��
 *      U32 ulMode����ǰtelnet�ͻ��˴���ʲôģʽ
 *      S8 *szBuffer�������
 *      U32 ulLength������泤��
 * ����ֵ��
 *      �ɹ�����0��ʧ�ܷ��أ�1
 */
S32 pts_server_cmd_analyse(U32 ulClientIndex, U32 ulMode, S8 *szBuffer, U32 ulLength)
{
    S8 *pszKeyWord[MAX_KEY_WORDS] = {0};
    S8 szErrorMsg[PT_DATA_BUFF_128], szCMDBak[MAX_CMD_LENGTH];
    S32 lRet = 0;
    S32 lKeyCnt = 0;
    S8 *pWord = NULL;
    S32 i=0;
    dos_strncpy(szCMDBak, szBuffer, MAX_CMD_LENGTH);
    szCMDBak[MAX_CMD_LENGTH - 1] = '\0';
    U32 ulStreamID = 0;
    S32 lResult = 0;
    S8 szDestIp[IPV6_SIZE] = {0};
    U16 usDestPort = 0;
    S8 achSql[PTS_SQL_STR_SIZE] = {0};
    S8 szSN[PT_DATA_BUFF_128] = {0};

    if (!szBuffer || !ulLength)
    {
        return PT_TELNET_FAIL;
    }
    logr_debug("Cli Server " "CMD " " Control Panel Recv CMD:%s", szBuffer);

    if ('\0' == szBuffer[0])
    {
        return lRet;
    }
    /* �������� */
    pWord = strtok(szBuffer, " ");
    while (pWord)
    {
        pszKeyWord[lKeyCnt] = dos_dmem_alloc(dos_strlen(pWord) + 1);
        if (!pszKeyWord[lKeyCnt])
        {
            logr_warning("%s", "Alloc fail.");
            lRet = PT_TELNET_FAIL;
            goto finished;
        }

        dos_strcpy(pszKeyWord[lKeyCnt], pWord);
        lKeyCnt++;
        pWord = strtok(NULL, " ");
        if (NULL == pWord)
        {
            break;
        }

        if (lKeyCnt >= MAX_KEY_WORDS)
        {
            break;
        }
    }

    if (dos_strcmp(pszKeyWord[0], "help") == 0)
    {
        telnet_send_data(ulClientIndex, MSG_TYPE_CMD, PTS_TELNET_HTLP, dos_strlen(PTS_TELNET_HTLP));
    }
    else if (dos_strcmp(pszKeyWord[0], "list") == 0)
    {
        /* �豸�б� */
        if (lKeyCnt == 1)
        {
            pts_send_ptc_list2cmd(ulClientIndex);
        }
        else
        {
            pts_send_ptc_list2cmd_by_internetIP(ulClientIndex, pszKeyWord[1]);
        }

        return PT_TELNET_LIST;
    }
    else if (dos_strcmp(pszKeyWord[0], "look") == 0)
    {
        /* �鿴�豸���� */
        if (lKeyCnt < 2)
        {
            snprintf(szErrorMsg, sizeof(szErrorMsg), "look id\r\n");
            lRet = PT_TELNET_FAIL;
            goto finished;
        }
        else
        {
            if (pts_is_int(pszKeyWord[1]))
            {
                sprintf(achSql,"select count(*) from ipcc_alias where id=%s", pszKeyWord[1]);
                if (!dos_sqlite3_record_is_exist(g_stMySqlite, achSql))  /* �ж��Ƿ���� */
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "ptc not exit id(%s)\r\n", pszKeyWord[1]);
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }
                pts_look_ptc_detail_msg(ulClientIndex, pszKeyWord[1]);
            }
            else
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "please input right id\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
        }
    }
    else if (dos_strcmp(pszKeyWord[0], "connect") == 0)
    {
        /* ����ptc */
        if (lKeyCnt < 2)
        {
            snprintf(szErrorMsg, sizeof(szErrorMsg), "connect id/sn [ip] [port]\r\n");
            lRet = PT_TELNET_FAIL;
            goto finished;
        }
        else
        {
            if (pts_is_ptc_id(pszKeyWord[1]))
            {
                dos_strcpy(szSN, pszKeyWord[1]);
            }
            else if (pts_is_int(pszKeyWord[1]))
            {
                lResult = pts_get_sn_by_id(pszKeyWord[1], szSN, PT_DATA_BUFF_128);
                if (lResult != DOS_SUCC)
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "get sn fail by id(%s)\r\n", pszKeyWord[1]);
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }
            }
            else if(pt_is_or_not_ip(pszKeyWord[1]) && lKeyCnt == 4)
            {
                lResult = pts_find_ptc_by_dest_addr(pszKeyWord[1], pszKeyWord[2], szSN);
                if (lResult != DOS_SUCC)
                {
                    /* û���ҵ�ptc */
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "not found ptc\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }
            }
            else
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "connect id/sn [ip] [port]\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
        }

        if (pts_is_ptc_id(szSN))
        {
            /* ��֤ptc ID�Ƿ��ǵ�½״̬ */
            sprintf(achSql,"select count(*) from ipcc_alias where sn='%s'", szSN);
            if (!dos_sqlite3_record_is_exist(g_stMySqlite, achSql))  /* �ж��Ƿ���� */
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "ptc not exit\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
            //sprintf(achSql,"select count(*) from ipcc_alias where sn='%s'", pszKeyWord[1]);
            //if (!dos_sqlite3_record_is_exist(g_stMySqlite, achSql))  /* �Ƿ�֧�������� */
            //{
            //    snprintf(szErrorMsg, sizeof(szErrorMsg), "ptc not support cmd\r\n");
            //    lRet = -1;
            //    goto finished;
            //}
            sprintf(achSql,"select count(*) from ipcc_alias where sn='%s' and register = 1", szSN);
            if (!dos_sqlite3_record_is_exist(g_stMySqlite, achSql))  /* �Ƿ��½ */
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "ptc not login\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }

            ulStreamID = pts_create_stream_id();
            lResult = pts_cmd_client_list_add(ulClientIndex, ulStreamID, (U8 *)szSN);
            if (lResult < 0)
            {
                lRet = PT_TELNET_FAIL;
                goto finished;
            }

            if (lKeyCnt == 2)
            {
                /* û������IP PORT��ʹ��Ĭ�ϵ� */
                dos_strncpy(szDestIp, PTS_TEL_SERV_IP_DEFAULT, IPV6_SIZE);
                usDestPort = PTS_TEL_SERV_PORT_DEFAULT;
            }
            else if (lKeyCnt == 3)
            {
                if (pt_is_or_not_ip(pszKeyWord[2]))
                {
                    dos_strncpy(szDestIp, pszKeyWord[2], IPV6_SIZE);
                    usDestPort = PTS_TEL_SERV_PORT_DEFAULT;
                }
                else if(pts_get_port_from_string(pszKeyWord[2], &usDestPort) == DOS_SUCC)
                {
                    dos_strncpy(szDestIp, PTS_TEL_SERV_IP_DEFAULT, IPV6_SIZE);
                }
                else
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "connect id/sn [ip] [port]\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }
            }
            else if (lKeyCnt == 4)
            {
                if (pt_is_or_not_ip(pszKeyWord[2]))
                {
                    dos_strncpy(szDestIp, pszKeyWord[2], IPV6_SIZE);
                }
                else
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "ip format error\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;

                }
                if (pts_get_port_from_string(pszKeyWord[3], &usDestPort) != DOS_SUCC)
                {
                    snprintf(szErrorMsg, sizeof(szErrorMsg), "port error\r\n");
                    lRet = PT_TELNET_FAIL;
                    goto finished;
                }
            }
            else
            {
                snprintf(szErrorMsg, sizeof(szErrorMsg), "connect [ip] [port]\r\n");
                lRet = PT_TELNET_FAIL;
                goto finished;
            }
            pts_save_msg_into_cache((U8 *)szSN, PT_DATA_CMD, ulStreamID, "", 0, szDestIp, usDestPort);
            lRet = PT_TELNET_CONNECT;
        }
        else
        {
             snprintf(szErrorMsg, sizeof(szErrorMsg), "please input right ptc ID\r\n");
             lRet = PT_TELNET_FAIL;
        }
    }
    else
    {
        snprintf(szErrorMsg, sizeof(szErrorMsg), "  Incorrect command.\r\n");
        lRet = PT_TELNET_FAIL;
    }
finished:
    for (i=0; i<lKeyCnt; i++)
    {
        if (pszKeyWord[i])
        {
            dos_dmem_free(pszKeyWord[i]);
        }
    }

    if (lRet < 0)
    {
        telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szErrorMsg, dos_strlen(szErrorMsg));
    }

    return lRet;

}

VOID pts_telnet_send_msg2ptc(U32 ulClientIndex, S8 *szBuff, U32 ulLen)
{
    S32 lResult = 0;
    S8 szErrorMsg[128] = {0};
    lResult = pts_cmd_client_search_by_socket(ulClientIndex);
    if (lResult < 0)
    {
        snprintf(szErrorMsg, sizeof(szErrorMsg), "can not found ptc\r\n");
        telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szErrorMsg, dos_strlen(szErrorMsg));
        snprintf(szErrorMsg, sizeof(szErrorMsg), "\r >");
        telnet_send_data(ulClientIndex, MSG_TYPE_CMD, szErrorMsg, dos_strlen(szErrorMsg));
    }
    else
    {
#if PTS_DEBUG
        S32 i = 0;
        for (i=0; i<ulLen; i++)
        {
            printf("%02x ", (U8)szBuff[i]);
        }
#endif
        pts_save_msg_into_cache(g_astCmdClient[lResult].aucID, PT_DATA_CMD, g_astCmdClient[lResult].ulStreamID, szBuff, ulLen, NULL, 0);
    }
}

/**
 * ������VOID pts_send_msg2cmd(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
 * ���ܣ�������Ϣ��cmd�ն�
 * ����
 * ����ֵ:
 */
VOID pts_send_msg2cmd(PT_NEND_RECV_NODE_ST *pstNeedRecvNode)
{
    S32              lClientSub              = 0;
    U32              ulArraySub              = 0;
    S8               *pcSendMsg              = NULL;
    list_t           *pstStreamList          = NULL;
    PT_CC_CB_ST      *pstCCNode              = NULL;
    PT_STREAM_CB_ST  *pstStreamNode          = NULL;
    PT_DATA_TCP_ST   *pstDataTcp             = NULL;
    PTS_CMD_CLIENT_CB_ST stClientCB;

    lClientSub = pts_cmd_client_search_by_stream(pstNeedRecvNode->ulStreamID);
    if(lClientSub < 0)
    {
        /* û���ҵ�stream��Ӧ���׽��� */
        pt_logr_debug("pts send msg to cmd : error not found stockfd");
        return;
    }
    stClientCB = g_astCmdClient[lClientSub];

    if (pstNeedRecvNode->ExitNotifyFlag)
    {
        /* ��Ӧ���� */
        pts_cmd_client_delete(lClientSub);
        telnet_close_client(stClientCB.lSocket);
        return;
    }

    pstCCNode = pt_ptc_list_search(g_pstPtcListRecv, pstNeedRecvNode->aucID);
    if(NULL == pstCCNode)
    {
        pt_logr_debug("pts send msg to proxy : not found ptc id is %s", pstNeedRecvNode->aucID);
        return;
    }

    pstStreamList = pstCCNode->astDataTypes[pstNeedRecvNode->enDataType].pstStreamQueHead;
    if (NULL == pstStreamList)
    {
        pt_logr_debug("pts send msg to proxy : not found stream list of type is %d", pstNeedRecvNode->enDataType);
        return;
    }

    pstStreamNode = pt_stream_queue_search(pstStreamList, pstNeedRecvNode->ulStreamID);
    if (NULL == pstStreamNode)
    {
        pt_logr_debug("pts send msg to proxy : not found stream node of id is %d", pstNeedRecvNode->ulStreamID);
        return;
    }

    if (NULL == pstStreamNode->unDataQueHead.pstDataTcp)
    {
        pt_logr_debug("pts send msg to proxy : not found data queue");
        return;
    }

    while(1)
    {
        /* ���Ͱ���ֱ�������� */
        pstStreamNode->lCurrSeq++;
        ulArraySub = (pstStreamNode->lCurrSeq) & (PT_DATA_RECV_CACHE_SIZE - 1);
        pstDataTcp = pstStreamNode->unDataQueHead.pstDataTcp;
        if (pstDataTcp[ulArraySub].lSeq == pstStreamNode->lCurrSeq)
        {
            pcSendMsg = pstDataTcp[ulArraySub].szBuff;
#if 0
            S32 i = 0;
            for (i=0; i<pstDataTcp[ulArraySub].ulLen; i++)
            {
                if (i%16 == 0)
                {
                    printf("\n");
                }
                printf("%02x ", (U8)(pcSendMsg[i]));
            }
            printf("\n");
            printf("---------------------\n");
#endif

#if 0
            static S32 i = 0;
            i++;
            if (i == 30)
            {
                S8 szSendBuf[8] = {0};
                sprintf(szSendBuf, "%c%c%c", IAC, DO, NAOLFD);
                telnet_send_data(stClientCB.lSocket, MSG_TYPE_CMD_RESPONCE, szSendBuf, 3);

                //sprintf(szSendBuf, "%c%c%c", IAC, WILL, SGA);
                //telnet_send_data(stClientCB.lSocket, MSG_TYPE_CMD_RESPONCE, szSendBuf, 3);
                //sprintf(szSendBuf, "%c%c%c", IAC, DO, TTYPE);
                //telnet_send_data(stClientCB.lSocket, MSG_TYPE_CMD_RESPONCE, szSendBuf, 3);
            }
            printf("!!!!!!!!!!!!!!!!i = %d\n\n", i);
#endif
            telnet_send_data(stClientCB.lSocket, MSG_TYPE_CMD_RESPONCE, pcSendMsg, pstDataTcp[ulArraySub].ulLen);
            pt_logr_debug("pts send msg to cmd serv len is: %d, stream : %d", pstDataTcp[ulArraySub].ulLen, stClientCB.ulStreamID);
        }
        else
        {
            pstStreamNode->lCurrSeq--;
            break;
        }

    } /* end of while(1) */

    return;
}

#ifdef __cplusplus
}
#endif /* __cplusplus */

