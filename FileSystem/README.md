# Samir Hadi Cisneros
# EXT-2 File System Simulator

This project aims to make an EXT-2 like file system implemented in C. 
It provides basic functionalities to create, read, and manipulate files and directories within the simulated file system.

## Run the code

1. Compile the code using makefile commands:

   ```
   make clean
   make all
   ```

2. Run the executable file:

   ```
   ./fs_sim disk.dat
   ```
   disk.dat is an argument you can use any name for this
   Replace `disk.dat` with any desired name for the disk.

## File Structure

The project consists of two main files:

- `fs_sim.c`: Handles the simulation of the EXT-2 file system.
- `fs.c`: Contains all the implemented functions for file system operations.

## Implemented Functions


### 1. `file_read()`

Reads a specified portion of a file starting from a given offset.
`read <name_of_file> <starting_offset> <size_to_read>`


### 2. `file_remove()`

Deletes a file from the file system.
`rm <name>`

### 3. `hard_link()`

Creates a hard link from a source file to a new file. Should be located in the same directory
`ln <source_file> <new_file>`

### 4. `dir_make()`

Creates a new new sub-directory with the specified name under the current directory.
`mkdir <name>`

### 5. `dir_remove()`

Recursively removes a sub-directory and all of its contents.
`rmdir <name>`

### 8. `dir_change()`

Changes the current directory to the specified directory.
`cd <name>`


## Testing

### Create a File
To create a file named `testFile` with a size of 30 bytes:
```bash
create testFile 30
```

### Read from a File
To read from `testFile` starting from offset 5 and reading 10 bytes:
```bash
read testFile 5 10
```

### Create a Hard Link
To create a hard link named `linkFile` pointing to `testFile`:
```bash
ln testFile linkFile
```

### Create a Directory
To create a directory named `testDir`:
```bash
mkdir testDir
```

### Change Directory
To change to the `testDir` directory:
```bash
cd testDir
```

### Remove a Directory
To remove the `testDir` directory:
```bash
rmdir testDir
```

### checking
Check removed and created files / directories by using the `cd` command and `cat` commands to check. 

## Exiting the Simulator
To exit the file system simulator, type:
```bash
exit
```





