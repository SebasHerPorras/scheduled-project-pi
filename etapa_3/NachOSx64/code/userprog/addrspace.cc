// addrspace.cc 
//	Routines to manage address spaces (executing user programs).
//
//	In order to run a user program, you must:
//
//	1. link with the -N -T 0 option 
//	2. run coff2noff to convert the object file to Nachos format
//		(Nachos object code format is essentially just a simpler
//		version of the UNIX executable object code format)
//	3. load the NOFF file into the Nachos file system
//		(if you haven't implemented the file system yet, you
//		don't need to do this last step)
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "addrspace.h"
#include "noff.h"

//----------------------------------------------------------------------
// SwapHeader
// 	Do little endian to big endian conversion on the bytes in the 
//	object file header, in case the file was generated on a little
//	endian machine, and we're now running on a big endian machine.
//----------------------------------------------------------------------

// this function converts virtual addresses to physical addresses Sebas implementation
// En addrspace.cc
AddrSpace::AddrSpace() {
    // No inicializa desde un ejecutable
    pageTable = nullptr;
    numPages = 0;
    
}

int VirtualToPhysical(int virtualAddr, TranslationEntry* pageTable) {
    int vpn = virtualAddr / PageSize;
    int offset = virtualAddr % PageSize;
    int ppn = pageTable[vpn].physicalPage;
    return ppn * PageSize + offset;
}
void LoadSegment(OpenFile* executable, int virtualAddr, int fileOffset, int size, TranslationEntry* pageTable) {
    char buffer[PageSize];

    while (size > 0) {
        int toRead = PageSize - (virtualAddr % PageSize);
        if (toRead > size) toRead = size;

        executable->ReadAt(buffer, toRead, fileOffset);

        int physAddr = VirtualToPhysical(virtualAddr, pageTable);
        bcopy(buffer, &machine->mainMemory[physAddr], toRead);

        virtualAddr += toRead;
        fileOffset += toRead;
        size -= toRead;
    }
}



static void 
SwapHeader (NoffHeader *noffH)
{
	noffH->noffMagic = WordToHost(noffH->noffMagic);
	noffH->code.size = WordToHost(noffH->code.size);
	noffH->code.virtualAddr = WordToHost(noffH->code.virtualAddr);
	noffH->code.inFileAddr = WordToHost(noffH->code.inFileAddr);
	noffH->initData.size = WordToHost(noffH->initData.size);
	noffH->initData.virtualAddr = WordToHost(noffH->initData.virtualAddr);
	noffH->initData.inFileAddr = WordToHost(noffH->initData.inFileAddr);
	noffH->uninitData.size = WordToHost(noffH->uninitData.size);
	noffH->uninitData.virtualAddr = WordToHost(noffH->uninitData.virtualAddr);
	noffH->uninitData.inFileAddr = WordToHost(noffH->uninitData.inFileAddr);
}

//----------------------------------------------------------------------
// AddrSpace::AddrSpace
// 	Create an address space to run a user program.
//	Load the program from a file "executable", and set everything
//	up so that we can start executing user instructions.
//
//	Assumes that the object code file is in NOFF format.
//
//	First, set up the translation from program memory to physical 
//	memory.  For now, this is really simple (1:1), since we are
//	only uniprogramming, and we have a single unsegmented page table
//
//	"executable" is the file containing the object code to load into memory
//----------------------------------------------------------------------

