/*
* THIS FILE IS FOR IP FORWARD TEST
*/
#include "sysInclude.h"
#include <vector>
using std::vector;
#include <iostream>
using std::cout;
// system support
extern void fwd_LocalRcv(char *pBuffer, int length);

extern void fwd_SendtoLower(char *pBuffer, int length, unsigned int nexthop);

extern void fwd_DiscardPkt(char *pBuffer, int type);

extern unsigned int getIpv4Address( );

// implemented by students

struct routeTableItem//·�ɱ�������ݽṹ
{
	unsigned int destIP;//Ŀ��IP��ַ
	unsigned int mask;//�������룬���ڽ�IP��ַ���а�λ�룬�Ӷ���·�ɱ������ƥ��
	unsigned int masklen;//ǰ׺�ĳ���,������������Ϊ1���ֵĳ���
	unsigned int nexthop;//��һ����ַ
};

vector<routeTableItem> m_table; //·�ɱ�

void stud_Route_Init()//��ʼ��������ֱ�����·�ɱ�
{
	m_table.clear();//����clear����
	return;
}

//��·�ɱ����·�ɵĺ���
void stud_route_add(stud_route_msg *proute)
{
	routeTableItem newTableItem;//�½�һ��·�ɱ���
	newTableItem.masklen = ntohl(proute->masklen);//��ȡǰ׺����
	newTableItem.mask = (1<<31)>>(ntohl(proute->masklen)-1); //������0������1//����ǰ׺���ȹ�����������
	newTableItem.destIP = ntohl(proute->dest);//��ȡĿ�ĵ�ַ
	newTableItem.nexthop = ntohl(proute->nexthop);//��ȡ��һ����ַ
	m_table.push_back(newTableItem);//����һ��������뵽·�ɱ��ĩβ
	return;
}

int stud_fwd_deal(char *pBuffer, int length)
{

	int TTL = (int)pBuffer[8];  //�������ڣ�TTL��
	int headerChecksum = ntohl(*(unsigned short*)(pBuffer+10)); //�ײ�У���
	int DestIP = ntohl(*(unsigned int*)(pBuffer+16)); //Ŀ�ĵ�ַ
    int headsum = pBuffer[0] & 0xf;  //��ȡͷ�����ȣ���4���ֽ�λ��λ��

	if(TTL <= 0) //TTL����
	{
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR); //����
		return 1;//����1
	}

	if(DestIP == getIpv4Address()) //Ŀ�ĵ�ַ�뱾����ַ��ͬ
	{
		fwd_LocalRcv(pBuffer, length); //��������
		return 0;//����0
	}

	//ת������
	bool Match = false;//�Ƿ�ƥ�䵽
	unsigned int longestMatchLen = 0;//���ƥ�䳤��
	int bestMatch = 0;//������¼������
	//�����ƥ�����·�ɱ��ȡ��һ��
	for(int i = 0; i < m_table.size(); i ++)
	{
		if(m_table[i].masklen > longestMatchLen && m_table[i].destIP == (DestIP & m_table[i].mask)) //ƥ��ɹ�����ƥ�䵽�ĳ��ȱ�֮ǰ����
		{
			bestMatch = i;//��¼������
			Match = true;//�޸�match����־���ҵ���ƥ���·����Ŀ
			longestMatchLen = m_table[i].masklen;//��¼����ǰ׺����
		}
	}

	if(Match) //ƥ��ɹ�������ת��
	{
		char *buffer = new char[length];
            memcpy(buffer,pBuffer,length);//���������ݽ��յ��µĻ�����buffer��
            //����TTL��ֵ
            buffer[8]--; //TTL - 1
            //���¼����ײ�У���
		int sum = 0;
            unsigned short int localCheckSum = 0;
            //ÿ16λһ��ӣ����ײ�У��ͣ�
            for(int j = 0; j < 2 * headsum ; j ++)
            {
                if (j != 5){ //����У���checksum����
                    sum = sum + (buffer[j*2]<<8)+(buffer[j*2+1]);
                    sum %= 65535; //�������
                }
            }
			localCheckSum = htons(~(unsigned short int)sum);//ȡ��
		memcpy(buffer+10, &localCheckSum, sizeof(unsigned short));//����У���
		//��ʼת��
		fwd_SendtoLower(buffer, length, m_table[bestMatch].nexthop);//���ú���ת�����飬���е�������������������ɹ����ҵ�����һ���ĵ�ַ
		return 0;
	}
	else //ƥ�䲻�ɹ�
	{
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE); //����
		return 1;
	}
	return 1;
}
