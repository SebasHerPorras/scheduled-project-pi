// exception.cc 
//	Entry point into the Nachos kernel from user programs.
//	There are two kinds of things that can cause control to
//	transfer back to here from user code:
//
//	syscall -- The user code explicitly requests to call a procedure
//	in the Nachos kernel.  Right now, the only function we support is
//	"Halt".
//
//	exceptions -- The user code does something that the CPU can't handle.
//	For instance, accessing memory that doesn't exist, arithmetic errors,
//	etc.  
//
//	Interrupts (which can also cause control to transfer from user
//	code into the Nachos kernel) are handled elsewhere.
//
// For now, this only handles the Halt() system call.
// Everything else core dumps.
//
// Copyright (c) 1992-1993 The Regents of the University of California.
// All rights reserved.  See copyright.h for copyright notice and limitation 
// of liability and disclaimer of warranty provisions.

#include "copyright.h"
#include "system.h"
#include "syscall.h"
#include <sys/socket.h>
#include <arpa/inet.h>	// for inet_pton
#include <fcntl.h>
#include <cstdint>
#include <unistd.h>




void NachOS_TraerFigura();
void traerAux(char* figura);
void returnFromSystemCall();
void TraerFiguraThread(void* arg);
void NachosForkThread(void* p);
void NachOS_Fork();
char* copyStringFromMachine(int userAddr);
void NachOS_ThreadStart(void* arg);

// array de bool sockets
bool isSocket[MAX_OPEN_FILES];

#define MAX_FIGURA_LEN 256

char* copyStringFromMachine(int userAddr) {
   char* kernelBuffer = new char[MAX_FIGURA_LEN];
   int i = 0;
   int ch;
   do {
       machine->ReadMem(userAddr + i, 1, &ch);
       kernelBuffer[i] = (char)ch;
       i++;
   } while (ch != 0 && i < MAX_FIGURA_LEN);
   kernelBuffer[MAX_FIGURA_LEN - 1] = '\0';
   return kernelBuffer;
}

/*
 *  System call interface: Halt()
 */
void NachOS_Halt() {		// System call 0

	DEBUG('a', "Shutdown, initiated by user program.\n");
   	interrupt->Halt();

}


/*
 *  System call interface: void Exit( int )
 */
void NachOS_Exit() {

}



/*
 *  System call interface: SpaceId Exec( char * )
 */
void NachOS_Exec() {		// System call 2
}


/*
 *  System call interface: int Join( SpaceId )
 */
void NachOS_Join() {		// System call 3
}


/*
 *  System call interface: void Create( char * )
 */
void NachOS_Create() {

}



/*
 *  System call interface: OpenFileId Open( char * )
 */
bool copyStringFromMachine(int from, char *to, unsigned size) {
   unsigned i = 0;
   int ch;

   while (i < size - 1) {
       machine->ReadMem(from + i, 1, &ch);
       to[i] = (char) ch;
       if (to[i] == '\0') break;
       i++;
   }
   to[i] = '\0';
   return true;
}

void NachOS_Open() {

}




/*
 *  System call interface: OpenFileId Write( char *, int, OpenFileId )
 */
void NachOS_Write() {

}





/*
 *  System call interface: OpenFileId Read( char *, int, OpenFileId )
 */
void NachOS_Read() { // System call 7

}



/*
 *  System call interface: void Close( OpenFileId )
 */
void NachOS_Close() {		// System call 8
   // Leer el descriptor de archivo desde el registro 4
   int fd = machine->ReadRegister(4);
   DEBUG('u', "Close syscall: fd=%d\n", fd);
   // printf("Close syscall: fd=%d\n", fd);

   int result = 0;

   if (fd < 0)
   {
      result = -1;
   }
   else if (isSocket[fd])
   {
      // printf("es socket\n");
      // Es un socket, cerrar con close()
      result = close(fd);
      isSocket[fd] = false;
   }
   else
   {
      // printf("es archivo regular\n");
      // Es archivo regular
      OpenFile *file = openFilesTable->openFiles[fd];
      if (file != nullptr)
      {
         openFilesTable->Close(fd);
         result = 0;
      }
      else
      {
         result = -1;
      }
   }

   machine->WriteRegister(2, result);

   // Avanzar el PC
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}

