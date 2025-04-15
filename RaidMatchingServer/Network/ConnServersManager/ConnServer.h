#pragma once

#include "Define.h"
#include "CircularBuffer.h"
#include "Packet.h"
#include "overLappedManager.h"

#include <cstdint>
#include <iostream>
#include <atomic>
#include <boost/lockfree/queue.hpp>

class ConnServer {
public:
	ConnServer(uint32_t bufferSize_, uint16_t connObjNum_, HANDLE sIOCPHandle_, OverLappedManager* overLappedManager_)
		:connObjNum(connObjNum_),  sIOCPHandle(sIOCPHandle_), overLappedManager(overLappedManager_) {
		serverSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);
		if (serverSkt == INVALID_SOCKET) {
			std::cout << "Client socket Error : " << GetLastError() << std::endl;
		}

		auto tIOCPHandle = CreateIoCompletionPort((HANDLE)serverSkt, sIOCPHandle_, (ULONG_PTR)0, 0);
		if (tIOCPHandle == INVALID_HANDLE_VALUE)
		{
			std::cout << "reateIoCompletionPort Fail :" << GetLastError() << std::endl;
		}

		circularBuffer = std::make_unique<CircularBuffer>(bufferSize_);
	}
	~ConnServer() {
		shutdown(serverSkt, SD_BOTH);
		closesocket(serverSkt);
	}

