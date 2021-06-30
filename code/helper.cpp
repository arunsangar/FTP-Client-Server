#include "helper.h"

//return minimum of two integers
int min(const int& fInt, const int& sInt) {
	if(fInt < sInt) return fInt;
	return sInt;
}

//return a the size of a file
int getFileSize(const char* fileName) {
	struct stat fileInfo;
	if(stat(fileName, &fileInfo) < 0) return -1;
	return fileInfo.st_size;
}