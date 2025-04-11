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
		objSkt = WSASocket(AF_INET, SOCK_STREAM, IPPROTO_IP, NULL, 0, WSA_FLAG_OVERLAPPED);

		if (objSkt == INVALID_SOCKET) {
			std::cout << "Client socket Error : " << GetLastError() << std::endl;
		}

		auto tIOCPHandle = CreateIoCompletionPort((HANDLE)objSkt, sIOCPHandle_, (ULONG_PTR)0, 0);

		if (tIOCPHandle == INVALID_HANDLE_VALUE)
		{
			std::cout << "reateIoCompletionPort()함수 실패 :" << GetLastError() << std::endl;
		}


		circularBuffer = std::make_unique<CircularBuffer>(bufferSize_);
	}
	~ConnServer() {
		shutdown(objSkt, SD_BOTH);
		closesocket(objSkt);
	}

public:
	SOCKET& GetSocket() {
		return objSkt;
	}

	bool WriteRecvData(const char* data_, uint32_t size_) {
		return circularBuffer->Write(data_, size_);
	}

	PacketInfo ReadRecvData(char* readData_, uint32_t size_) { // readData_는 값을 불러오기 위한 빈 값
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

	bool ServerRecv() {
		OverlappedEx* tempOvLap = (overLappedManager->getOvLap());

		if (tempOvLap == nullptr) { // 오버랩 풀에 여분 없으면 새로 오버랩 생성
			OverlappedEx* overlappedEx = new OverlappedEx;
			ZeroMemory(overlappedEx, sizeof(OverlappedEx));
			overlappedEx->wsaBuf.len = MAX_RECV_SIZE;
			overlappedEx->wsaBuf.buf = new char[MAX_RECV_SIZE];
			overlappedEx->taskType = TaskType::NEWSEND;
		}
		else {
			tempOvLap->wsaBuf.len = MAX_RECV_SIZE;
			tempOvLap->wsaBuf.buf = new char[MAX_RECV_SIZE];
			tempOvLap->taskType = TaskType::RECV;
		}

		DWORD dwFlag = 0;
		DWORD dwRecvBytes = 0;

		int tempR = WSARecv(objSkt, &(tempOvLap->wsaBuf), 1, &dwRecvBytes, &dwFlag, (LPWSAOVERLAPPED)tempOvLap, NULL);

		if (tempR == SOCKET_ERROR && (WSAGetLastError() != ERROR_IO_PENDING))
		{
			std::cout << " WSARecv() Fail : " << WSAGetLastError() << std::endl;
			return false;
		}

		return true;
	}

	void PushSendMsg(const uint32_t dataSize_, char* sendMsg) {

		OverlappedEx* tempOvLap = overLappedManager->getOvLap();

		if (tempOvLap == nullptr) { // 오버랩 풀에 여분 없으면 새로 오버랩 생성
			OverlappedEx* overlappedEx = new OverlappedEx;
			ZeroMemory(overlappedEx, sizeof(OverlappedEx));
			overlappedEx->wsaBuf.len = MAX_RECV_SIZE;
			overlappedEx->wsaBuf.buf = new char[MAX_RECV_SIZE];
			CopyMemory(overlappedEx->wsaBuf.buf, sendMsg, dataSize_);
			overlappedEx->taskType = TaskType::NEWSEND;

			sendQueue.push(overlappedEx); // Push Send Msg To User
			sendQueueSize.fetch_add(1);
		}
		else {
			tempOvLap->wsaBuf.len = MAX_RECV_SIZE;
			tempOvLap->wsaBuf.buf = new char[MAX_RECV_SIZE];
			CopyMemory(tempOvLap->wsaBuf.buf, sendMsg, dataSize_);
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
			int sCheck = WSASend(objSkt,
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

	// 8 bytes
	SOCKET objSkt;
	HANDLE sIOCPHandle;

	OverLappedManager* overLappedManager;

	// 2 bytes
	uint16_t connObjNum;

	// 1 bytes
	std::atomic<uint16_t> sendQueueSize{ 0 };
};