public:
	SOCKET& GetSocket() {
		return serverSkt;
	}

	bool WriteRecvData(const char* data_, uint32_t size_) { // Set recvdata in circular buffer 
		return circularBuffer->Write(data_, size_);
	}

	PacketInfo ReadRecvData(char* readData_, uint32_t size_) { // Get recvdata in circular buffer 
		CopyMemory(readData, readData_, size_);

		if (circularBuffer->Read(readData, size_)) {
			auto pHeader = (PACKET_HEADER*)readData;

			PacketInfo packetInfo;
			packetInfo.packetId = pHeader->PacketId;
			packetInfo.dataSize = pHeader->PacketLength;
			packetInfo.connObjNum = connObjNum;
			packetInfo.pData = readData;

			return packetInfo;
		}
	}

	void Reset() { // Reset connuser object socket
		shutdown(serverSkt, SD_BOTH);
		closesocket(serverSkt);

		serverSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (serverSkt == INVALID_SOCKET) {
			std::cout << "Client socket Error : " << GetLastError() << std::endl;
		}

		auto tIOCPHandle = CreateIoCompletionPort((HANDLE)serverSkt, sIOCPHandle, (ULONG_PTR)0, 0);

		if (tIOCPHandle == INVALID_HANDLE_VALUE)
		{
			std::cout << "reateIoCompletionPort Fail :" << GetLastError() << std::endl;
		}

	}

	bool PostAccept(SOCKET ServerSkt_) {
		acceptOvlap = {};

		acceptOvlap.taskType = TaskType::ACCEPT;
		acceptOvlap.connObjNum = connObjNum;
		acceptOvlap.wsaBuf.buf = nullptr;
		acceptOvlap.wsaBuf.len = 0;

		DWORD bytes = 0;
		DWORD flags = 0;

		if (AcceptEx(ServerSkt_, serverSkt, acceptBuf, 0, sizeof(SOCKADDR_IN) + 16, sizeof(SOCKADDR_IN) + 16, &bytes, (LPWSAOVERLAPPED)&acceptOvlap) == 0) {
			if (WSAGetLastError() != WSA_IO_PENDING) {
				std::cout << "AcceptEx Error : " << GetLastError() << std::endl;
				return false;
			}
		}

		return true;
	}

	bool ServerRecv() {
		OverlappedEx* tempOvLap = (overLappedManager->getOvLap());

		if (tempOvLap == nullptr) { // Allocate new overlap if pool is empty
			OverlappedEx* overlappedEx = new OverlappedEx;
			ZeroMemory(overlappedEx, sizeof(OverlappedEx));
			overlappedEx->wsaBuf.len = MAX_RECV_SIZE;
			overlappedEx->wsaBuf.buf = new char[MAX_RECV_SIZE];
			overlappedEx->connObjNum = connObjNum;
			overlappedEx->taskType = TaskType::NEWSEND;
		}
		else {
			tempOvLap->wsaBuf.len = MAX_RECV_SIZE;
			tempOvLap->wsaBuf.buf = new char[MAX_RECV_SIZE];
			tempOvLap->connObjNum = connObjNum;
			tempOvLap->taskType = TaskType::RECV;
		}

		DWORD dwFlag = 0;
		DWORD dwRecvBytes = 0;

		int tempR = WSARecv(serverSkt, &(tempOvLap->wsaBuf), 1, &dwRecvBytes, &dwFlag, (LPWSAOVERLAPPED)tempOvLap, NULL);
		if (tempR == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cout << " WSARecv() Fail : " << WSAGetLastError() << std::endl;
			return false;
		}

		return true;
	}

	void PushSendMsg(const uint32_t dataSize_, char* sendMsg) {
		OverlappedEx* tempOvLap = overLappedManager->getOvLap();

		if (tempOvLap == nullptr) { // Allocate new overlap if pool is empty
			OverlappedEx* overlappedEx = new OverlappedEx;
			ZeroMemory(overlappedEx, sizeof(OverlappedEx));
			overlappedEx->wsaBuf.len = MAX_RECV_SIZE;
			overlappedEx->wsaBuf.buf = new char[MAX_RECV_SIZE];
			CopyMemory(overlappedEx->wsaBuf.buf, sendMsg, dataSize_);
			overlappedEx->connObjNum = connObjNum;
			overlappedEx->taskType = TaskType::NEWSEND;

			sendQueue.push(overlappedEx); // Push Send Msg To User
			sendQueueSize.fetch_add(1);

			std::cout << "PushSendMsg ObjNum : " << connObjNum << std::endl;
		}
		else {
			tempOvLap->wsaBuf.len = MAX_RECV_SIZE;
			tempOvLap->wsaBuf.buf = new char[MAX_RECV_SIZE];
			CopyMemory(tempOvLap->wsaBuf.buf, sendMsg, dataSize_);
			tempOvLap->connObjNum = connObjNum;
			tempOvLap->taskType = TaskType::SEND;

			sendQueue.push(tempOvLap); // Push Send Msg To User
			sendQueueSize.fetch_add(1);
		}

		if (sendQueueSize.load() == 1) {
			ProcSend();
		}
	}

	void SendComplete() {
		sendQueueSize.fetch_sub(1);

		if (sendQueueSize.load() == 1) {
			ProcSend();
		}
	}

private:
	void ProcSend() {
		OverlappedEx* overlappedEx;

		if (sendQueue.pop(overlappedEx)) {
			DWORD dwSendBytes = 0;
			int sCheck = WSASend(serverSkt,
				&(overlappedEx->wsaBuf),
				1,
				&dwSendBytes,
				0,
				(LPWSAOVERLAPPED)overlappedEx,
				NULL);
		}
	}

	// 1024 bytes
	char readData[1024] = { 0 };

	// 136 bytes 
	boost::lockfree::queue<OverlappedEx*> sendQueue{ 10 };

	// 120 bytes
	std::unique_ptr<CircularBuffer> circularBuffer;

	// 64 bytes
	char acceptBuf[64] = { 0 };

	// 56 bytes
	OverlappedEx acceptOvlap;

	// 8 bytes
	SOCKET serverSkt;
	HANDLE sIOCPHandle;

	OverLappedManager* overLappedManager;

	// 2 bytes
	uint16_t connObjNum;

	// 1 bytes
	std::atomic<uint16_t> sendQueueSize{ 0 };
};