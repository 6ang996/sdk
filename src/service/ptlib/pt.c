#ifdef  __cplusplus
extern "C"{
#endif

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
#include <net/if.h>
#include <sys/ioctl.h>

#include <dos.h>
#include <pt/pt.h>
#include <pt/des.h>
#include <pt/md5.h>

PT_PROT_TYPE_EN g_aenDataProtType[PT_DATA_BUTT] = {PT_TCP, PT_TCP, PT_TCP};
static U32 g_ulPTCurrentLogLevel = LOG_LEVEL_NOTIC;  /* ��־��ӡ���� */

struct
{
    S32 a;
    U8 q1;
    S32 b;
    U8 q2;
    S32 c;
    U8 q3;
    S32 d;
}m_stIpFormat;

/**
 * ������BOOL pts_is_or_not_ip(S8 *szIp)
 * ���ܣ�
 *      1.�ж��Ƿ����ip��ַ�ĸ�ʽ
 * ����
 * ����ֵ:
 */
BOOL pt_is_or_not_ip(S8 *szIp)
{
    dos_memzero(&m_stIpFormat, sizeof(m_stIpFormat));
    sscanf(szIp, "%d%c%d%c%d%c%d", &m_stIpFormat.a, &m_stIpFormat.q1, &m_stIpFormat.b, &m_stIpFormat.q2, &m_stIpFormat.c, &m_stIpFormat.q3, &m_stIpFormat.d);
    if(m_stIpFormat.a<256 && m_stIpFormat.a>=0 && m_stIpFormat.b<256 && m_stIpFormat.b>=0 && m_stIpFormat.c<256 && m_stIpFormat.c>=0 && m_stIpFormat.d<256 && m_stIpFormat.d>=0)
    {
        if(m_stIpFormat.q1=='.' && m_stIpFormat.q2=='.' && m_stIpFormat.q3=='.')
        {
            return DOS_TRUE;
        }
        else
        {
            return DOS_FALSE;
        }
    }
    else
    {
        return DOS_FALSE;
    }
}

/**
 * ������S32 ptc_DNS_resolution(S8 *szDomainName, U8 paucIPAddr[][IPV6_SIZE], U32 ulIPSize)
 * ���ܣ�
 *      1.��������
 * ����
 *      VOID *arg :ͨ��ͨ�ŵ�sockfd
 * ����ֵ��void *
 */
S32 pt_DNS_resolution(S8 *szDomainName, U8 paucIPAddr[IPV6_SIZE])
{
    S8 **pptr = NULL;
    struct hostent *hptr = NULL;
    S8 szstr[PT_IP_ADDR_SIZE] = {0};
    S32 i = 0;

    hptr = gethostbyname(szDomainName);
    if (NULL == hptr)
    {
        pt_logr_info("gethostbyname error for host:%s\n", szDomainName);
        return DOS_FAIL;
    }

    switch (hptr->h_addrtype)
    {
        case AF_INET:
            /* break */
        case AF_INET6:
            pptr = hptr->h_addr_list;
            for(i=0; *pptr!=NULL; pptr++, i++)
            {
                if (i > 0)
                {
                    break;
                }
                inet_ntop(hptr->h_addrtype, *pptr, szstr, sizeof(szstr));
                pt_logr_debug("address:%s\n", szstr);
                inet_pton(hptr->h_addrtype, szstr, (VOID *)(paucIPAddr));
            }
            inet_ntop(hptr->h_addrtype, hptr->h_addr, szstr, sizeof(szstr));
            pt_logr_debug("first address: %s", szstr);
            printf("first address: %s\n", szstr);
            break;
        default:
            break;
    }

    return i;
}

S8 *pt_get_gmt_time(U32 ulExpiresTime)
{
   time_t timestamp = 0;
   struct tm *pstTm = NULL;

   timestamp = time(NULL) + ulExpiresTime;
   pstTm = gmtime(&timestamp);
    //Mon, 20 Jul 2009 23:00:00
   //snprintf(szGTMTime, PT_DATA_BUFF_64, "%s, ", pstTm->);

   return asctime(pstTm);
}

S32 pt_md5_encrypt(S8 *szEncrypt, S8 *szDecrypt)
{
    if (NULL == szEncrypt || NULL == szDecrypt)
    {
        return DOS_FAIL;
    }

    S32 i = 0;
    U8 aucDecrypt[PT_MD5_ARRAY_SIZE];
    MD5_CTX md5;

    MD5Init(&md5);
    MD5Update(&md5, (U8 *)szEncrypt, dos_strlen(szEncrypt));
    MD5Final(&md5, aucDecrypt);

    for(i=0;i<PT_MD5_ARRAY_SIZE;i++)
    {
        sprintf(szDecrypt + 2*i, "%02x", aucDecrypt[i]);
    }
    szDecrypt[PT_MD5_LEN-1] = '\0';

    return DOS_SUCC;
}

#if 0
/**
 * ������VOID pt_des_encrypt_or_decrypt(U8 *ulDesKey, S16 sProcessMode, U8 *ucData, U32 ulDataLen, U8 *ucDestData)
 * ���ܣ�
 *      1.DES����
 *      2.DES����
 * ����
 * ����ֵ��
 */
S32 pt_des_encrypt_or_decrypt(U8 *ulDesKey, S16 sProcessMode, U8 *ucData, U32 ulDataLen, U8 *ucDestData)
{
    if (NULL == ulDesKey || NULL == ucData || NULL == ucDestData)
    {
        return DOS_FAIL;
    }

    U32 ulBlockCount = 0;
    U16 usPadding = 0;
    U8 *pDataBlock = (U8*)dos_dmem_alloc(8);
    if (NULL == pDataBlock)
    {
        return DOS_FAIL;
    }

    U8 *pProcessedBlock = (U8*)dos_dmem_alloc(8);
    if (NULL == pProcessedBlock)
    {
        dos_dmem_free(pDataBlock);
        return DOS_FAIL;
    }

    key_set* pstKeySets = (key_set*)dos_dmem_alloc(17*sizeof(key_set));
    if (NULL == pstKeySets)
    {
        dos_dmem_free(pDataBlock);
        dos_dmem_free(pProcessedBlock);
        return DOS_FAIL;
    }

    generate_sub_keys("abcdefgh", pstKeySets);
    U8 *pTest = (U8 *)pstKeySets;
    U32 i = 0;

    for (i=0; i<17*sizeof(key_set); i++)
    {
        printf("%02x ", pTest[i]);
    }
    printf("\n");

    dos_strncpy(pDataBlock, ucData, ulDataLen);
    if (sProcessMode == ENCRYPTION_MODE)
    {
        usPadding = 8 - ulDataLen%8;
        memset((pDataBlock + 8 - usPadding), (U8)usPadding, usPadding);

        process_message(pDataBlock, pProcessedBlock, pstKeySets, sProcessMode);
        dos_memcpy(ucDestData, pProcessedBlock, 8);
    }
    else
    {
        process_message(pDataBlock, pProcessedBlock, pstKeySets, sProcessMode);
        usPadding = pProcessedBlock[7];
        if (usPadding < 8)
        {
            dos_memcpy(ucDestData, pProcessedBlock, 8 - usPadding);
        }
    }

    dos_dmem_free(pDataBlock);
    dos_dmem_free(pProcessedBlock);
    dos_dmem_free(pstKeySets);

    return DOS_SUCC;
}

#endif
/**
 * ������BOOL pts_is_ptc_id(S8* pcUrl)
 * ���ܣ�
 *      1.�ж��Ƿ���ptc ID
 * ����
 * ����ֵ��
 */
BOOL pts_is_ptc_id(S8* pcUrl)
{
    S32 i = 0;

    if(PTC_ID_LEN == dos_strlen(pcUrl))
    {
        for (i=0; i<PTC_ID_LEN; i++)
        {
            if ((pcUrl[i] < 'a' || pcUrl[i] > 'f') && (pcUrl[i] < '0' || pcUrl[i] > '9') && (pcUrl[i] < 'A' || pcUrl[i] > 'F'))
            {
                return DOS_FALSE;
            }
        }
        return DOS_TRUE;
    }
    return DOS_FALSE;
}

BOOL pts_is_int(S8* pcUrl)
{
    S32 i = 0;
    if (NULL == pcUrl)
    {
        return DOS_FALSE;
    }

    for (i=0; i<dos_strlen(pcUrl); i++)
    {
        if (pcUrl[i] < '0' || pcUrl[i] > '9')
        {
            return DOS_FALSE;
        }
    }
    return DOS_TRUE;
}

/**
 * ������VOID pt_log_set_level(U32 ulLevel)
 * ���ܣ�
 *      1.������־����
 * ����
 *      U32 ulLevel :�µļ���
 * ����ֵ��void
 */
VOID pt_log_set_level(U32 ulLevel)
{
    g_ulPTCurrentLogLevel = ulLevel;
}

/**
 * ������U32 pt_log_current_level()
 * ���ܣ�
 *      1.��õ�ǰ����־����
 * ����
 * ����ֵ��
 */
U32 pt_log_current_level()
{
    return g_ulPTCurrentLogLevel;
}

/**
 * ������VOID pt_logr_write(U32 ulLevel, S8 *pszFormat, ...)
 * ���ܣ�
 *      1.��־��ӡ����
 * ����
 * ����ֵ��
 */
VOID pt_logr_write(U32 ulLevel, S8 *pszFormat, ...)
{
    va_list argptr;
    char szBuf[PT_DATA_BUFF_1024];

    va_start(argptr, pszFormat);
    vsnprintf(szBuf, PT_DATA_BUFF_1024, pszFormat, argptr);
    va_end(argptr);
    szBuf[sizeof(szBuf) -1] = '\0';

    if (ulLevel <= g_ulPTCurrentLogLevel)
    {
        dos_log(ulLevel, LOG_TYPE_RUNINFO, szBuf);
    }
}

/**
 * ������PT_DATA_TCP_ST *pt_data_tcp_queue_create(U32 ulCacheSize)
 * ���ܣ�
 *      1.�������TCP���ݵ�����
 * ����
 *      U32 ulCacheSize : ��������
 * ����ֵ��
 *      NULL :����ռ�ʧ��
 */
PT_DATA_TCP_ST *pt_data_tcp_queue_create(U32 ulCacheSize)
{
    PT_DATA_TCP_ST *pstHead = NULL;

    pstHead = (PT_DATA_TCP_ST *)dos_dmem_alloc(sizeof(PT_DATA_TCP_ST) * ulCacheSize);
    if (NULL == pstHead)
    {
        perror("malloc");
        return NULL;
    }
    else
    {
        dos_memzero(pstHead, sizeof(PT_DATA_TCP_ST) * ulCacheSize);
        return pstHead;
    }
}

/**
 * ������PT_STREAM_CB_ST *pt_stream_node_create(U32 ulStreamID)
 * ���ܣ�
 *      1.����stream�ڵ�
 * ����
 *      U32 ulStreamID: ������stream�ڵ��id
 * ����ֵ��NULL: ����ʧ��
 */
PT_STREAM_CB_ST *pt_stream_node_create(U32 ulStreamID)
{
    PT_STREAM_CB_ST *stStreamNode = NULL;
    list_t *pstHead = NULL;
    stStreamNode = (PT_STREAM_CB_ST *)dos_dmem_alloc(sizeof(PT_STREAM_CB_ST));
    if (NULL == stStreamNode)
    {
        perror("dos_dmem_malloc");
        return NULL;
    }

    stStreamNode->unDataQueHead.pstDataTcp = NULL;
    stStreamNode->unDataQueHead.pstDataUdp = NULL;
    stStreamNode->ulStreamID = ulStreamID;
    stStreamNode->lMaxSeq = -1;

    stStreamNode->lCurrSeq = -1;
    stStreamNode->lConfirmSeq = -1;
    stStreamNode->hTmrHandle = NULL;
    stStreamNode->ulCountResend = 0;
    stStreamNode->pstLostParam = NULL;

    pstHead = &(stStreamNode->stStreamListNode);
    dos_list_init(pstHead);

    return stStreamNode;
}

/**
 * ������S32 pt_send_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, PT_MSG_TAG *pstMsgDes, S8 *acSendBuf, S32 lDataLen, U32 lCacheSize)
 * ���ܣ�
 *      1.�����Ϣ�����ͻ���
 * ����
 *      PT_STREAM_CB_ST *pstStreamNode  : ��������ڵ�stream�ڵ�
 *      PT_MSG_TAG *pstMsgDes           : ��Ϣ����
 *      S8 *acSendBuf                   : ��Ϣ����
 *      S32 lDataLen                    : ��Ϣ���ݴ�С
 *      U32 lCacheSize                  : ����ĸ���(�������������±�)
 * ����ֵ:
 */
S32 pt_send_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, S8 *acSendBuf, S32 lDataLen, U32 lCacheSize)
{
    if (NULL == pstStreamNode || NULL == acSendBuf)
    {
        return PT_SAVE_DATA_FAIL;
    }

    S32 lSeq = 0;
    U32 ulArraySub = 0;
    PT_DATA_TCP_ST *pstDataQueHead = pstStreamNode->unDataQueHead.pstDataTcp;
    if (NULL == pstDataQueHead)
    {
        return PT_SAVE_DATA_FAIL;
    }

    /* TODO ��ת�ж�,1023G,�ᷭת�� */
    lSeq = ++pstStreamNode->lMaxSeq;
    ulArraySub = lSeq & (lCacheSize - 1);  /*������,������������е�λ��*/
    dos_memcpy(pstDataQueHead[ulArraySub].szBuff, acSendBuf, lDataLen);
    pstDataQueHead[ulArraySub].lSeq = lSeq;
    pstDataQueHead[ulArraySub].ulLen = lDataLen;
    pstDataQueHead[ulArraySub].ExitNotifyFlag = DOS_FALSE;
    pt_logr_debug("save into send cache data seq : %d,  currSeq : %d, array :%d, stream : %d , data len : %d", lSeq , pstStreamNode->lCurrSeq, ulArraySub, pstStreamNode->ulStreamID, lDataLen);
    /* ÿ64��������һ�� */
    if (lSeq - pstStreamNode->lConfirmSeq >= PT_CONFIRM_RECV_MSG_SIZE)
    {
        return PT_NEED_CUT_PTHREAD;
    }
    return PT_SAVE_DATA_SUCC;
}

