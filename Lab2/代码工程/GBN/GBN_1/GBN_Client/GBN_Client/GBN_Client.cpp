// GBN_Client.cpp : �������̨Ӧ�ó������ڵ㡣
//


#include <stdio.h>
#include <stdlib.h>
#include <WinSock2.h>
#include <time.h>
#pragma comment(lib,"ws2_32.lib")
#define SERVER_PORT  12340 //�������ݵĶ˿ں�
#define SERVER_IP  "127.0.0.1" // �������� IP ��ַ
const int BUFFER_LENGTH = 1026;
const int SEQ_SIZE = 20;//���ն����кŸ�����Ϊ 1~20


/*Ϊ�ж����ݴ����Ƿ������ӻ��޸ĵı���*/
int finish;
/*Ϊ�ж����ݴ����Ƿ������ӻ��޸ĵı���*/


/****************************************************************/
/* -time �ӷ������˻�ȡ��ǰʱ��
-quit �˳��ͻ���
-testGBN [X] ���� GBN Э��ʵ�ֿɿ����ݴ���
[X] [0,1] ģ�����ݰ���ʧ�ĸ���
[Y] [0,1] ģ�� ACK ��ʧ�ĸ���
*/
/****************************************************************/
void printTips() {
	printf("*****************************************\n");
	printf("| -time to get current time |\n");
	printf("| -quit to exit client |\n");
	printf("| -testGBN to test the gbn |\n");
	printf("*****************************************\n");
}

//************************************
// Method: lossInLossRatio
// FullName: lossInLossRatio
// Access: public
// Returns: BOOL
// Qualifier: ���ݶ�ʧ���������һ�����֣��ж��Ƿ�ʧ,��ʧ�򷵻�TRUE�����򷵻� FALSE
// Parameter: float lossRatio [0,1]
//************************************
//lossRatio��ʧ��
BOOL lossInLossRatio(float lossRatio) {
	int lossBound = (int)(lossRatio * 100);//����ʧ��ת����100���ڵ�һ�����������������ʾ��������С���������ʾ������
	int r = rand() % 101;//�������һ��100���ڵ������
	if (r <= lossBound) {
		return TRUE;
	}
	return FALSE;
}



