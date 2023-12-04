#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include "fs.h"
#include "fs_util.h"
#include "disk.h"

char inodeMap[MAX_INODE / 8];
char blockMap[MAX_BLOCK / 8];
Inode inode[MAX_INODE];
SuperBlock superBlock;
Dentry curDir;
int curDirBlock;

int fs_mount(char *name)
{
	int numInodeBlock = (sizeof(Inode) * MAX_INODE) / BLOCK_SIZE;
	int i, index, inode_index = 0;

	// load superblock, inodeMap, blockMap and inodes into the memory
	if (disk_mount(name) == 1)
	{
		disk_read(0, (char *)&superBlock);
		if (superBlock.magicNumber != MAGIC_NUMBER)
		{
			printf("Invalid disk!\n");
			exit(0);
		}
		disk_read(1, inodeMap);
		disk_read(2, blockMap);
		for (i = 0; i < numInodeBlock; i++)
		{
			index = i + 3;
			disk_read(index, (char *)(inode + inode_index));
			inode_index += (BLOCK_SIZE / sizeof(Inode));
		}
		// root directory
		curDirBlock = inode[0].directBlock[0];
		disk_read(curDirBlock, (char *)&curDir);
	}
	else
	{
		// Init file system superblock, inodeMap and blockMap
		superBlock.magicNumber = MAGIC_NUMBER;
		superBlock.freeBlockCount = MAX_BLOCK - (1 + 1 + 1 + numInodeBlock);
		superBlock.freeInodeCount = MAX_INODE;

		// Init inodeMap
		for (i = 0; i < MAX_INODE / 8; i++)
		{
			set_bit(inodeMap, i, 0);
		}
		// Init blockMap
		for (i = 0; i < MAX_BLOCK / 8; i++)
		{
			if (i < (1 + 1 + 1 + numInodeBlock))
				set_bit(blockMap, i, 1);
			else
				set_bit(blockMap, i, 0);
		}
		// Init root dir
		int rootInode = get_free_inode();
		curDirBlock = get_free_block();

		inode[rootInode].type = directory;
		inode[rootInode].owner = 0;
		inode[rootInode].group = 0;
		gettimeofday(&(inode[rootInode].created), NULL);
		gettimeofday(&(inode[rootInode].lastAccess), NULL);
		inode[rootInode].size = 1;
		inode[rootInode].blockCount = 1;
		inode[rootInode].directBlock[0] = curDirBlock;

		curDir.numEntry = 1;
		strncpy(curDir.dentry[0].name, ".", 1);
		curDir.dentry[0].name[1] = '\0';
		curDir.dentry[0].inode = rootInode;
		disk_write(curDirBlock, (char *)&curDir);
	}
	return 0;
}

int fs_umount(char *name)
{
	int numInodeBlock = (sizeof(Inode) * MAX_INODE) / BLOCK_SIZE;
	int i, index, inode_index = 0;
	disk_write(0, (char *)&superBlock);
	disk_write(1, inodeMap);
	disk_write(2, blockMap);

	for (i = 0; i < numInodeBlock; i++)
	{
		index = i + 3;
		disk_write(index, (char *)(inode + inode_index));
		inode_index += (BLOCK_SIZE / sizeof(Inode));
	}
	// current directory
	disk_write(curDirBlock, (char *)&curDir);

	disk_umount(name);
}

int search_cur_dir(char *name)
{
	// return inode. If not exist, return -1
	int i;

	for (i = 0; i < curDir.numEntry; i++)
	{
		if (command(name, curDir.dentry[i].name))
			return curDir.dentry[i].inode;
	}
	return -1;
}

