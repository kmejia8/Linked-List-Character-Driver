/* Author: Karla Mejia

This code is provided here solely for educational and portfolio purposes.  
No permission is granted to copy, modify, or redistribute this code.  

*/

/*
This program acts as the user-space interface for the runners kernel module. It
allows the user to use the module without talking to it directly. It allows the user
to send commands, which will then be parsed and sent to the kernel so it will complete
the desired commands. 

Overall, it acts as a medium between the user and the kernel module.
*/

#include <stdio.h> // for printing
#include <string.h> // for string operations
#include <fcntl.h> // for controlling files
#include <unistd.h>

int main(int argc, char *argv[]) {
    int fd = open("/dev/runners", O_WRONLY); // file descriptor, tells kernel to open driver that was loaded with insmod in write mode

    char buffer[100]; // buffer for commands received from user, allowing for up to 100 characters

    // prints error if device was not able to be opened
    if (fd < 0) {
        perror("Unable to open device.");
        return 1;
    }

    // parses arguments from user
    // arg[0] will be the user-space program (this file)
    snprintf(buffer, sizeof(buffer), "%s", argv[1]); // first argument (at index 1) should be what we're doing to our list

    // past index 2 will be data about the runners, including lane, bib number, name, school, qualifier time, and record time
    for (int i = 2; i < argc; i++) { 
        strcat(buffer, " "); // separates arguments by a space
        strcat(buffer, argv[i]); 
    }

    strcat(buffer, "\n"); // ending buffer

    write(fd, buffer, strlen(buffer)); // copies buffer to kernel, which will be ran by runners_write in the module code

    close(fd); // closes file descriptor
    return 0;
}
