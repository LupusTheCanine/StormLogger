#define _WIN32_WINNT 0x501
#include <winsock2.h>
#include <ws2tcpip.h>
#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#pragma comment(lib,"Ws2_32.lib")
#include <stdbool.h>

/*
Build options:
-Wall -g -pedantic-errors -pedantic -Wextra
Linker options:
-lwsock32
-lws2_32
*/

int main()
{
	WSADATA wsaData;
	int iResult=WSAStartup(MAKEWORD(2,2),&wsaData);
	if (iResult!=0){
		printf("WSAStartup failed: %d\n",iResult);
		return 1;
	}
	struct addrinfo *result=NULL,hints;
	ZeroMemory(&hints,sizeof(hints));
	hints.ai_family=AF_INET;
	hints.ai_socktype=SOCK_STREAM;
	hints.ai_protocol=IPPROTO_TCP;
	hints.ai_flags=AI_PASSIVE;
	iResult=getaddrinfo(NULL,"18080",&hints,&result);
	if(iResult!=0){
		printf("getaddrinfo failed: %d\n",iResult);
		WSACleanup();
		return 1;
	}
	SOCKET ListenSocket = INVALID_SOCKET;
	ListenSocket=socket(result->ai_family,result->ai_socktype,result->ai_protocol);
	if (ListenSocket == INVALID_SOCKET) {
		printf("Error at socket(): %d\n", WSAGetLastError());
		freeaddrinfo(result);
		WSACleanup();
		return 1;
	}
	iResult=bind(ListenSocket,result->ai_addr,(int)result->ai_addrlen);
	 if (iResult == SOCKET_ERROR) {
        printf("bind failed with error: %d\n", WSAGetLastError());
        freeaddrinfo(result);
        closesocket(ListenSocket);
        WSACleanup();
        return 1;
    }
    freeaddrinfo(result);
    if ( listen( ListenSocket, SOMAXCONN ) == SOCKET_ERROR ) {
		printf( "Listen failed with error: %d\n", WSAGetLastError() );
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
    }

 char Header[4092]="";
 bool HeaderReady=false;
FILE *log=NULL;
bool ok=true;
do{

    SOCKET ClientSocket = INVALID_SOCKET;

	// Accept a client socket
	ClientSocket = accept(ListenSocket, NULL, NULL);
	if (ClientSocket == INVALID_SOCKET) {
		printf("accept failed: %d\n", WSAGetLastError());
		closesocket(ListenSocket);
		WSACleanup();
		return 1;
	}

	#define DEFAULT_BUFLEN 2048

char recvbuf[DEFAULT_BUFLEN];
int  iSendResult;
int recvbuflen = DEFAULT_BUFLEN;
char *msgOK="HTTP/1.1 200 OK\n\
Content-Length: 5\n\
Connection: Close\n\
\n\
READY\n";
char *msgERR="HTTP/1.1 500 SERVER ERROR\n\
Content-Length: 5\n\
Connection: Close\n\
\n\
ERROR\n";
do {

    iResult = recv(ClientSocket, recvbuf, recvbuflen, 0);
    if (iResult > 0) {
//		printf("%s\n---\n",recvbuf);
		char *tok=NULL;
		tok=strtok(recvbuf,"\n");
		int headSize,dataSize,lineSize;
		if(strncmp(tok,"GET REQC",8)==0){
			HeaderReady=false;
			ok=true;
			printf("Connected\n");
			if(log!=NULL){
				fclose(log);
				log=NULL;
			}
		}else if(sscanf(tok,"GET HEAD %d",&headSize)==1){
			if(log!=NULL){
				fclose(log);
				log=NULL;
			}
			ok=true;
			int hs=headSize;
			Header[0]='\0';
			for(int i=0;i<headSize;++i){
				tok=strtok(NULL,"\n");
				if(tok==NULL)break;
				strcat(Header,"\"");
				strcat(Header,tok);
				strcat(Header,"\"");
				if(--hs>0)
				{
					strcat(Header,",");
				}
			}
			if(hs)printf("ERR?");
			HeaderReady=true;
		}else if(sscanf(tok,"GET DATA %d",&dataSize)==1){
			if(!HeaderReady)ok=false;
			if(log==NULL){
				int ti=time(NULL);
				char fname[64];
				sprintf(fname,"%d.csv",ti);
				log=fopen(fname,"w");
				printf("created:%s\n%s\n",fname,Header);
				fprintf(log,"%s\n",Header);
			}
			for(int i=0;i<dataSize;++i)
			{
				tok=strtok(NULL,"\n");
				if(sscanf(tok,"LINE %d",&lineSize)!=1){
					break;
				}else
				{
					char Line[4092]="";
					int ls=lineSize;
					for(int j=0;j<lineSize;++j)
					{
						tok=strtok(NULL,"\n");
						strcat(Line,tok);
						if(--ls>0)strcat(Line," , ");
					}
					//printf("%s\n",Line);
					fprintf(log,"%s\n",Line);
				}
			}
			tok=strtok(NULL,"\n");
			if(strncmp(tok,"END",3)==0){
				fclose(log);
				log=NULL;
				printf("END\n");
			}

		}else{
		printf("DM");
		}
    } else if (iResult == 0){
    }else {
        printf("recv failed: %d\n", WSAGetLastError());
        closesocket(ClientSocket);
        WSACleanup();
        return 1;
    }

	iSendResult=send(ClientSocket,ok?msgOK:msgERR,200,0);
} while (iResult>0);
}while(1);
	return 0;
}
