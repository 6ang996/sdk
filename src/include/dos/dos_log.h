/*
 *            (C) Copyright 2014, ����Ѷͨ . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  dos_log.h
 *
 *  Created on: 2014-10-31
 *      Author: Larry
 *        Todo: logģ�鹫��ͷ�ļ�
 *     History:
 */

#ifndef __LOG_PUB__
#define __LOG_PUB__

#ifdef __cplusplus
extern "C"{
#endif /* __cplusplus */

/* log���� */
enum LOG_LEVEL{
    LOG_LEVEL_EMERG = 0,
    LOG_LEVEL_ALERT,
    LOG_LEVEL_CIRT,
    LOG_LEVEL_ERROR,
    LOG_LEVEL_WARNING,
    LOG_LEVEL_NOTIC,
    LOG_LEVEL_INFO,
    LOG_LEVEL_DEBUG,

    LOG_LEVEL_INVAILD
};

/* log���� */
enum LOG_TYPE{
    LOG_TYPE_RUNINFO = 0,
    LOG_TYPE_WARNING,
    LOG_TYPE_SERVICE,
    LOG_TYPE_OPTERATION,

    LOG_TYPE_INVAILD
};

/* logģ�鹫������ */
#if INCLUDE_SYSLOG_ENABLE
S32 dos_log_init();
S32 dos_log_start();
VOID dos_log_stop();

VOID dos_log(S32 _iLevel, S32 _iType, S8 *_pszMsg);
VOID dos_vlog(S32 _iLevel, S32 _iType, S8 *format, ...);
VOID dos_olog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *_pszMsg);
VOID dos_volog(S32 _lLevel, S8 *pszOpterator, S8 *pszOpterand, U32 ulResult, S8 *format, ...);
S32 dos_log_set_cli_level(U32 ulLeval);
#else

#define dos_log_init() \
    do{ \
        printf("Init the log."); \
    }while(0)

#define dos_log_start()
    do{ \
        printf("Start the log."); \
    }while(0)

#define dos_log_stop()
    do{ \
        printf("Stop the log."); \
    }while(0)

#define dos_log(_iLevel, _iType, _pszMsg) printf(_pszMsg)
#define dos_vlog(_iLevel, _iType, format, ...) printf(format, __VA_ARGS__)
#define dos_olog(_lLevel, pszOpterator, pszOpterand, ulResult, _pszMsg) printf(_pszMsg)
#define dos_volog(_lLevel, pszOpterator, pszOpterand, ulResult, format, ...) printf(format, __VA_ARGS__)
#define dos_log_set_cli_level(ulLeval) \
    do{ \
        printf("Set cli log level."); \
    }while(0)
#endif

/* �澯��Ϣ */
#define logw_debug(format, ... ) dos_vlog(LOG_LEVEL_DEBUG, LOG_TYPE_WARNING, (format), __VA_ARGS__)
#define logw_info(format, ... ) dos_vlog(LOG_LEVEL_INFO, LOG_TYPE_WARNING, (format), __VA_ARGS__)
#define logw_notice(format, ... ) dos_vlog(LOG_LEVEL_NOTIC, LOG_TYPE_WARNING, (format), __VA_ARGS__)
#define logw_warning(format, ... ) dos_vlog(LOG_LEVEL_WARNING, LOG_TYPE_WARNING, (format), __VA_ARGS__)
#define logw_error(format, ... ) dos_vlog(LOG_LEVEL_ERROR, LOG_TYPE_WARNING, (format), __VA_ARGS__)
#define logw_cirt(format, ... ) dos_vlog(LOG_LEVEL_CIRT, LOG_TYPE_WARNING, (format), __VA_ARGS__)
#define logw_alert(format, ... ) dos_vlog(LOG_LEVEL_ALERT, LOG_TYPE_WARNING, (format), __VA_ARGS__)
#define logw_emerg(format, ... ) dos_vlog(LOG_LEVEL_EMERG, LOG_TYPE_WARNING, (format), __VA_ARGS__)

