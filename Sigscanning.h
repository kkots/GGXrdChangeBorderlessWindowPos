#pragma once
#include <vector>

// byteSpecification is of the format "00 8f 1e ??". ?? means unknown byte.
// Converts a "00 8f 1e ??" string into two vectors:
// sig vector will contain bytes '00 8f 1e' for the first 3 bytes and 00 for every ?? byte.
// sig vector will be terminated with an extra 0 byte.
// mask vector will contain an 'x' character for every non-?? byte and a '?' character for every ?? byte.
// mask vector will be terminated with an extra 0 byte.
// Can additionally provide an size_t* position argument. If the byteSpecification contains a ">" character, position will store the offset of that byte.
// If multiple ">" characters are present, position must be an array able to hold all positions, and positionLength specifies the length of the array.
// If positionLength is 0, it is assumed the array is large enough to hold all > positions.
// Returns the number of > characters.
// How sigscan cache works is described in memoryFunctions.h at the top of the file
size_t byteSpecificationToSigMask(const char* byteSpecification, std::vector<char>& sig, std::vector<char>& mask, size_t* position = nullptr, size_t positionLength = 0);

BYTE* sigscan(BYTE* start, BYTE* end, const char* sig, size_t sigLength, const char* logname = nullptr);

BYTE* sigscan(BYTE* start, BYTE* end, const char* sig, const char* mask, const char* logname = nullptr);

BYTE* sigscanBackwards(BYTE* startBottom, BYTE* endTop, const char* sig, const char* mask);

// for finding function starts
BYTE* sigscanBackwards16ByteAligned(BYTE* startBottom, BYTE* endTop, const char* sig, const char* mask);

/// <param name="byteSpecification">Example: "80 f0 c7 ?? ?? ?? ?? e8". If contains only one > character, will return that specific byte instead</param>
BYTE* sigscanOffset(BYTE* start, BYTE* end, const char* byteSpecification, bool* error, const char* logname, size_t* position = nullptr);

BYTE* sigscanOffsetMain(BYTE* start, BYTE* end, const char* sig, const size_t sigLength, const char* mask = nullptr, const std::initializer_list<int>& offsets = std::initializer_list<int>{}, bool* error = nullptr, const char* logname = nullptr);

// If byteSpecification contains only one > character, returned uintptr_t will point to that byte if found and be 0 if the signature is not found
BYTE* sigscanBackwards16ByteAligned(BYTE*, const char* byteSpecification, size_t searchLimit = 1000, size_t* position = nullptr);
