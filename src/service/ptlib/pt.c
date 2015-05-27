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
#include <pt/md5.h>

PT_PROT_TYPE_EN g_aenDataProtType[PT_DATA_BUTT] = {PT_TCP, PT_TCP, PT_TCP};
static U32 g_ulPTCurrentLogLevel = LOG_LEVEL_NOTIC;  /* ��־��ӡ���� */
S32 g_ulUdpSocket = 0;
U8  gucIsTableInit  = 0;
U16 g_crc_tab[256];                    /* Function init_crc_tab() will initialize it */

extern S8 *pts_get_current_time();

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

void init_crc_tab( void )
{
    U16     i, j;
    U16     value;

    if ( gucIsTableInit )
        return;

    for ( i=0; i<256; i++ )
    {
        value = (U16)(i << 8);
        for ( j=0; j<8; j++ )
        {
            if ( (value&0x8000) != 0 )
                value = (U16)((value<<1) ^ CRC16_POLY);
            else
                value <<= 1;
        }

        g_crc_tab[i] = value;
    }
    gucIsTableInit = 1;

}

U16 load_calc_crc(U8* pBuffer, U32 ulCount)
{

    U16 usTemp = 0;

    init_crc_tab();

    while ( ulCount-- != 0 )
    {
        /* The masked statements can make out the REVERSE crc of ccitt-16b
        temp1 = (crc>>8) & 0x00FFL;
        temp2 = crc_tab[ ( (_US)crc ^ *buffer++ ) & 0xff ];
        crc   = temp1 ^ temp2;
        */
        usTemp = (U16)(( usTemp<<8 ) ^ g_crc_tab[ ( usTemp>>8 ) ^ *pBuffer++ ]);
    }

    return usTemp;
}


/**
 * ������S32 ptc_DNS_resolution(S8 *szDomainName, U8 paucIPAddr[][IPV6_SIZE], U32 ulIPSize)
 * ���ܣ�
 *      1.��������
 * ����
 *      VOID *arg :ͨ��ͨ�ŵ�sockfd
 * ����ֵ��void *
 */
