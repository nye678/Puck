#ifndef __PUCK_MEM_H_
#define __PUCK_MEM_H_

#include <stdint.h>

namespace puck
{
	#define KILOBYTE(num) (num * 1024)
	#define MEGABYTE(num) (KILOBYTE(num) * 1024)
	#define GIGABYTE(num) (MEGABYTE(num) * 1024)

	/* Boundary Tag Allocator */
	/* An allocator that manages a chunk of memory using boundary tags.
	   When a block is allocated a free block is split with boundary tags
	   added. When a block is freed then it will merge with any adjacent 
	   free blocks. */
	class BoundaryTagAllocator
	{
		/* Singly linked list used to manage free blocks. The next pointer
		   is stored inside the blocks memory. */
		struct FreeList
		{
			FreeList* next;
		};

		/* Tag are placed on either end of a block. The first bit is used
		   to determine if the block is free (0) or allocated (1). The remaining
		   bits are used to store the size of the block. Odd block sizes
		   will allocate an additional byte. */
		typedef uint32_t BoundaryTag;

	private:
		size_t _size;
		size_t _allocated;
		size_t _numBlocks;

		uintptr_t _head;
		uintptr_t _tail;

		FreeList* _freeList;

	public:
		/* Allocates a chunk of memory. */
		void* Alloc(size_t size);

		/* Frees a chunk of memory. */
		void Free(void* ptr);

	private:
		/* Splits a block creating a sized chunk. The remaining block is added
		   to the free list. The address of the sized block is returned. */
		uintptr_t SplitBlock(uintptr_t block, size_t size);

		/* Merges all blocks between start and end tags provided. */
		void MergeBlocks(BoundaryTag* startTag, BoundaryTag* endTag);

		/* Creates boundary tags for a chunk of memory. The pointer returned
		   is the start of the new block. */
		uintptr_t CreateBoundaryTagsAlloc(uintptr_t ptr, size_t size);
		uintptr_t CreateBoundaryTagsFree(uintptr_t ptr, size_t size);

		/* Sets the boundary tags for memory block. */
		void SetBoundaryTagsAlloc(uintptr_t ptr, size_t blockSize);
		void SetBoundaryTagsFree(uintptr_t ptr, size_t blockSize);

	public:
		BoundaryTagAllocator(void* head, size_t size);
	};

	/* Linear Allocator */
	/* An allocator which holds a large chunk of memory and allocates
	   chunks linearly. The allocator can be rewound to clear up memory.
	   Care will need to be take so that memory is not prematurely freed.*/
	class LinearAllocator
	{
	private:
		uintptr_t _start;
		uintptr_t _end;
		uintptr_t _current;

	public:
		/* Allocates a raw chunk of memory. */
		void* Alloc(size_t size);
		
		/* Allocates a raw chunk of aligned memory. */
		void* Alloc(size_t size, size_t align);

		/* Rewinds the allocator, any memory between pointer and
		   the current allocator position will be freed. */
		inline void Rewind(void* pointer)
		{
			_current = (uintptr_t)pointer;
		}

		/* Resets the allocator, freeing all memory that has been allocated */
		inline void Reset()
		{
			_current = _start;
		}

	public:
		LinearAllocator(void* start, size_t size);
	};

	/* Scope Stack */
	/* A scope stack is a scoped linear memory allocator which automatically
	   frees memory when destructed. Object destructors will even be called
	   automatically by the ScopeStack. */
	class ScopeStack
	{
		// All object allocations are made on 16 byte boundries.
		static const size_t Alignment = 16;

		// Object headers are allocated along with objects. Headers form a
		// linked list whose order reflects the order that object destructors
		// are called in.
		struct ObjectHeader
		{
			// Pointer to the next object header.
			ObjectHeader* chain;

			// Function pointer to a function that will destroy the object.
			void(*destroyObject)(void *ptr);
			
			// The number of allocated objects. Will equal 1 for single objects.
			// Arrays of objects will share a single header.
			size_t count;

			// The size of the object in bytes.
			size_t size;
		};

