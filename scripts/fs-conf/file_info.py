# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@togo: get information about file such as file line number,file name,function name
'''

import sys

def is_windows():
    '''
    @TODO: �жϵ�ǰ����ƽ̨�Ƿ�ΪWindowsƽ̨
    '''
    if 'win' in sys.platform:
        return True
    else:
        return False
    
def is_linux():
    '''
    @TODO: �жϵ�ǰ����ƽ̨�Ƿ�Ϊ����Linuxƽ̨
    '''
    if 'linux' in sys.platform:
        return True
    else:
        return False
    
def get_cur_runtime_info(sequence):
    '''
    @TODO: ��ӡ��ǰ�ļ�����Ϣ
    '''
    
    IS_OUTPUT = 1
    
    if IS_OUTPUT == 0:
        return -1
    
    f = sys._getframe(1)
    
    # ��ȡ��ǰ�ļ���
    seqFileName = f.f_code.co_filename
    if is_windows():
        seqFileName = seqFileName.split('\\')[-1]
    elif is_linux():
        seqFileName = seqFileName.split('/')[-1]
    
    # ��ȡ��ǰ�к�
    ulLineNumber = f.f_lineno
    
    # ��ȡ��ǰ���ں���
    seqFunName   = f.f_code.co_name
    
    print '%s:Line %d:In function %s:' % (seqFileName, ulLineNumber, seqFunName), sequence
    return 1