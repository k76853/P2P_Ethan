// Ex1_ethan.cpp : 定义控制台应用程序的入口点。
//
#include <WinSock.h>
#include <iostream>
#include <map>
#include <vector>
#include <sstream>
#include <ib/mysql.h>
#pragma comment(lib, "libmysql.lib")
#pragma comment(lib, "ws2_32.lib")
#define MAXSOCKETCON 200
SOCKET ClientSockets[MAXSOCKETCON];
SOCKADDR_IN ClientSocketAddrs[MAXSOCKETCON];
int NowClientSocket = 0;
SOCKET socketServer;
using namespace std;
map<string, string> fileAddr;
int CreateServerSocket(int Port)
{
	int iErrCode;
	WSADATA wsaData;
	iErrCode = WSAStartup(0x0202, &wsaData);
	int iRes;
	//创建Socket
	socketServer = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	//绑定
	SOCKADDR_IN addr;
	addr.sin_family = AF_INET;
	addr.sin_addr.s_addr = inet_addr("192.168.4.13");
	addr.sin_port = htons(Port);
	iRes = bind(socketServer, (LPSOCKADDR)&addr, sizeof(addr));
	if (iRes == SOCKET_ERROR)
	{
		closesocket(socketServer);
		return -1;
	}
	iRes = listen(socketServer, MAXSOCKETCON);
	if (iRes == SOCKET_ERROR)
	{
		closesocket(socketServer);
		return -1;
	}
	return 1;
}

DWORD WINAPI RecvProc(LPVOID pParam)
{
	char Data[256];
	int iBuffertcp = 0;
	TIMEVAL TimeOut = { 1, 0 };
	while (1)
	{
		if (NowClientSocket == 0)
		{
			continue;
		}
		fd_set ReadSet;
		FD_ZERO(&ReadSet);
		for (int i = 0; i<NowClientSocket; i++)
		{
			FD_SET(ClientSockets[i], &ReadSet);
		}
		select(0, &ReadSet, NULL, NULL, &TimeOut);
		for (int i = 0; i<NowClientSocket; i++)
		{
			if (FD_ISSET(ClientSockets[i], &ReadSet))
			{
				iBuffertcp = recv(ClientSockets[i], Data, 256, 0);
				if (iBuffertcp>0)
				{
					Data[iBuffertcp] = '\0';
					cout << ClientSockets[i] << ":";
					string cmd = Data;
					cout << cmd.data() << endl;
					if (cmd == "LIST"){
						string msg = "File List:\r\n";
						for (map<string, string>::iterator it = fileAddr.begin(); it != fileAddr.end(); ++it){
							msg += "FileName:" + it->first + " ";
							msg += "Ip:" + it->second + "\r\n";
						}
						send(ClientSockets[i], msg.data(), msg.size(), 0);
					}
					if (cmd.substr(0, 7) == "REQUEST"){
						string fn = cmd.substr(8);
						string msg = "IP";
						msg += fileAddr[fn];
						msg += " ";
						msg += fn;
						send(ClientSockets[i], msg.data(), msg.size(), 0);
					}
					if (cmd.substr(0, 3) == "ADD"){
						string fn = cmd.substr(4);
						string msg = "ADD Success！";
						fileAddr[fn] = inet_ntoa(ClientSocketAddrs[i].sin_addr);
						cout << "add:" << fileAddr[fn];
						send(ClientSockets[i], msg.data(), msg.size(), 0);
					}
					if (cmd.substr(0, 6) == "DELETE"){
						string fn = cmd.substr(7);
						string msg = "DELETE Success！";
						fileAddr.erase(fn);
						send(ClientSockets[i], msg.data(), msg.size(), 0);
					}
					if (cmd.substr(0, 4) == "QUIT"){
						int stOk = shutdown(ClientSockets[i], 2);
						int clOk = closesocket(ClientSockets[i]);
					}
				}
			}
		}
	}
}

DWORD WINAPI AcceptTcpProc(LPVOID pParam)
{
	SOCKADDR_IN addr;
	int addrLen = sizeof(addr);
	SOCKET tempSocket;
	while (1)
	{
		cout << "wait for accept" << endl;
		tempSocket = accept(socketServer, (sockaddr*)&addr, &addrLen);
		if (tempSocket == INVALID_SOCKET)
		{
			return 0;
		}
		ClientSockets[NowClientSocket] = tempSocket;
		ClientSocketAddrs[NowClientSocket] = addr;
		char ip[15];
		sprintf(ip, inet_ntoa(addr.sin_addr));
		string sip = ip;
		cout << sip.data() << endl;
		USHORT port = addr.sin_port;
		cout << port << endl;
		NowClientSocket++;
	}
}
int main()
{
	if (CreateServerSocket(7777)<1)return 0;
	try
	{
		MYSQL m_sqlCon;
		mysql_init(&m_sqlCon);
		// localhost:server root:name password database:test port:5029  
		if (!mysql_real_connect(&m_sqlCon, "localhost", "", "", "test", 3306, NULL, 0))	cout << "Connection failed!" << endl;
		string sqlstr = "SELECT IP,Name FROM File";
		MYSQL_RES *result = NULL;
		if (0 == mysql_query(&m_sqlCon, sqlstr.c_str())) {
			cout << "mysql_query() select data succeed" << endl;
			//get dataset
			result = mysql_store_result(&m_sqlCon);
			//get rowcount
			int rowcount = mysql_num_rows(result);
			cout << "row count: " << rowcount << endl;
			//get fieldcount
			unsigned int fieldcount = mysql_num_fields(result);
			MYSQL_ROW row = NULL;
			row = mysql_fetch_row(result);
			int j = 0;
			while (NULL != row) {
				fileAddr[row[1]] = row[0];
				row = mysql_fetch_row(result);
			}
		}
		else {
			cout << "mysql_query() select data failed" << endl;
			mysql_close(&m_sqlCon);
			system("pause");
			return -1;
		}
	}
	catch (...)
	{

	}
	CreateThread(NULL, NULL, &AcceptTcpProc, NULL, NULL, NULL);
	CreateThread(NULL, NULL, &RecvProc, NULL, NULL, NULL);
	while (1)
	{
		Sleep(1000);
	}
	return 0;
}