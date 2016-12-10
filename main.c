#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <string.h>
#include <unistd.h>
#include <sys/time.h>
#include <errno.h>

#define DRIVE ((const char *)"/home/michael/Documents/3207/FileSystem/mFileSystem/Drive10MB")
#define MAX_SIZE 100000

/* Needed functions
char* parsepath(char* path); DONE
struct mF getentry(char* buff, char* parsedpath);
int numblocks(int size);
int firstemptyblock();
char* createnewentry(char* name, char descriptor, int firstemptyblock);
int deleteentry(char* name, int block ?? maybe disk[block]);


*/


int FAT[19435];

#define DELIM "/"
char **parsepath(char *line) {
    int bufsize = 64, position = 0;
    char **tokens = malloc(bufsize * sizeof(char*));
    char *token;

    if (!tokens)
    {
        fprintf(stderr, "allocation error\n");
        exit(EXIT_FAILURE);
    }

    token = strtok(line, DELIM);
    while (token != NULL)
    {
        tokens[position] = token;
        position++;

        if (position >= bufsize)
        {
            bufsize += 64;
            tokens = realloc(tokens, bufsize * sizeof(char*));
            if (!tokens)
            {
                fprintf(stderr, "allocation error2\n");
                exit(EXIT_FAILURE);
            }
        }

        token = strtok(NULL, DELIM);
    }
    tokens[position] = NULL;
    return tokens;
}


typedef struct mFile {
	char bytearray[512];
} disk;

int mcreateFile(char* name, char* path, int size) {
    if (size > MAX_SIZE)
        return -1;

    //Parse Path
    char **parsedpath;
	parsedpath = parsepath(path);
	int i = 0;
	while (parsedpath[i] != NULL)
        printf("parsedpath = %s\n", parsedpath[i++]);

    int length = i-1;

    //mmap
    int fd;
    fd = open(DRIVE, O_RDWR);
    if (fd == -1)
        printf("This is an open error\n");

    off_t off = 102400; //this may not be enough

    disk *dk = (disk*)mmap(NULL, 9897600, PROT_READ | PROT_WRITE, MAP_SHARED, fd, off);
    if (dk == MAP_FAILED) {
        perror ("mmap");
        return -1;
    }

    close(fd);

    //make dummy entry for testing
    strcpy(dk[0].bytearray, "Hello 5\n");
    strcpy(dk[5].bytearray, "this 91\n");
    strcpy(dk[91].bytearray, "is 14\n");
    strcpy(dk[14].bytearray, "a 5647\n");
    strcpy(dk[5647].bytearray, "path 1000\n");

    int block = 0; //root
    for (int i = 0; parsedpath[i] != NULL; i++) {

        //printf("%d\n", i);
        char* match;
        //need exact match, not just contains (probably)

        int n;
        int number;
        if ((match = strstr(dk[block].bytearray, parsedpath[i])) != NULL){
            printf("We need the numeric value!\n");

            n = sscanf(dk[block].bytearray, "%*[^0123456789]%d", &number);
            if (n == -1)
                printf("Sscanf failed!\n");

            printf("%d\n", number);
            block = number;

        }
        else{
            printf("Invalid path!\n");
            break;

        }
        if (i == length) {
            printf("We found the end of the path!!\n");

            int numblocks = size/512 + 1;
            printf("Number of needed blocks: %d\n", numblocks);

            FILE *ptr;

            ptr = fopen(DRIVE,"r+b");
            if (ptr == NULL)
                printf("Fuck\n");

            //find first free block from FAT

            //int FAT[19435];

            ////READ FROM FAT WITH FOPEN, FREAD INTO INT ARRAY
            fread(FAT, 19435, sizeof(int), ptr);


            ////SEARCH ARRAY FOR FIRST EMPTYs
            //FREEBLOCKS HOLDS ALL THE BLOCK NUMBERS THAT WILL BE USED
            int freeblocks[10];
            for (int i = 0; i < 10; i++) {
                freeblocks[i] = -1;
            }

            int z = 0;
            for (z = 0; z < numblocks; z++) {
                i = 0;
                while(FAT[i] != -1) {
                    printf("Checking FAT # %d\n", i);
                    i++;
                }
                FAT[i] = 0;
                freeblocks[z] = i;
                printf("Empty FAT = %d\n", freeblocks[z]);
            }

            ///WRITE USED TO BLOCKS
            ///0 IS END OF BLOCK
            ///# IS NEXT BLOCK POINTER

            if(z == 1) {
                printf("We only need one block!\n");
                FAT[freeblocks[i]] = 0;
            }
            else {
                i = 0;
                while (freeblocks[i+1] != -1) {
                    printf("Assigning %d to FAT %d\n", freeblocks[i+1], freeblocks[i]);
                    FAT[freeblocks[i]] = freeblocks[i+1];
                    i++;
                }
            }


            //WRITE UPDATED FAT BACK TO MEMORY
            fseek(ptr, 0, SEEK_SET);
            int s = fwrite(FAT, sizeof(int), 19435, ptr);
            printf("Number of writes to write FAT: %d\n", s);
            fclose(ptr);


            //write file name TO DIRECTORY (FIRST BLOCK)
            /////ALSO NEED TO WRITE STORAGE BLOCK NEXT TO NAME
            char buffer[10];
            sprintf(buffer, "%d", freeblocks[0]);
            printf("Adding the file with pointer value %d\n", freeblocks[0]);

            strcat(dk[block].bytearray, name);
            strcat(dk[block].bytearray, " ");
            strcat(dk[block].bytearray, buffer);
            strcat(dk[block].bytearray, " ");
            //HEX 96000 here

            printf("We added the directory entry to the directory\n");

            break;
        }
    }

    printf("We're done. Unmapping...\n");

    int r = munmap(dk, 9897600);
    if (r == -1)
        printf("Oh no! %s\n", strerror(errno));

    return 0;
}

