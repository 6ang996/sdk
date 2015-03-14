# coding=utf-8
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 11th,2015
@todo: operation of customer
'''
import db_config
import file_info
import customer_file
import os

def add_customer(ulCustomerID):
    '''
    @param customer_id: �ͻ�id
    @todo: ����һ���ͻ�
    '''
    if str(ulCustomerID).strip() == '':
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
    
    #���ɸ��˻��������ļ�
    lRet = customer_file.generate_customer_file(ulCustomerID)
    if lRet == -1:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    return 1
         
def del_customer(ulCustomerID):
    '''
    @param customer_id: ��ɾ���Ŀͻ�id
    @todo: ɾ��һ���ͻ�
    '''
    if str(ulCustomerID).strip() == '':
        file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
        return -1
    file_info.get_cur_runtime_info('ulCustomerID is %s' % str(ulCustomerID))
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if -1 == seqCfgPath:
        file_info.get_cur_runtime_info('seqCfgPath is %d' % seqCfgPath)
        return -1
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
        
    # ɾ���ͻ���Ϣ
    sqeMgntDir = seqCfgPath + 'directory/' + str(ulCustomerID) + '/'
    seqMgntXml = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    if os.path.exists(sqeMgntDir):
        os.system('rm -rf %s' % sqeMgntDir)
    if os.path.exists(seqMgntXml):
        os.system('rm %s' % seqMgntXml)
    
    return 1
        