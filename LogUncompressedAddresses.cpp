#include <iostream>
#include <fstream>
#include <set>
#include <string>
#include "pin.H"

using namespace std;

static set<void*> written_addresses;	// Direcciones escritas
static set<void*> wx_addresses;		// Direcciones ejecutadas despues de escritas
static string prog_name;

/** register_as_written
	Para registrar la effective address usada como argumento 
	dentro de una instruccion de escritura.
**/
VOID register_as_written(void* eff_addr) {
	
	written_addresses.insert(eff_addr);
}

/** check_if_written
	Registra el IP en caso de que este coincida con una 
	direccion que hayamos escrito anteriormente (falso positivo).
**/
VOID check_if_written(void* ip) {
	
	if( written_addresses.count(ip) ) {
		wx_addresses.insert(ip);
	}
}


VOID Fini ( INT32 code , VOID * v ) { 
	
	ofstream ficheroWX;
	
	string filename = "wxlog_" + prog_name + ".txt";
	ficheroWX.open(filename);
	
	ficheroWX << "[i] Program name: " << prog_name << endl;
	ficheroWX << " WX ITEMS: " << wx_addresses.size() << endl;		// Conteo
	
	// Anadimos todas las direcciones
	for ( void* const& addr : wx_addresses ) {
		ficheroWX << " |_ " << addr << endl;
	}
	
	ficheroWX.close();
}

VOID Instruction ( INS ins, VOID * v ) {
	
	// Comprobamos si se ejecuta una zona de memoria que escribimos anteriormente
	INS_InsertPredicatedCall (
		ins, IPOINT_BEFORE , ( AFUNPTR ) check_if_written,
		IARG_INST_PTR,
		IARG_END
	);
	
	// https://github.com/jingpu/pintools/blob/master/source/tools/ManualExamples/pinatrace.cpp
	// Las instrucciones pueden tener cero o varios argumentos de memoria. memOp funciona a modo de indice, permitiendo acceder a todos los argumentos
	UINT32 memOperands = INS_MemoryOperandCount(ins);
	
	for (UINT32 memOp = 0; memOp < memOperands; memOp++) {
		
		if ( INS_MemoryOperandIsWritten(ins, memOp) ) {			// Comprobamos que la instruccion escriba en memoria
			INS_InsertPredicatedCall (
			ins, IPOINT_BEFORE , ( AFUNPTR ) register_as_written,	// La registramos de ser asi
			IARG_MEMORYOP_EA, memOp,
			IARG_END
			);
		}
		
	}
	
}

int main(int argc, char* argv[]) {
	
	prog_name = argv[6];
	
	// Initialize pin
	if ( PIN_Init(argc , argv)) return 1;

	// Register Instruction to be called to instrument instructions
	INS_AddInstrumentFunction(Instruction , 0);
	
	// Register Fini to be called when the application exits
	PIN_AddFiniFunction(Fini , 0);
	
	// Start the program , never returns
	PIN_StartProgram();
	
	return 0;
}