//NEED TO RETURN ADDRESS OF FIRST BLOCK OF REQUESTED FILE: USE MMAP POINTER = ADDR
void* mopen (char* path, char* name) {

    char **parsedpath;
	parsedpath = parsepath(path);
	int i = 0;
	while (parsedpath[i] != NULL)
        printf("parsedpath = %s\n", parsedpath[i++]);

    int length = i-1;

//mmap
    int fd;
    fd = open(DRIVE, O_RDWR);
    if (fd == -1)
        printf("This is an open error\n");

    off_t off = 102400; //this may not be enough

    disk *dk = (disk*)mmap(NULL, 9897600, PROT_READ | PROT_WRITE, MAP_SHARED, fd, off);
    if (dk == MAP_FAILED) {
        perror ("mmap");
        return NULL;
    }

    close(fd);

    int block = 0; //root
    for (int i = 0; parsedpath[i] != NULL; i++) {

        //printf("%d\n", i);
        char* match;
        //need exact match, not just contains (probably)

        int n;
        int number;
        if ((match = strstr(dk[block].bytearray, parsedpath[i])) != NULL){
            printf("We need the numeric value!\n");

            n = sscanf(dk[block].bytearray, "%*[^0123456789]%d", &number);
            if (n == -1)
                printf("Sscanf failed!\n");

            printf("%d\n", number);
            block = number;

        }
        else{
            printf("Invalid path!\n");
            break;

        }
    }

    if (i == length)
        return &dk[block];   //NEED BLOCK NUMBER IN CREATEFILE
    else                     //RETURN STRUCT? OR IS IT ENOUGH THAT DK IS THE BEGINNING OF THE ADDRESS
        return NULL;
}

//////...//////
////IF OPEN RETURNS THE MMAP POINTER, THEN THESE OTHER BITCHES SHOULD ALL CALL OPEN
////DOES MMAP PERSIST ACROSS FUNCTION CALLS?
////YES I THINK IT PERSISTS UNTIL PROCESS ENDS
////SO JUST PUT STUFF FROM CREATE FILE IN OPEN FILE. GET RETURN VALUE OF MMAP
////CAN THEN USE OPEN FILE WHENEVER YOU NEED AN MMAP POINTER
////PLUG IT IN AND GO

//DELETE FILE
//int mdelete(char* path, char* name)
/*
    SET THE FAT TABLE ENTRY TO FREE. DELETE FILE NAME AND NUMBER FROM DIRECTORY.
    get mmap pointer
    seek to directory
    open fat table
    update fat table entrry
    write back to memory
    strstr returns first of file name
    set filename and following number all to 0 or -1
*/

