/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#include "strstack.hxx"

class MERGE {
public:
	void AddChunk(PCHR MemoryData, INT MemoryDataLength,
		GPTYPE *MemoryIndex, INT MemoryIndexLength,
		GPTYPE GlobalStart);
/*	void AppendChunk(MERGEFP mfp, INT MemoryDataLength,
		GPTYPE *MemoryIndex, INT MemoryIndexLength,
		GPTYPE GlobalStart);
	void FinishRun(MERGEFP mfp);
*/

private:
	STRSTACK MergeFiles;
	void hsort(void *data, size_t nel, size_t width,
		int (*compar) (const void *, const void *));
	void buildHeap(void *data, size_t nel, size_t width,
		int (*compar) (const void *, const void * ), int position, int reverse=0);
	void buildGpHeap(GPTYPE *data, size_t heapsize, 
		int (*compar)(const void *, const void *), int position, int reverse);
	void GpHsort(GPTYPE *data, size_t nel, int (*compar) (const void *, const void *));
	void printint(int *data, int nel);
};
