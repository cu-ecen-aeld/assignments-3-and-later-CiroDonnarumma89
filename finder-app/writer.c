/*
10) Write a shell script finder-app/writer.sh as described below

    Accepts the following arguments: the first argument is a full path to a file (including filename) on the filesystem, referred to below as writefile; the second argument is a text string which will be written within this file, referred to below as writestr

    Exits with value 1 error and print statements if any of the arguments above were not specified

    Creates a new file with name and path writefile with content writestr, overwriting any existing file and creating the path if it doesn’t exist. Exits with value 1 and error print statement if the file could not be created.

-------------------------------------

3. Write a C application “writer” (finder-app/writer.c)  which can be used as an alternative to the “writer.sh” test script created in assignment1 and using File IO as described in LSP chapter 2.  See the Assignment 1 requirements for the writer.sh test script and these additional instructions:

    One difference from the write.sh instructions in Assignment 1:  You do not need to make your "writer" utility create directories which do not exist.  You can assume the directory is created by the caller.

    Setup syslog logging for your utility using the LOG_USER facility.

    Use the syslog capability to write a message “Writing <string> to <file>” where <string> is the text string written to file (second argument) and <file> is the file created by the script.  This should be written with LOG_DEBUG level.

    Use the syslog capability to log any unexpected errors with LOG_ERR level.

4. Write a Makefile which includes:

    A default target which builds the “writer” application

    A clean target which removes the “writer” application and all .o files

    Support for cross-compilation.  You should be able to generate an application for the native build platform when GNU make variable CROSS_COMPILE is not specified on the make command line.  When CROSS_COMPILE is specified with aarch64-none-linux-gnu- (note the trailing -)your makefile should compile successfully using the cross compiler installed in step 1.


*/


#include <stdio.h>
#include <syslog.h>

int main(int argc, char const *argv[])
{
    FILE* fd = NULL;
    int written_bytes = 0;
    const char* filestr = NULL;
    const char* writestr = NULL;

    openlog("writer", LOG_PID, LOG_USER);
    
    if (argc != 3) {
        syslog(LOG_ERR, "Invalid Number of arguments: %d", argc - 1);
        printf("Usage:\n\r\twriter writefile writestr");
        return 1;
    }

    filestr = argv[1];
    writestr = argv[2];

    syslog(LOG_DEBUG, "Writing %s to %s", writestr, filestr);


    fd = fopen(filestr, "w");

    if (fd == NULL)
    {
        perror(filestr);
        syslog(LOG_ERR, "Unable to create or open file: %s", filestr);
        return 1;
    }

    written_bytes = fprintf(fd, "%s", writestr);
    
    fclose(fd);
    closelog();

    /* code */
    return 0;
}