S32 pt_DNS_analyze(S8 *szDomainName, U8 paucIPAddr[IPV6_SIZE])
{
    S8 **pptr = NULL;
    struct hostent *hptr = NULL;
    S8 szstr[PT_IP_ADDR_SIZE] = {0};
    S32 i = 0;

    hptr = gethostbyname(szDomainName);
    if (NULL == hptr)
    {
        pt_logr_info("gethostbyname error for host:%s", szDomainName);
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
BOOL pts_is_ptc_sn(S8* pcUrl)
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
    if (DOS_ADDR_INVALID(pcUrl))
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
    if (DOS_ADDR_INVALID(pstStreamNode) || DOS_ADDR_INVALID(acSendBuf))
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
    pt_logr_debug("save into send cache seq : %d,  currSeq : %d, array :%d, stream : %d , data len : %d", lSeq , pstStreamNode->lCurrSeq, ulArraySub, pstStreamNode->ulStreamID, lDataLen);
    /* ÿ64��������һ�� */
    //if (lSeq - pstStreamNode->lConfirmSeq >= PT_CONFIRM_RECV_MSG_SIZE)
    pt_logr_debug("!!!!!!!lSeq : %d, pstStreamNode->lConfirmSeq : %d", lSeq, pstStreamNode->lConfirmSeq);
    if (lSeq - pstStreamNode->lConfirmSeq >= lCacheSize - 1)
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
    if (DOS_ADDR_INVALID(pstStreamNode) || DOS_ADDR_INVALID(pstMsgDes) || DOS_ADDR_INVALID(acRecvBuf))
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
    U32 ulArraySub = pstMsgDes->lSeq & (lCacheSize - 1);  /* ������,������������е�λ�� */

    if (pstStreamNode->lCurrSeq >= pstMsgDes->lSeq)
    {
        /* ���ѷ��ͣ�����Ҫ�洢 */
        return PT_SAVE_DATA_SUCC;
    }

    if ((pstDataQueHead[ulArraySub].lSeq == pstMsgDes->lSeq) &&  pstMsgDes->lSeq != 0)
    {
        /* ���Ѵ��ڡ�0�Ű�ʱ����ʹû�д棬Ҳ���� */
        return PT_SAVE_DATA_SUCC;
    }
    dos_memcpy(pstDataQueHead[ulArraySub].szBuff, acRecvBuf, lDataLen);
    pstDataQueHead[ulArraySub].lSeq = pstMsgDes->lSeq;
    pstDataQueHead[ulArraySub].ulLen = lDataLen;
    pstDataQueHead[ulArraySub].ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;

    /* ����stream node��Ϣ */
    pstStreamNode->lMaxSeq = pstStreamNode->lMaxSeq > pstMsgDes->lSeq ? pstStreamNode->lMaxSeq : pstMsgDes->lSeq;

    lRet = PT_SAVE_DATA_SUCC;

    if (pstMsgDes->lSeq - pstStreamNode->lCurrSeq >= lCacheSize)
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
            /* ������ʧ�ܣ��ر� */
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
PT_CC_CB_ST *pt_ptc_node_create(U8 *pcIpccId, S8 *szPtcVersion, struct sockaddr_in stDestAddr)
{
    PT_CC_CB_ST *pstNewNode = NULL;

    if (DOS_ADDR_INVALID(pcIpccId))
    {
        return NULL;
    }

    pstNewNode = (PT_CC_CB_ST *)dos_dmem_alloc(sizeof(PT_CC_CB_ST));
    if (NULL == pstNewNode)
    {
        perror("dos_dmem_malloc");
        return NULL;
    }

    dos_memcpy(pstNewNode->aucID, pcIpccId, PTC_ID_LEN);
    pstNewNode->stDestAddr = stDestAddr;

    pstNewNode->astDataTypes[PT_DATA_CTRL].enDataType = PT_DATA_CTRL;
    pstNewNode->astDataTypes[PT_DATA_CTRL].bValid = DOS_TRUE;
    dos_list_init(&pstNewNode->astDataTypes[PT_DATA_CTRL].stStreamQueHead);
    pstNewNode->astDataTypes[PT_DATA_CTRL].ulStreamNumInQue = 0;
    pstNewNode->astDataTypes[PT_DATA_WEB].enDataType = PT_DATA_WEB;
    pstNewNode->astDataTypes[PT_DATA_WEB].bValid = DOS_FALSE;
    dos_list_init(&pstNewNode->astDataTypes[PT_DATA_WEB].stStreamQueHead);
    pstNewNode->astDataTypes[PT_DATA_WEB].ulStreamNumInQue = 0;
    pstNewNode->astDataTypes[PT_DATA_CMD].enDataType = PT_DATA_CMD;
    pstNewNode->astDataTypes[PT_DATA_CMD].bValid = DOS_FALSE;
    dos_list_init(&pstNewNode->astDataTypes[PT_DATA_CMD].stStreamQueHead);
    pstNewNode->astDataTypes[PT_DATA_CMD].ulStreamNumInQue = 0;
    pstNewNode->ulUdpLostDataCount = 0;
    pstNewNode->ulUdpRecvDataCount = 0;
    pstNewNode->usHBOutTimeCount = 0;
    pstNewNode->stHBTmrHandle = NULL;
    pthread_mutex_init(&pstNewNode->pthreadMutex, NULL);

    //if (dos_strcmp(szPtcVersion, "1.1") == 0)
    //{
        /* 1.1�汾��֧��web��cmd */
    pstNewNode->astDataTypes[PT_DATA_WEB].bValid = DOS_TRUE;
    pstNewNode->astDataTypes[PT_DATA_CMD].bValid = DOS_TRUE;
    //}

    printf("!!!!!!!create new ptc node : %p\n", pstNewNode);

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
S32 pt_ptc_list_insert(list_t *pstPtcListHead, PT_CC_CB_ST *pstPtcNode)
{
    if (DOS_ADDR_INVALID(pstPtcNode) || DOS_ADDR_INVALID(pstPtcListHead))
    {
        return DOS_FAIL;
    }

    dos_list_add_tail(pstPtcListHead, &(pstPtcNode->stCCListNode));

    return DOS_SUCC;
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
    PT_STREAM_CB_ST *pstStreamNode = NULL;

    printf("!!!!!!!!!!!!!malloc stream : %d\n", ulStreamID);
    pstStreamNode = (PT_STREAM_CB_ST *)dos_dmem_alloc(sizeof(PT_STREAM_CB_ST));
    if (NULL == pstStreamNode)
    {
        perror("dos_dmem_malloc");
        return NULL;
    }

    pstStreamNode->unDataQueHead.pstDataTcp = NULL;
    pstStreamNode->unDataQueHead.pstDataUdp = NULL;
    pstStreamNode->ulStreamID = ulStreamID;
    pstStreamNode->lMaxSeq = -1;

    pstStreamNode->lCurrSeq = -1;
    pstStreamNode->lConfirmSeq = -1;
    pstStreamNode->hTmrHandle = NULL;
    pstStreamNode->ulCountResend = 0;
    pstStreamNode->pstLostParam = NULL;

    pstStreamNode->bIsUsing = DOS_FALSE;

    dos_list_init(&pstStreamNode->stStreamListNode);

    printf("create stream, %d\n", ulStreamID);

    return pstStreamNode;
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
S32 pt_stream_queue_insert(list_t *pstHead, list_t *pstStreamNode)
{
    if (DOS_ADDR_INVALID(pstHead) || DOS_ADDR_INVALID(pstStreamNode))
    {
        return DOS_FAIL;
    }

    dos_list_add_tail(pstHead, pstStreamNode);

    return DOS_SUCC;
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
    if (DOS_ADDR_INVALID(pstStreamListHead))
    {
        pt_logr_debug("search stream node in list : empty list!");
        return NULL;
    }

    if (dos_list_is_empty(pstStreamListHead))
    {
        return NULL;
    }

    list_t *pstNode = NULL;
    PT_STREAM_CB_ST *pstData = NULL;

    pstNode = pstStreamListHead;

    while (1)
    {
        pstNode = dos_list_work(pstStreamListHead, pstNode);
        if (!pstNode)
        {
            return NULL;
        }

        pstData = dos_list_entry(pstNode, PT_STREAM_CB_ST, stStreamListNode);
        if (pstData->ulStreamID == ulStreamID)
        {
            return pstData;
        }
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
S32 pt_delete_stream_node(list_t *pstStreamListNode, PT_DATA_TYPE_EN enDataType)
{
    if (DOS_ADDR_INVALID(pstStreamListNode) || enDataType >= PT_DATA_BUTT)
    {
        return DOS_FAIL;
    }

    PT_STREAM_CB_ST *pstStreamNode = dos_list_entry(pstStreamListNode, PT_STREAM_CB_ST, stStreamListNode);

    if (DOS_ADDR_INVALID(pstStreamListNode->next) || DOS_ADDR_INVALID(pstStreamListNode->prev))
    {
        return DOS_FAIL;
    }

    if (DOS_ADDR_VALID(pstStreamListNode->next) && DOS_ADDR_VALID(pstStreamListNode->prev))
    {
        dos_list_del(pstStreamListNode);
    }

    printf("delete stream node, %d\n", pstStreamNode->ulStreamID);

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
        printf("!!!!!!!!!!!!!!!free stream : %d\n", pstStreamNode->ulStreamID);
        dos_dmem_free(pstStreamNode);
        pstStreamNode = NULL;
    }

    return DOS_SUCC;
}

/**
 * ������list_t *pt_delete_ptc_node(list_t *stPtcListHead, PT_CC_CB_ST *pstPtcNode)
 * ���ܣ�
 *      1.�ͷ�ptc�µ���������Ŀռ䣬ɾ��ptc�ڵ�
 * ����
 * ����ֵ������ͷ���
 */
S32 pt_delete_ptc_node(PT_CC_CB_ST *pstPtcNode)
{
    if (DOS_ADDR_INVALID(pstPtcNode))
    {
        return DOS_FAIL;
    }

    S32 i = 0;
    list_t *pstStreamListHead = NULL;
    list_t *pstStreamListNode = NULL;

    /* �ͷŵ�cc�����е�stream */
    for (i=0; i<PT_DATA_BUTT; i++)
    {
        pstStreamListHead = &pstPtcNode->astDataTypes[i].stStreamQueHead;
        if (pstStreamListHead != NULL)
        {
            pstStreamListNode = pstStreamListHead;
            while (1)
            {
                pstStreamListNode = dos_list_work(pstStreamListHead, pstStreamListNode);
                if (NULL == pstStreamListNode)
                {
                    break;
                }

                pt_delete_stream_node(pstStreamListNode, i);
            }
        }
    }

    /* �ͷŵ�ptc */
    dos_list_del(&pstPtcNode->stCCListNode);
    pthread_mutex_destroy(&pstPtcNode->pthreadMutex);
    dos_dmem_free(pstPtcNode);
    pstPtcNode = NULL;

    return DOS_SUCC;
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
    if (DOS_ADDR_INVALID(pstHead) || DOS_ADDR_INVALID(pucID))
    {
        return NULL;
    }

    if (dos_list_is_empty(pstHead))
    {
        return NULL;
    }

    list_t  *pstNode = NULL;
    PT_CC_CB_ST *pstData = NULL;

    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (NULL == pstNode)
        {
            return NULL;
        }

        pstData = dos_list_entry(pstNode, PT_CC_CB_ST, stCCListNode);
        if (dos_memcmp(pstData->aucID, pucID, PTC_ID_LEN) == 0)
        {
            return pstData;
        }
    }
}


S32 pt_delete_ptc_resource(PT_CC_CB_ST *pstPtcNode)
{
    if (DOS_ADDR_INVALID(pstPtcNode))
    {
        return DOS_FAIL;
    }

    list_t *pstStreamListHead  = NULL;
    list_t *pstStreamListNode  = NULL;
    S32    i                   = 0;

    for (i=PT_DATA_CTRL+1; i<PT_DATA_BUTT; i++)
    {
        pstStreamListHead = &pstPtcNode->astDataTypes[i].stStreamQueHead;

        while (1)
        {
            pstStreamListNode = dos_list_work(pstStreamListHead, pstStreamListHead);
            if (DOS_ADDR_INVALID(pstStreamListNode))
            {
                break;
            }

            pt_delete_stream_node(pstStreamListNode, i);
        }
    }

    return DOS_SUCC;
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

    if (DOS_ADDR_INVALID(pstHead))
    {
        return NULL;
    }

    if (dos_list_is_empty(pstHead))
    {
        return NULL;
    }

    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (NULL == pstNode)
        {
            return NULL;
        }

        pstData = dos_list_entry(pstNode, PT_NEND_RECV_NODE_ST, stListNode);
        if (pstData->ulStreamID == ulStreamID && !pstData->ExitNotifyFlag)
        {
            return pstNode;
        }
    }
}

/**
 * ������list_t *pt_need_recv_node_list_insert(list_t *pstHead, PT_MSG_TAG *pstMsgDes)
 * ���ܣ�
 *      1.��Ҫ������Ϣ���ж����в����½ڵ�
 * ����
 * ����ֵ��
 */
S32 pt_need_recv_node_list_insert(list_t *pstHead, PT_MSG_TAG *pstMsgDes)
{
    if (DOS_ADDR_INVALID(pstHead) || DOS_ADDR_INVALID(pstMsgDes))
    {
        return DOS_FAIL;
    }

    PT_NEND_RECV_NODE_ST *pstNewNode = (PT_NEND_RECV_NODE_ST *)dos_dmem_alloc(sizeof(PT_NEND_RECV_NODE_ST));
    if (NULL == pstNewNode)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_memcpy(pstNewNode->aucID, pstMsgDes->aucID, PTC_ID_LEN);
    pstNewNode->enDataType = pstMsgDes->enDataType;
    pstNewNode->ulStreamID = pstMsgDes->ulStreamID;
    pstNewNode->ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;
    pstNewNode->lSeq = pstMsgDes->lSeq;

    dos_list_add_tail(pstHead, &(pstNewNode->stListNode));

    return DOS_SUCC;
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

    if (DOS_ADDR_INVALID(pstHead))
    {
        return NULL;
    }

    if (dos_list_is_empty(pstHead))
    {
        return NULL;
    }

    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (!pstNode)
        {
            return NULL;
        }

        pstData = dos_list_entry(pstNode, PT_NEND_SEND_NODE_ST, stListNode);
        if (pstData->ulStreamID == ulStreamID)
        {
            if (pstData->enCmdValue == PT_CMD_NORMAL && pstData->bIsResend != DOS_TRUE)
            {
                return pstNode;
            }
        }
    }
}

/**
 * ������list_t *pt_need_send_node_list_insert(list_t *pstHead, U8 *aucID, PT_MSG_TAG *pstMsgDes, PT_CMD_EN enCmdValue, BOOL bIsResend)
 * ���ܣ�
 *      1.������Ϣ�����в����½ڵ�
 * ����
 * ����ֵ������ͷ��ַ
 */
S32 pt_need_send_node_list_insert(list_t *pstHead, U8 *aucID, PT_MSG_TAG *pstMsgDes, PT_CMD_EN enCmdValue, BOOL bIsResend)
{
    if (DOS_ADDR_INVALID(pstHead) || DOS_ADDR_INVALID(aucID) || DOS_ADDR_INVALID(pstMsgDes))
    {
        return DOS_FAIL;
    }

    PT_NEND_SEND_NODE_ST *pstNewNode = (PT_NEND_SEND_NODE_ST *)dos_dmem_alloc(sizeof(PT_NEND_SEND_NODE_ST));
    if (NULL == pstNewNode)
    {
        DOS_ASSERT(0);

        return DOS_FAIL;
    }

    dos_memcpy(pstNewNode->aucID, aucID, PTC_ID_LEN);
    pstNewNode->enDataType = pstMsgDes->enDataType;
    pstNewNode->ulStreamID = pstMsgDes->ulStreamID;
    pstNewNode->lSeqResend = pstMsgDes->lSeq;
    pstNewNode->enCmdValue = enCmdValue;
    pstNewNode->bIsResend = bIsResend;
    pstNewNode->ExitNotifyFlag = pstMsgDes->ExitNotifyFlag;

    dos_list_add_tail(pstHead, &pstNewNode->stListNode);

    return DOS_SUCC;
}


STREAM_CACHE_ADDR_CB_ST *pt_stream_addr_create(U32 ulStreamID)
{
    STREAM_CACHE_ADDR_CB_ST *pstNewNode = NULL;

    pstNewNode = (STREAM_CACHE_ADDR_CB_ST *)dos_dmem_alloc(sizeof(STREAM_CACHE_ADDR_CB_ST));
    if (NULL == pstNewNode)
    {
        DOS_ASSERT(0);

        return NULL;
    }

    pstNewNode->ulStrreamID = ulStreamID;
    pstNewNode->pstPtcRecvNode = NULL;
    pstNewNode->pstPtcSendNode = NULL;
    pstNewNode->pstStreamRecvNode = NULL;
    pstNewNode->pstStreamSendNode = NULL;

    return pstNewNode;
}

VOID pt_stream_addr_delete(DLL_S *pList, DLL_NODE_S *pNode)
{
    if (DOS_ADDR_INVALID(pList) || DOS_ADDR_INVALID(pNode))
    {
        return;
    }

    STREAM_CACHE_ADDR_CB_ST *pstStreamAddr = NULL;

    dll_delete(pList, pNode);
    if (DOS_ADDR_VALID(pNode->pHandle))
    {
        pstStreamAddr = (STREAM_CACHE_ADDR_CB_ST *)pNode->pHandle;
        pstStreamAddr->pstPtcRecvNode = NULL;
        pstStreamAddr->pstPtcSendNode = NULL;
        pstStreamAddr->pstStreamRecvNode = NULL;
        pstStreamAddr->pstStreamSendNode = NULL;
        pstStreamAddr->ulStrreamID = 0;

        dos_dmem_free(pNode->pHandle);
        pNode->pHandle = NULL;
    }
    dos_dmem_free(pNode);
    pNode = NULL;
}

PT_SEND_MSG_PTHREAD *pt_send_msg_pthread_search(list_t* pstHead, U32 ulStreamID)
{
    if (DOS_ADDR_INVALID(pstHead))
    {
        return NULL;
    }

    if (dos_list_is_empty(pstHead))
    {
        return NULL;
    }

    list_t  *pstNode = NULL;
    PT_SEND_MSG_PTHREAD *pstData = NULL;

    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (DOS_ADDR_INVALID(pstNode))
        {
            return NULL;
        }

        pstData = dos_list_entry(pstNode, PT_SEND_MSG_PTHREAD, stList);
        if (pstData->ulStreamID == ulStreamID && pstData->bIsValid)
        {
            return pstData;
        }
    }
}

PT_SEND_MSG_PTHREAD *pt_send_msg_pthread_create(PT_NEND_RECV_NODE_ST *pstNeedRecvNode, S32 lSocket)
{
    PT_SEND_MSG_PTHREAD_PARAM   *pstPthreadParam    = NULL;
    PT_SEND_MSG_PTHREAD         *pstSendPthreadNode = NULL;

    pstPthreadParam = (PT_SEND_MSG_PTHREAD_PARAM *)dos_dmem_alloc(sizeof(PT_SEND_MSG_PTHREAD_PARAM));
    if (DOS_ADDR_INVALID(pstPthreadParam))
    {
        DOS_ASSERT(0);
        pt_logr_info("create send pthread param fail");

        return NULL;
    }

    dos_memcpy(pstPthreadParam->aucID, pstNeedRecvNode->aucID, PTC_ID_LEN);
    pstPthreadParam->enDataType = pstNeedRecvNode->enDataType;
    pstPthreadParam->ulStreamID = pstNeedRecvNode->ulStreamID;
    pstPthreadParam->lSocket = lSocket;
    pstPthreadParam->bIsNeedExit = DOS_FALSE;
    sem_init(&pstPthreadParam->stSemSendMsg, 0, 0);

    pstSendPthreadNode = (PT_SEND_MSG_PTHREAD *)dos_dmem_alloc(sizeof(PT_SEND_MSG_PTHREAD));
    if (DOS_ADDR_INVALID(pstSendPthreadNode))
    {
        DOS_ASSERT(0);
        pt_logr_info("create send pthread node fail");
        dos_dmem_free(pstPthreadParam);
        pstPthreadParam= NULL;

        return NULL;
    }

    pstSendPthreadNode->pstPthreadParam = pstPthreadParam;
    pstSendPthreadNode->bIsValid = DOS_TRUE;
    pstSendPthreadNode->ulStreamID = pstNeedRecvNode->ulStreamID;

    return pstSendPthreadNode;
}

S32 pt_send_msg_pthread_delete(list_t* pstHead, U32 ulStreamID)
{
    if (DOS_ADDR_INVALID(pstHead))
    {
        return DOS_FAIL;
    }

    if (dos_list_is_empty(pstHead))
    {
        return DOS_FAIL;
    }

    list_t  *pstNode = NULL;
    PT_SEND_MSG_PTHREAD *pstData = NULL;

    pstNode = pstHead;

    while (1)
    {
        pstNode = dos_list_work(pstHead, pstNode);
        if (DOS_ADDR_INVALID(pstNode))
        {
            return DOS_FAIL;
        }

        pstData = dos_list_entry(pstNode, PT_SEND_MSG_PTHREAD, stList);
        if (pstData->ulStreamID == ulStreamID)
        {
            dos_list_del(pstNode);
            pstData->bIsValid = DOS_FALSE;
            if (DOS_ADDR_VALID(pstData->pstPthreadParam))
            {
                dos_dmem_free(pstData->pstPthreadParam);
                pstData->pstPthreadParam = NULL;
            }

            dos_dmem_free(pstData);
            pstData = NULL;

            return DOS_TRUE;
        }
    }
}

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