/**
 * ������S32 pt_recv_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen, U32 lCacheSize)
 * ���ܣ�
 *      1.�����Ϣ�����ջ���
 * ����
 *      PT_STREAM_CB_ST *pstStreamNode  : ��������ڵ�stream�ڵ�
 *      PT_MSG_TAG *pstMsgDes           : ��Ϣ����
 *      S8 *acRecvBuf                   : ��Ϣ����
 *      S32 lDataLen                    : ��Ϣ���ݴ�С
 *      U32 lCacheSize                  : ����ĸ���(�������������±�)
 * ����ֵ:
 */
S32 pt_recv_data_tcp_queue_insert(PT_STREAM_CB_ST *pstStreamNode, PT_MSG_TAG *pstMsgDes, S8 *acRecvBuf, S32 lDataLen, U32 lCacheSize)
{
    if (NULL == pstStreamNode || NULL == pstMsgDes || NULL == acRecvBuf)
    {
        return PT_SAVE_DATA_FAIL;
    }

    PT_DATA_TCP_ST *pstDataQueHead = pstStreamNode->unDataQueHead.pstDataTcp;
    if (NULL == pstDataQueHead)
    {
        return PT_SAVE_DATA_FAIL;
    }

    S32 lRet = 0;
    U32 ulNextSendArraySub = 0;
    U32 ulArraySub = pstMsgDes->lSeq & (lCacheSize - 1);  /*������,������������е�λ��*/

    if (pstStreamNode->lCurrSeq >= pstMsgDes->lSeq)
    {
        /* ���ѷ��ͣ�����Ҫ�洢 */
        return PT_SAVE_DATA_SUCC;
    }

    if ((pstDataQueHead[ulArraySub].lSeq == pstMsgDes->lSeq) &&  pstMsgDes->lSeq != 0)
    {
        /*���Ѵ��ڡ�0�Ű�ʱ����ʹû�д棬Ҳ����*/
        return PT_SAVE_DATA_SUCC;
    }
    dos_memcpy(pstDataQueHead[ulArraySub].szBuff, acRecvBuf, lDataLen);
    pstDataQueHead[ulArraySub].lSeq = pstMsgDes->lSeq;
    pstDataQueHead[ulArraySub].ulLen = lDataLen;
    pstDataQueHead[ulArraySub].ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;

    /* ����stream node��Ϣ */
    pstStreamNode->lMaxSeq = pstStreamNode->lMaxSeq > pstMsgDes->lSeq ? pstStreamNode->lMaxSeq : pstMsgDes->lSeq;

    lRet = PT_SAVE_DATA_SUCC;

    if (pstMsgDes->lSeq - pstStreamNode->lCurrSeq == lCacheSize)
    {
        /* �����Ѿ����� */
        ulNextSendArraySub =  (pstStreamNode->lCurrSeq + 1) & (lCacheSize - 1);
        if (pstDataQueHead[ulNextSendArraySub].lSeq == pstStreamNode->lCurrSeq + 1)
        {
            /* ��Ҫִ�н����߳� */
            lRet = PT_NEED_CUT_PTHREAD;
        }
        else
        {
            /* TODO ������ʧ�ܣ��ر�socket */
            return PT_SAVE_DATA_FAIL;
        }
    }

    return lRet;
}

