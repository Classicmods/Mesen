#include "stdafx.h"
#include "UPnPPortMapper.h"
#include "Socket.h"

#ifdef WIN32
#pragma comment(lib,"ws2_32.lib") //Winsock Library
#define WIN32_LEAN_AND_MEAN
#include <winsock2.h>
#include <Windows.h>

Socket::Socket()
{
	WSADATA wsaDat;
	if(WSAStartup(MAKEWORD(2, 2), &wsaDat) != 0) {
		std::cout << "WSAStartup failed." << std::endl;
		SetConnectionErrorFlag();
	} else {
		_cleanupWSA = true;

		_socket = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
		if(_socket == INVALID_SOCKET) {
			std::cout << "Socket creation failed." << std::endl;
			SetConnectionErrorFlag();
		} else {
			SetSocketOptions();
		}
	}

	_sendBuffer = new char[200000];
	_bufferPosition = 0;
}

Socket::Socket(uintptr_t socket) 
{
	_socket = socket;

	if(socket == INVALID_SOCKET) {
		SetConnectionErrorFlag();
	} else {
		SetSocketOptions();
	}

	_sendBuffer = new char[200000];
	_bufferPosition = 0;
}

Socket::~Socket()
{
	if(_UPnPPort != -1) {
		UPnPPortMapper::RemoveNATPortMapping(_UPnPPort, IPProtocol::TCP);
	}

	if(_socket != INVALID_SOCKET) {
		Close();
	}
	if(_cleanupWSA) {
		WSACleanup();
	}

	delete[] _sendBuffer;
}

void Socket::SetSocketOptions()
{
	//Non-blocking mode
	u_long iMode = 1;
	ioctlsocket(_socket, FIONBIO, &iMode);
		
	//Set send/recv buffers to 256k
	int bufferSize = 0x40000;
	setsockopt(_socket, SOL_SOCKET, SO_RCVBUF, (char*)&bufferSize, sizeof(int));
	setsockopt(_socket, SOL_SOCKET, SO_SNDBUF, (char*)&bufferSize, sizeof(int));

	//Disable nagle's algorithm to improve latency
	u_long value = 1;
	setsockopt(_socket, IPPROTO_TCP, TCP_NODELAY, (char*)&value, sizeof(value));
}

void Socket::SetConnectionErrorFlag()
{
	_connectionError = true;
}

void Socket::Close()
{
	std::cout << "Socket closed." << std::endl;
	shutdown(_socket, SD_SEND);
	closesocket(_socket);
	SetConnectionErrorFlag();
}

bool Socket::ConnectionError()
{
	return _connectionError;
}

void Socket::Bind(uint16_t port)
{
	SOCKADDR_IN serverInf;
	serverInf.sin_family = AF_INET;
	serverInf.sin_addr.s_addr = INADDR_ANY;
	serverInf.sin_port = htons(port);

	if(UPnPPortMapper::AddNATPortMapping(port, port, IPProtocol::TCP)) {
		_UPnPPort = port;
	}

	if(bind(_socket, (SOCKADDR*)(&serverInf), sizeof(serverInf)) == SOCKET_ERROR) {
		std::cout << "Unable to bind socket." << std::endl;
		SetConnectionErrorFlag();
	}
}

bool Socket::Connect(const char* hostname, uint16_t port)
{
	// Resolve IP address for hostname
	struct hostent *host;
	if((host = gethostbyname(hostname)) == NULL) {
		std::cout << "Failed to resolve hostname." << std::endl;
		SetConnectionErrorFlag();
	} else {
		// Setup our socket address structure
		SOCKADDR_IN SockAddr;
		SockAddr.sin_port = htons(port);
		SockAddr.sin_family = AF_INET;
		SockAddr.sin_addr.s_addr = *((unsigned long*)host->h_addr);

		u_long iMode = 0;
		ioctlsocket(_socket, FIONBIO, &iMode);

		// Attempt to connect to server
		if(connect(_socket, (SOCKADDR*)(&SockAddr), sizeof(SockAddr)) == SOCKET_ERROR) {
			std::cout << "Failed to establish connection with server." << std::endl;
			SetConnectionErrorFlag();
		} else {
			iMode = 1;
			ioctlsocket(_socket, FIONBIO, &iMode);

			return true;
		}
	}

	return false;
}

void Socket::Listen(int backlog)
{
	if(listen(_socket, backlog) == SOCKET_ERROR) {
		std::cout << "listen failed." << std::endl;
		SetConnectionErrorFlag();
	}
}

shared_ptr<Socket> Socket::Accept()
{
	SOCKET socket = accept(_socket, nullptr, nullptr);
	return shared_ptr<Socket>(new Socket(socket));
}

int Socket::Send(char *buf, int len, int flags)
{
	int retryCount = 15;
	int nError = 0;
	int returnVal;
	do {
		//Loop until everything has been sent (shouldn't loop at all in the vast majority of cases)
		if(nError == WSAEWOULDBLOCK) {
			retryCount--;
			if(retryCount == 0) {
				//Connection seems dead, close it.
				std::cout << "Unable to send data, closing socket." << std::endl;
				Close();
				return 0;
			}
		}

		returnVal = send(_socket, buf, len, flags);

		nError = WSAGetLastError();
		if(nError != 0) {
			if(nError != WSAEWOULDBLOCK) {
				SetConnectionErrorFlag();
			} else {
				if(returnVal > 0) {
					//Sent partial data, adjust pointer & length
					buf += returnVal;
					len -= returnVal;
				}
			}
		}
	} while(nError == WSAEWOULDBLOCK);
		
	return returnVal;
}

void Socket::BufferedSend(char *buf, int len)
{
	memcpy(_sendBuffer+_bufferPosition, buf, len);
	_bufferPosition += len;
}

void Socket::SendBuffer()
{
	Send(_sendBuffer, _bufferPosition, 0);
	_bufferPosition = 0;
}

int Socket::Recv(char *buf, int len, int flags)
{
	int returnVal = recv(_socket, buf, len, flags);
				
	int nError = WSAGetLastError();
	if(nError != WSAEWOULDBLOCK && nError != 0) {
		SetConnectionErrorFlag();
	}

	if(returnVal == 0) {
		//Socket closed
		std::cout << "Socket closed by peer." << std::endl;
		Close();
	}

	return returnVal;
}

#else

Socket::Socket()
{
	
}

Socket::Socket(uintptr_t socket) 
{
	
}

Socket::~Socket()
{
	
}

void Socket::SetSocketOptions()
{
	
}

void Socket::SetConnectionErrorFlag()
{
	
}

void Socket::Close()
{
	
}

bool Socket::ConnectionError()
{
	return true;
}

void Socket::Bind(uint16_t port)
{

}

bool Socket::Connect(const char* hostname, uint16_t port)
{
	return false;
}

void Socket::Listen(int backlog)
{

}

shared_ptr<Socket> Socket::Accept()
{
	return shared_ptr<Socket>(new Socket());
}

int Socket::Send(char *buf, int len, int flags)
{
	return len;
}

void Socket::BufferedSend(char *buf, int len)
{

}

void Socket::SendBuffer()
{

}

int Socket::Recv(char *buf, int len, int flags)
{
	return len;
}


#endif
