#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

char *os_info(char *image, char *os){//retrive os name from boot sector
    strncpy(os, &image[3], 8); //OS_info starts at byte 3 and length is 8 byte
    os[8] = NULL; //get rid of weird character at the end
    return os;
}

char *disk_label(char *image, char *label){//retrive disk label from boot sector or root directory; will check boot sector first
    strncpy(label, &image[43], 11); //we will check if disk label is at boot sector
    if(label[0] == ' '){//retrieve disk label from the root sector
        int root = 0x00002600; //root directory starts at 0x00002600
        int attr;
        while(root < 0x00004200){//33 pre-defined sectors and each sector is 512 bytes large
            attr = image[root+11];//11 byte is where the attribute is
            if(attr == 0x00000008){ //if attribute is 0x08, it stores volume label
                strncpy(label, &image[root], 11);
                break;
            }
            root = root + 32; //if it's not the volume label, jump to next seector
        }
    }
    label[11] = NULL; //get rid of weird character at the end
    return label;
}

int free_size(char *image){
    int free_sec = 0;
    int low_4;
    int rest;
    int high_4;
    int total_sec = image[19]+(image[20] << 8);
    for(int n=2; n + 31 < total_sec; n++){//physical sector number = 33 + FAT entry number - 2
        int index;
        if(n%2 == 0){//check if the FAT entry is a even number or not
            low_4 = image[512+1+3*n/2]; //low 4 bit, FAT table start at the 2nd sector (skip boot sector)
            rest = image[512+3*n/2]; //rest of the 8 bit
            index = (low_4 << 8) + rest;
            if(index == 0x00){
                free_sec++;
            }
        }else{
            high_4 = image[512+3*n/2]; //high 4 bit
            rest = image[512+1+3*n/2]; //rest 8 bit
            index = (high_4 >> 4) + (rest << 4);
            if(index == 0x00){
                free_sec++;
            }
        }
    }
    return free_sec * 512;
}

int count_file(char *image, char *original){//count the total number of files
    int total_file = 0;
    while(image[0] != 0x00){//see if there's anything in the file name section
        if(image[0] != '.' && image[1] != '.' && image[0] != 1 && image[1] != 1 && image[0] != 0 && image[1] != 0 && image[26] != 0x00 && image[26] != 0x01 && image[11] != 0x0F && (image[11] & 0x08) != 0x08){//skip the empty directory & files, empty sectors, volume lable, long file name, and etc... 
            if(image[11] != 0x10){//check if it's a directory or not
                total_file++;
            }else{
                total_file += count_file(original + ((image[26] + 12) * 512), original); //(first logical sector + 31 - 19) * 512 bytes
            }
        }
        image = image + 32;//keep looping through the directory
    }
    return total_file;
}

int main(int argc, char *argv[])
{
	int fd;
	struct stat sb;

    char os[8];
    char label[11];
    int free_disk;
    int num_file;

	fd = open(argv[1], O_RDWR); //copy from mmap_test.c tutorial demo code
	fstat(fd, &sb);

	char * p = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // p points to the starting pos of your mapped memory
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}

    os_info(p, os);
    printf("OS Name: %s\n", os);
    disk_label(p, label);
    printf("Label of the disk: %s\n", label);
    printf("Total Size of the disk: %lu\n", (uint64_t)sb.st_size);
    printf("Free size of the disk: %d\n\n", free_size(p));
    printf("==============\n");
    num_file = count_file(&p[0x2600], &p[0x2600]); //we jump to root directory
    printf("The number of files in the disk: %d\n\n", num_file); 
    printf("==============\n");
    printf("Number of FAT copies: %d\n", p[16]);
    printf("Sectors per FAT: %d\n", p[22] + (p[23]<<8));
    munmap(p, sb.st_size);
	return 0;
}