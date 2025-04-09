#include "CircularBuffer.h"
#include <iostream>

// Write Data
bool CircularBuffer::Write(const char* data, uint32_t size_) {
    std::lock_guard<std::mutex> guard(bufferMutex);

    if ((bufferSize - currentSize) < size_) { // Empty Space
        return false;
    }

    uint32_t endSpace = bufferSize - writePos;

    if (endSpace >= size_) {
        std::memcpy(buffer + writePos, data, size_);
    }

    else {
        std::memcpy(buffer + writePos, data, endSpace);
        std::memcpy(buffer, data + endSpace, size_ - endSpace);
    }

    writePos = (writePos + size_) % bufferSize;
    currentSize += size_;

    return true;
}

// Read Data
bool CircularBuffer::Read(char* readData_, uint32_t size_) {
    std::lock_guard<std::mutex> guard(bufferMutex);

    if (currentSize < size_) {
        return false;
    }

    uint32_t endSpace = bufferSize - readPos;

    if (endSpace >= size_) {
        std::memcpy(readData_, buffer + readPos, size_);
    }
    else {
        std::memcpy(readData_, buffer + readPos, endSpace);
        std::memcpy(readData_ + endSpace, buffer, size_ - endSpace);
    }

    readPos = (readPos + size_) % bufferSize;
    currentSize -= size_;

    return true;
}

uint32_t CircularBuffer::DataSize() const {
    return currentSize;
}