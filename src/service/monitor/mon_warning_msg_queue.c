#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include "mon_warning_msg_queue.h"
#include "mon_def.h"

static MON_MSG_QUEUE_S * g_pstMsgQueue;

/**
 * ����:��ʼ���澯��Ϣ����
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_init_warning_msg_queue()
{
    g_pstMsgQueue = (MON_MSG_QUEUE_S *)dos_dmem_alloc(sizeof(MON_MSG_QUEUE_S));
    if (!g_pstMsgQueue)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    g_pstMsgQueue->ulQueueLength = 0;
    g_pstMsgQueue->pstHead = NULL;
    g_pstMsgQueue->pstRear = NULL;

    return DOS_SUCC;
}

/**
 * ����:��Ϣ���
 * ��������
 *   ����1: MON_MSG_S * pstMsg ��ӵ���Ϣ�����ַ
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_warning_msg_en_queue(MON_MSG_S * pstMsg)
{
    if (!pstMsg)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    if (0 == g_pstMsgQueue->ulQueueLength)//�������Ϊ�գ���˽ڵ�Ϊͷ���
    {
        pstMsg->next = NULL;
        pstMsg->prior = NULL;
        g_pstMsgQueue->pstHead = pstMsg;
        g_pstMsgQueue->pstRear = pstMsg;
    }
    else
    {//������зǿգ����ڶ���β�����Ԫ�ز�ͬʱά����βָ��Ͷ��г���
        pstMsg->next = NULL;
        pstMsg->prior = g_pstMsgQueue->pstRear;
        g_pstMsgQueue->pstRear->next = pstMsg;
        g_pstMsgQueue->pstRear = pstMsg;
    }
    ++(g_pstMsgQueue->ulQueueLength);

    return DOS_SUCC;
}


/**
 * ����:��Ϣ����
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_warning_msg_de_queue()
{
    if (0 == g_pstMsgQueue->ulQueueLength)
    {//����Ϊ��
        mon_trace(MON_TRACE_WARNING_MSG, LOG_LEVEL_INFO, "The Msg Queue is Empty.");
        return DOS_FAIL;
    }
    else
    {//���зǿգ��Ӷ�ͷɾ�����ݣ���ά�����е�ͷ���ָ��Ͷ��г���
        MON_MSG_S * p = g_pstMsgQueue->pstHead;
        g_pstMsgQueue->pstHead = g_pstMsgQueue->pstHead->next;
        if (g_pstMsgQueue->pstHead)
        {
            g_pstMsgQueue->pstHead->prior = NULL;
        }
        p->next = NULL;
        dos_dmem_free(p);
        p = NULL;
    }
    --g_pstMsgQueue->ulQueueLength;

    return DOS_SUCC;
}

BOOL mon_is_warning_msg_queue_empty()
{
    if (g_pstMsgQueue->ulQueueLength == 0)
    {
        return DOS_TRUE;
    }
    else
    {
        return DOS_FALSE;
    }
}

/**
 * ����:��ȡ�澯��Ϣ���еĶ�ͷ��ַ
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ����ظ澯��Ϣ���ж�ͷָ�룬ʧ�ܷ���NULL
 */
MON_MSG_QUEUE_S * mon_get_warning_msg_queue()
{
    return g_pstMsgQueue;
}

/**
 * ����:���ٸ澯��Ϣ����
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_destroy_warning_msg_queue()
{
    U32 ulRet = 0;

    if(!g_pstMsgQueue)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    if(!(g_pstMsgQueue->pstHead))
    {
        mon_trace(MON_TRACE_WARNING_MSG, LOG_LEVEL_ERROR, "Msg Queue\'s head is empty.");
        dos_dmem_free(g_pstMsgQueue);
        return DOS_FAIL;
    }

    while(g_pstMsgQueue->pstHead)
    {
        ulRet = mon_warning_msg_de_queue();
        if(DOS_SUCC != ulRet)
        {
            mon_trace(MON_TRACE_WARNING_MSG, LOG_LEVEL_ERROR, "Warning Msg DeQueue FAIL.");
            return DOS_FAIL;
        }
    }
    dos_dmem_free(g_pstMsgQueue);
    g_pstMsgQueue = NULL;

    return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif /* __cplusplus */


