#include "pin.H"
#include <iostream>
#include <stdio.h>
#include <list>

typedef struct {
	void* ptr;
	bool allocated;
} ProgramAllocatedAddress;

typedef void* (*FP_MALLOC)(size_t); 		// void* malloc(size_t size);
typedef void  (*FP_FREE)(void*);			// void  free(void* ptr);
static list<ProgramAllocatedAddress> used_addresses;	// Direcciones escritas y ejecutadas

static void* myMalloc(FP_MALLOC originalMalloc, size_t size, CONTEXT* ctx) {

    // Get the return address from where Sleep is called
    ADDRINT* esp = (ADDRINT*) PIN_GetContextReg(ctx, REG_STACK_PTR); // recupera el ESP del contexto
    ADDRINT rtnAddr = *esp;
	
	VOID* ptr = originalMalloc(size);
	fprintf (stdout, " [Malloc @0x%08X] of %d bytes (0x%08X)\n", rtnAddr , size, ptr ); // imprime info
	
	
	// Buscamos si la direccion ha sido reservada en el pasado
	bool found = false;
	for (auto& address : used_addresses) {
		if (address.ptr == ptr) { // Si la localizamos 
			if (address.allocated) { // Se esta reservando dos veces la misma direccion??
				cout << "    !) Double malloc detected?!?!?" << endl;
			
			} else {				// Marcamos como nueva reserva
				address.allocated = true;
				found = true;
			}
		}
	}
	
	if (!found) { // Si al final del bucle no la hemos encontrado, quiere decir que es una no usada hasta ahora
		ProgramAllocatedAddress add;
		add.ptr = ptr; 
		add.allocated = true;

		used_addresses.push_back(add); // La anadimos a la lista
	}
	
	return ptr; // Regresamos el puntero reservado por malloc
}

static void myFree(FP_FREE originalFree, void* ptr, CONTEXT* ctx) {

    // Get the return address from where Sleep is called
    ADDRINT* esp = (ADDRINT*) PIN_GetContextReg(ctx, REG_STACK_PTR); // recupera el ESP del contexto
    ADDRINT rtnAddr = *esp;

    fprintf (stdout, " [Free @0x%08X] (0x%08X). Data: %d\n", rtnAddr , ptr, ((char*)ptr)[0] ); // imprime la direccion de retorno y el puntero

	
	bool found = false;
	for (auto& address : used_addresses) {
        if (address.ptr == ptr) {
			found = true;
			
			if (address.allocated) { // Si estaba reservada
				address.allocated = false;  // la marcamos como liberada
				cout << "   OK) Valid free" << endl;  // avisamos
			
			} else {  // Si estaba liberada
				cout << "    !) Double free detected" << endl; // Avisamos del double free
			}
			break;
		}
    }
	
	// Si la direccion no esta en la lista, quiere decir que no la hemos reservado nosotros (falso positivo).
	if (!found) { cout << "    Falso positivo. Nuestro programa no ha hecho esta reserva" << endl; }
	
	originalFree(ptr);
	
}

// Instrument the malloc() and free() functions.
// note that there are malloc and free in the os loader and in libc
void Image(IMG img, VOID *v)
{
    cerr << "Hooking img: " << IMG_Name(img) << endl;

	// Find the malloc() function and add our function after it
    RTN mallocRtn = RTN_FindByName(img, "malloc");
    if (RTN_Valid(mallocRtn)){

        cerr << "Function name: " << RTN_Name(mallocRtn) << endl;
		
		// void* malloc (size_t size)
		PROTO proto = PROTO_Allocate(PIN_PARG(void*),
            CALLINGSTD_DEFAULT, "malloc",
            PIN_PARG(size_t),
            PIN_PARG_END()
        );
		
		RTN_ReplaceSignature(mallocRtn, (AFUNPTR) myMalloc,
            IARG_PROTOTYPE, proto,       // Use the defined prototype
            IARG_ORIG_FUNCPTR,           // Pass the original function pointer
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,  // Pass the first parameter 
            IARG_CONTEXT,                // Pass the execution context
            IARG_END
        );
		
		PROTO_Free(proto);
    }
	
	
    // Find the free() function and add our function before it
    RTN freeRtn = RTN_FindByName(img, "free");
    if (RTN_Valid(freeRtn)) {

        cerr << "Function name: " << RTN_Name(freeRtn) << endl;

		// void free (void* ptr)
		PROTO proto = PROTO_Allocate(PIN_PARG(void),
            CALLINGSTD_DEFAULT, "free",
            PIN_PARG(void*),
            PIN_PARG_END()
        );
		
		RTN_ReplaceSignature(freeRtn, (AFUNPTR) myFree,
            IARG_PROTOTYPE, proto,       // Use the defined prototype
            IARG_ORIG_FUNCPTR,           // Pass the original function pointer
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,  // Pass the first parameter void* ptr
            IARG_CONTEXT,                // Pass the execution context
            IARG_END
        );
		
		PROTO_Free(proto);
    }
}

int main(int argc, char *argv[]){
    // Initialize pin & symbol manager
    PIN_InitSymbols();
    PIN_Init(argc,argv);

    // Register Image to be called to instrument functions.
    IMG_AddInstrumentFunction(Image, 0);

    PIN_StartProgram(); // Never returns
    
    return 0;
}