/**
 * ������PT_CC_CB_ST *pt_ptc_node_create(U8 *pcIpccId, S8 *szPtcVersion, struct sockaddr_in stDestAddr, S32 lSocket)
 * ���ܣ�
 *      1.����ptc�ڵ�
 * ����
 *      U8 *pcIpccId     : PTC�豸ID
 *      S8 *szPtcVersion : ��½��PTC�İ汾��
 *      struct sockaddr_in stDestAddr : PTC�ĵ�ַ
 *      S32 lSocket     : udp �׽���
 * ����ֵ��void *
 */
PT_CC_CB_ST *pt_ptc_node_create(U8 *pcIpccId, S8 *szPtcVersion, struct sockaddr_in stDestAddr, S32 lSocket)
{
    PT_CC_CB_ST *pstNewNode = NULL;

    if (NULL == pcIpccId)
    {
        pt_logr_debug("create ptc node : age is NULL");
        return NULL;
    }

    pstNewNode = (PT_CC_CB_ST *)dos_dmem_alloc(sizeof(PT_CC_CB_ST));
    if (NULL == pstNewNode)
    {
        perror("dos_dmem_malloc");
        return NULL;
    }

    dos_memcpy(pstNewNode->aucID, pcIpccId, PTC_ID_LEN);
    pstNewNode->lSocket = lSocket;
    pstNewNode->stDestAddr = stDestAddr;

    pstNewNode->astDataTypes[PT_DATA_CTRL].enDataType = PT_DATA_CTRL;
    pstNewNode->astDataTypes[PT_DATA_CTRL].bValid = DOS_TRUE;
    pstNewNode->astDataTypes[PT_DATA_CTRL].pstStreamQueHead = NULL;
    pstNewNode->astDataTypes[PT_DATA_CTRL].ulStreamNumInQue = 0;
    pstNewNode->astDataTypes[PT_DATA_WEB].enDataType = PT_DATA_WEB;
    pstNewNode->astDataTypes[PT_DATA_WEB].bValid = DOS_FALSE;
    pstNewNode->astDataTypes[PT_DATA_WEB].pstStreamQueHead = NULL;
    pstNewNode->astDataTypes[PT_DATA_WEB].ulStreamNumInQue = 0;
    pstNewNode->astDataTypes[PT_DATA_CMD].enDataType = PT_DATA_CMD;
    pstNewNode->astDataTypes[PT_DATA_CMD].bValid = DOS_FALSE;
    pstNewNode->astDataTypes[PT_DATA_CMD].pstStreamQueHead = NULL;
    pstNewNode->astDataTypes[PT_DATA_CMD].ulStreamNumInQue = 0;
    pstNewNode->ulUdpLostDataCount = 0;
    pstNewNode->ulUdpRecvDataCount = 0;
    pstNewNode->usHBOutTimeCount = 0;
    pstNewNode->stHBTmrHandle = NULL;

    if (dos_strcmp(szPtcVersion, "1.1") == 0)
    {
        /* 1.1�汾��֧��web��cmd */
        pstNewNode->astDataTypes[PT_DATA_WEB].bValid = DOS_TRUE;
        pstNewNode->astDataTypes[PT_DATA_CMD].bValid = DOS_TRUE;
    }

    return pstNewNode;
}