void NachOS_TraerFigura() {	// System call 36
  
}

void NachOS_Fork() {
   DEBUG('u', "System Call: Fork\n");

   int funcAddr = machine->ReadRegister(4);
   Thread* newThread = new Thread("Forked Thread");

   // Clonar espacio de direcciones (incluyendo nueva pila)
   newThread->space = currentThread->space->Clone();

   // Configurar el stack pointer para el nuevo hilo
   int newStackPointer = currentThread->space->AllocateStackForClone();
   machine->WriteRegister(StackReg, newStackPointer);

   // Fork para ejecutar la función pasada
   newThread->Fork(NachOS_ThreadStart, (void*)(intptr_t)funcAddr);

   // Padre recibe 1 (valor arbitrario positivo)
   machine->WriteRegister(2, 1);

   // Avanzar PC
   returnFromSystemCall();
}

void NachOS_ThreadStart(void* func) {
   intptr_t funcAddr = reinterpret_cast<intptr_t>(func);
   printf("Hilo hijo creado, ejecutando función en dirección: %p\n", (void*)funcAddr);

   // Hijo recibe 0
   machine->WriteRegister(2, 0);

   // Configurar stack pointer (ya está configurado por Clone)
   // Configurar PC para la función
   machine->WriteRegister(PCReg, funcAddr);
   machine->WriteRegister(NextPCReg, funcAddr + 4);

   // Ejecutar
   machine->Run();
   ASSERT(false); // Nunca debe llegar aquí
}

void traerAux(char* figura) {
   printf("Traer figura: %s\n\n", figura);
   // Aquí puedes agregar código para traer la figura (p.ej. leer de disco, etc)
}




/*
 *  System call interface: void Yield()
 */
void NachOS_Yield() {		// System call 10
}


/*
 *  System call interface: Sem_t SemCreate( int )
 */
void NachOS_SemCreate() {		// System call 11
}


/*
 *  System call interface: int SemDestroy( Sem_t )
 */
void NachOS_SemDestroy() {		// System call 12
}


/*
 *  System call interface: int SemSignal( Sem_t )
 */
void NachOS_SemSignal() {		// System call 13
}


/*
 *  System call interface: int SemWait( Sem_t )
 */
void NachOS_SemWait() {		// System call 14
}


/*
 *  System call interface: Lock_t LockCreate( int )
 */
void NachOS_LockCreate() {		// System call 15
}


/*
 *  System call interface: int LockDestroy( Lock_t )
 */
void NachOS_LockDestroy() {		// System call 16
}


/*
 *  System call interface: int LockAcquire( Lock_t )
 */
void NachOS_LockAcquire() {		// System call 17
}


/*
 *  System call interface: int LockRelease( Lock_t )
 */
void NachOS_LockRelease() {		// System call 18
}


/*
 *  System call interface: Cond_t LockCreate( int )
 */
void NachOS_CondCreate() {		// System call 19
}


/*
 *  System call interface: int CondDestroy( Cond_t )
 */
void NachOS_CondDestroy() {		// System call 20
}


/*
 *  System call interface: int CondSignal( Cond_t )
 */
void NachOS_CondSignal() {		// System call 21
}


/*
 *  System call interface: int CondWait( Cond_t )
 */
void NachOS_CondWait() {		// System call 22
}


/*
 *  System call interface: int CondBroadcast( Cond_t )
 */
void NachOS_CondBroadcast() {		// System call 23
}


/*
 *  System call interface: Socket_t Socket( int, int )
 */