/* ���е�����־ */
#define logr_debug(format, ... ) dos_vlog(LOG_LEVEL_DEBUG, LOG_TYPE_RUNINFO, (format), __VA_ARGS__)
#define logr_info(format, ... ) dos_vlog(LOG_LEVEL_INFO, LOG_TYPE_RUNINFO, (format), __VA_ARGS__)
#define logr_notice(format, ... ) dos_vlog(LOG_LEVEL_NOTIC, LOG_TYPE_RUNINFO, (format), __VA_ARGS__)
#define logr_warning(format, ... ) dos_vlog(LOG_LEVEL_WARNING, LOG_TYPE_RUNINFO, (format), __VA_ARGS__)
#define logr_error(format, ... ) dos_vlog(LOG_LEVEL_ERROR, LOG_TYPE_RUNINFO, (format), __VA_ARGS__)
#define logr_cirt(format, ... ) dos_vlog(LOG_LEVEL_CIRT, LOG_TYPE_RUNINFO, (format), __VA_ARGS__)
#define logr_alert(format, ... ) dos_vlog(LOG_LEVEL_ALERT, LOG_TYPE_RUNINFO, (format), __VA_ARGS__)
#define logr_emerg(format, ... ) dos_vlog(LOG_LEVEL_EMERG, LOG_TYPE_RUNINFO, (format), __VA_ARGS__)

/* ҵ����־ */
#define logs_debug(format, ... ) dos_vlog(LOG_LEVEL_DEBUG, LOG_TYPE_SERVICE, (format), __VA_ARGS__)
#define logs_info(format, ... ) dos_vlog(LOG_LEVEL_INFO, LOG_TYPE_SERVICE, (format), __VA_ARGS__)
#define logs_notice(format, ... ) dos_vlog(LOG_LEVEL_NOTIC, LOG_TYPE_SERVICE, (format), __VA_ARGS__)
#define logs_warning(format, ... ) dos_vlog(LOG_LEVEL_WARNING, LOG_TYPE_SERVICE, (format), __VA_ARGS__)
#define logs_error(format, ... ) dos_vlog(LOG_LEVEL_ERROR, LOG_TYPE_SERVICE, (format), __VA_ARGS__)
#define logs_cirt(format, ... ) dos_vlog(LOG_LEVEL_CIRT, LOG_TYPE_SERVICE, (format), __VA_ARGS__)
#define logs_alert(format, ... ) dos_vlog(LOG_LEVEL_ALERT, LOG_TYPE_SERVICE, (format), __VA_ARGS__)
#define logs_emerg(format, ... ) dos_vlog(LOG_LEVEL_EMERG, LOG_TYPE_SERVICE, (format), __VA_ARGS__)

/* ������־ */
#define logo_debug(_pszOpterator, _pszOpterand, _ulResult, _format, ... ) \
        dos_volog(LOG_LEVEL_DEBUG, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), __VA_ARGS__)

#define logo_info(_pszOpterator, _pszOpterand, _ulResult, _format, ... ) \
        dos_volog(LOG_LEVEL_INFO, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), __VA_ARGS__)

#define logo_notice(_pszOpterator, _pszOpterand, _ulResult, _format, ... ) \
        dos_volog(LOG_LEVEL_NOTIC, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), __VA_ARGS__)

#define logo_warning(_pszOpterator, _pszOpterand, _ulResult, _format, ... ) \
        dos_volog(LOG_LEVEL_WARNING, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), __VA_ARGS__)

#define logo_error(_pszOpterator, _pszOpterand, _ulResult, _format, ... ) \
        dos_volog(LOG_LEVEL_ERROR, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), __VA_ARGS__)

#define logo_cirt(_pszOpterator, _pszOpterand, _ulResult, _format, ... ) \
        dos_volog(LOG_LEVEL_CIRT, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), __VA_ARGS__)

#define logo_alert(_pszOpterator, _pszOpterand, _ulResult, _format, ... ) \
        dos_volog(LOG_LEVEL_ALERT, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), __VA_ARGS__)

#define olog_emerg(_pszOpterator, _pszOpterand, _ulResult, _format, ... ) \
        dos_volog(LOG_LEVEL_EMERG, (_pszOpterator), (_pszOpterand), (_ulResult), (_format), __VA_ARGS__)

#ifdef __cplusplus
}
#endif /* __cplusplus */


#endif
