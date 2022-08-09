// Licensed to the .NET Foundation under one or more agreements.
// The .NET Foundation licenses this file to you under the MIT license.
//

#include <stddef.h>
#include <sys/mman.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <unistd.h>
#include <inttypes.h>
#include <stdio.h>
#include <string.h>
#include <assert.h>
#include <limits.h>
#include <errno.h>
#if defined(TARGET_LINUX) && !defined(MFD_CLOEXEC)
#include <linux/memfd.h>
#include <sys/syscall.h> // __NR_memfd_create
#define memfd_create(...) syscall(__NR_memfd_create, __VA_ARGS__)
#endif // TARGET_LINUX && !MFD_CLOEXEC
#include "minipal.h"

#if defined(TARGET_OSX) && defined(TARGET_AMD64)
#include <mach/mach.h>
#include <sys/sysctl.h>

bool IsProcessTranslated()
{
   int ret = 0;
   size_t size = sizeof(ret);
   if (sysctlbyname("sysctl.proc_translated", &ret, &size, NULL, 0) == -1)
   {
      return false;
   }
   return ret == 1;
}
#endif // TARGET_OSX && TARGET_AMD64

#ifndef TARGET_OSX

#ifdef TARGET_64BIT
static const off_t MaxDoubleMappedSize = 2048ULL*1024*1024*1024;
#else
static const off_t MaxDoubleMappedSize = UINT_MAX;
#endif

#endif // TARGET_OSX

bool VMToOSInterface::CreateDoubleMemoryMapper(void** pHandle, size_t *pMaxExecutableCodeSize)
{
#ifndef TARGET_OSX

#ifdef TARGET_FREEBSD
    int fd = shm_open(SHM_ANON, O_RDWR | O_CREAT, S_IRWXU);
#elif defined(TARGET_SUNOS) // has POSIX implementation
    char name[24];
    sprintf(name, "/shm-dotnet-%d", getpid());
    name[sizeof(name) - 1] = '\0';
    shm_unlink(name);
    int fd = shm_open(name, O_RDWR | O_CREAT | O_EXCL | O_NOFOLLOW, 0600);
#else // TARGET_FREEBSD
    int fd = memfd_create("doublemapper", MFD_CLOEXEC);
#endif // TARGET_FREEBSD

    if (fd == -1)
    {
        return false;
    }

    if (ftruncate(fd, MaxDoubleMappedSize) == -1)
    {
        close(fd);
        return false;
    }

    *pMaxExecutableCodeSize = MaxDoubleMappedSize;
    *pHandle = (void*)(size_t)fd;
#else // !TARGET_OSX

#ifdef TARGET_AMD64
    if (IsProcessTranslated())
    {
        // Rosetta doesn't support double mapping correctly
        return false;
    }
#endif // TARGET_AMD64

    *pMaxExecutableCodeSize = SIZE_MAX;
    *pHandle = NULL;
#endif // !TARGET_OSX

    return true;
}

void VMToOSInterface::DestroyDoubleMemoryMapper(void *mapperHandle)
{
#ifndef TARGET_OSX
    close((int)(size_t)mapperHandle);
#endif
}

extern "C" void* PAL_VirtualReserveFromExecutableMemoryAllocatorWithinRange(const void* lpBeginAddress, const void* lpEndAddress, size_t dwSize, int fStoreAllocationInfo);

