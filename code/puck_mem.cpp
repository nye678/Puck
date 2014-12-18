#include "puck_mem.h"
#include "string.h"
#include <new>

using namespace puck;

#define BlockLength(tagPtr) (*tagPtr & ~0x01)
#define BlockFree(tagPtr) (*tagPtr & 0x01)
#define SetBlockFree(tagPtr) (*tagPtr &= ~0x01)
#define SetBlockAllocated(tagPtr) (*tagPtr |= 0x01)

#define BlockTag(ptr) ((BoundaryTag*)((uintptr_t)ptr - sizeof(BoundaryTag)))
#define PrevBlockTag(ptr) ((BoundaryTag*)((uintptr_t)ptr - (2 * sizeof(BoundaryTag))))
#define BlockEndTag(ptr, size) ((BoundaryTag*)((uintptr_t)ptr + size))
#define NextBlockTag(ptr, size) ((BoundaryTag*)((uintptr_t)ptr + size + sizeof(BoundaryTag)))

#define BlockOverheadSize 2 * sizeof(BoundaryTag) + sizeof(FreeList)

inline static uintptr_t AlignPointerNext(uintptr_t ptr, size_t align)
{
	return (ptr + align - 1) & ~(uintptr_t)(align - 1);
}

BoundaryTagAllocator::BoundaryTagAllocator(void* head, size_t size)
	: _size(0), _allocated(0), _numBlocks(0), _head(0), _tail(0), _freeList(nullptr)
{
	_head = (uintptr_t)head;
	_tail = (uintptr_t)head + size;
	_allocated = 0;

	strcpy((char*)_head, "Mem Block Start");
	strcpy((char*)_tail - 14, "Mem Block End");

	auto headTag = (BoundaryTag*)(_head + 16);
	auto headEndTag = (BoundaryTag*)(_tail - 14 - sizeof(BoundaryTag));
	*headTag = 0;
	*headEndTag = 0;
	SetBlockAllocated(headTag);
	SetBlockAllocated(headEndTag);

	_size = (uintptr_t)headEndTag - (uintptr_t)headTag - 2 * sizeof(BoundaryTag);
	_freeList = (FreeList*)CreateBoundaryTagsFree((uintptr_t)headTag + sizeof(BoundaryTag), _size);		
	_freeList->next = nullptr;
	_numBlocks = 1;
}

void* BoundaryTagAllocator::Alloc(size_t size)
{
	if (!_freeList)
	{
		return nullptr;
	}

	// Size required to be a multiple of 2. This is so the 1 bit 
	// can be used as a flag.
	size_t adjustedSize = (size + 1) & ~(size_t)1;
	size_t totalSize = adjustedSize + BlockOverheadSize;

	if (totalSize > _size - _allocated)
	{
		return nullptr;
	}

	uintptr_t freeBlock = 0;
	FreeList* previous = nullptr;
	for (auto itor = _freeList; itor != nullptr; itor = itor->next)
	{
		auto tagPtr = BlockTag(itor);
		size_t blockLength = BlockLength(tagPtr);
		if (blockLength >= adjustedSize)
		{
			if (previous)
			{
				previous->next = itor->next;
			}
			else
			{
				_freeList = itor->next;
			}

			freeBlock = SplitBlock((uintptr_t)itor, adjustedSize);
			SetBoundaryTagsAlloc(freeBlock, adjustedSize);
			_allocated += adjustedSize;
			break;
		}

		previous = itor;
	}

	if (!freeBlock)
	{
		return nullptr;
	}

	return (void*)freeBlock;
}

uintptr_t BoundaryTagAllocator::SplitBlock(uintptr_t block, size_t size)
{
	auto startTag = BlockTag(block);
	size_t blockLength = BlockLength(startTag);

	// If the spliting the block would produce a tiny block (0-3 bytes) then don't.
	// A block has to at least fit a FreeList pointer in it.
	if (blockLength < size + BlockOverheadSize)
	{
		return block;
	}

	SetBoundaryTagsFree(block, size);

	FreeList* newBlock = (FreeList*)CreateBoundaryTagsFree(
		block + size + sizeof(BoundaryTag),
		blockLength - size - sizeof(BoundaryTag));
	newBlock->next = _freeList;
	_freeList = newBlock;
	_numBlocks++;
	
	return block;
}

