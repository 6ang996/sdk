# coding=utf-8

'''
@author: bubble
@copyright: Shenzhen dipcc technologies co.,ltd
@time: Feburary 13th,2015
@todo: operation of customer
'''

import file_info
import sip_customer
import sip_mgnt
import group_mgnt
import customer_mgnt

def config_operation(ulOpObj, ulOpType, ulObjID, seqParam1, seqParam2):
    '''
    @param op_obj: ��ʾ��������Ϊsip�˻�����ϯ�顢�ͻ��е�ĳһ������
    @param op_type: ��ʾ�������ͣ��������ӡ�ɾ�������ġ�Load�е�һ��
    @param obj_id: ���������id
    @param param1: ��������1���ɲ�������Ͳ������͹�ͬ����
    @param param2: ��������2���ɲ�������Ͳ������͹�ͬ����
    @todo: ���������ļ�����������ɲ�������Ͳ������͹�ͬ����
    '''
    if str(ulOpObj).strip() == '':
        file_info.get_cur_runtime_info('ulOpObj is %s' % str(ulOpObj))
        return -1
    if str(ulOpType).strip() == '':
        file_info.get_cur_runtime_info('ulOpType is %s' % str(ulOpType))
        return -1
    if str(ulObjID).strip() == '':
        file_info.get_cur_runtime_info('ulObjID is %s' % str(ulObjID))
        return -1
    
    # ���op_typeֵΪ0, ���������ΪLoad����ô�������е����ã�����������Ϊ��Ч����
    if str(ulOpType).strip() == '0': #Load
        ulOpObj = -1
        ulObjID = -1
        seqParam1 = None  #set it to be invalid data
        seqParam2 = None  #set it to be invalid data
    
        lRet = sip_customer.generate_customer()  #���Գɹ�
        if -1 == lRet:
            file_info.get_cur_runtime_info('lRet is %d' % lRet)
            return -1
        return 1
    
    # ���op_typeֵΪ1�� ���ʾ���Ӳ���
    elif str(ulOpType).strip() == '1':  #Add
        seqParam1 = None  #set it to be invalid data
        seqParam2 = None  #set it to be invalid data
        # ���op_objֵΪ0�����ʾ����һ��sip�˻���obj_id����sip�˻�id
        if str(ulOpObj).strip() == '0':  #Add sip
            lRet = sip_mgnt.add_sip(ulObjID)  #���Գɹ�
            if -1 == lRet:
                file_info.get_cur_runtime_info('lRet is %d' % lRet)
                return -1
            return 1
        # ���op_objΪ1�����ʾ����һ����ϯ�飬obj_id������ϯ��id
        elif str(ulOpObj).strip() == '1':  #Add group
            lRet = group_mgnt.add_group(ulObjID) # ���Գɹ�
            if -1 == lRet:
                file_info.get_cur_runtime_info('lRet is %d' % lRet)
                return -1
            return 1
        # ���op_objΪ2�����ʾ����һ���ͻ���obj_id����ͻ�id
        elif str(ulOpObj).strip() == '2':  #Add customer
            lRet = customer_mgnt.add_customer(ulObjID) # ���Գɹ�
            if -1 == lRet:
                file_info.get_cur_runtime_info('lRet is %d' % lRet)
                return -1
            return 1
        
    # ���op_typeֵΪ2�����ʾɾ������
    elif str(ulOpType).strip() == '2': #Delete
        # ���op_objΪ0����ʾɾ��һ��sip�˻�����ô����param2����Ϊ"delete"������param1Ϊsip�����ͻ���id�ַ�����obj_idΪsip�˻�id
        if str(ulOpObj).strip() == '0': #Delete sip
            if seqParam2.lower().strip() == 'delete':
                ulCustomerID = int(seqParam1)
                lRet = sip_mgnt.del_sip_from_group(ulObjID, ulCustomerID) # ���Գɹ�
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
        # ���op_objΪ1�����ʾɾ��һ����ϯ�飬��ô����param2����Ϊ"delete"������param1Ϊ����ϯ�������ͻ���id�ַ�����obj_idΪ��ϯ��id
        elif str(ulOpObj).strip() == '1':#Delete group
            if seqParam2.lower().strip() == 'delete':
                ulCustomerID = int(seqParam1)
                lRet = group_mgnt.del_group(ulObjID, ulCustomerID)  # ���Գɹ�
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
        # ���op_objΪ2�����ʾɾ��һ���ͻ�����ôparam2����Ϊ"delete"������param1����ΪNone��obj_idΪ�ͻ�id  
        elif str(ulOpObj).strip() == '2':#Delete customer
            if seqParam2.lower().strip() == 'delete' and seqParam1 == None:
                lRet = customer_mgnt.del_customer(ulObjID)  # ���Գɹ�
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
    # ���op_typeֵ��3����ô��ʾ���²���          
    elif str(ulOpType).strip() == '3': # Update
        # ���op_objΪ0�����ʾ����sip�˻����������
        if str(ulOpObj).strip() == '0': #Update sip
            # ���param2Ϊ�ַ���"change"�����ʾ��sip�˻����ĵ��µ���ϯ�飬obj_idΪsip�˻�id��param1Ϊ�µ���ϯ��id�ַ���
            if seqParam2.lower().strip() == 'change':
                ulNewGroupID = int(seqParam1)
                lRet = sip_mgnt.change_agent_group(ulObjID, ulNewGroupID)   # ���Գɹ�
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
            # ���param1��Ϊ�գ���param2����"change"����ô��ʾ�޸�sip�˻��Ĳ���ֵ������obj_idΪsip�˻�id��param1Ϊ�����ļ��Ĳ�����
            # param2Ϊ�����ļ��Ĳ�������Ӧ�Ĳ���ֵ
            elif seqParam1.strip() != '':
                lRet = sip_mgnt.modify_param_value(ulObjID, seqParam1, seqParam2) # ���Գɹ�
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
        # ���op_objΪ1�����ʾ����group�������
        elif str(ulOpObj).strip() == '1': #Update group
            # ���param2Ϊ"change"����ôparam1Ϊ�µ���ϯ��id�ַ�����obj_id��ԭ��ϯ��id
            if seqParam2.lower().strip() == 'change':
                ulNewGroupID = int(seqParam1)
                lRet = group_mgnt.modify_group_name(ulObjID, ulNewGroupID) # ���Գɹ�
                if -1 == lRet:
                    file_info.get_cur_runtime_info('lRet is %d' % lRet)
                    return -1
                return 1