int main(int argc, char* argv[])
{
	//�����׽��ֿ⣨���룩
	WORD wVersionRequested;
	WSADATA wsaData;
	//�׽��ּ���ʱ������ʾ
	int err;
	//�汾 2.2
	wVersionRequested = MAKEWORD(2, 2);
	//���� dll �ļ� Scoket ��
	err = WSAStartup(wVersionRequested, &wsaData);
	if (err != 0) {
		//�Ҳ��� winsock.dll
		printf("WSAStartup failed with error: %d\n", err);
		return 1;
	}
	if (LOBYTE(wsaData.wVersion) != 2 || HIBYTE(wsaData.wVersion) != 2)//LOBYTE��ʾ�汾�ŵ��ֽڣ�HIBYTE��ʾ���ֽڡ����汾���Ƿ���2.2
	{
		printf("Could not find a usable version of Winsock.dll\n");
		WSACleanup();
	}
	else {
		printf("The Winsock 2.2 dll was found okay\n");
	}
	SOCKET socketClient = socket(AF_INET, SOCK_DGRAM, 0);//��ʼ��һ���׽���
	SOCKADDR_IN addrServer;//��ʾ�ͻ��˵�ַ
	addrServer.sin_addr.S_un.S_addr = inet_addr(SERVER_IP);//�ͻ��˵�ַ��ֵΪ�����Ķ˵��ַ
	addrServer.sin_family = AF_INET;
	addrServer.sin_port = htons(SERVER_PORT);//���ö˿ں�
	//���ջ�����
	char buffer[BUFFER_LENGTH];
	ZeroMemory(buffer, sizeof(buffer));
	int len = sizeof(SOCKADDR);
	//Ϊ�˲���������������ӣ�����ʹ�� -time ����ӷ������˻�õ�ǰʱ��
	//ʹ�� -testGBN [X] [Y] ���� GBN ����[X]��ʾ���ݰ���ʧ����
	//  [Y]��ʾ ACK ��������
	printTips();
	int ret;
	int interval = 1;//�յ����ݰ�֮�󷵻� ack �ļ����Ĭ��Ϊ 1 ��ʾÿ�������� ack��0 ���߸�������ʾ���еĶ������� ack
	char cmd[128];//��������
	float packetLossRatio = 0.2; //Ĭ�ϰ���ʧ�� 0.2
	float ackLossRatio = 0.2;  //Ĭ�� ACK ��ʧ�� 0.2
	//��ʱ����Ϊ������ӣ�����ѭ����������
	srand((unsigned)time(NULL));
	finish = 0;
	while (true) {
		gets_s(buffer);//��������������뵽Buffer��
		ret =sscanf(buffer, "%s%f%f", &cmd, &packetLossRatio, &ackLossRatio);
		//��ʼ GBN ���ԣ�ʹ�� GBN Э��ʵ�� UDP �ɿ��ļ�����
		if (!strcmp(cmd, "-testGBN")) {
			printf("%s\n", "Begin to test GBN protocol, please don't abort the  process");
			printf("The loss ratio of packet is %.2f,the loss ratio of ack  is %.2f\n", packetLossRatio, ackLossRatio);//��ӡ������
			int stage = 0;
			finish = 0;//��ʾ���ݴ����Ƿ���ɣ����ֵΪ1������Ϊ0
			BOOL b;
			unsigned char u_code;//״̬��205 200�� ��ASCII��ʾ״̬��
			unsigned short seq;//�������к�
			unsigned short recvSeq;//���մ��ڴ�СΪ 1����ȷ�ϵ����к�
			unsigned short waitSeq;//�ȴ������к�
			sendto(socketClient, "-testGBN", strlen("-testGBN") + 1, 0,
				(SOCKADDR*)&addrServer, sizeof(SOCKADDR));//��������뷢�͵������
			while (true)
			{
				//�ȴ� server �ظ������� UDP Ϊ����ģʽ
				recvfrom(socketClient, buffer, BUFFER_LENGTH, 0, (SOCKADDR*)&addrServer, &len);


				/*Ϊ�ж����ݴ����Ƿ������ӻ��޸ĵ����*/
				if (!strcmp(buffer, "���ݴ���ȫ����ɣ�����\n")) {
					finish = 1;
					break;
				}
				/*Ϊ�ж����ݴ����Ƿ������ӻ��޸ĵ����*/


				switch (stage) {
				case 0://�ȴ����ֽ׶�
					u_code = (unsigned char)buffer[0];
					if ((unsigned char)buffer[0] == 205)
					{
						printf("Ready for file transmission\n");
						buffer[0] = 200;
						buffer[1] = '\0';
						sendto(socketClient, buffer, 2, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));//֪ͨ�������ˣ��Ѿ�׼���ô���
						stage = 1;//״̬ת��Ϊ�ȴ���������״̬
						recvSeq = 0;
						waitSeq = 1;
					}
					break;
				case 1://�ȴ��������ݽ׶�
					seq = (unsigned short)buffer[0];
					//�����ģ����Ƿ�ʧ
					b = lossInLossRatio(packetLossRatio);
					if (b) {	
						printf("The packet with a seq of %d loss\n", seq);
						continue;
					}
					printf("recv a packet with a seq of %d\n", seq);
					//������ڴ��İ�����ȷ���գ�����ȷ�ϼ���
					if (!(waitSeq - seq)) {
						++waitSeq;
						if (waitSeq == 21) {
							waitSeq = 1;
						}
						//�������
						printf("\n\n%s\n\n", &buffer[1]);//Buffer[1]�����ݿ�ʼ�ĵط�
						buffer[0] = seq;
						recvSeq = seq;
						buffer[1] = '\0';
					}
					else {
						//�����ǰһ������û���յ�����ȴ� Seq Ϊ 1 �����ݰ��������򲻷��� ACK����Ϊ��û����һ����ȷ�� ACK��
						if (!recvSeq) {
							continue;
						}
						buffer[0] = recvSeq;
						buffer[1] = '\0';
					}
					b = lossInLossRatio(ackLossRatio);
					if (b) {
						printf("The  ack  of  %d  loss\n", (unsigned char)buffer[0]);
						continue;
					}
					sendto(socketClient, buffer, 2, 0, (SOCKADDR*)&addrServer, sizeof(SOCKADDR));//���������˷���ACK
					printf("send a ack of %d\n", (unsigned char)buffer[0]);
					break;
				}
				Sleep(500);
			}
		}


		/*Ϊ�ж����ݴ����Ƿ������ӻ��޸ĵ����*/
		if (finish == 1) {
			printf("���ݴ���ȫ����ɣ�����\n\n");
			printTips();
			finish = 0;
			continue;
		}
		/*Ϊ�ж����ݴ����Ƿ������ӻ��޸ĵ����*/


		sendto(socketClient, buffer, strlen(buffer) + 1, 0,
			(SOCKADDR*)&addrServer, sizeof(SOCKADDR));
		ret = recvfrom(socketClient, buffer, BUFFER_LENGTH, 0, (SOCKADDR*)&addrServer, &len);
		printf("%s\n", buffer);
		if (!strcmp(buffer, "Good bye!")) {
			break;
		}
		printTips();
	}
	//�ر��׽���
	closesocket(socketClient);
	WSACleanup();
	return 0;
}