#ifndef __PT_MSG_H__
#define __PT_MSG_H__

#ifdef  __cplusplus
extern "C"{
#endif

#define PTC_ID_LEN          16       /* PTC ID ���� */
#define IPV6_SIZE           16       /* IPV6 �ֽ��� */

typedef enum
{
    PT_CMD_NORMAL = 0,          /* �������� */
    PT_CMD_RESEND,              /* �ش� */
    PT_CMD_CONFIRM,             /* ȷ�Ͻ��� */
    PT_CMD_BUTT

}PT_CMD_EN;

typedef struct tagMsg
{
    U32                     ulStreamID;
    S32                     lSeq;                            /* ���ݰ���� */
    U8                      aucID[PTC_ID_LEN];               /* PTC ID */
    U8                      aulServIp[IPV6_SIZE];

    U16                     usServPort;
    U8                      enDataType;                      /* ��Ϣ������ PT_DATA_TYPE_EN */
    U8                      ExitNotifyFlag;                  /* ֪ͨ stream ���� */

    U8                      enCmdValue;                      /* PT_CMD_EN */
    U8                      bIsEncrypt;                      /* �Ƿ���� */
    U8                      bIsCompress;                     /* �Ƿ�ѹ�� */
    S8                      Reserver[1];
}PT_MSG_TAG;

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */

#endif