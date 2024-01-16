#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <ctype.h>

int find_file(char *image, char *file){//check if the file we want to copy exists 
    int root = 0x2600; //we start from root directory
    char file_name[12];
    char file_extension[4];
    int i;
    while(root < 0x4200){//check the root directory only
        for(i=0; image[root+i] != ' ' && i<8; i++){
            file_name[i] = image[root+i];//copy file name
        }
        file_name[i] = NULL;
        for(i=0; i<3; i++){
            file_extension[i] = image[root+8+i];//copy file extension
        }
        file_extension[i] = NULL;
        strcat(file_name, ".");
        strcat(file_name, file_extension);
        if(image[root] == '.' || image[root] == '.' || image[root+11] == 0x0F || (image[root+11] & 0x08) == 0x08 || image[root+26] == 0x01 || image[root+26] == 0x0F){
            root = root + 32;
            continue;
        }
        if(strcmp(file_name, file) == 0){
            return root;
        }
        root = root + 32;    
    }
    return 0;
}

int fat_convert(char *image, int index){
    int low_4;
    int high_4;
    int rest;
    int value;
    if(index % 2 == 0){
        low_4 = image[512+1+3*index/2] & 0x0F; //low 4 bit, FAT table start at the 2nd sector (skip boot sector)
        rest = image[512+3*index/2] & 0xFF; //rest of the 8 bit
        value = (low_4 << 8) + rest;
    }else{
        high_4 = image[512+3*index/2] & 0xF0; //high 4 bit
        rest = image[512+1+3*index/2] & 0xFF; //rest 8 bit
        value = (high_4 >> 4) + (rest << 4);
    }
    return value;
}

int get_size(char *image, int index){
	int file_size;
	int byte_one = (image[index+28] & 0xFF);
	int byte_two = ((image[index+29] & 0xFF) << 8);
	int byte_three = ((image[index+30] & 0xFF)<< 16);
	int byte_four = ((image[index+31] & 0xFF) << 24);
	file_size = byte_one + byte_two + byte_three + byte_four;
	return file_size;
} 

void copy_file(char *image, char *file, int file_size, int cluster){
	FILE *new_file;
   	new_file = fopen(file, "wb"); //create a new file has the same name as the one we searches
    int i;
    int phy_address;
    int next_cluster = fat_convert(image, cluster);//get the value in FAT table base on logical cluster location
    int remain;
	while(0xFFF != next_cluster){
		phy_address = (cluster + 31) * 512; //convert to physical address
		for(i = 0; i < 512; i++){
			fputc(image[phy_address + i], new_file);//copy/write the content into the new file we create
		}
		cluster = next_cluster;
		next_cluster = fat_convert(image, cluster);//traverse through the content in the next sector
	}

	remain = file_size - (file_size/512)*512;//copy the last of files in the last sector 
	phy_address = (cluster + 31) * 512;
	for(i = 0; i < remain; i++){
		fputc(image[phy_address + i], new_file);
	}
	fclose(new_file);
	return;
}

int main(int argc, char *argv[]){
    int fd;
	struct stat sb;

    int i;
    char file_search[12];
    int file_check;
    int first_cluster;
    int file_size;

	fd = open(argv[1], O_RDWR); //copy from mmap_test.c tutorial demo code
	fstat(fd, &sb);

	char * p = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // p points to the starting pos of your mapped memory
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}
    for(i=0; argv[2][i] != ' ' && i<12; i++){
        file_search[i] = toupper(argv[2][i]);
    }
    file_check = find_file(p, file_search);
    if(file_check != 0){
        first_cluster = p[file_check+26] + (p[file_check+27] << 8) & 0xFF;
        file_size = get_size(p, file_check);
        copy_file(p, file_search, file_size, first_cluster);
    }else{
        printf("File not found\n");
    }

    munmap(p, sb.st_size);
	close(fd);
    return 0;
}