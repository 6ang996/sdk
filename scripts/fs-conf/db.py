# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: February 6th,2015
@todo: pakete the operation about database
'''

import MySQLdb
import file_info

def get_db_conn(seqDBHost, seqDBUsername, seqDBPassword, seqDBName): 
    '''
    @param db_host: ���ݿ�����ip
    @param db_username: ���ݿ��û���
    @param db_password: ���ݿ�����
    @param db_name: ���ݿ���
    @todo: �������ݿ�
    '''
    if seqDBHost.strip() == '':
        file_info.get_cur_runtime_info('seqDBHost is %s' % seqDBHost)
        return -1
    if seqDBUsername.strip() == '':
        file_info.get_cur_runtime_info('seqDBUsername is %s' % seqDBUsername)
        return -1
    if seqDBPassword.strip() == '':
        file_info.get_cur_runtime_info('seqDBPassword is %s' % seqDBPassword)
        return -1
    if seqDBName.strip() == '':
        file_info.get_cur_runtime_info('seqDBName is %s' % seqDBName)
        return -1
    
    file_info.get_cur_runtime_info("%s %s %s %s" % (seqDBHost, seqDBUsername, seqDBPassword, seqDBName))
    conn = MySQLdb.connect(seqDBHost,  seqDBUsername, seqDBPassword, seqDBName)
    if conn is None:
        file_info.get_cur_runtime_info('conn is %p' % conn)
        return -1
    return conn

def get_field_value(seqTableName, szFieldName, conn):
    '''
    @param table_name: ���ݿ����
    @param field_name: �ֶ�����
    @param conn: ���ݿ�����
    @todo: �����ݿ��ֶ��л�ȡsip�б�
    '''
    if seqTableName.strip() == '':
        file_info.get_cur_runtime_info('seqTableName is %s' % seqTableName)
        return -1
    if szFieldName.strip() == '':
        file_info.get_cur_runtime_info('szFieldName is %s' % szFieldName)
        return -1
    seqSQLCmd = 'SELECT DISTINCT %s FROM %s' % (szFieldName, seqTableName)
    file_info.get_cur_runtime_info('execute SQL:%s' % seqSQLCmd)
    cursor = conn.cursor()
    cursor.execute(seqSQLCmd)
    results = cursor.fetchall()
    file_info.get_cur_runtime_info(results)
    return results