void NachOS_Socket() { // System call 30
   // AF_INET_NachOS, SOCK_STREAM_NachOS como parametros
   int domain = machine->ReadRegister(4);
   int type = machine->ReadRegister(5);
   // Validar los parámetros
   // Traducción NachOS a POSIX
   domain = (domain == 0) ? AF_INET : AF_UNSPEC;
   type = (type == 0) ? SOCK_STREAM : SOCK_DGRAM;
   DEBUG('u', "Socket syscall: domain=%d, type=%d\n", domain, type);
   // Crear socket con syscall de c
   int sockfd = socket(domain, type, 0);
   if (sockfd < 0)
   {
      DEBUG('u', "Socket syscall: Error al crear socket\n");
      printf("Error al crear socket\n");
      machine->WriteRegister(2, -1);
   }
   else
   {
      DEBUG('u', "Socket syscall: Socket creado con fd=%d\n", sockfd);
      printf("Socket creado con fd=%d\n", sockfd);
      // Agregar socket a la tabla de archivos abiertos
      isSocket[sockfd] = true;
      machine->WriteRegister(2, sockfd);
   }
   // poner el sockfd en el registro 2 que es el de retorno
   // Avanzar el PC SIEMPRE
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
 }
 
 /*
  *  System call interface: Socket_t Connect( char *, int )
  */
 void NachOS_Connect() { // System call 31
   int sockfd = machine->ReadRegister(4);
   int addr_ptr = machine->ReadRegister(5); // dirección en user space
   int port = machine->ReadRegister(6);

   // Leer IP desde memoria de usuario (máx 16 bytes por seguridad)
   char ip[16]; // ejemplo: "127.0.0.1"
   for (int i = 0; i < 15; ++i)
   {
      char c;
      machine->ReadMem(addr_ptr + i, 1, (int *)&c);
      ip[i] = c;
      if (c == '\0')
         break;
   }
   ip[15] = '\0';
   //  const char* ipquemada = "163.178.104.62";
   DEBUG('u', "Connect syscall: sockfd=%d, ip=%s, port=%d\n", sockfd, ip, port);
   printf("Connect syscall: sockfd=%d, ip=%s, port=%d\n", sockfd, ip, port);

   // Crear sockaddr_in
   struct sockaddr_in serv_addr;
   serv_addr.sin_family = AF_INET;
   serv_addr.sin_port = htons(port);
   serv_addr.sin_addr.s_addr = inet_addr(ip);

   // Realizar conexión
   // printf("Connect syscall: intentando conectar...\n");
   int result = connect(sockfd, (struct sockaddr *)&serv_addr, sizeof(serv_addr));
   // printf("Connect syscall: resultado de connect=%d\n", result);
   if (result < 0)
   {
      perror("Connect syscall: error en connect");
      machine->WriteRegister(2, -1);
   }
   else
   {
      machine->WriteRegister(2, 0); // éxito
      DEBUG('u', "Connect syscall: conexión exitosa\n");
      printf("Connect syscall: conexión exitosa\n");
   }

   // Avanzar el PC SIEMPRE
   machine->WriteRegister(PrevPCReg, machine->ReadRegister(PCReg));
   machine->WriteRegister(PCReg, machine->ReadRegister(NextPCReg));
   machine->WriteRegister(NextPCReg, machine->ReadRegister(NextPCReg) + 4);
}



/*
 *  System call interface: int Bind( Socket_t, int )
 */
void NachOS_Bind() {		// System call 32
}


/*
 *  System call interface: int Listen( Socket_t, int )
 */
void NachOS_Listen() {		// System call 33
}


/*
 *  System call interface: int Accept( Socket_t )
 */
void NachOS_Accept() {		// System call 34
}


/*
 *  System call interface: int Shutdown( Socket_t, int )
 */
void NachOS_Shutdown() {	// System call 25
}

void returnFromSystemCall() {
   int pc = machine->ReadRegister(PCReg);
   machine->WriteRegister(PrevPCReg, pc);
   machine->WriteRegister(PCReg, pc + 4);
   // pc + 4 es la siguiente instrucción
   // pc + 8 es la siguiente instrucción después de la syscall
   machine->WriteRegister(NextPCReg, pc + 8);
}


