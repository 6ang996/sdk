# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 6th,2015
@todo: operation about Excel files
'''

import xlrd
import types
import file_info
 
  
def get_data_from_table(seqFileName, ulSheetIdx = 0):
    '''
    @param filename: �ļ���
    @param sheet_idx: Excel����ţ���һ�ű�Ϊ���0��Ĭ�ϴӱ��Ϊ0�ı���ȡ����
    @todo: ��Excel�ļ��л�ȡ����
    '''
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('seqFileName is %s' % seqFileName)
        return -1
    if ulSheetIdx < 0:
        file_info.get_cur_runtime_info('ulSheetIdx is %d' % ulSheetIdx)
        return -1
    book = xlrd.open_workbook(seqFileName, 'rb')
    return book.sheet_by_index(ulSheetIdx) #sheet_idx index
        
def excel_table_by_row(seqFileName, ulRow = 0, ulSheetIdx = 0):
    '''
    @param filename: �ļ���
    @param row: sip�����б����ڵ��б�ţ���1�б��Ϊ0��Ĭ�ϴӱ��Ϊ0���ж�ȡ����
    @param sheet_idx: sip�������ڵı���ţ���һ�ű���Ϊ0,Ĭ�ϴӱ��Ϊ0�ı���ȡ����
    @todo:  ��ȡExcelĳ������
    '''
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('seqFileName is %s' % seqFileName)
        return -1
    if ulRow < 0:
        file_info.get_cur_runtime_info('ulRow is %s' % str(ulRow))
        return -1
    if ulSheetIdx < 0:
        file_info.get_cur_runtime_info('ulSheetIdx is %s' % str(ulSheetIdx))
        return -1
    
    table = get_data_from_table(seqFileName, ulSheetIdx)
    if table is None:
        file_info.get_cur_runtime_info('tabe is %p' % table)
        return -1
    listRowList = table.row_values(ulRow)
    if listRowList == []:
        file_info.get_cur_runtime_info(listRowList)
        return -1
    for loop in range(len(listRowList)):
        listRowList[loop] = float_to_int(listRowList[loop])
    
    file_info.get_cur_runtime_info(listRowList)
    return listRowList


def excel_table_by_col(seqFileName, ulCol = 0, ulSheetIdx = 0):
    '''
    @param filename: �ļ���
    @param col: sip�����б����ڵ��б�ţ���1�б��Ϊ0��Ĭ�ϴӱ��Ϊ0���ж�ȡ����
    @param sheet_idx: sip�������ڵı���ţ���һ�ű���Ϊ0,Ĭ�ϴӱ��Ϊ0�ı���ȡ����
    @todo:  ��ȡExcelĳ������
    '''
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('seqFileName is %s' % seqFileName)
        return -1
    if ulCol < 0:
        file_info.get_cur_runtime_info('ulCol is %d' % ulCol)
        return -1
    if ulSheetIdx < 0:
        file_info.get_cur_runtime_info('ulSheetIdx is %d' % ulSheetIdx)
        return -1
    table = get_data_from_table(seqFileName, ulSheetIdx)
    listColList = table.col_values(ulCol)
    if listColList == []:
        file_info.get_cur_runtime_info(listColList)
        return -1
    for loop in range(len(listColList)):
        listColList[loop] = float_to_int(listColList[loop])
    
    file_info.get_cur_runtime_info(listColList)
    return listColList

def excel_table_by_cell(seqFileName, ulRow = 0, ulCol = 0, ulSheetIdx = 0):
    '''
    @param filename: �ļ���
    @param row: ��Ԫ�������б�ţ���һ�е��б��Ϊ0��Ĭ��Ϊ���Ϊ0����
    @param col: ��Ԫ�������б�ã���һ�е��б��Ϊ0��Ĭ��Ϊ���Ϊ0����
    @param sheet_idx: sip�������ڵı���ţ���һ�ű���Ϊ0,Ĭ�ϴӱ��Ϊ0�ı���ȡ����
    @todo: ��ȡĳһ����Ԫ�������
    '''
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('seqFileName is %s' % seqFileName)
        return -1
    if ulRow < 0:
        file_info.get_cur_runtime_info('ulRow is %d' % ulRow)
        return -1
    if ulCol < 0:
        file_info.get_cur_runtime_info('ulCol is %d' % ulCol)
        return -1
    if ulSheetIdx < 0:
        file_info.get_cur_runtime_info('ulSheetIdx is %d' % ulSheetIdx)
        return -1
    table = get_data_from_table(seqFileName, ulSheetIdx)
    if table is None:
        return -1
    value = table.cell(ulRow, ulCol).value
    value = float_to_int(value)
    file_info.get_cur_runtime_info('value is %s' % str(value))
    return value


def excel_table_by_table(seqFileName, ulSheetIdx = 0):
    '''
    @param filename: �ļ���
    @param sheet_idx: sip�������ڵı���ţ���һ�ű���Ϊ0,Ĭ�ϴӱ��Ϊ0�ı���ȡ����
    @todo: ��ȡĳһ��Excel����е���������
    '''
    if seqFileName.strip() == '':
        file_info.get_cur_runtime_info('seqFileName is %s' % seqFileName)
        return -1
    table = get_data_from_table(seqFileName, ulSheetIdx)
    if table is None:
        file_info.get_cur_runtime_info('table is %p' % table)
        return -1
    _list = []
    nRows = table.nrows
    for loop in range(nRows):
        rowlist = excel_table_by_row(seqFileName, loop, ulSheetIdx)
        if rowlist == -1:
            return -1
        _list.append(rowlist)

    file_info.get_cur_runtime_info(_list)
    return _list

def is_float_str(ulNum):
    '''
    @param num: ����
    @todo: �ж��Ƿ�Ϊ������
    '''
    if str(ulNum).endswith('.0'):
        return True
    return False

def float_to_int(ulNum):
    '''
    @param num: ����
    @todo: ������ת��Ϊ����
    '''
    if type(ulNum) == types.FloatType:
        return str(int(ulNum))
    elif  type(ulNum) == types.StringType and is_float_str(ulNum):
        ulNum = ulNum[:-2] #ȥ����β�ġ�.0���Ӵ�
        return ulNum
