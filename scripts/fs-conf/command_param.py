# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 5th,2015
@todo: get the data of python interpreter
'''

import sys

def get_param():
    '''
    @todo: ��ȡPython�����в����б�
    '''
    _list = sys.argv
    if len(_list) == 0:
        return -1
    return sys.argv