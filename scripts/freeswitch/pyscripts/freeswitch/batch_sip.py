# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@togo: pakete the operation about database
'''

import data_source
import command_param
import file_info

def batch_generate():
    '''
    @todo: 批量生成sip账户
    '''
    
    # 获取输入参数
    listList = command_param.get_param()
    if listList == -1:     
        file_info.get_cur_runtime_info(listList)
    
    # 如果指定的是参数'-e'，那么使用枚举方法生成sip账户
    if listList[1].lower() == '-e':
        file_info.get_cur_runtime_info('Enumeration method')
        listList = listList[2:]
        data_source.generate_by_enum(listList)
        return
    # 如果指定的参数'-r'，表示使用指定特定范围的方法生成sip账户
    elif listList[1].lower() == '-r':
        file_info.get_cur_runtime_info('Specify a particular range')
        listList = listList[2:]
        data_source.generate_by_range(listList)
        return
    # 如果执行的参数为'-m'，表示读取Excel表格中sip账户数据去生成sip账户
    elif listList[1].lower() == '-m':
        file_info.get_cur_runtime_info('data from Excel')
        listList = listList[2:]
        data_source.generate_by_excel(listList)
        return
    # 如果指定的参数'-t'，表示读取txt文件中sip账户数据区生成sip账户
    elif listList[1].lower() == '-t':
        file_info.get_cur_runtime_info('data from txt')
        listList = listList[2:]
        data_source.generate_by_txt(listList)
        return
    # 如果指定参数'-d'，表示通过指定特定的数据库的特定字段中的数据生成sip账户
    elif listList[1].lower() == '-d':
        file_info.get_cur_runtime_info('Field in the database table')
        listList = listList[2:]
        data_source.generate_by_db(listList)
        return
    
    
if __name__ == "__main__":
    batch_generate()