/*=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=|
|==============================================================================================================|
|     /                                                                                                        |
|    /      Name: Kevin Hoang                                                               July 9th, 2018     |                                                   
|   /                                                                                                          |
|  /        This program will read a FAT-32 Formatted Drive, printing the information about it, output the     |
| /         files from it, and be able to fetch and return a file from a drive.                                |
|/                                                                                                             |
|==============================================================================================================|
|=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=~=*/

// Directory Entry - 32 bytes
// Sector - 512 bytes
// Cluster - ^

#include <stdio.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <math.h>

#include "fat32.h"
#include "FSI.h"
#include "dir.h"

#define ULONG unsigned long 
#define BUFFER_SIZE 32

int fileID;      // File reading from
ULONG bps;		// Bytes Per Sector
ULONG spc;	// Sectors Per Cluster
ULONG dps;    // DirEntry per sector
ULONG reservedSec;  // Reserved sector location
ULONG dataSecStart; // Starting point for data sector
ULONG FATSz32;
ULONG bitMask = 0011111111;
unsigned char *buffer;
unsigned long *FAT;
fat32BS bootSector; 
DIR dirStruct;
FSI fsiStruct;

// Functions
ULONG getClusterNum(DIR dirEntry);
ULONG returnFirstSector (ULONG cluster);
void readDir(ULONG clusterNum, int loop);
int readSector(int sectorNum, unsigned char *buffer, int numSectors);
int readEntry(ULONG sectorNum, unsigned char *buffer, ULONG numSectors, ULONG entryNum);
void removeSpaces(char source[], char dest[]);
void readFile(ULONG clusterNum);
int readFAT(ULONG sectorNum, unsigned char *buffer, ULONG offset);
bool findFile(ULONG clusterNum, char *fileName, DIR * entry);
void appendDot(char file[]);


// Main - Main function
int main(int argc, char** argv)
{
    printf("-----------------------------------------------\n");
    int count;
    int numSectors;
    char *input;
    int rootDir_SecNum;

    buffer = malloc(BUFFER_SIZE);
    if (argc > 1) {
        // Open image
        fileID = open(argv[1], O_RDWR);

        // Read first 512 bytes -> boot sector
        count = read(fileID, buffer, 512);
        if (!count) {
            printf("Error (%d) - Boot Sector \n",count);
            exit(0);
        }
        memcpy(&bootSector, buffer, sizeof(fat32BS));
        // Get the boot sector information
        FAT = (unsigned long*)malloc(sizeof(unsigned long) * bootSector.BPB_FATSz32 * bps / 4);
        bps = bootSector.BPB_BytesPerSec;
        spc = bootSector.BPB_SecPerClus;
        numSectors = bps * bootSector.BPB_FATSz32 / 4;
        FATSz32 = bootSector.BPB_FATSz32;
        dps=bps/32;
        reservedSec = bootSector.BPB_RsvdSecCnt;
        rootDir_SecNum = returnFirstSector(bootSector.BPB_RootClus);

        // Read fsi
        readSector(bootSector.BPB_FSInfo, buffer, 1);
        memcpy(&fsiStruct, buffer, sizeof(FSI));

        if (fsiStruct.FSI_TrailSig != 0xAA55) {
            printf("Signatures don't match: 0.%lx\n", fsiStruct.FSI_TrailSig);
        }
        // Calculate
        ULONG freeSpace =  fsiStruct.FSI_Free_Count * spc * bps;

        if (argc == 3 && strcmp(argv[2], "info") == 0)
        {
            printf("Name: %s\n", bootSector.BS_OEMName);
            printf("Free Space: %ld\n", freeSpace);
            printf("Bytes per Sector: %ld\n", bps);
            printf("Sector per Cluster: %ld\n", spc);
            printf("Number of FATs = %d\n", bootSector.BPB_NumFATs);
            printf("Sectors Per FAT = %ld\n", FATSz32);
            printf("Usable Storage: %d\n", (bps/4)*bps*spc);
            printf("Number of Clusters (Sectors): %d\n", numSectors);
            printf("Number of Clusters (Kbs): %d\n", numSectors * bps * spc / 1000);
        }
        else if (argc == 3 && strcmp(argv[2], "list") == 0)
        {
            // Starting sector after FAT
            dataSecStart = bootSector.BPB_RsvdSecCnt + bootSector.BPB_NumFATs * bootSector.BPB_FATSz32;

            // Read root directory
            memset(buffer, 0, BUFFER_SIZE);

            // Recursively go through the entire thing*/
            readDir(bootSector.BPB_RootClus, 0);

        } else if (argc == 4 && strcmp(argv[2], "get") == 0) {
            // Starting sector after FAT
            dataSecStart = bootSector.BPB_RsvdSecCnt + bootSector.BPB_NumFATs * bootSector.BPB_FATSz32;

            // Read root directory
            memset(buffer, 0, BUFFER_SIZE);

            ULONG clusterNum = bootSector.BPB_RootClus;
            bool success;
            bool end;
            DIR * entry = (DIR*)malloc(sizeof(DIR));

            // Break it up into tokens
            input = strtok(argv[3], "/");
            while (input != NULL && !end) {
                printf("Input: %s\n", input);
                
                // Find file in current directory
                success = findFile(clusterNum, input, entry);
                input = strtok(NULL, "/");
                if (success)
                {
                    clusterNum = entry->DIR_FstClusLO;
                    if (entry->DIR_Attr == 32 && input == NULL) {
                        // File found
                        readFile(clusterNum);
                        end = true;
                    } else if (entry->DIR_Attr == 32 && input != NULL) {
                        printf("Invalid file search syntax. \n");
                    }
                    // Continue reading otherwise
                    memset(entry, 0, sizeof(DIR));
                } // if
            }

            if (clusterNum < 0)
            {
                printf("Cannot find path: %s\n", argv[3]);
            }
        }
    }
    printf("-----------------------------------------------\n");
    return 0;
}