int file_create(char *name, int size)
{
	int i;

	if (size > SMALL_FILE)
	{
		printf("Do not support files larger than %d bytes.\n", SMALL_FILE);
		return -1;
	}

	if (size < 0)
	{
		printf("File create failed: cannot have negative size\n");
		return -1;
	}

	int inodeNum = search_cur_dir(name);
	if (inodeNum >= 0)
	{
		printf("File create failed:  %s exist.\n", name);
		return -1;
	}

	if (curDir.numEntry + 1 > MAX_DIR_ENTRY)
	{
		printf("File create failed: directory is full!\n");
		return -1;
	}

	int numBlock = size / BLOCK_SIZE;
	if (size % BLOCK_SIZE > 0)
		numBlock++;

	if (numBlock > superBlock.freeBlockCount)
	{
		printf("File create failed: data block is full!\n");
		return -1;
	}

	if (superBlock.freeInodeCount < 1)
	{
		printf("File create failed: inode is full!\n");
		return -1;
	}

	char *tmp = (char *)malloc(sizeof(int) * size + 1);

	rand_string(tmp, size);
	printf("New File: %s\n", tmp);

	// get inode and fill it
	inodeNum = get_free_inode();
	if (inodeNum < 0)
	{
		printf("File_create error: not enough inode.\n");
		return -1;
	}

	inode[inodeNum].type = file;
	inode[inodeNum].owner = 1; // pre-defined
	inode[inodeNum].group = 2; // pre-defined
	gettimeofday(&(inode[inodeNum].created), NULL);
	gettimeofday(&(inode[inodeNum].lastAccess), NULL);
	inode[inodeNum].size = size;
	inode[inodeNum].blockCount = numBlock;
	inode[inodeNum].link_count = 1;

	// add a new file into the current directory entry
	strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
	curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
	curDir.dentry[curDir.numEntry].inode = inodeNum;
	printf("curdir %s, name %s\n", curDir.dentry[curDir.numEntry].name, name);
	curDir.numEntry++;

	// get data blocks
	for (i = 0; i < numBlock; i++)
	{
		int block = get_free_block();
		if (block == -1)
		{
			printf("File_create error: get_free_block failed\n");
			return -1;
		}
		// set direct block
		inode[inodeNum].directBlock[i] = block;

		disk_write(block, tmp + (i * BLOCK_SIZE));
	}

	// update last access of current directory
	gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);

	printf("file created: %s, inode %d, size %d\n", name, inodeNum, size);

	free(tmp);
	return 0;
}

int file_cat(char *name)
{
	int inodeNum, i, size;
	char str_buffer[512];
	char *str;

	// get inode
	inodeNum = search_cur_dir(name);
	size = inode[inodeNum].size;

	// check if valid input
	if (inodeNum < 0)
	{
		printf("cat error: file not found\n");
		return -1;
	}
	if (inode[inodeNum].type == directory)
	{
		printf("cat error: cannot read directory\n");
		return -1;
	}

	// allocate str
	str = (char *)malloc(sizeof(char) * (size + 1));
	str[size] = '\0';

	for (i = 0; i < inode[inodeNum].blockCount; i++)
	{
		int block;
		block = inode[inodeNum].directBlock[i];

		disk_read(block, str_buffer);

		if (size >= BLOCK_SIZE)
		{
			memcpy(str + i * BLOCK_SIZE, str_buffer, BLOCK_SIZE);
			size -= BLOCK_SIZE;
		}
		else
		{
			memcpy(str + i * BLOCK_SIZE, str_buffer, size);
		}
	}
	printf("%s\n", str);

	// update lastAccess
	gettimeofday(&(inode[inodeNum].lastAccess), NULL);

	free(str);

	// return success
	return 0;
}

int file_read(char *name, int offset, int size)
{
	// Step 1: Search for the inode number of the file in the current directory
	int inodeNum = search_cur_dir(name);

	// Check if the file exists
	if (inodeNum < 0)
	{
		printf("read error: file not found\n");
		return -1;
	}

	// Check if it is a file (not a directory)
	if (inode[inodeNum].type == directory)
	{
		printf("read error: cannot read a directory\n");
		return -1;
	}

	// Check if the read request is within the file size
	if (offset < 0 || offset >= inode[inodeNum].size || size <= 0)
	{
		printf("read error: invalid offset or size\n");
		return -1;
	}

	// Calculate the starting block index and offset within that block
	int startBlock = offset / BLOCK_SIZE;
	int startOffset = offset % BLOCK_SIZE;

	// Calculate the number of blocks to read
	int numBlocks = size / BLOCK_SIZE;
	if (size % BLOCK_SIZE > 0)
	{
		numBlocks++;
	}

	// Allocate a buffer to store the read data
	char *buffer = (char *)malloc(numBlocks * BLOCK_SIZE);
	if (buffer == NULL)
	{
		printf("read error: memory allocation failed\n");
		return -1;
	}

	// Read each block into the buffer
	for (int i = 0; i < numBlocks; i++)
	{
		int blockIndex = startBlock + i;
		int block = inode[inodeNum].directBlock[blockIndex];

		// Check if the block is valid
		if (block < 0 || block >= MAX_BLOCK)
		{
			printf("read error: invalid block index\n");
			free(buffer);
			return -1;
		}

		// Read the block into the buffer
		disk_read(block, buffer + i * BLOCK_SIZE);
	}

	// Print the read data
	printf("Read data: %.*s\n", size, buffer + startOffset);

	// Update last access time
	gettimeofday(&(inode[inodeNum].lastAccess), NULL);

	// Free the allocated buffer
	free(buffer);

	return 0;
}