/**
 * ������list_t *pt_ptc_list_insert(list_t *pstPtcListHead, PT_CC_CB_ST *pstPtcNode)
 * ���ܣ�
 *      1.���µ�ptc�ڵ���뵽ptc������
 * ����
 *      list_t *pstPtcListHead  : ����ͷ���
 *      PT_CC_CB_ST *pstPtcNode : ��Ҫ��ӵĽڵ�
 * ����ֵ��void *
 */
list_t *pt_ptc_list_insert(list_t *pstPtcListHead, PT_CC_CB_ST *pstPtcNode)
{
    if (NULL == pstPtcNode)
    {
        pt_logr_debug("ptc list insert: new node is NULL");
        return pstPtcListHead;
    }

    if (NULL == pstPtcListHead)
    {
        pstPtcListHead = &(pstPtcNode->stCCListNode);
        dos_list_init(pstPtcListHead);
    }
    else
    {
        dos_list_add_tail(pstPtcListHead, &(pstPtcNode->stCCListNode));
    }

    return pstPtcListHead;
}

/**
 * ������list_t *pt_stream_queue_insert(list_t *pstHead, list_t  *pstStreamNode)
 * ���ܣ�
 *      1.�����в����µĽڵ�
 * ����
 *      list_t  *pstHead        ��streamͷ�ڵ�
 *      list_t  *pstStreamNode  ����Ҫ����Ľڵ�
 * ����ֵ������ͷ�ڵ�
 */