int readSector(int sectorNum, unsigned char *buffer, int numSectors) 
{
	int dest, len;     /* used for error checking only */

    // Starting position in sector
	dest = lseek(fileID, sectorNum*bps, SEEK_SET);
	len = read(fileID, buffer, bps*numSectors);

	return len;
}


/*

// With a given directory - Read the files for it
void checkDirectory (DIR dirEntry, unsigned char *buffer) {
    // Find the first sector
    int sectorNum = returnFirstSector(dirEntry.DIR_FstClustLO, dataSecStart);

    readCluster(sectorNum);
}

void readCluster(int sectorNum) {
    // Shift pointer to that sector
    readSector(sectorNum, buffer, spc); 

    // Cast to DIR_Entry
    memcpy(&fsiEntry, buffer+i*sizeof(struct fsiEntry), sizeof(struct fsiEntry));

    // Find the first sector
    int sectorNum = returnFirstSector(dirEntry.DIR_FstClustLO, dataSecStart);

    // Continue looking if available - FIX THE CHECK
    if (sectorNum != 0xfffffff || sectorNum != 0xffffff7 || sectorNum != 0x0) {
        readCluster(sectorNum);
    }

    // Read data

}*/

int readEntry(ULONG sectorNum, unsigned char *buffer, ULONG numSectors, ULONG entryNum) 
{
	int dest, len;     /* used for error checking only */

    // Starting position in entry
	dest = lseek(fileID, sectorNum*bps+(entryNum*32), SEEK_SET);
    lseek(fileID, sectorNum*bps+(entryNum*32), SEEK_SET);

    // Read the entry
	len = read(fileID, buffer, 32);
    //printf("Len: %d\n", len);
	return len;
}
    /*printf("Dest: %d\n", dest);
    printf("Len: %d\n", len);
    printf("Buffer: %p\n", buffer);
    printf("File: %d\n", fileID);
    printf("File Address: %p\n", fileID);*/
/*
    // Get cluster number of dirEntry
    ULONG clusterNum = getClusterNum(dirEntry);
    DIR * currEntry;
    printf("Cluster Num 1: %ld\n", clusterNum);
*/

// Given a directory, read all the files in this directory
void readDir(ULONG clusterNum, int loop) {
    int i;
    int j;
    int k;
    char name[12];
    char nameNoSpaces[12];
    char nameNoSpace[12];
    DIR * currEntry;

    ULONG FATSecOffset = (clusterNum*4);
    ULONG nextCluster;

    // Get base sector
    ULONG baseSector = returnFirstSector(clusterNum);
    //printf("Cluster: %ld\n", clusterNum);
    /*printf("Base Sector 1: %ld\n", baseSector);
    printf("Num DirectEntries per Sector: %d\n", dps);*/
    // Read sector in each cluster
    for (i = 0; i < spc; i++) {
        // Read Dir Entry for each sector
        for (j = 0; j < dps; j++) {
            readEntry(baseSector+i, buffer, spc, j);
            // Read current dirEntry in sector, null terminate the name
            currEntry = (DIR * )buffer;
            memcpy(name, currEntry->DIR_Name, 11);
            name[11] = '\0';

            // Remove space in name
            removeSpaces(name, nameNoSpace);
            
            //memset(nameNoSpace, 0, 12);
            //removeSpaces(name, nameNoSpace);

            if ((currEntry->DIR_Attr == 16 || currEntry->DIR_Attr == 32) && j > 2) {
                printf("|");
                for (k = 0; k < loop*2; k++) {
                    printf("_");
                }

                /*printf("Attribute: %ld\n", currEntry->DIR_Attr);
                printf("Next Cluster Num: %d\n",currEntry->DIR_FstClusLO);
                getchar();*/
                if (currEntry->DIR_Attr == 16) {
                    printf("Directory: %s\n", nameNoSpace);
                    readDir(currEntry->DIR_FstClusLO, loop+1);
                } else if (currEntry->DIR_Attr == 32) {
                    appendDot(nameNoSpace);
                    printf("File: %s\n", nameNoSpace);
                }
            }
        }
    }

    // Insert thing here to check FAT to see if it continues
    //printf("Next cluster chain: %ld", FAT[clusterNum]);
    readFAT(reservedSec, (unsigned char *)FAT, FATSecOffset);
    nextCluster = FAT[0];
    nextCluster = nextCluster & 0x0FFFFFFF;

    /*printf("0x%lx, %ld\n", nextCluster, nextCluster);
        printf("Next cluster: %ld\n", nextCluster);
        printf("Sector Offset: %ld\n", FATSecOffset);*/
    if (nextCluster < 0xffffff7) {
        /*printf("0x%lx, %ld\n", nextCluster, nextCluster);
        printf("Next cluster: %ld\n", nextCluster);
        printf("Sector Number: %ld\n", FATSecNum);
        printf("Sector Offset: %ld\n", FATSecOffset);
        getchar();*/
        readDir(nextCluster, loop);
    } 
}

