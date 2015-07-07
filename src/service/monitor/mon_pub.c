#ifdef __cplusplus
extern "C"{
#endif

#include <dos.h>

#if (INCLUDE_BH_SERVER)
#if INCLUDE_RES_MONITOR

#include <pthread.h>
#include "mon_pub.h"
#include "mon_def.h"
#include "mon_monitor_and_handle.h"


pthread_t g_pMonthr, g_pHndthr;

/**
 * ����:���ɲ���ʼ��������Դ
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_init()
{
    U32 ulRet = 0;
    ulRet = mon_res_alloc();
    if(DOS_FAIL == ulRet)
    {
        mon_trace(MON_TRACE_PUB, LOG_LEVEL_ERROR, "Alloc resource FAIL.");
        return DOS_FAIL;
    }

    mon_trace(MON_TRACE_PUB, LOG_LEVEL_INFO, "Alloc resource SUCC.");
    return DOS_SUCC;
}

/**
 * ����: ������������
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_start()
{
    S32 lRet = 0;

    lRet = pthread_create(&g_pHndthr, NULL, mon_warning_handle, NULL);
    if (lRet != 0)
    {
        mon_trace(MON_TRACE_PUB, LOG_LEVEL_ERROR, "Create warning handle thread FAIL.");
        return DOS_FAIL;
    }

    lRet = pthread_create(&g_pMonthr, NULL, mon_res_monitor, NULL);
    if (lRet != 0)
    {
        mon_trace(MON_TRACE_PUB, LOG_LEVEL_ERROR, "Create resource monitor thread FAIL.");
        return DOS_FAIL;
    }

    return DOS_SUCC;
}

/**
 * ����:����ֹͣ���ͷ�������Դ
 * ��������
 *   �޲���
 * ����ֵ��
 *   �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAIL
 */
U32 mon_stop()
{
    U32 ulRet = 0;
    ulRet = mon_res_destroy();
    if(DOS_SUCC != ulRet)
    {
        mon_trace(MON_TRACE_PUB, LOG_LEVEL_ERROR, "Destroy Resource FAIL.");
        return DOS_FAIL;
    }
    return DOS_SUCC;
}

#endif //#if INCLUDE_RES_MONITOR
#endif //#if (INCLUDE_BH_SERVER)

#ifdef __cplusplus
}
#endif