list_t *pt_stream_queue_insert(list_t *pstHead, list_t  *pstStreamNode)
{
    if (NULL == pstStreamNode)
    {
        return pstHead;
    }

    if (NULL == pstHead)
    {
        pstHead = pstStreamNode;
        dos_list_init(pstHead);
    }
    else
    {
        dos_list_add_tail(pstHead, pstStreamNode);
    }

    return pstHead;
}


/**
 * ������PT_STREAM_CB_ST *pt_stream_queue_search(list_t *pstStreamListHead, U32 ulStreamID)
 * ���ܣ�
 *      1.��stream�����У�����stream IDΪulStreamID�Ľڵ�
 * ����
 *      list_t *pstStreamListHead    ��streamͷ���
 *      U32 ulStreamID               ����Ҫ���ҵ�stream ID
 * ����ֵ���ҵ�������stream node��ַ��δ�ҵ�����NULL
 */
PT_STREAM_CB_ST *pt_stream_queue_search(list_t *pstStreamListHead, U32 ulStreamID)
{
    if (NULL == pstStreamListHead)
    {
        pt_logr_debug("search stream node in list : empty list!");
        return NULL;
    }

    list_t *pstNode = NULL;
    PT_STREAM_CB_ST *pstData = NULL;

    pstNode = pstStreamListHead;
    pstData = dos_list_entry(pstNode, PT_STREAM_CB_ST, stStreamListNode);

    while (pstData->ulStreamID != ulStreamID && pstNode->next != pstStreamListHead)
    {
        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PT_STREAM_CB_ST, stStreamListNode);
    }

    if (pstData->ulStreamID == ulStreamID)
    {
        return pstData;
    }
    else
    {
        pt_logr_debug("search stream node in list : not found!");
        return NULL;
    }
}

