

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

typedef struct tagIOBuf
{
    U32 ulSize;          /* ��ǰBUFFER���ܳ��� */
    U32 ulLength;        /* ��ǰBUFFERʹ�õĳ��� */

    U8  *pszBuffer;      /* ����ָ�� */
}IO_BUF_CB;

/* ��̬��ʼ��IO_BUF_CB  */
#define IO_BUF_INIT       {0, 0, NULL}

/* ϵͳ�������󻺴泤�� */
#define IO_BUF_MAX_SIZE   10 * 1024

/**
 * ����: dos_iobuf_init
 * ����: ��ʼ��pstIOBufCB��ָ��Ļ��棬Ĭ�ϳ���ΪulLength
 * ����:
 *       IO_BUF_CB *pstIOBufCB: Ҫ��ʼ���Ļ���ṹ
 *       U32 ulLength: ��ʼ������
 * ����ֵ
 *      �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAUIL
 * �����Ȳ�Ϊ0ʱ����Ҫ�жϷ���ֵ
 */
U32 dos_iobuf_init(IO_BUF_CB *pstIOBufCB, U32 ulLength);

/**
 * ����: dos_iobuf_free
 * ����: �ͷ�pstIOBufCB��ָ��Ļ���
 * ����:
 *       IO_BUF_CB *pstIOBufCB: Ҫ�ͷŵĻ���ṹ
 * ����ֵ
 *      �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAUIL
 */
U32 dos_iobuf_free(IO_BUF_CB *pstIOBufCB);

/**
 * ����: dos_iobuf_dup
 * ����: ��pstIOBufCBSrc��ָ��Ļ���copy��pstIOBufCBDst�У���������
 * ����:
 *      IO_BUF_CB *pstIOBufCBSrc : Դ����
 *      IO_BUF_CB *pstIOBufCBDst : Ŀ�Ļ���
 * ����ֵ
 *      �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAUIL
 */
U32 dos_iobuf_dup(IO_BUF_CB *pstIOBufCBSrc, IO_BUF_CB *pstIOBufCBDst);

/**
 * ����: dos_iobuf_resize
 * ����: ���³�ʼ��pstIOBufCB��ָ��Ļ��棬���ı��䳤��Ϊulength
 * ����:
 *       IO_BUF_CB *pstIOBufCB: Ҫ��ʼ���Ļ���ṹ
 *       U32 ulLength: ��ʼ������
 * ����ֵ
 *      �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAUIL
 */
U32 dos_iobuf_resize(IO_BUF_CB *pstIOBufCB, U32 ulength);

/**
 * ����: dos_iobuf_append
 * ����: �򻺴�pstIOBufCB׷������
 * ����:
 *      IO_BUF_CB *pstIOBufCB: �����Ȼ���
 *      const VOID *pData : ��Ҫ����ӵ�����
 *      U32 ulength       : ����ĳ���
 * ����ֵ
 *      �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAUIL
 */
U32 dos_iobuf_append(IO_BUF_CB *pstIOBufCB, const VOID *pData, U32 ulength);

/**
 * ����: dos_iobuf_append_size
 * ����: ������pstIOBufCB���ݣ�����Ϊulength
 * ����:
 *       IO_BUF_CB *pstIOBufCB: Ҫ���ݵĻ���
 *       U32 ulLength: ���ٴ�С
 * ����ֵ
 *      �ɹ�����DOS_SUCC��ʧ�ܷ���DOS_FAUIL
 */
U32 dos_iobuf_append_size(IO_BUF_CB *pstIOBufCB, U32 ulength);

/**
 * ����: dos_iobuf_get_data
 * ����: ��ȡ����pstIOBufCB��������
 * ����:
 *       IO_BUF_CB *pstIOBufCB: ��Ҫ����������
 * ����ֵ
 *      �ɹ���������ָ�룬ʧ�ܷ���NULL
 */
U8 *dos_iobuf_get_data(IO_BUF_CB *pstIOBufCB);

/**
 * ����: dos_iobuf_get_data
 * ����: ��ȡ����pstIOBufCB�ı�ʹ�õĳ���
 * ����:
 *       IO_BUF_CB *pstIOBufCB: ��Ҫ����������
 * ����ֵ
 *      �ɹ����ػ��泤�ȡ�ʧ�ܷ���U32_BUTT
 */
U32 dos_iobuf_get_length(IO_BUF_CB *pstIOBufCB);

/**
 * ����: dos_iobuf_get_data
 * ����: ��ȡ����pstIOBufCB���ܴ�С
 * ����:
 *       IO_BUF_CB *pstIOBufCB: ��Ҫ����������
 * ����ֵ
 *      �ɹ��������ݳ��ȡ�ʧ�ܷ���U32_BUTT
 */
U32 dos_iobuf_get_size(IO_BUF_CB *pstIOBufCB);


#ifdef __cplusplus
}
#endif /* __cplusplus */