void BoundaryTagAllocator::Free(void* ptr)
{
	auto freedBlock = (uintptr_t)ptr;
	auto tagPtr = BlockTag(freedBlock);
	auto blockLength = BlockLength(tagPtr);

	auto prevBlockTag = PrevBlockTag(freedBlock);
	auto nextBlockTag = NextBlockTag(freedBlock, blockLength);

	if (BlockFree(prevBlockTag) && BlockFree(nextBlockTag))
	{
		// Previous block is already in free list, but we will need to remove entry for next.
		auto prevBlockLength = BlockLength(prevBlockTag);
		auto prevBlock = (uintptr_t)prevBlockTag - prevBlockLength;

		auto nextBlockLength = BlockLength(nextBlockTag);
		auto nextBlock = (uintptr_t)nextBlockTag + sizeof(BoundaryTag);

		// Need to remove the entry from the freelist for the next block.
		FreeList *prev = nullptr, *itor = _freeList;
		for (itor; itor != nullptr; itor = itor->next)
		{
			if (itor == (FreeList*)nextBlock)
				break;
			prev = itor;
		}

		if (prev)
			prev->next = itor->next;
		else
			_freeList->next = itor->next; 

		SetBoundaryTagsFree(prevBlock, prevBlockLength + blockLength + nextBlockLength + 4 * sizeof(BoundaryTag));
		_numBlocks -= 2;
	}
	else if (BlockFree(prevBlockTag))
	{
		// Previous block is already in free list, no need to mess with free list.
		auto prevBlockLength = BlockLength(prevBlockTag);
		auto prevBlock = (uintptr_t)prevBlockTag + prevBlockLength;
		SetBoundaryTagsFree(prevBlock, prevBlockLength + blockLength + 2 * sizeof(BoundaryTag));
		_numBlocks--;
	}
	else if (BlockFree(nextBlockTag))
	{
		auto nextBlock = (uintptr_t)nextBlockTag + sizeof(BoundaryTag);

		// Need to move free list pointer to point to the beginning of the freed block.
		FreeList *prev = nullptr, *itor = _freeList;
		for (itor; itor != nullptr; itor = itor->next)
		{
			if (itor == (FreeList*)nextBlock)
				break;
			prev = itor;
		}

		auto nextBlockLength = BlockLength(nextBlockTag);
		SetBoundaryTagsFree(freedBlock, blockLength + nextBlockLength + 2 * sizeof(BoundaryTag));
		_numBlocks--;

		if (!prev)
		{

			_freeList = (FreeList*)freedBlock;
			_freeList->next = itor->next;
		}
		else
		{
			prev->next = (FreeList*)freedBlock;
			prev->next->next = itor->next;
		}
	}
	else
	{
		SetBoundaryTagsFree(freedBlock, blockLength);
		auto freeList = (FreeList*)freedBlock;
		freeList->next = _freeList;
		_freeList = freeList;
	}

	_allocated -= blockLength;
}

uintptr_t BoundaryTagAllocator::CreateBoundaryTagsAlloc(uintptr_t ptr, size_t size)
{
	auto blockptr = ptr + sizeof(BoundaryTag);
	SetBoundaryTagsAlloc(blockptr, size - sizeof(BoundaryTag));
	return blockptr;
}

uintptr_t BoundaryTagAllocator::CreateBoundaryTagsFree(uintptr_t ptr, size_t size)
{
	auto blockptr = ptr + sizeof(BoundaryTag);
	SetBoundaryTagsFree(blockptr, size - sizeof(BoundaryTag));
	return blockptr;
}

