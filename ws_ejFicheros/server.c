#include "soapH.h"
#include "wsFileServer.nsmap"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>


#define DEBUG 1

int calculateFileSize (char* fileName);


int main(int argc, char **argv){ 

  int primarySocket, secondarySocket;
  struct soap soap;

  	if (argc < 2) {
    	printf("Usage: %s <port>\n",argv[0]); 
		exit(-1); 
  	}

	// Init environment
  	soap_init(&soap);

	// Bind 	
	primarySocket=soap_bind(&soap, NULL, atoi(argv[1]), 100);

	// Check result of binding		
	if (primarySocket < 0) {
  		soap_print_fault(&soap, stderr); exit(-1); 
	}

	// Listen to next connection
	while (1) { 

		// accept
	    secondarySocket = soap_accept(&soap);

		if(secondarySocket < 0) {
			soap_print_fault(&soap, stderr); exit(-1);
	  	}

		// Execute invoked operation
			soap_serve(&soap);

		// Clean up!
	  	soap_end(&soap);
	}

  return 0;
}


int calculateFileSize (char* fileName){

	struct stat st;
	int result;

		if (stat(fileName, &st) == -1){
			printf ("[calculateFileSize] Error while executing stat(%s)\n", fileName);
			result = -1;
		}
		else{
			result = st.st_size;
		}

	return(result);
}


int wsFilens__getFileSize (struct soap *soap, wsFilens__tFileName fileName, int *result){

	*result = calculateFileSize (fileName.__ptr);

	if (DEBUG)
		printf ("wsFilens__getFileSize (%s) = %d\n", fileName.__ptr, *result);

	return(SOAP_OK);
}


int wsFilens__createFile (struct soap *soap, wsFilens__tFileName fileName, int *result){

	int fd;

		*result = 0;
			
		// Create file	
		fd = open(fileName.__ptr, (O_WRONLY | O_TRUNC | O_CREAT), 0777);

		// Error?
		if (fd < 0){
			printf ("[wsFilens__createFile] Error while creating(%s)\n", fileName.__ptr);
			*result = -1;
		}
		else
			close (fd);

		if (DEBUG)
			printf ("wsFilens__createFile (%s) = %d\n", fileName.__ptr, *result);


	return (SOAP_OK);
}

int wsFilens__writeFile  (struct soap *soap, wsFilens__tFileName fileName, wsFilens__tData data, int *result){


	int fd, bytesWritten;

	*result = 0;


	// Open file
	fd = open (fileName.__ptr, O_WRONLY | O_APPEND);

	// Error?
	if (fd < 0){
		printf ("[wsFilens__writeFile] Error while opening(%s)\n", fileName.__ptr);
		*result = -1;
	}
	else{


		// Write data
		bytesWritten = write (fd, data.__ptr, data.__size);

		// Error?
		if (bytesWritten < 0){
			printf ("[wsFilens__writeFile] Error while writing(%s)\n", fileName.__ptr);
			*result = -1;
		}

		// Close file
		close (fd);
	}

	return (SOAP_OK);
}
int wsFilens__readFile (struct soap *soap ,wsFilens__tFileName fileName, int offset, int size, wsFilens__tData *data){
	int fd, bytesRead;
	struct stat st;

	// Open file
	fd = open (fileName.__ptr, O_RDONLY);

	// Error?
	if (fd < 0){
		printf ("[wsFilens__readFile] Error while opening(%s)\n", fileName.__ptr);
		return (SOAP_OK);
	}
	else{
		lseek (fd, offset, SEEK_SET);

		// Allocate memory
		data->__ptr = malloc (size);
		data->__size = size;
		// Read data
		bytesRead = read (fd, data->__ptr, size);

		// Error?
		if (bytesRead < 0){
			printf ("[wsFilens__readFile] Error while reading(%s)\n", fileName.__ptr);
		}
		else{
			printf(" Lectura de %d bytes desde el offset %d del archivo %s\n", bytesRead, offset, fileName.__ptr);
			data->__size = bytesRead;
		}
		// Close file
		close (fd);
	}

	return (SOAP_OK);
}
