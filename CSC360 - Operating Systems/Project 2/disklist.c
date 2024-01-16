#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <stdint.h>
#include <sys/mman.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#define timeOffset 14 //offset of creation time in directory entry
#define dateOffset 16 //offset of creation date in directory entry

static unsigned int bytes_per_sector;

void print_date_time(char * directory_entry_startPos){//copy from sample_time_date_2.c, tutorial demo code
	
	int time, date;
	int hours, minutes, day, month, year;
	
	time = *(unsigned short *)(directory_entry_startPos + timeOffset);
	date = *(unsigned short *)(directory_entry_startPos + dateOffset);
	
	//the year is stored as a value since 1980
	//the year is stored in the high seven bits
	year = ((date & 0xFE00) >> 9) + 1980;
	//the month is stored in the middle four bits
	month = (date & 0x1E0) >> 5;
	//the day is stored in the low five bits
	day = (date & 0x1F);
	
	printf("%d-%02d-%02d ", year, month, day);
	//the hours are stored in the high five bits
	hours = (time & 0xF800) >> 11;
	//the minutes are stored in the middle 6 bits
	minutes = (time & 0x7E0) >> 5;
	
	printf("%02d:%02d\n", hours, minutes);
	
	return;	
}

char *print_file(char *image, char *original){
	int file_size;
	char file_name[12];
	char file_extension[4];
	char file_type;
	char *sub_dir = NULL;
	int i;
	do{
		while(image[0] != 0x00){
			if(image[26] != 0x00 && image[26] != 0x01 && image[11] != 0x0F && (image[11] & 0x08) != 0x08){
				file_size = 0;
				if(image[11]  == 0x10){//check if it's a file or subdirectory
					file_type = 'D'; //if it's a subdirectory, we don't need to check its file size
					if(image[0] == '.' || image[1] == '.' || image[0] == 1 || image[1] == 1 || image[0] == 0 || image[1] == 0){//skip empty directory
						image = image + 32;
						continue;
					}
				}else{
					file_type = 'F';
					file_size = get_size(image);
				}
				for(i=0; image[i] != ' ' && i<8; i++){//copy the file name 
					file_name[i] = image[i];
				}
				file_name[i] = NULL;
				for(i=0; i<3; i++){//copy the extension name
					file_extension[i] = image[i+8];
				}
				file_extension[i] = NULL;
				if(file_type == 'F'){
					strcat(file_name, ".");
				}
				strcat(file_name, file_extension); //combine the name and extension to make a standard file name
				if((image[11] & 0x08) == 0){
					printf("%c %10u %20s ", file_type, file_size, file_name); //print out file type, name, size, name, date, time
					print_date_time(image);
				}
				if(image[11] == 0x10){//recursion when a subdirectory is found
					sub_dir = image;
					image = print_file(image + 32, original);
					char sub_name[8];
					strncpy(sub_name, sub_dir, 8);
					sub_name[8] = NULL;
					printf("\n");
					printf("%s\n", sub_name); //print out the name of the subdirectory we will traverse through
					printf("=============\n");
					image = print_file(original + (sub_dir[26]+12) * 512, original);
				}
			}
			image += 32;
		}
		sub_dir = image;
	}while(sub_dir[0] != 0x00);
	return sub_dir;
}


int get_size(char *image){
	int file_size;
	int byte_one = (image[28] & 0xFF);
	int byte_two = ((image[28+1] & 0xFF) << 8);
	int byte_three = ((image[28+2] & 0xFF)<< 16);
	int byte_four = ((image[28+3] & 0xFF) << 24);
	file_size = byte_one + byte_two + byte_three + byte_four;
	return file_size;
}

int main(int argc, char *argv[]){
    int fd;
	struct stat sb;

    char os[8];
    char label[11];
    int free_disk;

	fd = open(argv[1], O_RDWR); //copy from mmap_test.c tutorial demo code
	fstat(fd, &sb);

	char * p = mmap(NULL, sb.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0); // p points to the starting pos of your mapped memory
	if (p == MAP_FAILED) {
		printf("Error: failed to map memory\n");
		exit(1);
	}
	printf("Root\n");
	printf("==============\n");
	print_file(p+0x2600, p+0x2600);
	munmap(p, sb.st_size);
	close(fd);
    return 0;
}