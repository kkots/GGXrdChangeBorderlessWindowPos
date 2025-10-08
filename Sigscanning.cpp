#include "framework.h"
#include "Sigscanning.h"

static int findChar(const char* str, char searchChar) {
	for (const char* c = str; *c != '\0'; ++c) {
		if (*c == searchChar) return (int)(uintptr_t)(c - str);
	}
	return -1;
}

#define byteSpecificationError \
	return numOfTriangularChars;

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
size_t byteSpecificationToSigMask(const char* byteSpecification, std::vector<char>& sig, std::vector<char>& mask, size_t* position, size_t positionLength) {
	if (position && positionLength == 0) positionLength = UINT_MAX;
	size_t numOfTriangularChars = 0;
	sig.clear();
	mask.clear();
	unsigned long long accumulatedNibbles = 0;
	int nibbleCount = 0;
	bool nibblesUnknown = false;
	const char* byteSpecificationPtr = byteSpecification;
	bool nibbleError = false;
	const char* nibbleSequenceStart = byteSpecification;
	while (true) {
		char currentChar = *byteSpecificationPtr;
		if (currentChar == '>') {
			if (position && numOfTriangularChars < positionLength) {
				*position = sig.size();
				++position;
			}
			++numOfTriangularChars;
			nibbleSequenceStart = byteSpecificationPtr + 1;
		} else if (currentChar == '(') {
			nibbleCount = 0;
			nibbleError = false;
			nibblesUnknown = false;
			accumulatedNibbles = 0;
			if (byteSpecificationPtr <= nibbleSequenceStart) {
				byteSpecificationError
			}
			const char* moduleNameEnd = byteSpecificationPtr;
			++byteSpecificationPtr;
			bool parseOk = true;
			#define skipWhitespace \
				while (*byteSpecificationPtr != '\0' && *byteSpecificationPtr <= 32) { \
					++byteSpecificationPtr; \
				}
			#define checkQuestionMarks \
				if (parseOk) { \
					if (strncmp(byteSpecificationPtr, "??", 2) != 0) { \
						parseOk = false; \
					} else { \
						byteSpecificationPtr += 2; \
					} \
				}
			#define checkWhitespace \
				if (parseOk) { \
					if (*byteSpecificationPtr == '\0' || *byteSpecificationPtr > 32) { \
						parseOk = false; \
					} else { \
						while (*byteSpecificationPtr != '\0' && *byteSpecificationPtr <= 32) { \
							++byteSpecificationPtr; \
						} \
					} \
				}
			skipWhitespace
			checkQuestionMarks
			checkWhitespace
			checkQuestionMarks
			checkWhitespace
			checkQuestionMarks
			checkWhitespace
			checkQuestionMarks
			skipWhitespace
			#undef skipWhitespace
			#undef checkQuestionMarks
			#undef checkWhitespace
			if (*byteSpecificationPtr != ')') {
				parseOk = false;
			}
			if (!parseOk) {
				byteSpecificationError
			}
		} else if (currentChar != ' ' && currentChar != '\0') {
			char currentNibble = 0;
			if (currentChar >= '0' && currentChar <= '9' && !nibblesUnknown) {
				currentNibble = currentChar - '0';
			} else if (currentChar >= 'a' && currentChar <= 'f' && !nibblesUnknown) {
				currentNibble = currentChar - 'a' + 10;
			} else if (currentChar >= 'A' && currentChar <= 'F' && !nibblesUnknown) {
				currentNibble = currentChar - 'A' + 10;
			} else if (currentChar == '?' && (nibbleCount == 0 || nibblesUnknown)) {
				nibblesUnknown = true;
			} else {
				nibbleError = true;
			}
			accumulatedNibbles = (accumulatedNibbles << 4) | currentNibble;
			++nibbleCount;
			if (nibbleCount > 16) {
				nibbleError = true;
			}
		} else {
			if (nibbleCount) {
				if (nibbleError) {
					byteSpecificationError
				}
				do {
					if (!nibblesUnknown) {
						sig.push_back(accumulatedNibbles & 0xff);
						mask.push_back('x');
						accumulatedNibbles >>= 8;
					} else {
						sig.push_back(0);
						mask.push_back('?');
					}
					nibbleCount -= 2;
				} while (nibbleCount > 0);
				nibbleCount = 0;
				nibblesUnknown = false;
			}
			if (currentChar == '\0') {
				break;
			}
			nibbleSequenceStart = byteSpecificationPtr + 1;
		}
		++byteSpecificationPtr;
	}
	sig.push_back('\0');
	mask.push_back('\0');
	#undef byteSpecificationError
	return numOfTriangularChars;
}

BYTE* sigscan(BYTE* start, BYTE* end, const char* sig, size_t sigLength, const char* logname) {
	(logname);
	
	// Boyer-Moore-Horspool substring search
	// A table containing, for each symbol in the alphabet, the number of characters that can safely be skipped
	size_t step[256];
	for (int i = 0; i < _countof(step); ++i) {
		step[i] = sigLength;
	}
	for (size_t i = 0; i < sigLength - 1; i++) {
		step[(BYTE)sig[i]] = sigLength - 1 - i;
	}
	
	BYTE pNext;
	end -= sigLength;
	for (BYTE* p = start; p <= end; p += step[pNext]) {
		int j = (int)sigLength - 1;
		pNext = *(BYTE*)(p + j);
		if (sig[j] == (char)pNext) {
			for (--j; j >= 0; --j) {
				if (sig[j] != *(char*)(p + j)) {
					break;
				}
			}
			if (j < 0) {
				return p;
			}
		}
	}

	return nullptr;
}

