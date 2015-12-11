#ifndef __MC_DEF__
#define __MC_DEF__


#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* ¼���ļ�·�� */
#define MC_RECORD_FILE_PATH       "/ipcc/var/data/voicerecord"

/* ¼���ļ�ת������·�� */
#define MC_SCRIPT_PATH            "/usr/bin/codec_convert.sh"

/* ���ת���߳��� */
#define MC_MAX_WORKING_TASK_CNT   2

/* ���ȴ�ʱ�������������ʱ����CPUռ�ó�����20%���£��Ϳ��Լ���ִ��ת������ */
#define MC_WAITING_TIME           60

/* ��������ÿ�μ��ص������� */
#define MC_MAX_DATA_PRE_TIME      100

/* �ļ�����󳤶� */
#define MC_MAX_FILENAME_LEN       128

#define MC_ROOT_UID               0
#define MC_NOBODY_UID             99

/* ������ÿ���ڵ���ڴ��С */
#define MC_MAX_FILENAME_BUFF_LEN  (MC_MAX_FILENAME_LEN+(sizeof(DLL_NODE_S)))

typedef enum tagMCTaskWorkingStatus
{
    /* ת������״̬�����Թ��� */
    MC_WORKING_STATUS_WORKING,

    /* ת������״̬���ȴ����� */
    MC_WORKING_STATUS_WAITING,
}MC_WORKING_STATUS_EN;

typedef enum tagMCTaskStatus
{
    /* ��������״̬: ��ʼ��ת�� */
    MC_TSK_STATUS_INIT,

    /* ��������״̬: ����״̬ */
    MC_TSK_STATUS_RUNNING,

    /* ��������״̬: �ȴ����� */
    MC_TSK_STATUS_WAITING,

    /* ��������״̬: �˳�״̬ */
    MC_TSK_STATUS_EXITED,
}MC_TASK_STATUS_EN;


typedef struct tagMCServerCB
{
    U32              ulStatus;   /* Refer to MC_TASK_STATUS_EN */

    pthread_t        pthServer;  /* �߳̾�� */
    pthread_mutex_t  mutexQueue; /* ���е��� */
    pthread_cond_t   condQueue;  /* ������������ */
    DLL_S            stQueue;    /* �������� */

    U32              ulTotalProc;
    U32              ulSuccessProc;
}MC_SERVER_CB;

VOID mc_log(BOOL blMaster, U32 ulLevel, S8 *szFormat, ...);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __MC_DEF__ */



