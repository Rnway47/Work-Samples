# Objective: Build a Simple File System

## Part 1 Display Information About the File System (/diskinfo disk.IMA)
* The program should display  <br>**OS Name** <br>   **Label of the Disk**<br> **Total Size of the Disk**<br> 
  **the Number of the Files in the Disk**<br>   **Number of FAT Copies**<br>   **Sectors per FAT** <br>
* Example of the Output : <br>
**OS Name: <br>**
**Label of the disk: <br>
Total size of the disk: <br>
Free size of the disk: <br>
============== <br>
The number of files in the disk (including all files in the root directory and files in all subdirectories): <br>
============= <br>
Number of FAT copies: <br>
Sectors per FAT: <br>**

## Part 2 Display the Content of the File System (./disklist disk.IMA)
* Starting from the **Root Directory**, it should display
  - **Directory name**
  - List of Files or Sub-Directories:
    - **File type** (**F** for regular files, **D** for directories)
    - **File size** in bytes (10 characters maximum)
    - **File name** (20 characters maximum)
    - **File creation date and time**

## Part 3 Copy a File from the Root Directory to the Current Linus Directory (./diskget disk.IMA ANS1.PDF)
* If the specified files exists in the **Root Directory**, copy it to the **current directory** in Linux
* If not, output **File not Found**

## Part 4 Copy a file from the Current Linux Directory to a Specified Directory of the File System (./diskput disk.IMA /subdir2/foo.txt)
* If the specified file exists in the current Linux directory and there is enough space for the file, copy the file to the specified directory of the file system
* If the specified file cannot be found, output **File not found**
* If the size of the specified file exceeds the amount of free space in file system, output **Not enough free space in the disk image**