BYTE* sigscan(BYTE* start, BYTE* end, const char* sig, const char* mask, const char* logname) {
	(logname);
	BYTE* lastScan = end - strlen(mask) + 1;
	for (BYTE* addr = start; addr < lastScan; addr++) {
		for (size_t i = 0;; i++) {
			if (mask[i] == '\0')
				return addr;
			if (mask[i] != '?' && sig[i] != *(char*)(addr + i))
				break;
		}
	}

	return nullptr;
}

BYTE* sigscanBackwards(BYTE* startBottom, BYTE* endTop, const char* sig, const char* mask) {
	BYTE* lastScan = endTop;
	for (BYTE* addr = startBottom - strlen(mask) + 1; addr >= lastScan; addr--) {
		for (size_t i = 0;; i++) {
			if (mask[i] == '\0')
				return addr;
			if (mask[i] != '?' && sig[i] != *(char*)(addr + i))
				break;
		}
	}
	return nullptr;
}

// for finding function starts
BYTE* sigscanBackwards16ByteAligned(BYTE* startBottom, BYTE* endTop, const char* sig, const char* mask) {
	BYTE* lastScan = endTop;
	for (
		BYTE* addr = (BYTE*)(
				(uintptr_t)(startBottom - strlen(mask) + 1)
				#if defined( _WIN64 )
				& 0xFFFFFFFFFFFFFFF0ULL
				#else
				& 0xFFFFFFF0
				#endif
			);
		addr >= lastScan;
		addr -= 16
	) {
		for (size_t i = 0;; i++) {
			if (mask[i] == '\0')
				return addr;
			if (mask[i] != '?' && sig[i] != *(char*)(addr + i))
				break;
		}
	}
	return nullptr;
}

BYTE* sigscanOffset(BYTE* start, BYTE* end, const char* byteSpecification, bool* error, const char* logname, size_t* position) {
	std::vector<char> sig;
	std::vector<char> mask;
	size_t positionLength = 0;
	size_t ownPosition = 0;
	if (!position) {
		position = &ownPosition;
		positionLength = 1;
	}
	size_t numOfTriangularChars = byteSpecificationToSigMask(byteSpecification, sig, mask, position, positionLength);
	BYTE* result = sigscanOffsetMain(start, end, sig.data(), 0, mask.data(), {}, error, logname);
	if (numOfTriangularChars == 1 && result) {
		return result + *position;
	} else {
		return result;
	}
}

// Offsets work the following way:
// 1) Empty offsets: just return the sigscan result;
// 2) One offset: add number (positive or negative) to the sigscan result and return that;
// 3) Two offsets:
//    Let's say you're looking for code which mentions a function. You enter the code bytes as the sig to find the code.
//    Then you need to apply an offset from the start of the code to find the place where the function is mentioned.
//    Then you need to read 4 bytes of the function's address to get the function itself and return that.
//    (The second offset would be 0 for this, by the way.)
//    So here are the exact steps this function takes:
//    3.a) Find sigscan (the code in our example);
//    3.b) Add the first offset to sigscan (the offset of the function mention within the code);
//    3.c) Interpret this new position as the start of a 4-byte address which gets read, producing a new address.
//    3.d) The result is another address (the function address). Add the second offset to this address (0 in our example) and return that.
// 4) More than two offsets:
//    4.a) Find sigscan;
//    4.b) Add the first offset to sigscan;
//    4.c) Interpret this new position as the start of a 4-byte address which gets read, producing a new address.
//    4.d) The result is another address. Add the second offset to this address.
//    4.e) Repeat 4.c) and 4.d) for as many offsets as there are left. Return result on the last 4.d).
BYTE* sigscanOffsetMain(BYTE* start, BYTE* end, const char* sig, size_t sigLength, const char* mask, const std::initializer_list<int>& offsets, bool* error, const char* logname) {
	BYTE* sigscanResult;
	if (mask) {
		if (findChar(mask, '?') == -1) {
			sigLength = strlen(mask);
			sigscanResult = sigscan(start, end, sig, sigLength, logname);
		} else {
			sigscanResult = sigscan(start, end, sig, mask, logname);
		}
	} else {
		sigscanResult = sigscan(start, end, sig, sigLength, logname);
	}
	if (!sigscanResult) {
		if (error) *error = true;
		return 0;
	}
	BYTE* addr = sigscanResult;
	bool isFirst = true;
	for (int it : offsets) {
		if (!isFirst) {
			addr = *(BYTE**)addr;
		}
		addr += it;
		isFirst = false;
	}
	return addr;
}

BYTE* sigscanBackwards16ByteAligned(BYTE* ptr, const char* byteSpecification, size_t searchLimit, size_t* position) {
	std::vector<char> sig;
	std::vector<char> mask;
	size_t positionLength = 0;
	size_t ownPosition = 0;
	if (!position) {
		position = &ownPosition;
		positionLength = 1;
	}
	size_t numOfTriangularChars = byteSpecificationToSigMask(byteSpecification, sig, mask, position, positionLength);
	BYTE* result = sigscanBackwards16ByteAligned(ptr, ptr - searchLimit, sig.data(), mask.data());
	if (numOfTriangularChars == 1 && result) {
		return result + *position;
	} else {
		return result;
	}
}
