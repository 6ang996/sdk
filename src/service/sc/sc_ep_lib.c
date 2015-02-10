/*
 *            (C) Copyright 2014, DIPCC . Co., Ltd.
 *                    ALL RIGHTS RESERVED
 *
 *  �ļ���: sc_ep_sb_adapter.c
 *
 *  ����ʱ��: 2015��1��9��11:13:01
 *  ��    ��: Larry
 *  ��    ��: SCģ�鹫���⺯��
 *  �޸���ʷ:
 */

U32 sc_get_trunk_id(S8 *pszIPAddr, S8 *pszGatewayName)
{
    SC_TRACE_IN(pszIPAddr, pszGatewayName, 0, 0);

    /* ע������ط�Ӧ��ʹ�� && ���ţ�ֻҪ��һ���Ϸ��ͺ� */
    if (DOS_ADDR_INVALID(pszGatewayName) && DOS_ADDR_INVALID(pszIPAddr))
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return U32_BUTT;
    }

    /* ȥ���ݿ��ѯ */

    SC_TRACE_OUT();
    return U32_BUTT;
}

U32 sc_get_custom_id(S8 *pszNumber, U32 ulNumType)
{
    SC_TRACE_IN(pszNumber, ulNumType, 0, 0);

    if (DOS_ADDR_INVALID(pszNumber) || ulNumType >= SC_NUM_TYPE_BUTT)
    {
        DOS_ASSERT(0);

        SC_TRACE_OUT();
        return U32_BUTT;
    }

    SC_TRACE_OUT();
    return U32_BUTT;
}