int mdelete(char* path, char* name) {
//Parse Path
    char **parsedpath;
	parsedpath = parsepath(path);
	int i = 0;
	while (parsedpath[i] != NULL)
        printf("parsedpath = %s\n", parsedpath[i++]);

    int length = i-1;

    //mmap
    int fd;
    fd = open(DRIVE, O_RDWR);
    if (fd == -1)
        printf("This is an open error\n");

    off_t off = 102400;

    disk *dk = (disk*)mmap(NULL, 9897600, PROT_READ | PROT_WRITE, MAP_SHARED, fd, off);
    if (dk == MAP_FAILED) {
        perror ("mmap");
        return -1;
    }

    close(fd);

    int block = 0; //root
    for (int i = 0; parsedpath[i] != NULL; i++) {

        //printf("%d\n", i);
        char* match;
        //need exact match, not just contains (probably)

        int n;
        int number;
        if ((match = strstr(dk[block].bytearray, parsedpath[i])) != NULL){
            printf("We need the numeric value!\n");

            n = sscanf(dk[block].bytearray, "%*[^0123456789]%d", &number);
            if (n == -1)
                printf("Sscanf failed!\n");

            printf("%d\n", number);
            block = number;

        }
        else{
            printf("Invalid path!\n");
            break;

        }
        if (i == length) {
            printf("We found the end of the path!!\n");

            if ((match = strstr(dk[block].bytearray, name)) != NULL){
                printf("We need the numeric value!\n");

                n = sscanf(dk[block].bytearray, "%*[^0123456789]%d", &number);
                if (n == -1)
                    printf("Sscanf failed!\n");

                printf("%d\n", number);
                block = number;
            }
            else{
                printf("Invalid file name!\n");
                break;

            }

            FILE *ptr;

            ptr = fopen(DRIVE,"r+b");
            if (ptr == NULL)
                printf("Fuck\n");

            //find first free block from FAT

            //int FAT[19435];

            ////READ FROM FAT WITH FOPEN, FREAD INTO INT ARRAY
            fread(FAT, 19435, sizeof(int), ptr);
            fseek(ptr, 0, SEEK_SET);
            printf("FAT[500] = %d\n", FAT[19434]);

            ////ERASE ALL RELEVANT FAT ENTRIES
            int current = block;
            int next;
            while (FAT[current] != 0) {
                next = FAT[current];
                printf("Fat[current] was %d\n", FAT[current]);
                FAT[current] = -1;
                printf("Fat[current] is now %d\n", FAT[current]);
                current = next;
            }
            FAT[current] = -1;

            //WRITE UPDATED FAT BACK TO MEMORY


            int s = fwrite(FAT, sizeof(int), 19435, ptr);

            printf("Number of writes to write FAT: %d\n", s);
            fclose(ptr);

            //WE ARE AT DISK[BLOCK]
            //match is pointer to file name

            printf("WOW\n");
            //delete file name
            //32 is space
            while(*match != 32){
                printf("%c\n", *match);
                *match = 0;
                printf("%c\n", *match);
                match++;
            }
            *match = 0;
            match++;
            printf("DOUBLE WOW\n");
            //delete file number
            while(*match != 32){
                printf("%c\n", *match);
                *match = 0;
                printf("%c\n", *match);
                match++;
            }
            *match = 0;

            printf("TRIPLE WOW\n");

            break;
        }
    }

    printf("We're done. Unmapping...\n");

    int r = munmap(dk, 9897600);
    if (r == -1)
        printf("Oh no! %s\n", strerror(errno));

    return 0;
}

//CLOSE FILE
//int mclose(char* addr)
/*
    CLOSE FILE AT ADDRESS??
*/

//READ FILE
//char* mread(char*addr, char* name)
/*
    READ DATA AT MEMORY BLOCK. ADDRESS OF BLOCK RETURNED FROM OPEN. ACTUALLY YOU DON'T EVEN NEED THAT.
    JUST ASK FOR THE ADDRESS OF THE FILE IN PATH.
*/

//WRITE FILE
//int mwrite(char* path, char* name, char* text)
/*
    JUST FUCKING CONCATENATE TO THE BLOCK. FUCKING SIMPLE.
*/





void initMetaData() {


    FILE *ptr;

    ptr = fopen(DRIVE,"r+b");
    if (ptr == NULL)
        printf("Fuck\n");

    //int FAT[19435];
    char freeTable[19435];

    for (int i = 0; i < 19435; i++) {
        FAT[i] = -1;
        freeTable[i] = 0;
    }
//TESTING PURPOSES
    FAT[0] = 0;
    FAT[1] = 0;
    FAT[2] = 0;
    FAT[3] = 0;
    FAT[4] = 0;
    FAT[5] = 0;
    FAT[91] = 0;
    FAT[14] = 0;
    FAT[5647] = 0;
    FAT[1000] = 0;

    fwrite(FAT, sizeof(int), 19435, ptr);
    fseek(ptr, 77740, SEEK_SET);
    fwrite(freeTable, sizeof(char), 19435, ptr);

    fclose(ptr);
    /*HOW TO REPRESENT FAT TABLE IN DRIVE.
        1) INT ARRAY > 4 BYTES PER INT
            HUGE META DATA ALLOTMENT
            METADATA CANNOT BE MMAPPED (WHICH ISN'T A HUGE DEAL)
                INSTEAD:
                    READ AND WRITE TO METADATA INSTEAD OF MMAP
                    STORE START ADDRESSES OF TABLES AS CONSTANTS IN METADATA


    FREETABLE CAN BE CHAR ARRAY: 0 OR 1


*/


}



int main() {
    int c;
    char path[] = "Hello/this/is/a/path";
    char name[] = "file1.txt";
    c = mcreateFile(name, path, 4000);
    char path2[] = "Hello/this/is";
    char name2[] = "file2.txt";
    c = mcreateFile(name2, path2, 1000);
    char path3[] = "Hello/this/is/a/path";
    char name3[] = "file1.txt";
    c = mdelete(path3, name3);
    char path4[] = "Hello/this/is";
    char name4[] = "file2.txt";
    c = mdelete(path4, name4);
    printf("%d\n", c);
//    initMetaData();
}


