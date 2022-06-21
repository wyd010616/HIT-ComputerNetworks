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

struct routeTableItem//路由表单项的数据结构
{
	unsigned int destIP;//目的IP地址
	unsigned int mask;//子网掩码，用于将IP地址进行按位与，从而在路由表里进行匹配
	unsigned int masklen;//前缀的长度,即子网掩码中为1部分的长度
	unsigned int nexthop;//下一跳地址
};

vector<routeTableItem> m_table; //路由表

void stud_Route_Init()//初始化函数，直接清空路由表
{
	m_table.clear();//调用clear函数
	return;
}

//向路由表添加路由的函数
void stud_route_add(stud_route_msg *proute)
{
	routeTableItem newTableItem;//新建一个路由表单项
	newTableItem.masklen = ntohl(proute->masklen);//提取前缀长度
	newTableItem.mask = (1<<31)>>(ntohl(proute->masklen)-1); //左移填0右移填1//根据前缀长度构建子网掩码
	newTableItem.destIP = ntohl(proute->dest);//提取目的地址
	newTableItem.nexthop = ntohl(proute->nexthop);//提取下一跳地址
	m_table.push_back(newTableItem);//将这一个单项加入到路由表的末尾
	return;
}

int stud_fwd_deal(char *pBuffer, int length)
{

	int TTL = (int)pBuffer[8];  //生命周期（TTL）
	int headerChecksum = ntohl(*(unsigned short*)(pBuffer+10)); //首部校验和
	int DestIP = ntohl(*(unsigned int*)(pBuffer+16)); //目的地址
    int headsum = pBuffer[0] & 0xf;  //获取头部长度（以4个字节位单位）

	if(TTL <= 0) //TTL错误
	{
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_TTLERROR); //丢弃
		return 1;//返回1
	}

	if(DestIP == getIpv4Address()) //目的地址与本机地址相同
	{
		fwd_LocalRcv(pBuffer, length); //本机接收
		return 0;//返回0
	}

	//转发操作
	bool Match = false;//是否匹配到
	unsigned int longestMatchLen = 0;//最大匹配长度
	int bestMatch = 0;//用来记录表的序号
	//按照最长匹配查找路由表获取下一跳
	for(int i = 0; i < m_table.size(); i ++)
	{
		if(m_table[i].masklen > longestMatchLen && m_table[i].destIP == (DestIP & m_table[i].mask)) //匹配成功，且匹配到的长度比之前更大
		{
			bestMatch = i;//记录表的序号
			Match = true;//修改match，标志着找到了匹配的路由项目
			longestMatchLen = m_table[i].masklen;//记录最大的前缀长度
		}
	}

	if(Match) //匹配成功，进行转发
	{
		char *buffer = new char[length];
            memcpy(buffer,pBuffer,length);//将分组内容接收到新的缓冲区buffer中
            //更新TTL的值
            buffer[8]--; //TTL - 1
            //重新计算首部校验和
		int sum = 0;
            unsigned short int localCheckSum = 0;
            //每16位一相加（除首部校验和）
            for(int j = 0; j < 2 * headsum ; j ++)
            {
                if (j != 5){ //不是校验和checksum部分
                    sum = sum + (buffer[j*2]<<8)+(buffer[j*2+1]);
                    sum %= 65535; //补码求和
                }
            }
			localCheckSum = htons(~(unsigned short int)sum);//取反
		memcpy(buffer+10, &localCheckSum, sizeof(unsigned short));//填入校验和
		//开始转发
		fwd_SendtoLower(buffer, length, m_table[bestMatch].nexthop);//调用函数转发分组，其中第三个参数表明了上面成功的找到了下一跳的地址
		return 0;
	}
	else //匹配不成功
	{
		fwd_DiscardPkt(pBuffer, STUD_FORWARD_TEST_NOROUTE); //丢弃
		return 1;
	}
	return 1;
}
