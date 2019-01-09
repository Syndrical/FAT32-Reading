# Compiling
To run the program you must navigate to the folder containing the program, using the cd command to change your current path

## To compile
* If the readfat32.o is not in the current directory, type make to create the object file

## To run
* To run the program, type readfat32 <image path> <command> in order to run the program
Commands: info, list, get

## Commands
* info - Gives info about the formatted drive
   >Example: readfat32 <image path> info
* list - Lists all files in the drive
   >Example: readfat32 <image path> list
* get - Opens given file. Requires a path to the file after get
   >Example: readfat32 <image path> get <path to file>
