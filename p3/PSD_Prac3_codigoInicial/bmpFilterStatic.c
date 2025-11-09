#include "bmpBlackWhite.h"
#include "mpi.h"
#include <time.h>
#include <fcntl.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <math.h>

/** Show log messages */
#define SHOW_LOG_MESSAGES 1
/** Enable output for filtering information */
#define DEBUG_FILTERING 0
/** Show information of input and output bitmap headers */
#define SHOW_BMP_HEADERS 1

int main(int argc, char** argv) {

    tBitmapFileHeader imgFileHeaderInput;
    tBitmapInfoHeader imgInfoHeaderInput;
    tBitmapFileHeader imgFileHeaderOutput;
    tBitmapInfoHeader imgInfoHeaderOutput;
    char* sourceFileName;
    char* destinationFileName;
    int inputFile, outputFile;
    unsigned char *inputBuffer;       // Buffer con la imagen original
    unsigned char *outputBuffer;      // Buffer temporal para cabeceras
    unsigned int rowSize;             // Tamaño de cada fila en bytes
    unsigned int rowsPerProcess;      // Filas por worker
    unsigned int threshold;           // Umbral
    unsigned int imageHeight, imageWidth;
    unsigned int totalImageBytes;
    double timeStart, timeEnd;
    int size, rank, tag;
    MPI_Status status;

    // Inicialización de MPI
    MPI_Init(&argc, &argv);
    MPI_Comm_size(MPI_COMM_WORLD, &size);
    MPI_Comm_rank(MPI_COMM_WORLD, &rank);
    tag = 1;
    srand(time(NULL));

    // Verificación de número de procesos
    if (size <= 2) {
        if (rank == 0)
            printf("This program must be launched with (at least) 3 processes\n");
        MPI_Finalize();
        exit(0);
    }

    // Verificación de argumentos
    if (argc != 4) {
        if (rank == 0)
            printf("Usage: ./bmpFilterStatic sourceFile destinationFile threshold\n");
        MPI_Finalize();
        exit(0);
    }

    // Argumentos
    sourceFileName = argv[1];
    destinationFileName = argv[2];
    threshold = atoi(argv[3]);

    // ==========================
    // MASTER PROCESS
    // ==========================
    if (rank == 0) {

        timeStart = MPI_Wtime();

        // Leer cabeceras del BMP
        readHeaders(sourceFileName, &imgFileHeaderInput, &imgInfoHeaderInput);
        imgFileHeaderOutput = imgFileHeaderInput;
        imgInfoHeaderOutput = imgInfoHeaderInput;

        // Calcular dimensiones y tamaño de fila
        rowSize = (((imgInfoHeaderInput.biBitCount * imgInfoHeaderInput.biWidth) + 31) / 32) * 4;
        imageWidth = imgInfoHeaderInput.biWidth;
        imageHeight = abs(imgInfoHeaderInput.biHeight);
        totalImageBytes = rowSize * imageHeight;

        // Crear archivo de salida con las cabeceras
        writeHeaders(destinationFileName, &imgFileHeaderOutput, &imgInfoHeaderOutput);

        // Mostrar información opcional
        if (SHOW_BMP_HEADERS) {
            printf("Source BMP headers:\n");
            printBitmapHeaders(&imgFileHeaderInput, &imgInfoHeaderInput);
            printf("Destination BMP headers:\n");
            printBitmapHeaders(&imgFileHeaderOutput, &imgInfoHeaderOutput);
        }

        // Abrir archivo fuente y leer los datos de píxeles
        if ((inputFile = open(sourceFileName, O_RDONLY)) < 0) {
            printf("ERROR: Source file cannot be opened: %s\n", sourceFileName);
            exit(1);
        }

        // Reservar memoria y cargar los píxeles en memoria
        inputBuffer = (unsigned char*) malloc(totalImageBytes);
        if (!inputBuffer) {
            printf("ERROR: Cannot allocate memory for input image\n");
            close(inputFile);
            exit(1);
        }
        lseek(inputFile, imgFileHeaderInput.bfOffBits, SEEK_SET);
        read(inputFile, inputBuffer, totalImageBytes);
        close(inputFile);

        // Distribuir filas entre workers
        int numWorkers = size - 1;
        rowsPerProcess = imageHeight / numWorkers;
        int extra = imageHeight % numWorkers;

        if (SHOW_LOG_MESSAGES)
            printf("Master: numWorkers=%d, rowsPerProcess=%u, extra=%d\n", numWorkers, rowsPerProcess, extra);

        // Enviar a todos los workers: dimensiones y threshold
        int imageDimensions[3] = { imageWidth, imageHeight,imgInfoHeaderInput.biBitCount };
        for (int worker = 1; worker <= numWorkers; worker++) {
            MPI_Send(imageDimensions, 3, MPI_INT, worker, tag, MPI_COMM_WORLD);
            MPI_Send(&threshold, 1, MPI_INT, worker, tag, MPI_COMM_WORLD);
        }

        // Abrir archivo de salida (ya creado con las cabeceras)
        if ((outputFile = open(destinationFileName, O_WRONLY)) < 0) {
            printf("ERROR: Target file cannot be opened for writing: %s\n", destinationFileName);
            exit(1);
        }
       
        // Enviar bloques de filas a cada worker
        unsigned int currentRow = 0;
        for (int worker = 1; worker <= numWorkers; worker++) {
             
            int assignedRows = rowsPerProcess + (worker <= extra ? 1 : 0);
            int bytesToSend = assignedRows * rowSize;
            MPI_Send(&assignedRows, 1, MPI_INT, worker, tag, MPI_COMM_WORLD);
            MPI_Send(inputBuffer + (currentRow * rowSize), bytesToSend, MPI_UNSIGNED_CHAR, worker, tag, MPI_COMM_WORLD);
            if (SHOW_LOG_MESSAGES)
                printf("Master: sent %d rows (%d bytes) to worker %d (rows %u..%u)\n",
                       assignedRows, bytesToSend, worker, currentRow, currentRow + assignedRows - 1);

            currentRow += assignedRows;
            
        }
         

        // Recibir bloques procesados de los workers y escribirlos en el archivo final
        currentRow = 0;
        for (int worker = 1; worker <= numWorkers; worker++) {
    
            int assignedRows = rowsPerProcess + (worker <= extra ? 1 : 0);
            int bytesToRecv = assignedRows * rowSize;

            unsigned char *recvBuffer = (unsigned char*) malloc(bytesToRecv);
            MPI_Recv(recvBuffer, bytesToRecv, MPI_UNSIGNED_CHAR, worker, tag, MPI_COMM_WORLD, &status);

            // Escribir en la posición correspondiente del archivo
            lseek(outputFile, imgFileHeaderOutput.bfOffBits + (off_t)currentRow * rowSize, SEEK_SET);
            write(outputFile, recvBuffer, bytesToRecv);
            free(recvBuffer);

            if (SHOW_LOG_MESSAGES)
                printf("Master: received %d rows (%d bytes) from worker %d and wrote rows %u..%u\n",
                       assignedRows, bytesToRecv, worker, currentRow, currentRow + assignedRows - 1);

            currentRow += assignedRows;
        }

        // Cerrar y liberar
        close(outputFile);
        free(inputBuffer);

        timeEnd = MPI_Wtime();
        printf("\nFiltering time: %f seconds\n", timeEnd - timeStart);
    }

    // ==========================
    // WORKER PROCESS
    // ==========================
    else {
        int imgDimensions[3];
        MPI_Recv(imgDimensions, 3, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
        MPI_Recv(&threshold, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);

        unsigned int imgWidth = imgDimensions[0];
        unsigned int imgHeight = imgDimensions[1];
        unsigned int bitCount = imgDimensions[2];
       

        rowSize = (((bitCount * imgWidth) + 31) / 32) * 4;

        int assignedRows;
        MPI_Recv(&assignedRows, 1, MPI_INT, 0, tag, MPI_COMM_WORLD, &status);
        unsigned int bytesToReceive = assignedRows * rowSize;

        unsigned char *bufferWorker = (unsigned char*) malloc(bytesToReceive);
        MPI_Recv(bufferWorker, bytesToReceive, MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD, &status);

        unsigned char *outputBufferWorker = (unsigned char*) malloc(bytesToReceive);
       // memcpy(outputBufferWorker, bufferWorker, bytesToReceive);
        memset(outputBufferWorker, 0, bytesToReceive);
        // Aplicar el filtrado pixel a pixel
        printf("imageWidth: %u\n", imgWidth);
        for (int r = 0; r < assignedRows; r++) {
            unsigned char *rowIn = bufferWorker + r * rowSize;
            unsigned char *rowOut = outputBufferWorker + r * rowSize;
           // printf("Processing row %d/%d\n", r + 1, assignedRows);

            for (unsigned int c = 0; c < imgWidth; c++) {
                tPixelVector vector;
                unsigned int numPixels = 0;

                if (c > 0) vector[numPixels++] = rowIn[c - 1];
                vector[numPixels++] = rowIn[c];
                if (c + 1 < imgWidth) vector[numPixels++] = rowIn[c + 1];

                unsigned char result = calculatePixelValue(vector, numPixels, threshold, DEBUG_FILTERING);
                rowOut[c] = result;
            }
        }

        // Enviar las filas procesadas al master
        MPI_Send(outputBufferWorker, bytesToReceive, MPI_UNSIGNED_CHAR, 0, tag, MPI_COMM_WORLD);
        printf("aaaaaaaaaaaaaaaaaaaaaa\n");

        free(bufferWorker);
        free(outputBufferWorker);

    }

    MPI_Finalize();
    return 0;
}
