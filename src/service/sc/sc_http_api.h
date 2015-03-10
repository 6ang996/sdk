/**
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_task.h
 *
 *  ����ʱ��: 2014��12��16��10:15:56
 *  ��    ��: Larry
 *  ��    ��: ҵ�����ģ�飬Ⱥ���������ⲿ�ṩ�Ľӿں���
 *  �޸���ʷ:
 */

#ifndef __SC_HTTP_API_H__
#define __SC_HTTP_API_H__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

#include <dos.h>

/* include public header files */
/* define marcos */
#define SC_API_PARAMS_MAX_NUM          24
#define SC_HTTP_REQ_MAX_LEN            64

/* HTTP��Ӧ���ݴ����� */
enum SC_HTTP_ERRNO
{
	SC_HTTP_ERRNO_SUCC = 0,                      /* �޴��� */
	SC_HTTP_ERRNO_INVALID_USR = 0xF0000001,      /* ����У��ʧ�ܣ����Ϸ����û� */
	SC_HTTP_ERRNO_INVALID_DATA,                  /* ����У��ʧ�ܣ����Ϸ������� */
	SC_HTTP_ERRNO_INVALID_TASK_STATUS,           /* ����У��ʧ�ܣ����Ϸ������� */
	SC_HTTP_ERRNO_INVALID_REQUEST,               /* ����У��ʧ�ܣ����Ϸ������� */
	SC_HTTP_ERRNO_INVALID_CMD,                   /* ����У��ʧ�ܣ����Ϸ��������� */
	SC_HTTP_ERRNO_INVALID_PARAM,                 /* ����У��ʧ�ܣ����Ϸ��Ĳ��� */
	SC_HTTP_ERRNO_CMD_EXEC_FAIL,                 /* ����ִ��ʧ�� */
	SC_HTTP_ERRNO_SERVER_ERROR,                  /* ����ִ��ʧ�� */

	SC_HTTP_ERRNO_BUTT
};


/* define enums */
enum tagAPICMDList
{
    SC_API_CMD_RELOAD                   = 0,      /* API�����֣�reload�����ļ� */
    SC_API_CMD_TASK_CTRL,                         /* API�����֣������������ */
    SC_API_CMD_CALL_CTRL,                         /* API�����֣����и�Ԥ */
    SC_API_CMD_NUM_VERIFY,                        /* API�����֣�������� */
    // ------------------------------------------------------------
    SC_API_CMD_GATEWAY,                           /* API�����֣����ؿ��� */
    //------------------------------------------------------------

    SC_API_CMD_BUTT
};

enum tagAPICMDActionList
{
    SC_API_CMD_ACTION_FORCE             = 0,      /* API����reload����ֵ��ǿ��ˢ��ָ����XML���� */
    SC_API_CMD_ACTION_FORCE_ALL,                  /* API����reload����ֵ��ǿ��ˢ���������� */
    SC_API_CMD_ACTION_CONDITIONAL,                /* API����reload����ֵ����������ˢ�£���Ӱ��ҵ�� */

    SC_API_CMD_ACTION_START,                      /* API����task ctrl����ֵ���������� */
    SC_API_CMD_ACTION_STOP,                       /* API����task ctrl����ֵ��ֹͣ���� */
    SC_API_CMD_ACTION_CONTINUE,                   /* API����task ctrl����ֵ�������������� */
    SC_API_CMD_ACTION_PAUSE,                      /* API����task ctrl����ֵ����ͣ���� */

    SC_API_CMD_ACTION_MONITORING,                 /* API����call ctrl����ֵ���������� */
    SC_API_CMD_ACTION_HUNGUP,                     /* API����call ctrl����ֵ���ҶϺ��� */

    SC_API_CMD_ACTION_SIGNIN,                     /* ��ϯǩ�룬��½ */
    SC_API_CMD_ACTION_SIGNOFF,                    /* ��ϯǩ�����ǳ� */
    SC_API_CMD_ACTION_ONLINE,                     /* ��ϯ���ߣ����� */
    SC_API_CMD_ACTION_OFFLINE,                    /* ��ϯ���ߣ���æ */

    //---------------------------------------------------
    SC_API_CMD_ACTION_GATEWAY_ADD,                       /* ����һ������ */
    SC_API_CMD_ACTION_GATEWAY_DELETE,                    /* ɾ��һ������ */
    SC_API_CMD_ACTION_GATEWAY_UPDATE,                    /* ���ظ���     */

    SC_API_CMD_ACTION_SIP_ADD,                           /* ����һ��sip�˻� */
    SC_API_CMD_ACTION_SIP_DELETE,                        /* ɾ��һ��sip�˻� */
    SC_API_CMD_ACTION_SIP_UPDATE,                        /* ����һ��sip�˻� */
    //---------------------------------------------------

    SC_API_CMD_ACTION_BUTT
};

enum tagAPIParams
{
    SC_API_VERSION                      = 0,      /* API������API�汾�� */
    SC_API_PARAM_CMD,                             /* API���������� */
    SC_API_PARAM_ACTION,                          /* API�����������ֵ */
    SC_API_PARAM_CUSTOM_ID,                       /* API�������û�ID */
    SC_API_PARAM_TASK_ID,                         /* API����������ID */
    SC_API_PARAM_UUID,                            /* API����������Ψһ��ʾ�� */
    SC_API_PARAM_NUM,                             /* API����������˵ĺ��� */
    SC_API_PARAM_EXTRACODE,                       /* API������������˸����� */

    SC_API_PARAM_BUTT
};

/* define structs */
typedef struct tagSCAPIParams
{
    S8    *pszStringName;                       /* �ַ���ֵ */
    S8    *pszStringVal;                       /* �ַ���ֵ */

    list_t  stList;
}SC_API_PARAMS_ST;

typedef struct tagHttpDataBuf
{
    S8          *pszBuff;                       /* ��������ָ�� */
    U32         ulLength;                       /* ���泤�� */
}SC_DATA_BUF_ST;


typedef struct tagHttpClientCB
{
    U32               ulValid;                  /* �Ƿ���� */
    U32               ulIndex;                  /* ��ǰ��� */
    U32               ulCurrentSrv;             /* ��ǰ��������� */

	S32               lSock;                    /* ��ǰ���ӵ�socket */

	U32               ulResponseCode;           /* HTTP��Ӧ�� */
    U32               ulErrCode;                /* HTTP��Ӧ�� */

    SC_DATA_BUF_ST    stDataBuff;

    list_t  stParamList;                        /* ��������б� */
	list_t  stResponseDataList;   /* ������ڵ�  refer to RESULT_DATA_NODE_ST */
}SC_HTTP_CLIENT_CB_S;


typedef U32 (*http_req_handle)(SC_HTTP_CLIENT_CB_S *pstClient);

typedef struct tagHttpRequestProcess
{
    S8              *pszRequest;
    http_req_handle callback;
}SC_HTTP_REQ_REG_TABLE_SC;

/* declare functions */
U32 sc_http_api_process(SC_HTTP_CLIENT_CB_S *pstClient);


#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif /* __SC_TASK_PUB_H__ */