#ifdef TARGET_OSX
bool IsMapJitFlagNeeded()
{
    static volatile int isMapJitFlagNeeded = -1;

    if (isMapJitFlagNeeded == -1)
    {
        int mapJitFlagCheckResult = 0;
        int pageSize = sysconf(_SC_PAGE_SIZE);
        // Try to map a page with read-write-execute protection. It should fail on Mojave hardened runtime and higher.
        void* testPage = mmap(NULL, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
        if (testPage == MAP_FAILED && (errno == EACCES))
        {
            // The mapping has failed with EACCES, check if making the same mapping with MAP_JIT flag works
            testPage = mmap(NULL, pageSize, PROT_READ | PROT_WRITE | PROT_EXEC, MAP_ANONYMOUS | MAP_PRIVATE | MAP_JIT, -1, 0);
            if (testPage != MAP_FAILED)
            {
                mapJitFlagCheckResult = 1;
            }
        }

        if (testPage != MAP_FAILED)
        {
            munmap(testPage, pageSize);
        }

        isMapJitFlagNeeded = mapJitFlagCheckResult;
    }

    return (bool)isMapJitFlagNeeded;
}
#endif // TARGET_OSX

typedef struct 
{
    unsigned char ident[16];
    uint16_t    type;
    uint16_t    machine;
    uint32_t    version;
    uint64_t    entry;
    uint64_t    phoff;
    uint64_t    shoff;
    uint32_t    flags;
    uint16_t    ehsize;
    uint16_t    phentsize;
    uint16_t    phnum;
    uint16_t    shentsize;
    uint16_t    shnum;
    uint16_t    shstrndx;
} ElfHeader;

typedef struct
{
    uint32_t  type;
    uint32_t  flags; 
    uint64_t  offset;
    uint64_t  vaddr; 
    uint64_t  paddr; 
    uint64_t  filesz;
    uint64_t  memsz; 
    uint64_t  align; 
} ProgramHeader;

typedef struct 
{
    uint32_t namesz;
    uint32_t descsz;
    uint32_t type;
    unsigned char name[4];
    unsigned char desc[20];
} NoteSegment;

void WriteFakeElfHeader(void *pTargetRW)
{
    ElfHeader *fakeElfHeader = reinterpret_cast<ElfHeader *>(pTargetRW);
    // Magic number
    fakeElfHeader->ident[0] = 0x7F;
    fakeElfHeader->ident[1] = 'E';
    fakeElfHeader->ident[2] = 'L';
    fakeElfHeader->ident[3] = 'F';

    // 64 bit
    fakeElfHeader->ident[4] = 2;
    // Little endian
    fakeElfHeader->ident[5] = 1;
    // Version
    fakeElfHeader->ident[6] = 1;
    // ABI 0 = System V
    fakeElfHeader->ident[7] = 0;
    // ABI version
    fakeElfHeader->ident[8] = 0;
    // Padding
    for (int i = 9; i < 16; ++i)
    {
        fakeElfHeader->ident[i] = 0;
    }

    // 0 = none
    fakeElfHeader->type = 0;
    // 3E = amd64
    fakeElfHeader->machine = 0x3E;
    fakeElfHeader->version = 1;
    fakeElfHeader->entry = 0x0;
    // offset to ph table, immediately after this
    fakeElfHeader->phoff = sizeof(ElfHeader);
    fakeElfHeader->shoff = 0;
    fakeElfHeader->flags = 0;
    fakeElfHeader->ehsize = sizeof(ElfHeader);
    fakeElfHeader->phentsize = sizeof(ProgramHeader);
    // One note header
    fakeElfHeader->phnum = 1;
    fakeElfHeader->shentsize = 0;
    fakeElfHeader->shnum = 0;
    fakeElfHeader->shstrndx = 0;
    
    ProgramHeader *fakeProgramHeader = reinterpret_cast<ProgramHeader *>((void *)((uintptr_t)pTargetRW + sizeof(ElfHeader)));
    NoteSegment *fakeNoteSegment = reinterpret_cast<NoteSegment *>((void *)((uintptr_t)pTargetRW + sizeof(ElfHeader) + sizeof(ProgramHeader)));
    
    // 0x4 == PT_NOTE
    fakeProgramHeader->type = 0x4;
    fakeProgramHeader->flags = 0;
    fakeProgramHeader->offset = sizeof(ElfHeader) + sizeof(ProgramHeader);
    fakeProgramHeader->vaddr = (uint64_t)fakeNoteSegment;
    fakeProgramHeader->paddr = 0;
    fakeProgramHeader->filesz = sizeof(NoteSegment);
    fakeProgramHeader->memsz = sizeof(NoteSegment);
    fakeProgramHeader->align = 0;

    unsigned char fakeBuildId[20] = { 0xA1, 0x43, 0x6D, 0x06, 0x5B, 0xAD, 0x43, 0x8A, 0xBA, 0x5D, 0x77, 0x82, 0x13, 0x0A, 0xB5, 0x6E, 0xAA, 0xAB, 0xAC, 0xAD };

    // "GNU\0"
    fakeNoteSegment->namesz = 4;
    fakeNoteSegment->descsz = sizeof(fakeBuildId);
    // 3 = NT_GNU_BUILD_ID
    fakeNoteSegment->type = 3;
    memcpy(fakeNoteSegment->name, "GNU", 4);
    memcpy(fakeNoteSegment->desc, &fakeBuildId, sizeof(fakeBuildId));
}

void* VMToOSInterface::ReserveDoubleMappedMemory(void *mapperHandle, size_t offset, size_t size, const void *rangeStart, const void* rangeEnd)
{
    int fd = (int)(size_t)mapperHandle;

    bool isUnlimitedRange = (rangeStart == NULL) && (rangeEnd == NULL);

    if (isUnlimitedRange)
    {
        rangeEnd = (void*)SIZE_MAX;
    }

    void* result = PAL_VirtualReserveFromExecutableMemoryAllocatorWithinRange(rangeStart, rangeEnd, size, 0 /* fStoreAllocationInfo */);
#ifndef TARGET_OSX
    if (result != NULL)
    {
        void *temp = mmap(result, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, offset);
        if (temp == MAP_FAILED)
        {
            assert(false);
            result = NULL;
        }

        // Map the shared memory over the range reserved from the executable memory allocator.
        WriteFakeElfHeader(result);

        result = mmap(result, size, PROT_READ | PROT_EXEC, MAP_SHARED | MAP_FIXED, fd, offset);
        if (result == MAP_FAILED)
        {
            assert(false);
            result = NULL;
        }
    }
#endif // TARGET_OSX

    // For requests with limited range, don't try to fall back to reserving at any address
    if ((result != NULL) || !isUnlimitedRange)
    {
        return result;
    }

#ifndef TARGET_OSX
    void *temp = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
    if (temp == MAP_FAILED)
    {
        assert(false);
        result = NULL;
    }
 
    WriteFakeElfHeader(result);

    result = mmap(result, size, PROT_READ | PROT_EXEC, MAP_SHARED, fd, offset);
    if (result == MAP_FAILED)
    {
        assert(false);
        result = NULL;
    }

#else
    int mmapFlags = MAP_ANON | MAP_PRIVATE;
    if (IsMapJitFlagNeeded())
    {
        mmapFlags |= MAP_JIT;
    }
    result = mmap(NULL, size, PROT_NONE, mmapFlags, -1, 0);
#endif    
    if (result == MAP_FAILED)
    {
        assert(false);
        result = NULL;
    }

    return result;
}

void *VMToOSInterface::CommitDoubleMappedMemory(void* pStart, size_t size, bool isExecutable)
{
    if (isExecutable)
    {
        return pStart;
    }

    if (mprotect(pStart, size, isExecutable ? (PROT_READ | PROT_EXEC) : (PROT_READ | PROT_WRITE)) == -1)
    {
        return NULL;
    }

    // If not executable, clear ELF header now
    assert(size >= 0x9C);
    memset(pStart, 0, 0x9C);

    return pStart;
}

bool VMToOSInterface::ReleaseDoubleMappedMemory(void *mapperHandle, void* pStart, size_t offset, size_t size)
{
#ifndef TARGET_OSX
    int fd = (int)(size_t)mapperHandle;
    mmap(pStart, size, PROT_READ | PROT_WRITE, MAP_SHARED | MAP_FIXED, fd, offset);
    memset(pStart, 0, size);
#endif // TARGET_OSX
    return munmap(pStart, size) != -1;
}

void* VMToOSInterface::GetRWMapping(void *mapperHandle, void* pStart, size_t offset, size_t size)
{
#ifndef TARGET_OSX
    int fd = (int)(size_t)mapperHandle;
    return mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, offset);
#else // TARGET_OSX
#ifdef TARGET_AMD64
    vm_address_t startRW;
    vm_prot_t curProtection, maxProtection;
    kern_return_t kr = vm_remap(mach_task_self(), &startRW, size, 0, VM_FLAGS_ANYWHERE | VM_FLAGS_RANDOM_ADDR,
                                mach_task_self(), (vm_address_t)pStart, FALSE, &curProtection, &maxProtection, VM_INHERIT_NONE);

    if (kr != KERN_SUCCESS)
    {
        return NULL;
    }

    int st = mprotect((void*)startRW, size, PROT_READ | PROT_WRITE);
    if (st == -1)
    {
        munmap((void*)startRW, size);
        return NULL;
    }

    return (void*)startRW;
#else // TARGET_AMD64
    // This method should not be called on OSX ARM64
    assert(false);
    return NULL;
#endif // TARGET_AMD64
#endif // TARGET_OSX
}

bool VMToOSInterface::ReleaseRWMapping(void* pStart, size_t size)
{
    return munmap(pStart, size) != -1;
}