// Given a directory, find a file/directory in this directory
bool findFile(ULONG clusterNum, char *fileName, DIR *entry) {
    int i;
    int j;
    int k;
    char name[12];
    char nameNoSpace[12];
    DIR * currEntry;
    ULONG foundClusterNum = -1;
    ULONG currentClusterNum = -1;
    bool success = false;
    ULONG FATSecOffset = (clusterNum*4);
    ULONG nextCluster;

    // Get base sector
    ULONG baseSector = returnFirstSector(clusterNum);
    //printf("Cluster: %ld\n", clusterNum);
    /*printf("Base Sector 1: %ld\n", baseSector);
    printf("Num DirectEntries per Sector: %d\n", dps);*/
    // Read sector in each cluster
    for (i = 0; i < spc && !success; i++) {
        // Read Dir Entry for each sector
        for (j = 0; j < dps && !success; j++) {
            readEntry(baseSector+i, buffer, spc, j);
            // Read current dirEntry in sector, null terminate the name
            currEntry = (DIR * )buffer;
            memcpy(name, currEntry->DIR_Name, 11);
            name[11] = '\0';

            // Remove space in name
            removeSpaces(name, nameNoSpace);
            
            // Append dot if file
            if (currEntry->DIR_Attr == 32) {
                appendDot(nameNoSpace);
            }

            // Print whatever is contained in dirEntry
            if ((currEntry->DIR_Attr == 16 || currEntry->DIR_Attr == 32) && j > 2) {
                if (strcasecmp(nameNoSpace, fileName) == 0) {
                    success = true;
                    memcpy(entry, currEntry, sizeof(DIR));
                }
            }
        }
    }

    // See if fat cluster chain continues
    readFAT(reservedSec, (unsigned char *)FAT, FATSecOffset);
    nextCluster = FAT[0];
    nextCluster = nextCluster & 0x0FFFFFFF;

    if (nextCluster < 0xffffff7 && !success) {
        findFile(nextCluster, fileName, entry);
    }

    // Check fat
    return success; 
}

// Given a directory, read all the files in this directory
void readFile(ULONG clusterNum) {
    int i;
    int j;
    int k;
    char name[12];
    char nameNoSpace[12];
    DIR * currEntry;
    ULONG nextCluster;
    ULONG FATSecNum = reservedSec + (clusterNum*4/bps);
    ULONG FATSecOffset = (clusterNum*4);

    // Get base sector
    ULONG baseSector = returnFirstSector(clusterNum);

    
    // Read sector in each cluster
    for (i = 0; i < spc; i++) {
        // Read Dir Entry for each sector
        for (j = 0; j < dps; j++) {
            readEntry(baseSector+i, buffer, spc, j);

            // Read current dirEntry in sector, null terminate the name
            currEntry = (DIR * )buffer;

            // Print file contents
            printf("%s", currEntry->DIR_Name);
        }
    } 

    // See if fat cluster chain continues
    readFAT(reservedSec, (unsigned char *)FAT, FATSecOffset);
    nextCluster = FAT[0];
    nextCluster = nextCluster & 0x0FFFFFFF;

    if (nextCluster < 0xffffff7) {
        readFile(nextCluster);
    }
}

int readFAT(ULONG sectorNum, unsigned char *buffer, ULONG offset) 
{
	int dest, len;     /* used for error checking only */

    // Starting position in entry
	dest = lseek(fileID, sectorNum*bps+offset, SEEK_SET);
    // Read the entry
	len = read(fileID, buffer, 32);
    //printf("Len: %d\n", len);
	return len;
}

// Returns the first sector number for cluster
ULONG returnFirstSector (ULONG cluster) {
    return dataSecStart + (cluster-2) * spc;
} 

// Returns cluster number from direct entry
ULONG getClusterNum(DIR dirEntry){
	return ((dirEntry.DIR_FstClusHI<<16) + dirEntry.DIR_FstClusLO);
}

// Remove spaces from given string
void removeSpaces(char source[], char dest[]) {
    int i;
    int j = 0;

    for (i = 0; i < strlen(source); i++) {
        if (source[i] != ' ') {
            dest[j] = source[i];
            j++;
        }
    }
    dest[j] = '\0';
}

void appendDot(char file[]) {
    int i;
    char temp = '.';
    char temp2;
    for (i = strlen(file)-3; i < strlen(file)+1; i++) {
        temp2 = file[i];
        file[i] = temp;
        temp = temp2;
    }
}