/**
 * ������list_t *pt_delete_stream_resource(list_t *pstStreamListHead, list_t *pstStreamListNode, PT_DATA_TYPE_EN enDataType)
 * ���ܣ�
 *      1.�ͷ�һ��stream
 * ����
 *      list_t *pstStreamListHead  ��
 *      list_t *pstStreamListNode  ��
 *      PT_DATA_TYPE_EN enDataType ���׽���
 * ����ֵ��
 */
list_t *pt_delete_stream_node(list_t *pstStreamListHead, list_t *pstStreamListNode, PT_DATA_TYPE_EN enDataType)
{
    if (NULL == pstStreamListHead || NULL == pstStreamListNode || enDataType >= PT_DATA_BUTT)
    {
        return pstStreamListHead;
    }
    PT_STREAM_CB_ST *pstStreamNode = dos_list_entry(pstStreamListNode, PT_STREAM_CB_ST, stStreamListNode);
    if (pstStreamListHead == pstStreamListNode)
    {
        /* �ͷŵ�һ���ڵ㣬ͷ��㷢���ı� */
        if (pstStreamListHead == pstStreamListHead->next)
        {
            pstStreamListHead = NULL;
        }
        else
        {
            pstStreamListHead = pstStreamListNode->next;
        }
    }
    dos_list_del(pstStreamListNode);

    if (g_aenDataProtType[enDataType] == PT_UDP)
    {
        /* ��Ҫ�ͷŴ洢data�Ŀռ� */
    }
    else
    {
        /* TCP */
        if (pstStreamNode->hTmrHandle != NULL)
        {
            dos_tmr_stop(&pstStreamNode->hTmrHandle);
            pstStreamNode->hTmrHandle = NULL;
        }
        if (pstStreamNode->pstLostParam != NULL)
        {
            pstStreamNode->pstLostParam->pstStreamNode = NULL;
            dos_dmem_free(pstStreamNode->pstLostParam);
            pstStreamNode->pstLostParam = NULL;
        }
        if (pstStreamNode->unDataQueHead.pstDataTcp != NULL)
        {
            dos_dmem_free(pstStreamNode->unDataQueHead.pstDataTcp);
            pstStreamNode->unDataQueHead.pstDataTcp = NULL;
        }
        dos_dmem_free(pstStreamNode);
        pstStreamNode = NULL;
    }
    return pstStreamListHead;
}