		// Returns a pointer to the object (or first object) from a header.
		inline static void* ObjectPointerFromHeader(ObjectHeader* header)
		{
			return (void*)((uintptr_t)header + sizeof(ObjectHeader));
		}

		// Returns a pointer to the header of an allocated object.
		// Don't use on PODs!!
		inline static ObjectHeader* HeaderFromObjectPointer(uintptr_t ptr)
		{
			return (ObjectHeader*)(ptr - sizeof(ObjectHeader));
		}

		// Calls the destructor for type T objects.
		template <typename T> static void DestructorCall(void *ptr);

		// Calls the destructor for every object in the array.
		template <typename T> static void ArrayDestructorCall(void *ptr);

	private:
		LinearAllocator* _allocator;
		ObjectHeader* _headerChain;
		void* _rewindPoint;

	public:
		// Allocates a raw chunk of memory.
		void* Alloc(size_t size);

		// Constructs a new object.
		template <typename T> T* NewObject();

		// Constructs a new object with the given parameters.
		template <typename T, typename... Args> T* NewObject(Args... args);

		// Constructs a new object array.
		template <typename T> T* NewObjectArray(size_t count);

		// Constructs a new POD object. Destructors will not be
		// called for POD objects.
		template <typename T> T* NewPOD();
		
	private:
		ObjectHeader* AllocWithHeader(size_t size);
		ObjectHeader* AllocWithHeader(size_t size, size_t count);

	public:
		ScopeStack(LinearAllocator* allocator);
		ScopeStack(const ScopeStack &);
		~ScopeStack();
	};

	template <typename T>
	T* ScopeStack::NewObject()
	{
		ObjectHeader* header = AllocWithHeader(sizeof(T));
		T* result = new (ObjectPointerFromHeader(header)) T;

		header->destroyObject = &ScopeStack::DestructorCall<T>;
		header->chain = _headerChain;
		_headerChain = header;

		return result;
	}

	template <typename T, typename... Args>
	T* ScopeStack::NewObject(Args... args)
	{
		ObjectHeader* header = AllocWithHeader(sizeof(T));
		T* result = new (ObjectPointerFromHeader(header)) T(args...);

		header->destroyObject = &ScopeStack::DestructorCall<T>;
		header->chain = _headerChain;
		_headerChain = header;

		return result;
	}

	template <typename T>
	T* ScopeStack::NewObjectArray(size_t count)
	{
		size_t b = sizeof(T[5]);
		size_t c = count * sizeof(T);

		ObjectHeader* header = AllocWithHeader(sizeof(T), count);
		T* result = new (ObjectPointerFromHeader(header)) T[count];

		header->destroyObject = &ScopeStack::ArrayDestructorCall<T>;
		header->chain = _headerChain;
		_headerChain = header;

		return result;
	}

	template <typename T>
	T* ScopeStack::NewPOD()
	{
		void* podPtr = Alloc(sizeof(T));
		return new (podPtr) T;
	}

	template <typename T>
	void ScopeStack::DestructorCall(void *ptr)
	{
		static_cast<T*>(ptr)->~T();
	}

	template <typename T>
	void ScopeStack::ArrayDestructorCall(void *ptr)
	{
		auto header = HeaderFromObjectPointer(ptr);
		auto arrayPtr = static_cast<T*>(ptr);
		
		for (size_t i = 0; i < header->count; ++i)
		{
			arrayPtr[i].~T();
		}
	}

	/* Memory Manager */
	/* Manages a large block of pre-allocated memory. The block is split into
	   persistant and temporary sections. Object lifetime should be taken into
	   consideration when deciding where to allocate memory from. */
	class MemManager
	{
	private:
		BoundaryTagAllocator* _pStorage;
		LinearAllocator* _tStorage;

		uintptr_t _head;
		size_t _size;

	public:
		/* Allocates a block of memory in persistant storage. */
		void* Alloc(size_t size);

		/* Frees a block of memory in persistant storage. */
		void Free(void* ptr);

		ScopeStack PushScope();

		MemManager(void* ptr, size_t size, size_t persistant);
	};
}

#endif