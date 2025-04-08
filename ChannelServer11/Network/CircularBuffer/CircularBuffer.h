#pragma once
#include <mutex>
#include <cstdint>
#include <windows.h>

class CircularBuffer {
public:
	CircularBuffer(uint32_t bufferSize) : bufferSize(bufferSize), buffer(new char[bufferSize]) {}
	~CircularBuffer() {
		delete[] buffer;
	}

	// Write Data
	bool Write(const char* data, uint32_t size_);

	// Read Data
	bool Read(char* readData_, uint32_t size_);

	uint32_t DataSize() const;

private:
	// 8 bytes
	char* buffer;
	const uint32_t bufferSize;
	uint32_t writePos = 0; // Current Write Position
	uint32_t readPos = 0;  // Current Read Position
	uint32_t currentSize = 0; // Current Buffer Size

	// 80 bytes
	std::mutex bufferMutex;
};