/**
 * ������list_t *pt_delete_ptc_node(list_t *stPtcListHead, PT_CC_CB_ST *pstPtcNode)
 * ���ܣ�
 *      1.�ͷ�ptc�µ���������Ŀռ䣬ɾ��ptc�ڵ�
 * ����
 * ����ֵ������ͷ���
 */
list_t *pt_delete_ptc_node(list_t *stPtcListHead, PT_CC_CB_ST *pstPtcNode)
{
    if (NULL == stPtcListHead || NULL == pstPtcNode)
    {
        return stPtcListHead;
    }

    //printf("pt ptc list delete pucID : %.16s\n", pstPtcNode->aucID);
    S32 i = 0;
    list_t *pstStreamListHead = NULL;

    /* �ͷŵ�cc�����е�stream */
    for (i=0; i<PT_DATA_BUTT; i++)
    {
        pstStreamListHead = pstPtcNode->astDataTypes[i].pstStreamQueHead;
        if (pstStreamListHead != NULL)
        {
            while (pstStreamListHead->next != pstStreamListHead)
            {
                pstStreamListHead = pt_delete_stream_node(pstStreamListHead, pstStreamListHead, i);
            }
            pt_delete_stream_node(pstStreamListHead, pstStreamListHead, i);
        }
    }
    /* �ͷŵ�ptc */
    if (stPtcListHead == &pstPtcNode->stCCListNode)
    {
        if (stPtcListHead->next == stPtcListHead)
        {
            stPtcListHead = NULL;
        }
        else
        {
            stPtcListHead = stPtcListHead->next;
        }
    }
    dos_list_del(&pstPtcNode->stCCListNode);
    dos_dmem_free(pstPtcNode);
    pstPtcNode = NULL;
    return stPtcListHead;
}

/**
 * ������PT_CC_CB_ST *pt_ptc_list_search(list_t* pstHead, U8 *pucID)
 * ���ܣ�
 *      1.����ptc ID ��ptc�����в���ptc�ڵ�
 * ����
 * ����ֵ��
 */
PT_CC_CB_ST *pt_ptc_list_search(list_t* pstHead, U8 *pucID)
{
    list_t  *pstNode = NULL;
    PT_CC_CB_ST *pstData = NULL;
    S32 lResult = 0;
    //printf("pt ptc list search pucID : %.16s\n", pucID);
    if (NULL == pstHead)
    {
        //printf("search ptc list : not found!\n");
        pt_logr_debug("%s", "search ptc list : empty list!");
        return NULL;
    }
    else if (NULL == pucID)
    {
        //printf("search ptc list : not found!\n");
        return NULL;
    }

    pstNode = pstHead;
    pstData = dos_list_entry(pstNode, PT_CC_CB_ST, stCCListNode);
    while (dos_memcmp(pstData->aucID, pucID, PTC_ID_LEN) && pstNode->next!=pstHead)
    {
        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PT_CC_CB_ST, stCCListNode);
    }

    lResult = dos_memcmp(pstData->aucID, pucID, PTC_ID_LEN);
    if (lResult == 0)
    {
        return pstData;
    }
    else
    {
        //printf("search ptc list : not found!\n");
        pt_logr_debug("search ptc list : not found!");
        return NULL;
    }
}

/**
 * ������list_t *pt_need_recv_node_list_search(list_t *pstHead, U32 ulStreamID)
 * ���ܣ�
 *      1.����Ҫ������Ϣ�����У�����ulStreamID�Ƿ����(ExitNotifyFlagΪtrue״̬�ĳ���)
 * ����
 * ����ֵ��
 */
list_t *pt_need_recv_node_list_search(list_t *pstHead, U32 ulStreamID)
{
    list_t  *pstNode = NULL;
    PT_NEND_RECV_NODE_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        pt_logr_debug("search need recv node list : empty list!");
        return NULL;
    }

    pstNode = pstHead;
    pstData = dos_list_entry(pstNode, PT_NEND_RECV_NODE_ST, stListNode);
    while (pstNode->next != pstHead)
    {
        if (pstData->ulStreamID == ulStreamID)
        {
            if (!pstData->ExitNotifyFlag)
            {
                break;
            }

        }
        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PT_NEND_RECV_NODE_ST, stListNode);
    }

    if (pstData->ulStreamID == ulStreamID)
    {
        return &pstData->stListNode;
    }
    else
    {
        pt_logr_debug("search need recv node list : not found!");
        return NULL;
    }
}