void BoundaryTagAllocator::SetBoundaryTagsAlloc(uintptr_t ptr, size_t blockSize)
{
	auto startTag = BlockTag(ptr);
	auto endTag = BlockEndTag(ptr, blockSize);
	*startTag = blockSize;
	*endTag = blockSize;
	SetBlockAllocated(startTag);
	SetBlockAllocated(endTag);
}

void BoundaryTagAllocator::SetBoundaryTagsFree(uintptr_t ptr, size_t blockSize)
{
	auto startTag = BlockTag(ptr);
	auto endTag = BlockEndTag(ptr, blockSize);
	*startTag = blockSize;
	*endTag = blockSize;
	SetBlockFree(startTag);
	SetBlockFree(endTag);
}

LinearAllocator::LinearAllocator(void* start, size_t size)
	: _start((uintptr_t)start), _end((uintptr_t)start + size), _current((uintptr_t)start)
{}

void* LinearAllocator::Alloc(size_t size)
{
	if (_current + size >= _end)
	{
		return nullptr;
	}

	uintptr_t ptr = _current;
	_current += size;

	return (void*)ptr;
}

void* LinearAllocator::Alloc(size_t size, size_t align)
{
	if (_current + size >= _end)
	{
		return nullptr;
	}

	uintptr_t alignedPtr = AlignPointerNext(_current, align);
	_current = alignedPtr + size;

	return (void*)alignedPtr;
}

ScopeStack::ScopeStack(LinearAllocator* allocator)
	: _allocator(allocator), _headerChain(nullptr), _rewindPoint(0)
{
	_rewindPoint = _allocator->Alloc(16);
	strcpy((char*)_rewindPoint, "--Scope Stack--");
}

ScopeStack::ScopeStack(const ScopeStack &other)
	: _allocator(other._allocator), _headerChain(other._headerChain), _rewindPoint(other._rewindPoint)
{}

ScopeStack::~ScopeStack()
{
	for (auto itor = _headerChain; itor != nullptr; itor = itor->chain)
	{
		itor->destroyObject(ObjectPointerFromHeader(itor));
	}

	_allocator->Rewind(_rewindPoint);
}

void* ScopeStack::Alloc(size_t size)
{
	void* ptr = _allocator->Alloc(size);
	memset(ptr, 0, size);
	return ptr;
}

ScopeStack::ObjectHeader* ScopeStack::AllocWithHeader(size_t size)
{
	return AllocWithHeader(size, 1);
}

ScopeStack::ObjectHeader* ScopeStack::AllocWithHeader(size_t size, size_t count)
{
	void* headerPtr = _allocator->Alloc(sizeof(ObjectHeader));
	void* objectPtr = _allocator->Alloc(size * count);

	auto header = new (headerPtr) ObjectHeader;
	header->count = count;
	header->size = size;

	return header;
}

MemManager::MemManager(void* ptr, size_t size, size_t persistant)
	: _head((uintptr_t)ptr), _size(size), _pStorage(nullptr), _tStorage(nullptr)
{
	uintptr_t pStorageAddress = _head;
	uintptr_t pStorageStart = pStorageAddress + sizeof(BoundaryTagAllocator);

	_pStorage = new ((void*)pStorageAddress) BoundaryTagAllocator((void*)pStorageStart, persistant);

	uintptr_t tStorageAddress = pStorageStart + persistant;
	uintptr_t tStorageStart = tStorageAddress + sizeof(LinearAllocator);
	size_t temporary = _head + _size - tStorageStart;

	_tStorage = new ((void*)tStorageAddress) LinearAllocator((void*)tStorageStart, temporary);
}

void* MemManager::Alloc(size_t size)
{
	void* ptr = _pStorage->Alloc(size);
	memset(ptr, 0, size);
	return ptr;
}

void MemManager::Free(void* ptr)
{
	_pStorage->Free(ptr);
}

ScopeStack MemManager::PushScope()
{
	return ScopeStack(_tStorage);
}