int file_stat(char *name)
{
	char timebuf[28];
	int inodeNum = search_cur_dir(name);
	if (inodeNum < 0)
	{
		printf("file cat error: file is not exist.\n");
		return -1;
	}

	printf("Inode\t\t= %d\n", inodeNum);
	if (inode[inodeNum].type == file)
		printf("type\t\t= File\n");
	else
		printf("type\t\t= Directory\n");
	printf("owner\t\t= %d\n", inode[inodeNum].owner);
	printf("group\t\t= %d\n", inode[inodeNum].group);
	printf("size\t\t= %d\n", inode[inodeNum].size);
	printf("link_count\t= %d\n", inode[inodeNum].link_count);
	printf("num of block\t= %d\n", inode[inodeNum].blockCount);
	format_timeval(&(inode[inodeNum].created), timebuf, 28);
	printf("Created time\t= %s\n", timebuf);
	format_timeval(&(inode[inodeNum].lastAccess), timebuf, 28);
	printf("Last acc. time\t= %s\n", timebuf);
}

int file_remove(char *name)
{
	// get inode number
	int inodeNumber = search_cur_dir(name);

	// check for errors (size, offset, type)
	if (inodeNumber == -1)
	{
		// search_cur_dir returns -1 if file not found
		printf("removal error: file does not exist\n");
		return -1;
	}
	if (inode[inodeNumber].type == directory)
	{
		printf("removal error: file_remove cannot remove directory\n");
		return -1;
	}

	// Adjust directory entry array
	int directoryEntry = -1;
	int i;

	for (i = 0; i < curDir.numEntry; i++)
	{
		if (command(name, curDir.dentry[i].name))
		{
			directoryEntry = i;
			break;
		}
	}

	if (directoryEntry == -1)
	{
		printf("removal error: directory entry not found\n");
		return -1;
	}

	// Remove the entry by shifting the remaining entries
	for (i = directoryEntry; i < curDir.numEntry - 1; i++)
	{
		curDir.dentry[i] = curDir.dentry[i + 1];
	}
	curDir.numEntry--;

	// Update last update time for the current directory
	gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);

	// Check hard links and delete the file if link_count == 1
	if (inode[inodeNumber].link_count == 1)
	{
		// Reset inode bitmap and block bitmap
		set_bit(inodeMap, inodeNumber, 0);
		superBlock.freeInodeCount++;
		for (i = 0; i < inode[inodeNumber].blockCount; i++)
		{
			set_bit(blockMap, inode[inodeNumber].directBlock[i], 0);
			superBlock.freeBlockCount++;
		}

		// Reset the inode
		memset(&inode[inodeNumber], 0, sizeof(Inode));
	}
	else
	{
		// If link_count > 1, decrement link_count and return
		inode[inodeNumber].link_count--;

		printf("File %s has multiple hard links. link_count decremented to %d\n", name, inode[inodeNumber].link_count);
		return 0;
	}

	// Write changes to disk
	disk_write(curDirBlock, (char *)&curDir);
	disk_write(1, inodeMap);
	disk_write(2, blockMap);
	disk_write(0, (char *)&superBlock);

	printf("File removed: %s\n", name);
	return 0;
}

int dir_make(char *name)

{
	int i;
	int DirBlock = get_free_block();
	int newInodeNum = search_cur_dir(name);
	Dentry childDir;

	if (newInodeNum >= 0)
	{
		printf("mkdir error: directory with the name '%s' already exists\n", name);
		return -1;
	}

	if (curDir.numEntry >= MAX_DIR_ENTRY)
	{
		printf("mkdir error: directory is full, no space for new entry\n");
		return -1;
	}

	if (superBlock.freeInodeCount < 1)
	{
		printf("mkdir error: no inode available\n");
		return -1;
	}

	newInodeNum = get_free_inode();

	if (newInodeNum < 0)
	{
		printf("mkdir error: no inode available.\n");
		return -1;
	}

	// Initialize the new inode similar to fs_mount function
	inode[newInodeNum].type = directory;
	inode[newInodeNum].owner = 0;
	inode[newInodeNum].group = 0;
	gettimeofday(&(inode[newInodeNum].created), NULL);
	gettimeofday(&(inode[newInodeNum].lastAccess), NULL);
	inode[newInodeNum].size = 1;
	inode[newInodeNum].blockCount = 1;
	inode[newInodeNum].link_count = 1;
	inode[newInodeNum].directBlock[0] = DirBlock;
	childDir.numEntry = 2;

	// Set directory entry for ".."	inside new directory
	strncpy(childDir.dentry[1].name, "..", 2);
	childDir.dentry[1].name[2] = '\0';
	childDir.dentry[1].inode = curDir.dentry[0].inode;

	// Set directory entry for "." inside new directory
	strncpy(childDir.dentry[0].name, ".", 1);
	childDir.dentry[0].name[1] = '\0';
	childDir.dentry[0].inode = newInodeNum;
	disk_write(DirBlock, (char *)&childDir);

	// add the new directory with its name into the current directory
	strncpy(curDir.dentry[curDir.numEntry].name, name, strlen(name));
	curDir.dentry[curDir.numEntry].name[strlen(name)] = '\0';
	curDir.dentry[curDir.numEntry].inode = newInodeNum;
	curDir.numEntry++;
	disk_write(curDirBlock, (char *)&curDir);

	printf("Directory created: %s, inode %d\n", name, newInodeNum);

	return 0;
}

// recursive function to remove directory and its contents.
int dir_remove(char *name)
{
	int i;
	int DirBlock;	  // stores the block index of the directory being removed
	int removalIndex; // index of directory entry to be removed

	// ignore ".", "..", can be dangerous to remove.
	if (strcmp(".", name) == 0 || strcmp("..", name) == 0)
	{
		return -1;
	}

	// base case when current name is a directory
	int inodeNum = search_cur_dir(name);
	if (inodeNum >= 0)
	{

		if (inode[inodeNum].type != directory)
		{
			printf("Not a directory");
			return -1;
		}

		// free the block that is being removed
		DirBlock = inode[inodeNum].directBlock[0];
		set_bit(blockMap, DirBlock, 0);
		superBlock.freeBlockCount++;

		// free the inode that is being removed
		set_bit(inodeMap, inodeNum, 0);
		superBlock.freeInodeCount++;

		Dentry childDir;
		disk_read(DirBlock, (char *)&childDir);

		// Iterate through the entries in the directory
		for (i = 0; i < childDir.numEntry; i++)
		{
			int childInode = childDir.dentry[i].inode;

			// recursively remove subdirectories and files
			if (inode[childInode].type == directory)
			{
				dir_remove(childDir.dentry[i].name);
			}
			else
			{
				file_remove(childDir.dentry[i].name);
			}
		}

		// Clear directory entry array
		childDir.numEntry = 0;

		// Write changes to disk
		disk_write(DirBlock, (char *)&childDir);

		// Find the index of the directory entry in the current directory
		for (i = 0; i < curDir.numEntry; i++)
		{

			if (strcmp(curDir.dentry[i].name, name) == 0)
			{
				removalIndex = i;
				break;
			}
		}

		// Remove the entry by shifting the other entries
		for (i = removalIndex; i < curDir.numEntry - 1; i++)
		{
			curDir.dentry[i] = curDir.dentry[i + 1];
		}
		curDir.numEntry--;
		disk_write(curDirBlock, (char *)&curDir);
		printf("%s directory removed successfully", name);

		return 0;
	}
	else
	{
		printf("Directory not found");
		return -1;
	}
}

int dir_change(char *name)
{
	// Search for the inode of the specified directory in the current directory
	int targetInode = search_cur_dir(name);

	if (targetInode < 0)
	{
		// Directory not found
		printf("cd error: directory '%s' not found\n", name);
		return -1;
	}

	// Check if the target inode represents a directory
	if (inode[targetInode].type != directory)
	{
		printf("cd error: '%s' is not a directory\n", name);
		return -1;
	}

	// Update the current directory to the specified directory
	curDirBlock = inode[targetInode].directBlock[0];
	disk_read(curDirBlock, (char *)&curDir);

	// Update last access time for the current directory
	gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);

	printf("Changed directory to: %s\n", name);
	return 0;
}

int ls()
{
	int i;
	for (i = 0; i < curDir.numEntry; i++)
	{
		int n = curDir.dentry[i].inode;
		if (inode[n].type == file)
			printf("type: file, ");
		else
			printf("type: dir, ");
		printf("name \"%s\", inode %d, size %d byte\n", curDir.dentry[i].name, curDir.dentry[i].inode, inode[n].size);
	}

	return 0;
}

int fs_stat()
{
	printf("File System Status: \n");
	printf("# of free blocks: %d (%d bytes), # of free inodes: %d\n", superBlock.freeBlockCount, superBlock.freeBlockCount * 512, superBlock.freeInodeCount);
}

int hard_link(char *src, char *dest)
{
	// search currrent directory to see that file exists
	int sourceInode = search_cur_dir(src);
	if (sourceInode < 0)
	{
		printf("hard link error: source file not found\n");
		return -1;
	}

	// make sure there isn't an existing file that corrresponds to the new file so they don't overwrite
	int newFileInode = search_cur_dir(dest);
	if (newFileInode >= 0)
	{
		printf("hard link error: new file already exists\n");
		return -1;
	}

	// make sure there is space in the current directory entry (curDir) for the new entry.
	if (curDir.numEntry >= MAX_DIR_ENTRY)
	{
		printf("hard link error: directory is full, no space for new entry\n");
		return -1;
	}

	// Add a new dentry for the new file in the current directory.
	strncpy(curDir.dentry[curDir.numEntry].name, dest, strlen(dest));
	curDir.dentry[curDir.numEntry].name[strlen(dest)] = '\0';
	curDir.dentry[curDir.numEntry].inode = sourceInode;
	curDir.numEntry++;

	// Increment the link_count of the inode associated with the source file.
	inode[sourceInode].link_count++;

	// Update last access time for the current directory
	gettimeofday(&(inode[curDir.dentry[0].inode].lastAccess), NULL);

	// Write changes to disk
	disk_write(curDirBlock, (char *)&curDir);
	disk_write(1, inodeMap);
	disk_write(0, (char *)&superBlock);

	printf("Hard link created: %s -> %s\n", dest, src);

	return 0;
}

int execute_command(char *comm, char *arg1, char *arg2, char *arg3, char *arg4, int numArg)
{

	printf("\n");
	if (command(comm, "df"))
	{
		return fs_stat();

		// file command start
	}
	else if (command(comm, "create"))
	{
		if (numArg < 2)
		{
			printf("error: create <filename> <size>\n");
			return -1;
		}
		return file_create(arg1, atoi(arg2)); // (filename, size)
	}
	else if (command(comm, "stat"))
	{
		if (numArg < 1)
		{
			printf("error: stat <filename>\n");
			return -1;
		}
		return file_stat(arg1); //(filename)
	}
	else if (command(comm, "cat"))
	{
		if (numArg < 1)
		{
			printf("error: cat <filename>\n");
			return -1;
		}
		return file_cat(arg1); // file_cat(filename)
	}
	else if (command(comm, "read"))
	{
		if (numArg < 3)
		{
			printf("error: read <filename> <offset> <size>\n");
			return -1;
		}
		return file_read(arg1, atoi(arg2), atoi(arg3)); // file_read(filename, offset, size);
	}
	else if (command(comm, "rm"))
	{
		if (numArg < 1)
		{
			printf("error: rm <filename>\n");
			return -1;
		}
		return file_remove(arg1); //(filename)
	}
	else if (command(comm, "ln"))
	{
		return hard_link(arg1, arg2); // hard link. arg1: src file or dir, arg2: destination file or dir

		// directory command start
	}
	else if (command(comm, "ls"))
	{
		return ls();
	}
	else if (command(comm, "mkdir"))
	{
		if (numArg < 1)
		{
			printf("error: mkdir <dirname>\n");
			return -1;
		}
		return dir_make(arg1); // (dirname)
	}
	else if (command(comm, "rmdir"))
	{
		if (numArg < 1)
		{
			printf("error: rmdir <dirname>\n");
			return -1;
		}
		return dir_remove(arg1); // (dirname)
	}
	else if (command(comm, "cd"))
	{
		if (numArg < 1)
		{
			printf("error: cd <dirname>\n");
			return -1;
		}
		return dir_change(arg1); // (dirname)
	}
	else
	{
		fprintf(stderr, "%s: command not found.\n", comm);
		return -1;
	}
	return 0;
}
