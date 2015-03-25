#ifdef __cplusplus
extern "C" {
#endif /* __cplusplus */

#include <dos.h>

#if INCLUDE_SERVICE_PYTHON

#include <python2.6/Python.h>
#include <dos/dos_types.h>
#include <dos/dos_config.h>
#include <dos/dos_py.h>


/*
 * ������: U32 py_init()
 * ����:   �޲���
 * ����:   ��ʼ��Python��
 * ����ֵ: ��ʼ��ʧ���򷵻�DOS_FAIL���ɹ��򷵻�DOS_SUCC
 **/
U32 py_init()
{
    /*��ʼ��Python*/
    Py_Initialize();
    if (!Py_IsInitialized())
    {
       DOS_ASSERT(0);
       return DOS_FAIL;
    }

    return DOS_SUCC;
}


/*
 * ������: U32   py_exec_func(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...)
 * ����:   const char *pszModule:  �����õ�Pythonģ����
 *         const char *pszFunc:    ��Ҫ���õ�Python�ӿ�����
 *         const char *pszPyFormat:Python��ʽ������
 * ����:   C���Ե���Python�����ӿ�
 * ����ֵ: ִ��ʧ���򷵻�DOS_FAIL;ִ�гɹ�ʱ�����ڷ���ֵ�򷵻�Python�ķ���ֵ�����򷵻�DOS_SUCC
 *  !!!!!!!!!!!!!!!�ر�˵��!!!!!!!!!!!!!!!!!!
 *  ����pszPyFormat��ʽΪPython������ʽ������C���Բ�����ʽ����������:
 *      "s": (string) [char *] 
 *      "z": (string or None) [char *] ����NULL��β��C�ַ���Stringת��ΪPython��������ַ���Ϊ���򷵻�None
 *      "u": (Unicode string) [Py_UNICODE *] Unicode(UCS-2��UCS-4)�ַ���תΪPython Unicode�������Ϊ���򷵻�None
 *      "i": (integer) [int] 
 *      "b": (integer) [char] 
 *      "h": (integer) [short int] 
 *      "l": (integer) [long int] 
 *      "B": (integer) [unsigned char] 
 *      "H": (integer) [unsigned short int] 
 *      "I": (integer/long) [unsigned int] 
 *      "k": (integer/long) [unsigned long] 
 *      "d": (float) [double] 
 *      "f": (float) [float]
 *      "O": ��ʾһ��Python����
 *  ���в�����ʽ�б���Ҫʹ������������
 * 
 *  ʹ��ʾ��
 *  ulRet = py_exec_func("router", "del_route", "(i)", ulGatewayID);
 **/
U32   py_exec_func(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...)
{
    S8   szPyScriptPath[MAX_PY_SCRIPT_LEN] = {0,};
    U32  ulRet = DOS_SUCC;
    S32  lRet = 0;
    S8   szImportPath[MAX_PY_SCRIPT_LEN] = {0,};
    PyObject *pstPyMod = NULL, *pstPyFunc = NULL, *pstParam = NULL, *pstRetVal = NULL;
    va_list vargs;

    if (!pszModule || !pszFunc || !pszPyFormat)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }

    /* ���ù���·�� */
    ulRet = config_get_py_path(szPyScriptPath, sizeof(szPyScriptPath));
    if (0 > ulRet)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        return DOS_FAIL;
    }

    logr_info("%s:Line %d: The current python work directory is: %s"
                , dos_get_filename(__FILE__), __LINE__, szPyScriptPath);

    dos_snprintf(szImportPath, sizeof(szImportPath), "sys.path.append(\'%s\')", szPyScriptPath);

    PyRun_SimpleString("import sys");
    PyRun_SimpleString(szImportPath);

    /* ����ģ�� */
    pstPyMod = PyImport_ImportModule(pszModule);
    if (!pstPyMod)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }

    /* ���Һ��� */
    pstPyFunc = PyObject_GetAttrString(pstPyMod, pszFunc);
    if (!pstPyFunc)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }

    /* �������� */
    va_start(vargs, pszPyFormat);
    pstParam = Py_VaBuildValue( pszPyFormat, vargs);
    if (!pstParam)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }
    va_end(vargs);

    /* �������� */
    pstRetVal = PyEval_CallObject(pstPyFunc, pstParam);
    if (!pstRetVal)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }

    /* ��ȡpython��������ֵ */
    PyArg_Parse(pstRetVal, "i", &lRet);
    if (ulRet < 0)
    {
        DOS_ASSERT(0);
        ulRet = DOS_FAIL;
        goto py_finished;
    }

py_finished:
    if (pstRetVal)
    {
        Py_DECREF(pstRetVal);
        pstRetVal = NULL;
    }
    if (pstParam)
    {
        Py_DECREF(pstParam);
        pstParam= NULL;
    }
    if (pstPyFunc)
    {
        Py_DECREF(pstPyFunc);
        pstPyFunc = NULL;
    }
    if (pstPyMod)
    {
        Py_DECREF(pstPyMod);
        pstPyMod= NULL;
    }
    return ulRet;
}


/*
 * ������: U32  py_deinit()
 * ����:   �޲���
 * ����:   ж��Python��
 * ����ֵ: ж��ʧ���򷵻�DOS_FAIL���ɹ��򷵻�DOS_SUCC
 **/
U32  py_deinit()
{
    /* ж��Pythonģ�� */
    Py_Finalize();

    return DOS_SUCC;
}

#endif /* endof INCLUDE_SERVICE_PYTHON */

#ifdef __cplusplus
}
#endif /* __cplusplus */

