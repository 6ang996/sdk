#!/bin/sh

#ptc�������ű�
#declare -i g_processID=0

help(){
    echo "Usage: $0 <process_name>"
    exit 0
}

# ������Χ���
if [ "$#" != 1 ]; 
then   
    help
fi

#ɱ��֮ǰ�Ľ���
#ps -ef | grep ptcd | grep -v grep | awk '{print $2}' | while read pid
#	 do
#		echo $pid
#		kill -9 $pid
#     done

exec ./ptcd &

#������ʵ���Ƿ��Ѿ�����
# while [ 1 ]; do

    DTTERM=`pgrep ptcd | wc -l`
	echo "$DTTERM"
    if [ "$DTTERM" == 1 ]
    then  
        echo "process exit and date is: `date`" 
#��ȷ������Ϣ����־�ļ�
    else
        echo "restart process: $1 and date is: `date`"
		mv ptcd_old ptcd
        exec ./${1} &
    fi
#���ʱ����
    # sleep 1                        
# done