# coding=utf-8
'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 3rd,2015
@todo: generate the head of customer
'''
from xml.dom.minidom import Document
import customer_head
import group
import dom_to_xml
import db_conn
import file_info
import db_config
import os

def generate_customer_file(ulCustomerID):
    '''
    @param customer_id: �ͻ�id
    @todo: ���ɿͻ������ļ�
    '''
    if str(ulCustomerID).strip() == '':
        file_info.get_cur_runtime_info('ulCustomerID is %d' % ulCustomerID)
        return -1
    
    # ���ҳ�����customer id��`customer_id`�� group id
    seqSQLCmd = 'SELECT DISTINCT CONVERT(id, CHAR(10)) AS id FROM tbl_group WHERE tbl_group.customer_id = %s' % ulCustomerID
    file_info.get_cur_runtime_info('seqSQLCmd is %s' % seqSQLCmd)
    doc = Document()
    
    seqCfgPath = db_config.get_db_param()['fs_config_path']
    if seqCfgPath[-1] != '/':
        seqCfgPath = seqCfgPath + '/'
    
    if os.path.exists(seqCfgPath) is False:
        os.makedirs(seqCfgPath)
        
    seqMgntDir = seqCfgPath + 'directory/' + str(ulCustomerID) + '/'
    if os.path.exists(seqMgntDir) is False:
        os.mkdir(seqMgntDir)
        
    seqCfgPath = seqCfgPath + 'directory/' + str(ulCustomerID) + '.xml'
    lRet = db_conn.connect_db()
    if -1 == lRet:
        file_info.get_cur_runtime_info('lRet is %d' % lRet)
        return -1
    cursor = db_conn.CONN.cursor()
    if cursor is None:
        file_info.get_cur_runtime_info('The database connection does not exist.')
        return -1
    ulCusCount = cursor.execute(seqSQLCmd)
    arrGroupResult = cursor.fetchall()
    if len(arrGroupResult) == 0:
        file_info.get_cur_runtime_info('len(arrGroupResult) is %d' % len(arrGroupResult))
        return -1
    db_conn.CONN.close()
    file_info.get_cur_runtime_info(arrGroupResult)
    
    # ���ɿͻ������ļ�ͷ��
    lRet = customer_head.generate_customer_head(seqCfgPath, doc)
    if lRet == -1:
        return -1
    (domIncludeNode , domGroupsNode) = lRet
    for loop in range(0, ulCusCount):
        # ������ϯ�鲢������ڵ�DOM Nodeָ��
        domGroupNode = group.generate_group(doc, arrGroupResult[loop][0], ulCustomerID)
        if -1 == domGroupNode:
            file_info.get_cur_runtime_info('domGroupNode is %d' % domGroupNode)
            return -1
        # ��group�����groups
        domGroupsNode.appendChild(domGroupNode)
        # ��include�ڵ������doc
        doc.appendChild(domIncludeNode)
        # ����xml
        lRet = dom_to_xml.dom_to_pretty_xml(seqCfgPath, doc)
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        # ȥ��xml�ļ�ͷ����xml����
        lRet = dom_to_xml.del_xml_head(seqCfgPath)
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        
        return 1
        