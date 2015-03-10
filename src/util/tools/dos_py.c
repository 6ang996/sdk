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
 * ������: U32 py_init_py()
 * ����:   �޲���
 * ����:   ��ʼ��Python��
 * ����ֵ: ��ʼ��ʧ���򷵻�DOS_FAIL���ɹ��򷵻�DOS_SUCC
 **/
U32 py_init_py()
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
 * ������: U32   py_c_call_py(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...)
 * ����:   const char *pszModule:  �����õ�Pythonģ����
 *         const char *pszFunc:    ��Ҫ���õ�Python�ӿ�����
 *         const char *pszPyFormat:Python��ʽ������
 * ����:   C���Ե���Python�����ӿ�
 * ����ֵ: ִ��ʧ���򷵻�DOS_FAIL;ִ�гɹ�ʱ�����ڷ���ֵ�򷵻�Python�ķ���ֵ�����򷵻�DOS_SUCC
 *  !!!!!!!!!!!!!!!�ر�˵��!!!!!!!!!!!!!!!!!!
 *  ����pszPyFormat��ʽΪPython������ʽ������C���Բ�����ʽ����������:
 *      s: ��ʾ�ַ���
 *      i: ���ͱ���
 *      f: ��ʾ������
 *      O: ��ʾһ��Python����
 *  ���в�����ʽ�б���Ҫʹ������������������ʹ�ü�: http://blog.chinaunix.net/uid-22920230-id-3443571.html
 **/
U32   py_c_call_py(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...)
{
    S8   szPyScriptPath[MAX_PY_SCRIPT_LEN] = {0,};
    U32  ulRet;
    S8   szImportPath[MAX_PY_SCRIPT_LEN] = {0,};
    PyObject *pstPyMod, *pstPyFunc, *pstParam, *pstRetVal;
    va_list vargs;

    if (!pszModule || !pszFunc || !pszPyFormat)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* ���ù���·�� */
    ulRet = config_get_py_path(szPyScriptPath, sizeof(szPyScriptPath));
    if (0 > ulRet)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    logr_info("%s:Line %d: The current python work directory is: %s"
                , dos_get_filename(__FILE__), __LINE__, szPyScriptPath);

    dos_snprintf(szImportPath, sizeof(szImportPath), "sys.path.append(\'%s\')", szPyScriptPath);
    szImportPath[ MAX_PY_SCRIPT_LEN - 1] = '\0';

    PyRun_SimpleString("import sys");
    PyRun_SimpleString(szImportPath);

    /* ����ģ�� */
    pstPyMod = PyImport_ImportModule(pszModule);
    if (!pstPyMod)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* ���Һ��� */
    pstPyFunc = PyObject_GetAttrString(pstPyMod, pszFunc);
    if (!pstPyFunc)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

    /* �������� */
    va_start(vargs, pszPyFormat);
    pstParam = Py_VaBuildValue( pszPyFormat, vargs );
    if (!pstParam)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }
    va_end(vargs);

    /* �������� */
    pstRetVal = PyEval_CallObject(pstPyFunc, pstParam);
    if (!pstRetVal)
    {
        DOS_ASSERT(0);
        return DOS_FAIL;
    }

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

    return DOS_SUCC;
}


/*
 * ������: U32  py_deinit_py()
 * ����:   �޲���
 * ����:   ж��Python��
 * ����ֵ: ж��ʧ���򷵻�DOS_FAIL���ɹ��򷵻�DOS_SUCC
 **/
U32  py_deinit_py()
{
    /* ж��Pythonģ�� */
    Py_Finalize();

    return DOS_SUCC;
}

#endif /* endof INCLUDE_SERVICE_PYTHON */

#ifdef __cplusplus
}
#endif /* __cplusplus */

