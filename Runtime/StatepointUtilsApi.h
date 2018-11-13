/*
* StatepointUtilsApi.h
* https://github.com/kavon/llvm-statepoint-utils/blob/f810b5641b7a40a1c658be09b2f98e30b5d0e1f7/src/include/api.h
* Modified from original
*/

#pragma once
#include <stdio.h>
#include <stdint.h>
#include <stdbool.h>
#include <vector>
#include <memory>
#include <unordered_map>
#include "Runtime.h"

/**** Types ****/

typedef struct
{
	// kind < 0 means this is a base pointer
	// kind >= 0 means this is a pointer derived from base pointer in slot number "kind"
	int32_t kind;

	// offsets are relative to the base of a frame. 
	// See Figure 1 below for our defintion of "base"
	int32_t offset;
} pointer_slot_t;

/*
FIGURE 1
Stack grows towards low addresses.
high addresses
... ETC ...
-------------- <- base2
frame 2's ret addr
-------------- <- start of frame 2 (computed with: base1 + base1's frameSize)
frame 1's contents
-------------- <- base1, aka base for offsets into frame 1 (8 bytes above start of frame 1)
frame 1's ret addr
-------------- <- start of frame 1 (what you get immediately after a callq)
low addresses
*/

typedef struct
{
	// NOTE flags & calling convention didn't seem useful to include in the map.
	uint64_t retAddr;
	uint64_t frameSize;     // in bytes

									// all base pointers come before derived pointers in the slot array. you can use this
									// fact to quickly update the derived pointers by referring back to the base pointers
									// while scanning the slots.
	uint16_t numSlots;
	std::vector<pointer_slot_t> slots;
} frame_info_t;


typedef struct
{
	uint16_t numEntries;
	size_t sizeOfEntries; // total memory footprint of the entries
	frame_info_t* entries;
} table_bucket_t;

typedef struct
{
	uint64_t size;
	table_bucket_t* buckets;
} statepoint_table_t;

typedef std::unique_ptr<std::unordered_map<uint64_t, std::unique_ptr<frame_info_t>>> StackMapTable;
StackMapTable generate_table(BYTE* map);
void DumpFrame(frame_info_t* frame);