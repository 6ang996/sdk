#ifndef __PTS_H__
#define __PTS_H__

#ifdef  __cplusplus
extern "C"{
#endif

#include <dos/dos_types.h>
#include <pt/dos_sqlite3.h>

#define PTS_SQLITE_DB_PATH         "var/pts.db"    /* sqlite ���ݿ��·�� */
#define PTS_WEB_SERV_PORT_DEFAULT  80          /* ����proxy��Ĭ�϶˿� */
#define PTS_WEB_SERV_IP_DEFAULT    "127.0.0.1" /* ����proxy��Ĭ��ip */
#define PTS_TEL_SERV_IP_DEFAULT    "127.0.0.1" /* ����proxy��Ĭ��ip */
#define PTS_TEL_SERV_PORT_DEFAULT  23          /* ����proxy��Ĭ�϶˿� */
#define PTS_SOCKET_CACHE           10 * 1024 * 1024 /* socket �շ������С */

extern DOS_SQLITE_ST *g_pstMySqlite;

S32 pts_main();
U32 pts_create_stream_id();
S32 pts_create_tcp_socket(U16 usTcpPort);
VOID pts_change_remark(S8 *szUrl, U32 ulConnfd);
S32 pts_get_password_callback(VOID *para, S32 n_column, S8 **column_value, S8 **column_name);
//S32 pts_md5_encrypt(S8 *szEncrypt, S8 *szDecrypt);

#ifdef  __cplusplus
}
#endif  /* end of __cplusplus */


#endif