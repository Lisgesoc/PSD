//gsoap wsFilens service name: wsFileServer
//gsoap wsFilens service style: rpc
//gsoap wsFilens service encoding: encoded
//gsoap wsFilens service namespace: urn:wsFilens

/** Length of data buffer */
#define MAX_DATA_SIZE 4094

/** Length for tString */
#define NAME_LENGTH 128

/** Typedef for the file name */
typedef char *xsd__string;

/** Struct that contains a file name */
typedef struct tFileName{
	int __size;	
	xsd__string __ptr;	
}wsFilens__tFileName;

/** Binary data */
typedef struct tData{
	int __size;
	unsigned char *__ptr;
}wsFilens__tData;

int wsFilens__getFileSize (wsFilens__tFileName fileName, int *result);
int wsFilens__createFile (wsFilens__tFileName fileName, int *result);
int wsFilens__writeFile (wsFilens__tFileName fileName, wsFilens__tData data, int *result);
int wsFilens__readFile (wsFilens__tFileName fileName, int offset, int size, wsFilens__tData *data);
