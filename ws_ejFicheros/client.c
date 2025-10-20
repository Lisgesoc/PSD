#include "soapH.h"
#include "wsFileServer.nsmap"
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define DEBUG 1

void showError (char* msg){

	printf ("[Client] %s\n", msg);
	exit (1);
}

/**
 * Copy a file from server side to client side
 */
void receiveFile (char *host, char* src, char* dst){
	wsFilens__tFileName fileName;
	fileName.__ptr = src;	
	fileName.__size = strlen (src);
	int size, fd;
	struct soap soap;

	soap_init(&soap);

	soap_call_wsFilens__getFileSize(&soap, host, "", fileName, &size);
	if (soap.error) {
	  	soap_print_fault(&soap, stderr); 
		showError ("Error calling getFileSize");
  	}

	if( size == -1){
		showError ("File does not exist on server");
	}	

	wsFilens__tData data;
	data.__size =0;

	wsFilens__tData data_total;
	data_total.__size =size;
	data_total.__ptr = malloc(size);
	int reciveBytes =0;
	if(size >MAX_DATA_SIZE){
		while(reciveBytes < size){

			if((size - reciveBytes) > MAX_DATA_SIZE){
				data.__size = MAX_DATA_SIZE;
			}else{
				data.__size = size - reciveBytes;
			}
			data.__ptr = malloc (data.__size);
			soap_call_wsFilens__readFile(&soap, host, "", fileName, reciveBytes, data.__size, &data);
			if (soap.error) {
				soap_print_fault(&soap, stderr); showError ("Error calling readFile");
			}
			memcpy(data_total.__ptr + reciveBytes, data.__ptr, data.__size);
			reciveBytes+= data.__size;
			free(data.__ptr);
		}
	}else{
		soap_call_wsFilens__readFile(&soap, host, "", fileName, 0, size, &data_total);
		if (soap.error) {
			soap_print_fault(&soap, stderr); showError ("Error calling readFile");
		}
		printf(" Tamaño del archivo recibido: %d bytes\n", data_total.__size);
	}




	fd = open(dst, O_WRONLY | O_CREAT | O_TRUNC, 0777);
	lseek(fd, 0, SEEK_SET);
	write(fd, data_total.__ptr, data_total.__size);
	close(fd);
    printf(" Archivo recibido exitosamente: %s (%d bytes)\n", src, data_total.__size);
	soap_end(&soap);
	soap_done(&soap);

}


/**
 * Copy a file from client side to server side
 */
void sendFile (char *host, char* src, char* dst){
	wsFilens__tFileName fileName;
	wsFilens__tData data;
	struct stat st;
	int fd, result;

	fileName.__size = strlen (dst);
	fileName.__ptr = malloc(fileName.__size +1);
	memcpy(fileName.__ptr, dst, fileName.__size);

	struct soap soap;

	soap_init(&soap);

	fd=open (src, O_RDONLY);
	lseek (fd, 0, SEEK_SET);
	stat(src, &st);

	data.__ptr = malloc(MAX_DATA_SIZE);

	soap_call_wsFilens__createFile(&soap, host, "", fileName, &result);
	if (soap.error) {
	  	soap_print_fault(&soap, stderr); showError ("Error calling createFile");	

	}

	int readBytes=0;
	if(st.st_size >MAX_DATA_SIZE){
		while(readBytes< st.st_size){
			if((st.st_size - readBytes) > MAX_DATA_SIZE){
				data.__size = MAX_DATA_SIZE;
			}else{
				data.__size = st.st_size - readBytes;
			}
			readBytes += data.__size;
			read (fd, data.__ptr, data.__size);
			
			soap_call_wsFilens__writeFile(&soap, host, "", fileName, data, &result);
		}
	}else{
		data.__size =read (fd, data.__ptr, st.st_size);	
		soap_call_wsFilens__writeFile(&soap, host, "", fileName, data, &result);
	}

	
	if (soap.error) {	
		soap_print_fault(&soap, stderr); 
		showError ("Error calling writeFile");
	}


	close (fd);	
    printf(" Archivo enviado exitosamente: %s (%d bytes)\n", fileName.__ptr, data.__size);
	free(fileName.__ptr);
	free(data.__ptr);
	soap_end(&soap);
	soap_done(&soap);
}
int main(int argc, char **argv){ 
    
  	// Check arguments
	if(argc != 5) {
		printf("Usage: %s http://server:port [sendFile|receiveFile] sourceFile destinationFile\n",argv[0]);
		exit(1);
	}

	// Check mode
	else{

		// Send file to server
		if (strcmp (argv[2], "sendFile") == 0){
			sendFile (argv[1], argv[3], argv[4]);
		}

		// Receive file from server
		else if (strcmp (argv[2], "receiveFile") == 0){
			receiveFile (argv[1], argv[3], argv[4]);
		}

		// Wrong mode!
		else{
			printf("Wrong mode!\nusage: %s serverIP [sendFile|receiveFile] sourceFile destinationFile\n", argv[0]);
			exit(1);
		}
	}	

  return 0;
}