/**
 * ������list_t *pt_need_recv_node_list_insert(list_t *pstHead, PT_MSG_TAG *pstMsgDes)
 * ���ܣ�
 *      1.��Ҫ������Ϣ���ж����в����½ڵ�
 * ����
 * ����ֵ��
 */
list_t *pt_need_recv_node_list_insert(list_t *pstHead, PT_MSG_TAG *pstMsgDes)
{
    if (NULL == pstMsgDes)
    {
        pt_logr_debug("pt_need_recv_node_list_insert : arg pstMsgDes is NULL");
        return pstHead;
    }

    PT_NEND_RECV_NODE_ST *pstNewNode = (PT_NEND_RECV_NODE_ST *)dos_dmem_alloc(sizeof(PT_NEND_RECV_NODE_ST));
    if (NULL == pstNewNode)
    {
        perror("malloc");
        return pstHead;
    }

    dos_memcpy(pstNewNode->aucID, pstMsgDes->aucID, PTC_ID_LEN);
    pstNewNode->enDataType = pstMsgDes->enDataType;
    pstNewNode->ulStreamID = pstMsgDes->ulStreamID;
    pstNewNode->ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;

    if (NULL == pstHead)
    {
        pstHead = &(pstNewNode->stListNode);
        dos_list_init(pstHead);
    }
    else
    {
        dos_list_add_tail(pstHead, &(pstNewNode->stListNode));
    }

    return pstHead;
}

/**
 * ������list_t *pt_need_send_node_list_search(list_t *pstHead, U32 ulStreamID)
 * ���ܣ�
 *      1.������Ϣ�����в���ulStreamID�Ľڵ�
 * ����
 * ����ֵ��
 */
list_t *pt_need_send_node_list_search(list_t *pstHead, U32 ulStreamID)
{
    list_t  *pstNode = NULL;
    PT_NEND_SEND_NODE_ST *pstData = NULL;

    if (NULL == pstHead)
    {
        pt_logr_debug("search need send node list : empty list!");
        return NULL;
    }

    pstNode = pstHead;
    pstData = dos_list_entry(pstNode, PT_NEND_SEND_NODE_ST, stListNode);
    while (pstNode->next != pstHead)
    {
        if (pstData->ulStreamID == ulStreamID)
        {
            if (pstData->enCmdValue == PT_CMD_NORMAL && pstData->bIsResend != DOS_TRUE)
            {
                break;
            }
        }

        pstNode = pstNode->next;
        pstData = dos_list_entry(pstNode, PT_NEND_SEND_NODE_ST, stListNode);
    }

    if (pstData->ulStreamID == ulStreamID)
    {
        return &pstData->stListNode;
    }
    else
    {
        pt_logr_debug("search need recv node list : not found!");
        return NULL;
    }
}

/**
 * ������list_t *pt_need_send_node_list_insert(list_t *pstHead, U8 *aucID, PT_MSG_TAG *pstMsgDes, PT_CMD_EN enCmdValue, BOOL bIsResend)
 * ���ܣ�
 *      1.������Ϣ�����в����½ڵ�
 * ����
 * ����ֵ������ͷ��ַ
 */
list_t *pt_need_send_node_list_insert(list_t *pstHead, U8 *aucID, PT_MSG_TAG *pstMsgDes, PT_CMD_EN enCmdValue, BOOL bIsResend)
{
    if (NULL == aucID || NULL == pstMsgDes)
    {
        pt_logr_debug("pt_need_recv_node_list_insert : arg is NULL");
        return pstHead;
    }

    PT_NEND_SEND_NODE_ST *pstNewNode = (PT_NEND_SEND_NODE_ST *)dos_dmem_alloc(sizeof(PT_NEND_SEND_NODE_ST));
    if (NULL == pstNewNode)
    {
        perror("malloc");
        return pstHead;
    }

    dos_memcpy(pstNewNode->aucID, aucID, PTC_ID_LEN);
    pstNewNode->enDataType = pstMsgDes->enDataType;
    pstNewNode->ulStreamID = pstMsgDes->ulStreamID;
    pstNewNode->lSeqResend = pstMsgDes->lSeq;
    pstNewNode->enCmdValue = enCmdValue;
    pstNewNode->bIsResend = bIsResend;
    pstNewNode->ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;

    if (NULL == pstHead)
    {
        pstHead = &pstNewNode->stListNode;
        dos_list_init(pstHead);
    }
    else
    {
        dos_list_add_tail(pstHead, &pstNewNode->stListNode);
    }

    return pstHead;
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */
