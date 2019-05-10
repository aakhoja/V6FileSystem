//Victor Abraham and Aman Khoja 
//CS 4348 Project 2 Part 2 

#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <ctype.h> 
#include <fcntl.h>
#include <string.h>

// Superblock and iNode
typedef struct{
	unsigned short isize;
	unsigned short fsize;
	unsigned short nfree;
	unsigned short free[100];
	unsigned short ninode;
	unsigned short inode[100];
	char flock;
	char ilock;
	char fmod;
	unsigned short time[2];
} superblock;
typedef struct {
	unsigned short flags;
	char nlinks;
	char uid;
	char gid;
	char size0;
	unsigned short size1;
	unsigned short addr[8];
	unsigned short actime[2];
	unsigned short modtime[2];
} inode;

typedef struct {
	unsigned short inode;
	char name[14];
} directoryEntry;

typedef struct {
	unsigned short size;
	unsigned short addresses[100];
} chainBlock;

typedef struct {
	unsigned char contents[512];
} charBlock;

int ckFileType( char * in );
int fileSize( char * in );
int mount( char * path );
inode getInode( int n );
char * toLower(char * str);
void toggletest(char * arg);
int initfs( char * n1, char * n2);
int freeBlock (unsigned short n);
int allocateBlock();
int cpin( char * outsidePath, char * insidePath);
int test;
char * filesystemName[100];
int fd; 
superblock super;

directoryEntry readDirectoryEntry( int block, int n){
	directoryEntry file = { 0, "" };
	fseek(fd, 512 * block + 16*n, SEEK_SET);
	fread(&file, 16, 1, fd);
	return file;
}

charBlock readCharBlock(int n){
	charBlock buffer;
	fseek(fd, 512 * n, SEEK_SET);
	fread(&buffer, 512, 1, fd);
	return buffer;
}

chainBlock readChainBlock(int n){
	if (test){ printf("|Reading Chainblock...\n");}
	chainBlock buffer;
	if (test){ printf("| Seeking to %d (%x)...\n", n, 512*n);}
	fseek(fd, 512 * n, SEEK_SET);
	if (test){ printf("| Freading...\n");}
	int error = fread(&buffer, sizeof(buffer), 1, fd);
	if (test){ printf("| Fread error = %d\n", error);}
	if (test){ printf("|Chainblock size = %d, address[0] = %d\n", buffer.size, buffer.addresses[0]);}

	return buffer;
}

void readSuperblock(){
	fseek(fd, 512, SEEK_SET);
	fread(&super, sizeof(super), 1, fd);
}

void writeChainBlock(int n, chainBlock buffer){
	if(test){printf("|Writing %d bytes to file at block %d (%x)...", sizeof(buffer), n, 512 * n);}
	fseek(fd, 512 * n, SEEK_SET);
	int error = fwrite(&buffer, sizeof(buffer), 1, fd);
	if(test){printf(" fwrite returned: %d\n",error );}

}

void writeCharBlock(int n, charBlock buffer){
	if(test){printf("|Writing %d bytes to file at block %d (%x)...", sizeof(buffer), n, 512 * n);}
	fseek(fd, 512 * n, SEEK_SET);
	int error = fwrite(&buffer, 512, 1, fd);
	if(test){printf(" fwrite returned: %d\n",error );}
}

void writeSuperblock(){
	if(test){printf("|Writing %d bytes to the superblock of file...", sizeof(super));}
	fseek(fd, 512, SEEK_SET);
	int error = fwrite(&super, sizeof(super), 1, fd);
	if(test){printf(" fwrite returned: %d\n",error );}
}

void writeInode( int n, inode node ){
	if(test){printf("|Writing %d bytes to file at block %d (%x)...", sizeof(node), n, 512 * 2 + (n-1) * 32);}
	fseek(fd, 512 * 2 + (n-1) * 32, SEEK_SET);
	int error = fwrite(&node, sizeof(node), 1, fd);
	if(test){printf(" fwrite returned: %d\n",error );}
}

void writeDirectoryEntry( int block, int n, directoryEntry entry){
	if(test){printf("|Writing %d bytes to file at block %d (%x)...", sizeof(entry), block, 512 * block + 16*n);}
	fseek(fd, 512 * block + 16*n, SEEK_SET);
	int error = fwrite(&entry, 16, 1, fd);
	if(test){printf(" fwrite returned: %d\n",error );}
}

int main(int argc, char *argv[]){
	//User Input
	while(1){
		char input[100]; 
		int j;
		for(j=0; j < 100; j++){ input[j] = '\0'; }
		printf(">");
		fflush(stdin);
		int error = fgets(input, 100, stdin);
		input[ strlen(input) - 1 ] = '\0';
		if (strlen(input) != 0){
			char delim[] = " \n";
			int i = 0;
			char * args[20];
      args[i] = toLower(strtok(input, delim));
      while(args[i] != NULL){args[++i]=strtok(NULL,delim);}
			if       (strcmp(args[0],"mount"     )==0){ mount     (args[1]);
			}else if (strcmp(args[0],"ckfiletype")==0){ ckFileType(args[1]);
			}else if (strcmp(args[0],"filesize"  )==0){ fileSize  (args[1]);
			}else if (strcmp(args[0],"initfs"    )==0){ initfs    (args[1], args[2]);
			}else if (strcmp(args[0],"cpin"      )==0){ cpin      (args[1], args[2]);
			}else if (strcmp(args[0],"cpout"     )==0){ cpout     (args[2], args[1]);
			}else if (strcmp(args[0],"mkdir"     )==0){ mkdirectory( args[1] );
			}else if (strcmp(args[0],"rm"        )==0){ removecommand( args[1] );
			}else if (strcmp(args[0],"q"         )==0){
				printf("Program TERMINATED!!\n");
				if (fd != -1){ fflush(fd); fclose(fd); }
				exit(0);
			}else{
				printf(" Invalid command: \"%s\"\n", args[0]);
			}
		}
	}
	return 0;
}

inode getInode( int n ){
	inode node;
	if (fd == -1){
		printf(" Error getting inode: No filesystem mounted.\n");
		return node;
	}
	fseek(fd, 512 * 2 + (n-1) * 32, SEEK_SET);
	fread(&node, 1, sizeof(node), fd);
	return node;
}

char * toLower(char * str){
	int i = 0;
	if (str == NULL){ return str; }
	while( str[i] ) {
		str[i] = (char)tolower(str[i]);
		i++;
	}
	return str;
}


// Part #1
int mount( char * path ){
	if (path == NULL){
		printf(" Error mounting: no path specified\n");
		return -1;
	}
	printf(" Mounting filesytem... ");

	fd = fopen(path, "r+b");

	strcpy( filesystemName, path);

	readSuperblock();
	if (fd == -1){
		printf("Error mounting file: \"%s\". ", path);
		printf("No file mounted.\n");
		return -1;
	}
	printf("filesystem mounted successfully.\n");
	return fd;
}

// ckfiletype
int ckFileType( char * in ){
	if (fd == -1){
		printf(" Error checking file type: no filesystem mounted.\n");
		return -1;
	}
	if (in == NULL){
		printf(" Error checking file type: no inode number provided.\n");
		return -1;
	}

	if (test){ printf("|Finding iNode %s...\n", in); }
	int n = 0;
	if (in){ n = atoi(in); }
	if (n < 1 ){
		printf(" Error checking file type: invalid inode \"%d\".\n", n);
		return -1;
	}

	inode node = (inode) getInode( n );
	if (test){ printf("|Getting flags from iNode...\n"); }
	unsigned short flags = node.flags;
  if (test){ printf("|Read flags as:\n"); }
	if (test){ printf("| dec: %d\n", flags);}
	if (test){ printf("| hex: %x\n", flags);}
	if (test){ printf("| oct: %o\n", flags);}
	int allocated = (flags & (1 << 15)) >> 15;
	if (!allocated){
		printf(" Inode %d: Not Allocated.\n", n);
		return 0;
	}

	int fileType = (flags & (11 << 13)) >> 13;
	printf(" Inode %d:\n", n);
	printf("  File type: ");
	switch (fileType){
		case 0: printf("Plain File\n");
			break;
		case 1: printf("Char-Type Special File\n");
			break;
		case 2: printf("Directory\n");
			break;
		case 3: printf("Block-Type Special File\n");
	}

	int isLarge = (flags & (1 << 12)) >> 12;
	switch( isLarge ){
		case 0: printf("  Small File (<4kB)\n");
			break;
		case 1: printf("  Large File (>4kB)\n");
	}

	fileSize( in );

	return 0;
}

// filesize
int fileSize( char * in ){

	if (fd == -1){
		printf(" Error checking file size: no filesystem mounted.\n");
		return -1;
	}

	if (in == NULL){
		printf(" Error checking file size: no inode number provided.\n");
		return -1;
	}

	if (test){ printf("|Finding iNode %s...\n", in); }
	int m = 0;
	if (in){ m = atoi(in); }
	if (m < 1){
		printf(" Error checking file size: invalid inode \"%d\".\n", m);
		return -1;
	}

	inode node = (inode) getInode( m );
	if (test){ printf("| Size 0 = %d\n", (int)(node.size0)); }
	if (test){ printf("| Size 1 = %d\n", (int)(node.size1)); }

	int size0 = ((int)(node.size0)) << 16;
	int size1 = (int)(node.size1);
	int size = size0 + size1;

	printf(" Size of file: %d bytes\n",size);
}

//****** Part #2
//  a.) initfs n1 n2 
//initfs should accept two arguments:
// (1) number n1 indicating the total number of blocks in the disk (fsize) and
// (2) number n2 representing the total number of blocks that store i-nodes in the disk (isize)
int initfs( char * n1, char * n2){

	if (fd == -1){
		printf(" Error initializing filesystem: no filesystem mounted.\n");
		return -1;
	}
	if (n1 == NULL && n2 == NULL){
		printf(" Error initializing filesystem: arguments invalid.\n");
		return -1;
	}

	fflush(fd);
	fclose(fd);
	fd = fopen( filesystemName, "w+b");

	if (fd == -1){
		printf("Could not clear file \"%s\", also mounted file is closed sorry\n", filesystemName);
		return -1;
	}

	unsigned short numBlocks;
	unsigned short iBlocks;
	if (n1){ numBlocks = atoi(n1); }
	if (n2){ iBlocks   = atoi(n2); }
	if (iBlocks >= numBlocks){
		printf(" Error initializing filesystem: too many iNode blocks.\n");
		return -1;
	}

	super.isize  = iBlocks;
	super.fsize  = numBlocks;
	super.nfree  = 0;
	super.ninode = 100;
	super.time[0] = 2;
	super.time[1] = 2;

	int i;
	for (i = 0; i < 100; i++){
			super.free[i] = 0;
	}

	writeSuperblock();

	for (i = iBlocks + 2; i < numBlocks; i++){
		freeBlock( (unsigned short) i);
	}

	inode node;
	node.flags = 0100000 | 040000;
	node.nlinks = 1;
	node.uid = '\0';
	node.gid = '\0';
	node.size0 = '\0';
	node.size1 = 32;

	node.addr[0] = allocateBlock();
	node.addr[1] = 0;
	node.addr[2] = 0;
	node.addr[3] = 0;
	node.addr[4] = 0;
	node.addr[5] = 0;
	node.addr[6] = 0;
	node.addr[7] = 0;

	if(test){printf("|Allocated block #%d to the root directory.\n", node.addr[0]);}

	directoryEntry autoEntries = {1,"ROOT"};

	if(test){printf("|Writting root directory's contents (.)\n");}
	writeDirectoryEntry( node.addr[0], 0, autoEntries);

	if(test){printf("|Writting root directory's inode.\n");}
	writeInode(1, node);
	fflush(fd);
	fclose(fd);
	fd = fopen( filesystemName, "r+b");
}


//b.) cpin f1 f2
//For this command, f1 is the file that is in the unix machine that you run the fsaccess program. For
//example, if you run this on cs2.utdallas.edu, then f1 is the file that exists in cs2.utdallas.edu
//File f2 is the v6 file that you will create. All file names on the v6 file system are absolute path
//names (starting with /)
//For example, if the command is cpin test.txt /testcopy.txt
//then create file called testcopy.txt in the root directory of the v6 file system and make its contents
//same as contents of the file test.txt of the unix machine where this program is executed. (In our case,
//cs2.utdallas.edu is where test.txt file exists.)
int cpin( char * outsidePath, char * insidePath){
	if (fd == -1){
		printf(" Error copying file: no filesystem mounted.\n");
		return -1;
	}

	if (outsidePath == NULL || insidePath == NULL || strlen(outsidePath) < 1 || strlen(insidePath) < 1){
		printf(" Error copying file: malformed args.\n");
		return -1;
	}

	char delim[] = "/";
	int i = 0;
	char * args[50];
	args[i] = strtok(insidePath, delim);
	while(args[i] != NULL){args[++i]=strtok(NULL,delim);}

	if(test){printf("|Attempting to find: %s\n", insidePath);}

	unsigned short currentAddress = 1;
	inode directory = getInode(1);

	for (i = 0; args[i] != NULL; i++){
		if(test){printf("|Searching for path element: %s\n", args[i]);}
		if (args[i+1] == NULL){
			if(test){printf("|Searching for inode for file: %s\n", args[i]);}
			inode node;
			int l;
			if(test){printf("|Searching through %d inodes in %d blocks.\n", super.isize*16, super.isize);}
			for (l = 1; l < super.isize * 16 + 1; l++){
				node = getInode(l);
				if (! (node.flags & 0100000) ){
					if(test){printf("| Inode %d with flags: %o found.\n", l, node.flags );}
					if(test){printf("|Searching if file already exists ...\n");}
					int j;
					for (j = 0; j < 8; j++){
						if (directory.addr[j] < 1){
							if(test){printf("|Reached end of directory entries.\n");}
							break;
						}
						if(test){printf("|Searching directory entries in block %d\n", directory.addr[j] );}
						int k;
						for (k = 0; k < 32; k++){
							directoryEntry file = readDirectoryEntry( directory.addr[j], k);
							if (test) { printf("| Reading block %d, entry %d (%x) - ", directory.addr[j],k, 512 * directory.addr[j] + 16*k);}
							if (test) { printf("Checking if \"%s\" == \"%s\"\n", args[i], file.name);}
							if (strcmp((char *)file.name, args[i]) == 0){
								printf(" Error making file: file %s already exists.\n", args[i] );
								return 0;
							}
						}
					}
					if(test){printf("|Finding unallocated directory entry...\n");}

					for (j = 0; j < 8; j++){
						if (directory.addr[j] < 1){
							if(test){printf("|Directory data block full, adding new block...\n");}
							directory.addr[j] = allocateBlock();
						}


						int k;
						for (k = 0; k < 32; k++){

							directoryEntry file = readDirectoryEntry( directory.addr[j], k );

							if(test){printf("|Checking file #%d, inode:%d, \"%s\"\n", k, file.inode, file.name);}
							if (file.inode == 0){
								if(test){printf("|Found unallocated directory entry #%d.\n", k);}

								int outsideFile = fopen(outsidePath, "rb");
								fseek(outsideFile, 0, SEEK_END);
								int size = ftell(outsideFile);
								fseek(outsideFile, 0, SEEK_SET);
								if(test){printf("|File to be copied is of size: %d.\n", size);}

								node.flags = 0100000;
								node.size0 = 0;
								node.size1 = size;

								int adderIndex = 0;

								while(size > 0){
									if (adderIndex == 8){
										printf("File too large (>4kB). Truncating rest of file.\n");
										break;
									}


									node.addr[adderIndex] = allocateBlock();
									if(test){printf("|Allocating block #%d.\n", node.addr[adderIndex]);}
									int readSize = 512;

									charBlock block;

									if (size < 512){
										if (test){printf("|Size < 512, changing readsize\n");}
										int m;
										for (m = 0; m < 512; m++){ block.contents[m] = '\0'; }
										readSize = size;
									}

									if(test){printf("|Reading %d bytes from outside file: %d\n", readSize, outsideFile);}

									int bytesRead = fread(&block, 1, readSize, outsideFile);
									if(test){printf("|Read \"%s\" from outside file.\n", block.contents);}
									if(test){printf("|Read block contins %d bytes.\n", sizeof(block.contents));}
									if(test){printf("|writing to inside file...\n");}
									writeCharBlock(node.addr[adderIndex], block);
									if(test){printf("|writing successful.\n");}
									size -= 512;
									adderIndex++;
								}
								fclose(outsideFile);

								if(test){printf("|Writting new directory's inode.\n");}
								writeInode( l, node );

								if(test){printf("|Writting parent directory's entry for new directory...\n");}

								directoryEntry newFile = { l, "" };
								strcpy((char *) newFile.name, args[i]);

								if(test){printf("|Writting new file entry \"%s\", inode:%d\n",newFile.name, newFile.inode );}
								writeDirectoryEntry( directory.addr[j], k, newFile);

								return 1;
							}
						}
					}
					printf(" Error: Directory full.\n");
					return -1;

				}else{
					if(test){printf("| Inode %d with flags: %o not available.\n", l, node.flags );}
				}
			}
			printf(" Error making directory: directory full / no inode available.\n" );
			return 0;
		}else{

			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] > 1){
					int k;
					for (k = 0; k < 512/16; k++){
						directoryEntry file = readDirectoryEntry( directory.addr[j], k);

						if (strcmp(file.name, args[i]) == 0){

							inode newDirectory = getInode(file.inode);
							if (newDirectory.flags & 040000){
								directory = getInode(file.inode);
								currentAddress = file.inode;
								j = 8;
								break;
							}else{
								printf(" %s is not a directory.\n", args[i]);
							}
						}
					}
				}else{
					printf(" No such path in the V6 filesystem:\n ");
					int k;
					for(k = 0; k <=i; k++){
						printf("/%s", args[k]);
					}
					printf("\n");
					return -1;
				}
			}
		}
	}
	printf("\n");
	return 1;
}


// c.) cpout f3 f4
//f3 is the existing v6 file in the v6 file system. The name f3 will always start with / and is an absolute
//path name. File name f4 denotes the file in the unix machine where you run this program. Copy the
//contents of f3 to the newly created file f4.
int cpout( char * outsidePath, char * insidePath){
	if (fd == -1){
		printf(" Error copying file: no filesystem mounted.\n");
		return -1;
	}

	if (outsidePath == NULL || insidePath == NULL || strlen(outsidePath) < 1 || strlen(insidePath) < 1){
		printf(" Error copying file: malformed args.\n");
		return -1;
	}

	char delim[] = "/";
	int i = 0;
	char * args[50];
	args[i] = strtok(insidePath, delim);
	while(args[i] != NULL){args[++i]=strtok(NULL,delim);}

	if(test){printf("|Attempting to find: %s\n", insidePath);}

	unsigned short currentAddress = 1;
	inode directory = getInode(1);

	for (i = 0; args[i] != NULL; i++){
		if(test){printf("|Searching for path element: %s\n", args[i]);}
		if (args[i+1] == NULL){
			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] < 1){
					if(test){printf("|Reached end of directory entries.\n");}
					break;
				}

				if(test){printf("|Searching directory entries in block %d\n", directory.addr[j] );}

				int k;
				for (k = 0; k < 32; k++){
					directoryEntry file = readDirectoryEntry( directory.addr[j], k);
					if (test) { printf("| Reading block %d, entry %d (%x) - ", directory.addr[j],k, 512 * directory.addr[j] + 16*k);}
					if (test) { printf("Checking if \"%s\" == \"%s\"\n", args[i], file.name);}
					if (strcmp((char *)file.name, args[i]) == 0){

						inode node = getInode(file.inode);
						int l;
						int outsideFile = fopen(outsidePath, "w+b");
						int bytesToWrite = node.size1;
						for (l = 0; l < 8; l++){
							if (node.addr[l] > 0 && bytesToWrite > 0){
								charBlock block;
								fseek(fd, 512 * node.addr[l], SEEK_SET);
								int bytesRead = fread(&block, 512, 1, fd);
								if (bytesToWrite < 512){
									fwrite(&block, bytesToWrite, 1, outsideFile);
								}else{
									fwrite(&block, 512, 1, outsideFile);
									bytesToWrite -= bytesRead;
								}

							}
						}
						int error = fclose(outsideFile);
						return 0;
					}
				}
			}
			return 0;
		}else{

			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] > 1){
					int k;
					for (k = 0; k < 512/16; k++){
						directoryEntry file = readDirectoryEntry( directory.addr[j], k);

						if (strcmp(file.name, args[i]) == 0){

							inode newDirectory = getInode(file.inode);
							if (newDirectory.flags & 040000){
								directory = getInode(file.inode);
								currentAddress = file.inode;
								j = 8;
								break;
							}else{
								printf(" %s is not a directory.\n", args[i]);
							}
						}
					}
				}else{
					printf(" No such path in the V6 filesystem:\n ");
					int k;
					for(k = 0; k <=i; k++){
						printf("/%s", args[k]);
					}
					printf("\n");
					return -1;
				}
			}
		}
	}
	printf("\n");
	return 1;
}

///////////////////////////////////////////////////////////////////////////////
//d.) mkdir newdirectory
//create a new directory in the v6 file system. For example if the command is mkdir /user then create
//directory user in the root directory of the v6 file system.
int mkdirectory(char * name){
	if (fd == -1){
		printf(" Error making directory: no filesystem mounted.\n");
		return -1;
	}
	if (strlen(name) < 1){
		printf(" Error making directory: no name given.\n");
		return -1;
	}

	char delim[] = "/";
	int i = 0;
	char * args[50];
	args[i] = strtok(name, delim);
	while(args[i] != NULL){args[++i]=strtok(NULL,delim);}

	if(test){printf("|Attempting to find: %s\n", name);}

	unsigned short currentAddress = 1;
	inode directory = getInode(1);

	for (i = 0; args[i] != NULL; i++){
		if(test){printf("|Searching for path element: %s\n", args[i]);}
		if (args[i+1] == NULL){
			if(test){printf("|Searching for inode for file: %s\n", args[i]);}
			inode node;
			int l;
			if(test){printf("|Searching through %d inodes in %d blocks.\n", super.isize*16, super.isize);}
			for (l = 1; l < super.isize * 16 + 1; l++){
				node = getInode(l);
				if (! (node.flags & 0100000) ){
					if(test){printf("| Inode %d with flags: %o found.\n", l, node.flags );}

					if(test){printf("|Searching if file already exists ...\n");}

					int j;
					for (j = 0; j < 8; j++){
						if (directory.addr[j] < 1){
							if(test){printf("|Reached end of directory entries.\n");}
							break;
						}

						if(test){printf("|Searching directory entries in block %d\n", directory.addr[j] );}

						int k;
						for (k = 0; k < 32; k++){
							directoryEntry file = readDirectoryEntry( directory.addr[j], k);
							if (test) { printf("| Reading block %d, entry %d (%x) - ", directory.addr[j],k, 512 * directory.addr[j] + 16*k);}
							if (test) { printf("Checking if \"%s\" == \"%s\"\n", args[i], file.name);}
							if (strcmp((char *)file.name, args[i]) == 0){
								printf(" Error making directory: file %s already exists.\n", args[i] );
								return 0;
							}
						}
					}
					if(test){printf("|Finding unallocated directory entry...\n");}

					for (j = 0; j < 8; j++){
						if (directory.addr[j] < 1){
							if(test){printf("|Directory data block full, adding new block...\n");}
							directory.addr[j] = allocateBlock();
						}


						int k;
						for (k = 0; k < 32; k++){

							directoryEntry file = readDirectoryEntry( directory.addr[j], k );

							if(test){printf("|Checking file #%d, inode:%d, \"%s\"\n", k, file.inode, file.name);}
							if (file.inode == 0){
								if(test){printf("|Found unallocated directory entry #%d.\n", k);}

								node.flags = 0140000;
								node.size0 = 0;
								node.size1 = 32;
								node.addr[0] = allocateBlock();

								directoryEntry autoEntries[2] = {
									{l,              "." },
									{currentAddress, ".."}
								};

								if(test){printf("|Writting new directory's contents (., ..)\n");}
								writeDirectoryEntry( node.addr[0], 0, autoEntries[0]);
								writeDirectoryEntry( node.addr[0], 1, autoEntries[1]);


								if(test){printf("|Writting new directory's inode.\n");}
								writeInode( l, node );

								if(test){printf("|Writting parent directory's entry for new directory...\n");}

								directoryEntry newFile = { l, "" };
								strcpy((char *) newFile.name, args[i]);

								if(test){printf("|Writting new file entry \"%s\", inode:%d\n",newFile.name, newFile.inode );}

								writeDirectoryEntry( directory.addr[j], k, newFile);

								return 1;
							}
						}
					}
					printf(" Error: Directory full.\n");
					return -1;

				}else{
					if(test){printf("| Inode %d with flags: %o not available.\n", l, node.flags );}
				}
			}
			printf(" Error making directory: directory full / no inode available.\n" );
			return 0;
		}else{

			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] > 1){
					int k;
					for (k = 0; k < 512/16; k++){
						directoryEntry file = readDirectoryEntry( directory.addr[j], k);

						if (strcmp(file.name, args[i]) == 0){

							inode newDirectory = getInode(file.inode);
							if (newDirectory.flags & 040000){
								directory = getInode(file.inode);
								currentAddress = file.inode;
								j = 8;
								break;
							}else{
								printf(" %s is not a directory.\n", args[i]);
							}
						}
					}
				}else{
					printf(" No such path in the V6 filesystem:\n ");
					int k;
					for(k = 0; k <=i; k++){
						printf("/%s", args[k]);
					}
					printf("\n");
					return -1;
				}
			}
		}


	}
	printf("\n");
}

///////////////////////////////////////////////////////////////////////////////
// e.) rm f5 remove the file f5 from the v6 file system and free all the data blocks and free the i-node
int removecommand(char * name){
	if (fd == -1){
		printf(" Error remove directory: no filesystem mounted.\n");
		return -1;
	}
	if (strlen(name) < 1){
		printf(" Error removing directory: no name given.\n");
		return -1;
	}

	char delim[] = "/";
	int i = 0;
	char * args[50];
	args[i] = strtok(name, delim);
	while(args[i] != NULL){args[++i]=strtok(NULL,delim);}


	if(test){printf("|Attempting to find: %s\n", name);}

	unsigned short currentAddress = 1;
	inode directory = getInode(1);

	for (i = 0; args[i] != NULL; i++){
		if(test){printf("|Searching for path element: %s\n", args[i]);}
		if (args[i+1] == NULL){
			if(test){printf("|Searching for inode for file: %s\n", args[i]);}
			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] < 1){
					if(test){printf("|Reached end of directory entries.\n");}
					break;
				}

				if(test){printf("|Searching directory entries in block %d\n", directory.addr[j] );}

				int k;
				for (k = 0; k < 32; k++){
					directoryEntry file = readDirectoryEntry( directory.addr[j], k);
					if (test) { printf("| Reading block %d, entry %d (%x) - ", directory.addr[j],k, 512 * directory.addr[j] + 16*k);}
					if (test) { printf("Checking if \"%s\" == \"%s\"\n", args[i], file.name);}
					if (strcmp((char *)file.name, args[i]) == 0){
						if (test){ printf("|File found in entry %d.\n", k );}
						inode node = getInode(file.inode);

						if (!node.flags & 100000){
							printf(" Error: file's inode does not exist.\n");
							return -1;
						}
						int l;
						if (node.flags & 040000){

							for (l = 0; l < 8; l++){
								if (node.addr[l] > 1){
									int m;
									for (m = 0; m < 32; m++){
											directoryEntry entry = readDirectoryEntry(node.addr[l], m);
											if ( !( (l == 0) && (m<2) ) && (entry.inode != 0)){
												printf(" Error removing file: file is directory that contains files.\n");
												return -1;
											}
									}
								}
							}
						}

						directoryEntry blankFile = {0, ""};
						writeDirectoryEntry( directory.addr[j], k, blankFile);

						for (l = 0; l < 8; l++){
							if (node.addr[l] > 1){
								if(test){printf("|Freeing node address[%d]: %d (%x)\n",l, node.addr[l] ,node.addr[l]*512 );}
								freeBlock( node.addr[l] );
							}
						}

						node.flags = 0;
						writeInode(file.inode, node);

						return 0;
					}
				}
			}
			printf(" Error: File does not exist in directory.\n");
			return -1;
		}else{

			int j;
			for (j = 0; j < 8; j++){
				if (directory.addr[j] > 1){
					int k;
					for (k = 0; k < 512/16; k++){
						directoryEntry file = readDirectoryEntry( directory.addr[j], k);

						if (strcmp(file.name, args[i]) == 0){

							inode newDirectory = getInode(file.inode);
							if (newDirectory.flags & 040000){
								directory = getInode(file.inode);
								currentAddress = file.inode;
								j = 8;
								break;
							}else{
								printf(" %s is not a directory.\n", args[i]);
							}
						}
					}
				}else{
					printf(" No such path in the V6 filesystem:\n ");
					int k;
					for(k = 0; k <=i; k++){
						printf("/%s", args[k]);
					}
					printf("\n");
					return -1;
				}
			}
		}


	}
	printf("\n");

}


int allocateBlock(){
	if (fd == -1){
		printf(" Error allocating block: no filesystem mounted.\n");
		return -1;
	}

	if(test){ printf("|Allocating block...\n");}
	if(test){ printf("| super.nfree = %d\n", super.nfree);}
	if (super.nfree > 0){

		int blockNumber = super.free[super.nfree - 1];
		if(test){ printf("| blockNumber = %d\n", blockNumber);}
		super.nfree--;
		if (super.nfree == 0){
			if(test){ printf("| Reading chainblock at %d (%x)\n", super.free[0], super.free[0] * 512);}
			chainBlock block = readChainBlock(super.free[0]);
			if(test){ printf("| Read Chainblock.\n");}
			// block = readChainBlock(block.addresses[0]);

			if(test){ printf("| Setting super.nfree = %d\n", block.size);}
			super.nfree = block.size;
			if(test){ printf("|  setting addresses...\n");}
			int i;
			for (i = 0; i < super.nfree; i++){
				super.free[i] = block.addresses[i];
			}
			if(test){ printf("|  addresses set.\n");}

		}
		if(test){ printf("| Updating superblock...\n");}
		writeSuperblock();
		if(test){ printf("| Done.\n");}

		charBlock clear;
		int i;
		for (i=0;i<512;i++){clear.contents[i]='\0';}
		writeCharBlock(blockNumber, clear);

		return blockNumber;
	}else{
		printf(" Error allocating block: filesystem full.\n");
		return -1;
	}
}

int freeBlock (unsigned short n){
	if (fd == -1){
		printf(" Error freeing block: no filesystem mounted.\n");
		return -1;
	}
	if ( n <= 1 ){
		printf(" Error freeing block: block reserved.\n");
		return -1;
	}


	if (super.nfree < 100){
		super.free[super.nfree] = n;
		super.nfree++;

		charBlock block;
		int i;
		for (i=0; i < 512; i++){
			block.contents[i] = '\0';
		}
		writeCharBlock(n, block);
		writeSuperblock();
		return 1;
	}else{

		chainBlock block;

		block.size = super.nfree;

		int i;
		for (i = 0; i < super.nfree; i++){
			block.addresses[i] = super.free[i];
		}
		charBlock clear;
		for (i=0;i<512;i++){clear.contents[i]='\0';}
		writeCharBlock(n, clear);
		writeChainBlock(n, block);
		super.nfree = 0;
		super.free[super.nfree] = n;
		super.nfree++;
		writeSuperblock();
		return 1;

	}
}

void toggletest(char * arg){
	if (arg == NULL){
		test = !test;
	}else{
		test = atoi(arg);
	}
	if(test){ printf(" test enabled.\n");
	}else{     printf(" test disabled.\n");}
}

