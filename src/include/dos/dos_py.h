#ifndef __DOS_PY_H__
#define __DOS_PY_H__

#define MAX_PY_SCRIPT_LEN 256  //����python·������󳤶�
#define MAX_PY_PARAM_CNT  8    //��������������

#if INCLUDE_SERVICE_PYTHON

U32   py_init_py();
U32   py_c_call_py(const char *pszModule, const char *pszFunc, const char *pszPyFormat, ...);
U32   py_deinit_py();

#endif /* INCLUDE_SERVICE_PYTHON */


#endif  // end __DOS_PY_H__

