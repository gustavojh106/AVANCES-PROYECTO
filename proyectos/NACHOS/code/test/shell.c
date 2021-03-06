#include "syscall.h"

#if 0
int main() {


     SpaceId newProc;
     OpenFileId input = ConsoleInput;
     OpenFileId output = ConsoleOutput;
     char prompt[2], ch, buffer[60];
     int i;


     prompt[0] = '-';
     prompt[1] = '-';

     while( 1 ) {
		Write(prompt, 2, output);

		i = 0;
		
		do {	
			Read(&buffer[i], 1, input); 
		} while( buffer[i++] != '\n' );

		buffer[--i] = '\0';

		if( i > 0 ) {
			newProc = Exec(buffer);
			Join(newProc);
		}
     }	
	
	
}
#endif

#if 1
int main() {
	
	char * buffer = (char *)"../test/pingPong";

    int newProc = Exec(buffer);
	Join(newProc);
    char n[1];
    n[0] = newProc + '0';
    char * nP = n;
    
    Write("\ngetting out of the Join after waiting the thread # ", 52, 1); 
    Write(nP, 1, 1);
    Write("\nFINISHING....\n", 15,1); 	
 }
#endif
