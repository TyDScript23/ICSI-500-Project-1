#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <signal.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <fcntl.h>
#include "encDec.h"

int pipe1[2];
int pipe2[2];

FILE* originalFile;
FILE* originalFile2;
FILE* binaryFileIn;
FILE* binaryFileOut;
FILE* upperCaseFile;
FILE* finishingFile; 
FILE* inputFile;

void producerConsumerChild();
void consumerProducerParent();

int main(){

    //initialize the children
    pid_t child1;

    //create the pipes and check to see if they failed
    if (pipe(pipe1) < 0 || pipe(pipe2) < 0){
        fprintf(stderr, "Pipe Failed");
        return 1;
    }

    //create the first child
    child1 = fork();

    //check the first child process
    if (child1 < 0){
        fprintf(stderr, "Fork Failed");
        return 1;
    }
    else if(child1 > 0){
        consumerProducerParent();
        wait(NULL);
    }
    else{ 
        producerConsumerChild();
    }

    return 0;
}

//defintion for a void function
void consumerProducerParent(){

        printf("Consumer-Producer: Parent Process\n");

        //close the unused ends of the pipes
        close(pipe1[1]);
        close(pipe2[0]);

        //save to revert standard output later
        int saved_stdout = dup(fileno(stdout));

        if (saved_stdout == -1) {
            perror("dup");
            exit(EXIT_FAILURE);
        }

        //save to revert standard input later
        int saved_stdin = dup(fileno(stdin));

        if (saved_stdin == -1) {
            perror("dup");
            exit(EXIT_FAILURE);
        }

        //--------------------------------------------------------------------------------------------------
        //parent is receiving the data from the child
        char catchFrames[100][255];

        int i = 0;

        while(readPipe(pipe1[0], catchFrames[i], sizeof(catchFrames[i])) > 0)
        {
            catchFrames[i][254] = '\0';
            i++;
        }

        close(pipe1[0]);

        //now let's decode each frame using a function 
        char* binaryWithParityBits = unconvertTheFrames(catchFrames);

        //and remove the parity bits
        char* binaryMessage = removeParityBits(binaryWithParityBits); 

        //and restore the old message back to its normal self
        char* ogmessage = decodeMessage(binaryMessage); 
        
        printf("Our original message is: %s\n", ogmessage);
        
        //---------------------------------------------------------------------------------------------
        //  OUTPUT FILE
        //put this message in an output file
        originalFile = fopen("output.txt", "r");

        if(originalFile == NULL){
            fprintf(stderr, "Error opening file");
            exit(EXIT_FAILURE);
        }

        int write1 = fwrite(ogmessage, sizeof(char), strlen(ogmessage), originalFile);

        if(write1 < 0)
        {
            fprintf(stderr, "Error writing to file");
            exit(EXIT_FAILURE);
        }

        //set the standard input file to the original file
        if (dup2(fileno(originalFile), STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }
        
        fflush(stdin);
        fclose(originalFile);

        //-----------------------------------------------------------------------------------
        // UPPERCASE FILE

        upperCaseFile = fopen("upperCase.txt", "a");

        if (upperCaseFile == NULL) {
            fprintf(stderr, "Error opening upperCase.txt");
            exit(EXIT_FAILURE);
        }

        // Redirect stdout to the upperCase file
        if (dup2(fileno(upperCaseFile), STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        toUpperCase("output.txt");
        fflush(upperCaseFile);
        // Close upperCaseFile and restore stdout
        fclose(upperCaseFile);

        // Restore stdout
        if (dup2(saved_stdout, STDOUT_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(saved_stdout);  // Close saved_stdout after restoring

        //restore the original stdin (back to the console)
        if (dup2(saved_stdin, STDIN_FILENO) == -1) {
            perror("dup2");
            exit(EXIT_FAILURE);
        }

        close(saved_stdin);

        //we are now opening the input file
        upperCaseFile = fopen("upperCase.txt", "r");

        //check if the file opened successfully
        if(upperCaseFile == NULL){
            fprintf(stderr, "Error opening file");
            exit(EXIT_FAILURE);
        }

        // Move to the end of the file to get the size
        fseek(upperCaseFile, 0, SEEK_END);
        //get the size of the file
        long fileSize2 = ftell(upperCaseFile);
        fseek(upperCaseFile, 0, SEEK_SET); // Reset to the beginning of the file

        printf("The file size is: %ld", fileSize2);

        char* buffer2 = malloc(fileSize2 + 1);  // Allocate buffer dynamically to match file size
        if (buffer2 == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        fread(buffer2, sizeof(char), fileSize2, upperCaseFile);
        buffer2[fileSize2] = '\0';  // Null-terminate the string

        fclose(upperCaseFile);

        printf("Buffer2: %s\n", buffer2);

        //use a function to covert the buffer string to an array of bits
        char* bitArray2 = encodeMessage(buffer2);

        //write bitArray to a file
        binaryFileOut = fopen("filename.chck", "w");

        if(binaryFileOut == NULL){
            fprintf(stderr, "Error opening file");
            exit(EXIT_FAILURE);
        }

        int write2 = fwrite(bitArray2, sizeof(char), strlen(bitArray2), binaryFileOut);

        if(write2 < 0)
        {
            fprintf(stderr, "Error writing to file");
            exit(EXIT_FAILURE);
        }

        fclose(binaryFileOut);

        //add the parity bits to the file
        char* bitArrayWithParity2 = addParityBits(bitArray2);
    
        //split the modified bitArray into frames
        char** frameArray2 = convertToFrames(bitArrayWithParity2);

        for(int i = 0; i < sizeof(frameArray2); i++)
        {
            printf("Frame %d: %s\n", i+1, frameArray2[i]);
            writePipe(pipe2[1], frameArray2[i], strlen(frameArray2[i]));
        }
        
        close(pipe2[1]);
}

void producerConsumerChild(){
        printf("Consumer-Producer: Child Process\n");
        //close the unused ends of the pipes
        close(pipe1[0]);
        close(pipe2[1]);

        //---------------------------------------------------------------------------
        //THE INPUT FILE

        //we are now opening the input file
        inputFile = fopen("filename.inpf", "r");

        //check if the file opened successfully
        if(inputFile == NULL){
            fprintf(stderr, "Error opening file");
            exit(EXIT_FAILURE);
        }

        // Move to the end of the file to get the size
        fseek(inputFile, 0, SEEK_END);
        long fileSize = ftell(inputFile);
        fseek(inputFile, 0, SEEK_SET); // Reset to the beginning of the file

        printf("The file size is %ld", fileSize);

        char* buffer = malloc(fileSize + 1);  // Allocate buffer dynamically to match file size
        if (buffer == NULL) {
            fprintf(stderr, "Memory allocation failed\n");
            exit(EXIT_FAILURE);
        }

        fread(buffer, sizeof(char), fileSize, inputFile);
        buffer[fileSize] = '\0';  // Null-terminate the string

        //use a function to covert the buffer string to an array of bits
        char* bitArray = encodeMessage(buffer);

        //write bitArray to a file
        binaryFileIn = fopen("filename.binf", "w");

        if(binaryFileIn == NULL){
            fprintf(stderr, "Error opening file");
            exit(EXIT_FAILURE);
        }

        int write3 = fwrite(bitArray, sizeof(char), strlen(bitArray), binaryFileIn);
        
        if(write3 < 0)
        {
            fprintf(stderr, "Error writing to file");
            exit(EXIT_FAILURE);
        }

        fclose(binaryFileIn);

        //add the parity bits to the file
        char* bitArrayWithParity = addParityBits(bitArray);
    
        //split the modified bitArray into frames
        char** frameArray = convertToFrames(bitArrayWithParity);

        for(int i = 0; i < sizeof(frameArray); i++)
        {
            printf("Frame %d: %s\n", i+1, frameArray[i]);
            writePipe(pipe1[1], frameArray[i], strlen(frameArray[i]));
        }

        fclose(inputFile);
        close(pipe1[1]);

        //-----------------------------------------------------------------------------------------------

        //parent is receiving the data from the child
        char catchFrames2[100][255];

        printf("We have reached");

        int i = 0;

        while(readPipe(pipe2[0], catchFrames2[i], sizeof(catchFrames2[i])) > 0)
        {
            catchFrames2[i][254] = '\0';
            i++;
        }

        //close the read end of the pipe
        close(pipe2[0]);


        //now let's decode each frame using a function 
        char* binaryWithParityBits2 = unconvertTheFrames(catchFrames2);

        //and remove the parity bits
        char* binaryMessage2 = removeParityBits(binaryWithParityBits2); 

        //and restore the old message back to its normal self
        char* ogmessage2 = decodeMessage(binaryMessage2); 
        
        printf("Our original message sent back over is: %s\n", ogmessage2);

        //put this message in an output file
        originalFile2 = fopen("output2.txt", "w");

        if(originalFile2 == NULL){
            fprintf(stderr, "Error opening file");
            exit(EXIT_FAILURE);
        }

        int write4 = fwrite(ogmessage2, sizeof(char), strlen(ogmessage2), originalFile2);

        if(write4 < 0)
        {
            fprintf(stderr, "Error writing to file");
            exit(EXIT_FAILURE);
        }

        fclose(originalFile2);

        //write bitArray to a file
        finishingFile = fopen("filename.done", "w");

        if(finishingFile == NULL){
            fprintf(stderr, "Error opening file");
            exit(EXIT_FAILURE);
        }

        fwrite(ogmessage2, sizeof(char), strlen(ogmessage2), finishingFile);
        fclose(finishingFile);
}