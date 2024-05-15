#include "pin.H"
#include <iostream>


void StrCollBefore(const char* str1, const char* str2) {
	cout << "  [STRCOLL] str1: " << str1 << endl;
	cout << "  [STRCOLL] str2: " << str2 << endl;
}

void StrlenBefore(const char* str) {
	cout << "  [STRLEN] str: " << str << endl;
}

void StrcmpBefore(const char* str1, const char* str2) {
	cout << "  [STRCMP] str1: " << str1 << endl;
	cout << "  [STRCMP] str2: " << str2 << endl;
}

void Image(IMG img, VOID *v)
{

    RTN strlenRtn = RTN_FindByName(img, "strlen");
    if ( RTN_Valid(strlenRtn) ){

        cerr << " Function name: " << RTN_Name(strlenRtn) << endl;

		RTN_Open(strlenRtn);
		
		RTN_InsertCall(strlenRtn, 
			IPOINT_BEFORE, (AFUNPTR) StrlenBefore,
            		IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			IARG_END
		);
			
		RTN_Close(strlenRtn);
		
		
    }
	
    RTN strcmpRtn = RTN_FindByName(img, "strcmp");
    if ( RTN_Valid(strcmpRtn) ){

        cerr << " Function name: " << RTN_Name(strcmpRtn) << endl;
		
		RTN_Open(strcmpRtn);
		
		RTN_InsertCall(strcmpRtn, 
			IPOINT_BEFORE, (AFUNPTR) StrcmpBefore,
            IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			IARG_END
		);
		
		RTN_Close(strcmpRtn);
		
    }
	
	RTN strColl = RTN_FindByName(img, "strcoll");
	if ( RTN_Valid(strColl) ) {
		
		cerr << " Function name: " << RTN_Name(strColl) << endl;
		
		RTN_Open(strColl);
		
		RTN_InsertCall(strColl,
			IPOINT_BEFORE, (AFUNPTR) StrCollBefore,
			IARG_FUNCARG_ENTRYPOINT_VALUE, 0,
			IARG_FUNCARG_ENTRYPOINT_VALUE, 1,
			IARG_END
		);
		
		RTN_Close(strColl);
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

