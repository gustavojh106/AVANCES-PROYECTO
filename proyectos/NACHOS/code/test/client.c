#include "syscall.h"


int main()
{
    int idSocket = Socket(2, 1);
    char debug[2];
    debug[0] = idSocket + '0';
    debug[1] = '\n';

    const char * c = (char *)"163.178.104.187";
    //const char * c = (char *)"127.0.0.1";
    
    int result = Connect(idSocket, c, 80);

    const char * req = (char *)"GET / HTTP/1.1\r\nhost: redes.ecci\r\n\r\n";
    Write(req, 36, idSocket);

    char buff[512];
    Read(buff, 512, idSocket);

    Write(buff, 512, 1);

}