AddrSpace::AddrSpace(OpenFile *executable)
{
    NoffHeader noffH;
    unsigned int i, size;

    executable->ReadAt((char *)&noffH, sizeof(noffH), 0);
    if ((noffH.noffMagic != NOFFMAGIC) && 
        (WordToHost(noffH.noffMagic) == NOFFMAGIC)) {
        SwapHeader(&noffH);
        printf("\n\nLeyendo el archivo ejecutable...\n");
        printf("\n\nMagic leído: 0x%x\n", noffH.noffMagic);
        printf("Magic esperado: 0x%x\n", NOFFMAGIC);
        // ASSERT(noffH.noffMagic == NOFFMAGIC);
    }

// how big is address space?
    size = noffH.code.size + noffH.initData.size + noffH.uninitData.size 
			+ UserStackSize;	// we need to increase the size
						// to leave room for the stack
    numPages = divRoundUp(size, PageSize);
    size = numPages * PageSize;

    ASSERT(numPages <= NumPhysPages);		// check we're not trying
						// to run anything too big --
						// at least until we have
						// virtual memory

    DEBUG('a', "Initializing address space, num pages %d, size %d\n", 
					numPages, size);
// first, set up the translation 
    
    pageTable = new TranslationEntry[numPages];
    for (i = 0; i < numPages; i++) {
        memoryLock->Acquire();             // proteger acceso al bitmap
        int frame = MiMapa->Find();        // buscar una página física libre
        memoryLock->Release();
        
        ASSERT(frame != -1);               // si no hay memoria suficiente, abortar
    
        pageTable[i].virtualPage = i;
        pageTable[i].physicalPage = frame;
        pageTable[i].valid = true;
        pageTable[i].use = false;
        pageTable[i].dirty = false;
        pageTable[i].readOnly = false;
    }
    
    
// zero out the entire address space, to zero the unitialized data segment 
// and the stack segment
// we need to increase the size to leave room for the stack
    // Limpiar solo las páginas de datos no inicializados y pila
    unsigned totalSize = noffH.code.size + noffH.initData.size + noffH.uninitData.size + UserStackSize;
    unsigned totalPages = divRoundUp(totalSize, PageSize);

    for (unsigned j = 0; j < totalPages; j++) {
        unsigned pageStart = j * PageSize;

        bool isCodePage = (pageStart >= (unsigned)noffH.code.virtualAddr) && (pageStart < (unsigned)(noffH.code.virtualAddr + noffH.code.size));
        bool isDataPage = (pageStart >= (unsigned)noffH.initData.virtualAddr) && (pageStart < (unsigned)(noffH.initData.virtualAddr + noffH.initData.size));

        if (!isCodePage && !isDataPage) {
            int physPage = pageTable[j].physicalPage;
            bzero(&machine->mainMemory[physPage * PageSize], PageSize);
        }
    }


    // then, copy in the code and data segments into memory
    if (noffH.code.size > 0) {
        DEBUG('a', "Loading code segment, size %d\n", noffH.code.size);
        LoadSegment(executable, noffH.code.virtualAddr, noffH.code.inFileAddr,
                    noffH.code.size, pageTable);
        printf("from::Addrspace, finished loading code segment, virtual to physical\n");
    }

    if (noffH.initData.size > 0) {
        DEBUG('a', "Loading initData segment, size %d\n", noffH.initData.size);
        LoadSegment(executable, noffH.initData.virtualAddr, noffH.initData.inFileAddr,
                    noffH.initData.size, pageTable);
        printf("from::Addrspace, finished loading initData segment, virtual to physical\n");
    }
    nextFreeVirtualAddr = noffH.code.virtualAddr + noffH.code.size + noffH.initData.size + noffH.uninitData.size;

    // Por si acaso la dirección pasa del límite de la memoria asignada:
    if (nextFreeVirtualAddr > static_cast<int>(numPages * PageSize)) {
        nextFreeVirtualAddr = numPages * PageSize;
    }
    // printf("AddrSpace::AddrSpace: %d pages allocated\n", numPages);
    printf("\nfrom::Addrspace, finished loading memory pages\n");
   

}

//----------------------------------------------------------------------
// AddrSpace::~AddrSpace
// 	Dealloate an address space.  Nothing for now!
//----------------------------------------------------------------------

AddrSpace::~AddrSpace()
{
   delete pageTable;
}

