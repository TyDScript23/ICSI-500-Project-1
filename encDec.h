#ifndef ENCDEC_H
#define ENCDEC_H

#include <stddef.h>
#include <string.h>
#include <stdio.h>

#define CHAR_BITS 8


//definition for the readPipe function
ssize_t readPipe(int fd, void* buf, size_t count){
    return read(fd, buf, count);
}

//definition for the writePipe function
ssize_t writePipe(int fd, const void* buf, size_t count){
    return write(fd, buf, count);
}

//converts the input string to bits
char* encodeMessage(const char* inputbuffer)
{
    printf("Our input: %s\n", inputbuffer);
    //the size of the original message
    size_t length = strlen(inputbuffer);
    size_t bitArrayLength = length * CHAR_BITS;
    char* bitArray = (char*)malloc((bitArrayLength + 1) * sizeof(char));
    if (!bitArray) {
        perror("malloc failed");
        return NULL;
    }
    
    int index = 0;
    for (size_t i = 0; i < length; i++) {
        for (int j = CHAR_BITS - 1; j >= 0; j--) {
            bitArray[index++] = (inputbuffer[i] >> j) & 1 ? '1' : '0';
        }
    }
    bitArray[bitArrayLength + 1] = '\0';

    printf("Encode Message: %s\n",bitArray);

    return bitArray;
}

char* decodeMessage(char* encodedMessage) {
    int length = strlen(encodedMessage);

    // Make sure the binary string is a multiple of 8
    if (length % 8 != 0) {
        fprintf(stderr, "Invalid binary string length. Must be a multiple of 8.\n");
        return NULL;
    }

    // Allocate memory for the ASCII string (length / 8 characters + null terminator)
    int asciiLength = length / 8;
    char* asciiString = malloc(asciiLength + 1); // +1 for the null terminator

    if (asciiString == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return NULL;
    }

    // Iterate over the binary string in chunks of 8
    for (int i = 0; i < length; i += 8) {
        char byte[9] = {0}; // Temporary array to hold each byte (8 bits + null terminator)

        // Copy 8 bits into the byte array
        strncpy(byte, encodedMessage + i, 8);

        // Convert the binary string to an integer
        int asciiValue = (int)strtol(byte, NULL, 2);

        // Store the corresponding ASCII character
        asciiString[i / 8] = (char)asciiValue;
    }

    // Null-terminate the ASCII string
    asciiString[asciiLength] = '\0';

    return asciiString;
}

//modify the binary string to have parity bits
char* addParityBits(char* bitArray)
{
    //this is where we will add the parity bit to each character
    for(int i = 0; i < strlen(bitArray); i += 8)
    {
        int count1 = 0; 

        for (int j = i; j < (i + 8); j++)
        {
            if(bitArray[j] == '1')
            {
                count1++;
            }
        }

        if(count1 % 2 == 0)
        {
            bitArray[i] = '1';
        }
    }

    return bitArray;
}

char* removeParityBits(char* bitArray){
    for(int i = 0; i < strlen(bitArray); i += 8)
    {
        bitArray[i] = '0';
    }

    return bitArray;
}

//convert the input bits to frames
char** convertToFrames(char* bitArray)
{
    //do a cealing function for the number of frames
    int numFrames = strlen(bitArray) / 64;

    if(strlen(bitArray) % 64 != 0)
    {
        numFrames += 1;
    }

    //allocate memory for the frameArray and copy the bits to it
    char** frameArray = (char**)malloc(numFrames * sizeof(char*));

    for(int i = 0; i < numFrames; i++){
        frameArray[i] = (char*)malloc(89);
        if(!frameArray[i]){
            perror("malloc failed");
            return NULL;
        }

        //set the first 16 bits of the frame to be "2222"
        strncpy(frameArray[i], "0001011000010110", 17);

        frameArray[i][16] = '\0';
        if((strlen(bitArray) % 64 != 0) && ((i == (numFrames - 1))))
        {
            //We are now handling the last frame
            int remainder = strlen(bitArray) % 64;

            // Convert the remainder to binary
            char binaryString[8];

            binaryString[0] = '\0'; // Initialize the binary string
            int index = 0;

            // Generate binary representation
            for (int i = 7; i >= 0; i--) {
                if (remainder & (1 << i)) {
                    binaryString[index++] = '1';
                } else { // Start adding '0's after the first '1'
                    binaryString[index++] = '0';
                }
            }

            // If no bits were added, the number is 0
            if (index == 0) {
                binaryString[index++] = '0';
            }

            binaryString[index] = '\0';

            strncat(frameArray[i], binaryString, 9);
            frameArray[i][24] = '\0';
        }
        else{ 
            strncat(frameArray[i], "01000000", 9);
            frameArray[i][24] = '\0';
        }

        strncat(frameArray[i], bitArray, 64);

        frameArray[i][88] = '\0';
    }

    //count the number of ones in frameArray
    
    return frameArray;
}

//definition of decodeFrames which decodes each frame received
char* unconvertTheFrames(char frameArray[100][255])
{
    char* originalMessage = malloc(1024);

    if (originalMessage == NULL) {
        fprintf(stderr, "Memory allocation failed\n");
        exit(EXIT_FAILURE);
    }

    originalMessage[0] = '\0';

    int i = 0;

    while(frameArray[i][0] != '\0')
    {
        //fetch the message length section from our frame
        char bitCount[8] = {0};

        for(int j = 16; j < 24; j++) {
            if(frameArray[i][j] == '1') {
                bitCount[j - 16] = '1';
            } else {
                bitCount[j - 16] = '0';
            }
        }

        //convert this binary string into a decimal value

        int decimalValue = 0;
        for (int j = 0; j < strlen(bitCount); j++) {
            // Shift the current decimal value left by 1 (multiply by 2)
            decimalValue <<= 1;

            // Add 1 if the current bit is '1'
            if (bitCount[j] == '1') {
                decimalValue |= 1;
            }
        }

        char messagepiece[1024] = ""; // Initialize messagepiece as an empty string
        int messageIndex = 0; // Index to track position in messagepiece

        for (int j = 0; j < decimalValue; j++) {
            if (frameArray[i][j + 24] == '1') {
                messagepiece[messageIndex++] = '1';
            } else {
                messagepiece[messageIndex++] = '0';
            }
        }

        // Null-terminate messagepiece
        messagepiece[messageIndex] = '\0';

        printf("Our original binary string with parity bits was: %s\n", messagepiece);
        // Check if there's enough space in originalMessage before concatenation
        if (strlen(originalMessage) + strlen(messagepiece) < 1024) {
            strcat(originalMessage, messagepiece); // Concatenate safely
        } else {
            fprintf(stderr, "Original message overflow risk\n");
            free(originalMessage);
            exit(EXIT_FAILURE);
        }

        i++;
    }

    // Print the final decoded message
    printf("Final original message: %s\n", originalMessage);
    
    return originalMessage;
}

//definition for the toLower function that returns a string
int toUpperCase(char* input){
    //read and convert the file contents to lowercase
    pid_t child;
    child = fork();

    if(child < 0){
        fprintf(stderr, "Fork Failed");
        return -1;
    }
    else if(child > 0){
        wait(NULL);
        return -1;
    }
    else{
        //convert the characters to lower case in the child process
        return execl("/usr/bin/tr", "tr", "a-z", "A-Z", (char*)NULL);
    }
}

#endif