//----------------------------------------------------------------------
// ExceptionHandler
// 	Entry point into the Nachos kernel.  Called when a user program
//	is executing, and either does a syscall, or generates an addressing
//	or arithmetic exception.
//
// 	For system calls, the following is the calling convention:
//
// 	system call code -- r2
//		arg1 -- r4
//		arg2 -- r5
//		arg3 -- r6
//		arg4 -- r7
//
//	The result of the system call, if any, must be put back into r2. 
//
// And don't forget to increment the pc before returning. (Or else you'll
// loop making the same system call forever!
//
//	"which" is the kind of exception.  The list of possible exceptions 
//	are in machine.h.
//----------------------------------------------------------------------

void
ExceptionHandler(ExceptionType which)
{
    int type = machine->ReadRegister(2);
    

    switch ( which ) {

       case SyscallException:
          switch ( type ) {
             case SC_Halt:		// System call # 0
                NachOS_Halt();
                break;
             case SC_Exit:		// System call # 1
                NachOS_Exit();
                break;
             case SC_Exec:		// System call # 2
               DEBUG('u', "SC_Exit recibido\n");
               NachOS_Exit();
               break;   
            
             case SC_Join:		// System call # 3
                NachOS_Join();
                break;

             case SC_Create:		// System call # 4
                NachOS_Create();
                break;
             case SC_Open:		// System call # 5
                NachOS_Open();
                break;
             case SC_Read:		// System call # 6
                NachOS_Read();
                break;
             case SC_Write:		// System call # 7
                NachOS_Write();
                break;
             case SC_Close:		// System call # 8
                NachOS_Close();
                break;

             case SC_Fork:		// System call # 9
                NachOS_Fork();
                break;
             case SC_Yield:		// System call # 10
                NachOS_Yield();
                break;

             case SC_SemCreate:         // System call # 11
                NachOS_SemCreate();
                break;
             case SC_SemDestroy:        // System call # 12
                NachOS_SemDestroy();
                break;
             case SC_SemSignal:         // System call # 13
                NachOS_SemSignal();
                break;
             case SC_SemWait:           // System call # 14
                NachOS_SemWait();
                break;

             case SC_LckCreate:         // System call # 15
                NachOS_LockCreate();
                break;
             case SC_LckDestroy:        // System call # 16
                NachOS_LockDestroy();
                break;
             case SC_LckAcquire:         // System call # 17
                NachOS_LockAcquire();
                break;
             case SC_LckRelease:           // System call # 18
                NachOS_LockRelease();
                break;

             case SC_CondCreate:         // System call # 19
                NachOS_CondCreate();
                break;
             case SC_CondDestroy:        // System call # 20
                NachOS_CondDestroy();
                break;
             case SC_CondSignal:         // System call # 21
                NachOS_CondSignal();
                break;
             case SC_CondWait:           // System call # 22
                NachOS_CondWait();
                break;
             case SC_CondBroadcast:           // System call # 23
                NachOS_CondBroadcast();
                break;

             case SC_Socket:	// System call # 30
		NachOS_Socket();
               break;
             case SC_Connect:	// System call # 31
		NachOS_Connect();
               break;
             case SC_Bind:	// System call # 32
		NachOS_Bind();
               break;
             case SC_Listen:	// System call # 33
		NachOS_Listen();
               break;
             case SC_Accept:	// System call # 32
		NachOS_Accept();
               break;
             case SC_Shutdown:	// System call # 33
		NachOS_Shutdown();
               break;
             case SC_TraerFigura:	// System call # 34
      NachOS_TraerFigura();
               break;

             default:
                printf("Unexpected syscall exception %d\n", type );
                ASSERT( false );
                break;
          }
          break;

       case PageFaultException: {
          break;
       }

       case ReadOnlyException:
          printf( "Read Only exception (%d)\n", which );
          ASSERT( false );
          break;

       case BusErrorException:
          printf( "Bus error exception (%d)\n", which );
          ASSERT( false );
          break;

       case AddressErrorException:
          printf( "Address error exception (%d)\n", which );
          ASSERT( false );
          break;

       case OverflowException:
          printf( "Overflow exception (%d)\n", which );
          ASSERT( false );
          break;

       case IllegalInstrException:
          printf( "Ilegal instruction exception (%d)\n", which );
          ASSERT( false );
          break;

       default:
          printf( "Unexpected exception %d\n", which );
          ASSERT( false );
          break;
    }

}