//----------------------------------------------------------------------
// AddrSpace::InitRegisters
// 	Set the initial values for the user-level register set.
//
// 	We write these directly into the "machine" registers, so
//	that we can immediately jump to user code.  Note that these
//	will be saved/restored into the currentThread->userRegisters
//	when this thread is context switched out.
//----------------------------------------------------------------------

void
AddrSpace::InitRegisters()
{
    int i;

    for (i = 0; i < NumTotalRegs; i++)
	machine->WriteRegister(i, 0);

    // Initial program counter -- must be location of "Start"
    machine->WriteRegister(PCReg, 0);	

    // Need to also tell MIPS where next instruction is, because
    // of branch delay possibility
    machine->WriteRegister(NextPCReg, 4);

   // Set the stack register to the end of the address space, where we
   // allocated the stack; but subtract off a bit, to make sure we don't
   // accidentally reference off the end!
    machine->WriteRegister(StackReg, numPages * PageSize - 16);
    DEBUG('a', "Initializing stack register to %d\n", numPages * PageSize - 16);
}

//----------------------------------------------------------------------
// AddrSpace::SaveState
// 	On a context switch, save any machine state, specific
//	to this address space, that needs saving.
//
//	For now, nothing!
//----------------------------------------------------------------------

void AddrSpace::SaveState() 
{}

//----------------------------------------------------------------------
// AddrSpace::RestoreState
// 	On a context switch, restore the machine state so that
//	this address space can run.
//
//      For now, tell the machine where to find the page table.
//----------------------------------------------------------------------

void AddrSpace::RestoreState() 
{
    machine->pageTable = pageTable;
    machine->pageTableSize = numPages;
}
int AddrSpace::AllocateStackForClone() {
    // Asignar nueva pila en la parte superior del espacio de direcciones
    int stackPointer = numPages * PageSize - 16; // Mismo desplazamiento que InitRegisters
    return stackPointer;
}

// Modificar el método Clone para mejor manejo de memoria
AddrSpace* AddrSpace::Clone() {
    AddrSpace* newSpace = new AddrSpace();
    newSpace->numPages = this->numPages;
    newSpace->pageTable = new TranslationEntry[numPages];
    
    // Calcular páginas de stack (últimas 8 páginas)
    const unsigned stackStartPage = numPages - divRoundUp(UserStackSize, PageSize);
    
    for (unsigned i = 0; i < numPages; i++) {
        if (i >= stackStartPage) {
            // Páginas de stack: nuevas copias
            memoryLock->Acquire();
            int frame = MiMapa->Find();
            memoryLock->Release();
            
            ASSERT(frame != -1);
            
            newSpace->pageTable[i].virtualPage = i;
            newSpace->pageTable[i].physicalPage = frame;
            newSpace->pageTable[i].valid = true;
            newSpace->pageTable[i].use = false;
            newSpace->pageTable[i].dirty = false;
            newSpace->pageTable[i].readOnly = false;
            
            // Inicializar pila vacía
            bzero(&machine->mainMemory[frame * PageSize], PageSize);
        } else {
            // Páginas de código/datos: compartidas (read-only)
            newSpace->pageTable[i] = this->pageTable[i];
            newSpace->pageTable[i].readOnly = true;
        }
    }
    
    return newSpace;
}

int AddrSpace::AllocateMemory(int size) {
    // Alinear size a múltiplo de 4 para seguridad (opcional)
    int alignedSize = (size + 3) & ~3;

    // Verificar que no se pase del límite del espacio virtual
    if (nextFreeVirtualAddr + alignedSize > static_cast<int>(numPages * PageSize)) {
        printf("[AllocateMemory] No hay espacio suficiente para asignar %d bytes\n", size);
        return -1;  // Error: no hay espacio
    }

    int allocatedAddr = nextFreeVirtualAddr;
    nextFreeVirtualAddr += alignedSize;

    printf("[AllocateMemory] Asignado %d bytes en dirección virtual %d\n", size, allocatedAddr);
    return allocatedAddr;
}


