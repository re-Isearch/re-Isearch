/*
Copyright (c) 2020-21 Project re-Isearch and its contributors: See CONTRIBUTORS.
It is made available and licensed under the Apache 2.0 license: see LICENSE
*/
#pragma ident  "@(#)merge.cxx  1.3 06/27/00 20:30:54 BSN"

/************************************************************************
************************************************************************/

/*@@@
File:		merge.cxx
Version:	1.00
Description:	Class IDB
Author:		Jon Magid, jem@cnidr.org
@@@*/

#include <memory.h>
#include <stdlib.h>
#include <stdio.h>
#include "common.hxx"
#include "merge.hxx"

void MERGE::printint(int *data, int nel) {
	for (int i = 0; i < nel; i++)
		printf("%d ", *(data + i));
	puts("");
}

void MERGE::buildGpHeap(GPTYPE *data, size_t heapsize, 
	int (*compar)(const void *, const void *), int position, int reverse) {

	int childpos;
	GPTYPE value;

	value = data[position];

	while (position < heapsize) {
		childpos = position * 2 + 1;
		if (childpos < heapsize) {
			if ((childpos < heapsize) && 
				( ((*compar)( ((void *)(data + childpos + 1)), 
					((void *)(data + childpos)))) == 1 ))
						childpos++;
			if ( ((*compar)((void *)&value, (void *)(data + childpos))) > 0) {
				data[position] = value;
				return;
			}
			else {
				data[position] = data[childpos];
				position = childpos;
			}
		}
		else {
			data[position] = value;
			return;
		}
	}
}

void MERGE::buildHeap(void *data, size_t heapsize, size_t width,  
	int (*compar) (const void *, const void *), int position, int reverse) {

	int childpos, cmpstatus; 
	void *value;

	value = (void *)malloc(width);
	memcpy(value, ((char *)data + (width * position) ), width);

	while (position < heapsize) {
		childpos = position * 2 + 1;

		//if there is a child...
		if (childpos < heapsize) {

			//if there is another child
			if (childpos + 1 < heapsize) {
				
				//make childpos equal to the greatest child
				//(unless we're reversed)
				cmpstatus =  (*compar)( ((char *)data + (width * (childpos + 1))), 
					((char *)data + (width * childpos)) );

				if  ( cmpstatus > 0 && !reverse)
					childpos++;
				else if (cmpstatus < 0 && reverse)
					childpos++;
			
			}
			
			//is unlocated value larger than the largest child?
			cmpstatus = ( (*compar)( value, ((char *)data + (width * childpos) )) );

			//unless we're reversed (smaller than the smallest child)
			if (reverse) 
				cmpstatus = 0 - cmpstatus;

			//value is bigger, we're done.
			if (cmpstatus > 0) {
					memcpy( ((char *)data + (position * width) ), value, width);
					free(value);
					return;
			} //child is bigger or same size, back through the loop 
			//again to find a place for value
			else {
					memcpy( ((char *)data + (position * width) ), 
						((char *)data + (childpos * width) ), width);
					position=childpos;
			}
		}

		//node has no child. loop is finished
		else {
			memcpy(((char *)data+ (position * width) ), value, width);
			free(value);
			return;
		}
	}
}

void MERGE::GpHsort(GPTYPE *data, size_t nel, int (*compar) 
	(const void *, const void *)) {

	int i, tmp;
	for (i = nel/2; i > 0; i--)
		buildGpHeap(data, nel, compar, i, 0);
	
	for (i = nel - 1; i > 0; i--) {
		tmp = data[i];
		data[i] = data[0];
		data[0] = tmp;
		buildGpHeap(data, i, compar, 0, 0);
	}
}

void MERGE::hsort(void *data, size_t nel, size_t width, 
	int (*compar) (const void *, const void *)) {

	int i;
	void *tmp;
	for (i = nel/2; i >=0; i--) 
		buildHeap(data, nel, width, compar, i, 0);

/*	FILE *fp = fopen("heap.dbg","wb");
	if (!fp) {
		perror("heap.dbg");
		exit(1);
	}
	fwrite(data, width, nel, fp);
	fclose(fp);
*/

	tmp = malloc(width);
	for (i = nel - 1; i > 0; i--) {
		memcpy(tmp, ((char *)data + (i * width)), width);
		memcpy(((char *)data + (i * width)), data, width);
		memcpy(data, tmp, width);
		buildHeap(data, i, width, compar, 0, 0);
	}
	free(tmp);
}

