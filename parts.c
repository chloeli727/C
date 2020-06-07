#include <stdio.h>
#include <stdint.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <string.h>
#include <limits.h>
#include <assert.h>
#include <time.h>
#include <sys/mman.h>
#include <fcntl.h> //open
#include <unistd.h> //close
#include <sys/types.h>
#include <arpa/inet.h>

//this structure is taken from tutorialX slides
//super block
struct __attribute__((__packed__)) superblock_t {
    uint8_t   fs_id [8];
    uint16_t block_size;
    uint32_t file_system_block_count;
    uint32_t fat_start_block;
    uint32_t fat_block_count;
    uint32_t root_dir_start_block;
    uint32_t root_dir_block_count;
};

//this structure is taken from tutorialX slides
//time and date entry
struct __attribute__((__packed__)) dir_entry_timedate_t {
    uint16_t year;
    uint8_t month;
    uint8_t day;
    uint8_t hour;
    uint8_t minute;
    uint8_t second;
};

//this structure is taken from tutorialX slides
//directory entry
struct __attribute__((__packed__)) dir_entry_t {
    uint8_t status;
    uint32_t starting_block;
    uint32_t block_count;
    uint32_t size;
    struct dir_entry_timedate_t create_time;
    struct dir_entry_timedate_t modify_time;
    uint8_t filename[31];
    uint8_t unused[6];
};

//part1
int diskinfo(int argc, char* argv[]){
    if (argc != 2) {
        if(argc < 2){
            printf("Error! Please enter an input img file\n");
        }else if(argc > 2){
            printf("Error! Too many inputs, only need one input img file\n");
        }
        return 0;
    }

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    
    if(fstat(fd, &buffer) != 0){//returns 0 if successful
        printf("Error! No such img file!\n");
        return 0;
    }

    //tamplate:   pa=mmap(addr, len, prot, flags, fildes, off);
    //c will implicitly cast void* to char*, while c++ does NOT
    void* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct superblock_t* sb;
    sb=(struct superblock_t*)address;
    
    printf("Super block information:\n");
   
    printf("Block size: %d\n", htons(sb->block_size));
    printf("Block count: %d\n", ntohl(sb->file_system_block_count)); 
    printf("FAT starts: %d\n", ntohl(sb->fat_start_block));
    printf("FAT blocks: %d\n", ntohl(sb->fat_block_count));
    printf("Root directory start: %d\n", ntohl(sb->root_dir_start_block));
    printf("Root directory blocks: %d\n", ntohl(sb->root_dir_block_count));
    printf("\n");

    printf("FAT information:\n");
    int freeblocks = 0;
    int reservedblocks = 0;
    int allocatedblocks = 0;
    int FAT_start = 0;
    int FAT_end = 0;
    FAT_start = htons(sb->block_size) * ntohl(sb->fat_start_block);
    FAT_end = htons(sb->block_size) * ntohl(sb->fat_block_count) + FAT_start;

    int i;
    int pt = 0;
    for (i = FAT_start; i < FAT_end; i+= 4){
        memcpy(&pt,address + i,4);
        pt = htonl(pt);
        if(pt == 0){
            freeblocks++;
        }else if(pt == 1){
            reservedblocks++;
        }else{
            allocatedblocks++;
        }
    }
    printf("Free Blocks: %d\n", freeblocks);
    printf("Reserved Blocks: %d\n", reservedblocks);
    printf("Allocated Blocks: %d\n", allocatedblocks);
    
    munmap(address,buffer.st_size);
    close(fd);
    return 0;
}

//helper function for part2 for printing the disklist
void print_disklist(char* addr, int i){
    
    int status = 0;
    unsigned char* file_name;
    int file_size = 0;
    int year = 0;
    int month = 0;
    int day = 0;
    int hr = 0;
    int min = 0;
    int sec = 0;

    struct dir_entry_t* dir_e;
    dir_e = (struct dir_entry_t*)(addr + i);

    status = dir_e->status;
    file_size = ntohl(dir_e->size);
    file_name = dir_e->filename;
    year = htons(dir_e->modify_time.year);
    month = dir_e->modify_time.month;
    day = dir_e->modify_time.day;
    hr = dir_e->modify_time.hour;
    min = dir_e->modify_time.minute;
    sec = dir_e->modify_time.second;

    if(status == 3){
        status = 'F';
    }else if(status == 5){
        status = 'D';
    }
    
    if(status != 0){
        printf("%c %10d %30s %d/%02d/%02d %02d:%02d:%02d\n",
            status, file_size, file_name, year, month, day, hr, min, sec);
    }
    
}

//part2
int disklist(int argc, char* argv[]){
    if (argc < 2) {
        printf("Error! Please enter an input img file\n");  
        return 0;
    }

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    
    if(fstat(fd, &buffer) != 0){//returns 0 if successful
        printf("Error! No such img file!\n");
        return 0;
    }

    //tamplate:   pa=mmap(addr, len, prot, flags, fildes, off);
    //c will implicitly cast void* to char*, while c++ does NOT
    void* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct superblock_t* sb;
    sb = (struct superblock_t*)address;
    
    int blocksize = 0;
    int root_dir_start = 0;
    int root_start = 0;
    int root_end = 0;

    blocksize = htons(sb->block_size);
    root_dir_start = ntohl(sb->root_dir_start_block);
    root_start = root_dir_start * blocksize;
    root_end = root_start + blocksize;

    int i;
    int pt = 0;
    for(i = root_start; i < root_end; i+= 64){
        memcpy(&pt, address + i, 1);
        print_disklist(address, i);
    }

    munmap(address,buffer.st_size);
    return 0;
}

//part3
int diskget(int argc, char* argv[]){
    if (argc != 4) {
        printf("Error! Please enter exactly 4 arguements\n");
        return 0;
    }

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    
    if(fstat(fd, &buffer) != 0){//returns 0 if successful
        printf("Error! No such img file!\n");
        return 0;
    }

    char *desired_file;
    char *copy_file;
    desired_file = argv[2];
    copy_file = argv[3];

    //tamplate:   pa=mmap(addr, len, prot, flags, fildes, off);
    //c will implicitly cast void* to char*, while c++ does NOT
    void* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct superblock_t* sb;
    sb = (struct superblock_t*)address;

    int blocksize = 0;
    int FAT_start = 0;
    int root_dir_start = 0;
    int root_start = 0;
    int root_end = 0;

    blocksize = htons(sb->block_size);
    FAT_start = ntohl(sb->fat_start_block);
    root_dir_start = ntohl(sb->root_dir_start_block);
    
    root_start = root_dir_start * blocksize;
    root_end = root_start + blocksize;

    int pt = 0;
    int i;
    int j;
    char file_name[31];
    int file_size = 0;
    FILE *fptr;
    int start_block = 0;
    int num_blocks = 0;
    int FAT_addr = 0;
    int remain_size = 0;

    for (i = root_start; i < root_end; i += 64) {
        struct dir_entry_t* dir_e;
        dir_e = (struct dir_entry_t*)(address + i);

        pt = dir_e->status;
        if (pt == 3) {
            memcpy(&file_name, address + i + 27, 31);
            //search for file name from root directory
            if (strcmp(file_name, desired_file) == 0) {
                fptr = fopen(copy_file, "wb");
                //obtain #blocks and starting block from the entry
                num_blocks = ntohl(dir_e->block_count);
                start_block = ntohl(dir_e->starting_block);
                file_size = ntohl(dir_e->size);

                for (j = 0; j < num_blocks-1; j++) {
                    fwrite(address + (blocksize * start_block), 1, blocksize, fptr);
                    FAT_addr = FAT_start * blocksize + start_block * 4;

                    memcpy(&pt, address + FAT_addr, 4);
                    pt = htonl(pt);
                    start_block = pt;
                }
                //calculate remaining size
                remain_size = file_size % blocksize;

                if (remain_size == 0) {
                    remain_size = blocksize;
                }
                //fwrite remaining part
                fwrite(address + (blocksize * start_block), 1, remain_size, fptr);
                FAT_addr = FAT_start * blocksize + start_block * 4;
                memcpy(&pt, address + FAT_addr, 4);

                munmap(address, buffer.st_size);
                close(fd);
                return 0;
            }
        }
    }
    printf("File not found\n");
    munmap(address, buffer.st_size);
    close(fd);
    return 0;
}

//part4
int diskput(int argc, char* argv[]){
    if (argc != 4) {
        printf("Error! Please enter exactly 4 arguements\n");
        return 0;
    }

    char *desired_file;
    char *copy_file;
    desired_file = argv[2];
    copy_file = argv[3];

    int fd = open(argv[1], O_RDWR);
    struct stat buffer;
    if(fstat(fd, &buffer) != 0){//returns 0 if successful
        printf("Error! No such img file!\n");
        return 0;
    }

    int fd2 = open(desired_file, O_RDONLY);
    struct stat buffer2;
    if(fstat(fd2, &buffer2) != 0){//returns 0 if successful
        printf("File not found\n");
        return 0;
    }

    int file_size = 0;
    file_size = buffer2.st_size; //get file attribute (size)
    
    close(fd2);

    //tamplate:   pa=mmap(addr, len, prot, flags, fildes, off);
    //c will implicitly cast void* to char*, while c++ does NOT
    void* address=mmap(NULL, buffer.st_size, PROT_READ | PROT_WRITE, MAP_SHARED, fd, 0);

    struct superblock_t* sb;
    sb = (struct superblock_t*)address;

    int blocksize = 0;
    int FAT_start = 0;
    int FAT_end = 0;
    int root_start = 0;

    blocksize = htons(sb->block_size);
    FAT_start = htons(sb->block_size) * ntohl(sb->fat_start_block);
    FAT_end = htons(sb->block_size) * ntohl(sb->fat_block_count) + FAT_start;
    root_start = ntohl(sb->root_dir_start_block) * blocksize;

    //number of blocks
    int num_blocks = 0;
    num_blocks = file_size / blocksize;
    if(file_size % blocksize != 0){
        num_blocks++;
    }

    FILE *fptr;
    fptr = fopen(desired_file, "rb");

    int blocks_used = 0;
    int end1 = 0xFFFFFFFF;
    int end2 = 0xFFFF;
    int start_block = 0;
    int FAT_prev = 0;
    int memory_loc = 0;
    int location = 0;
    char basket[blocksize];

    int i;
    for (i = FAT_start; i < FAT_end; i += 4) {
        int pt = 0;
        memcpy(&pt, address + i, 4);
        pt = htonl(pt);
    
        if (pt == 0) {
            if(blocks_used == 0) {
               start_block = (i - FAT_start)/4;
               FAT_prev = i;
               blocks_used++;

               fread(basket, blocksize, 1, fptr);
               memcpy(address + (start_block * blocksize), &basket, blocksize);
               
                if(num_blocks == blocks_used){
                    memcpy(address + i, &end1, 4);
                    break;
                }else{
                    continue;
                }
            }
            memory_loc = (i - FAT_start)/4;
            fread(basket, blocksize, 1, fptr);
            memcpy(address + (memory_loc * blocksize), &basket, blocksize);

            location = htonl(memory_loc);
            memcpy(address + FAT_prev, &location, 4);
            FAT_prev = i;
            blocks_used++;

            if(num_blocks == blocks_used){
                memcpy(address + i, &end1, 4);
                break;
            }
        }
    }

    int end_dir = 0;
    end_dir = root_start + blocksize;

    int status = 3;
    int j;
    for (j = root_start; j < end_dir; j += 64) {
        int pt = 0;
        memcpy(&pt, address + j, 1);

        if (pt == 0){
            memcpy(address + j, &status, 1);

            start_block = ntohl(start_block);
            memcpy(address + j + 1, &start_block, 4);

            num_blocks = htonl(num_blocks);
            memcpy(address + j + 5, &num_blocks, 4);

            file_size = htonl(file_size);
            memcpy(address + j + 9, &file_size, 4);

            time_t now;
            time(&now);
            struct tm *tm = localtime(&now);
            
            int year = 0;
            int month = 0;
            int day = 0;
            int hr = 0;
            int min = 0;
            int sec = 0;
 
            //put create time and modify time
            year = htons(tm->tm_year + 1900);
            memcpy(address + j + 13, &year, 2);
            memcpy(address + j + 20, &year, 2);

            month = tm->tm_mon + 1;
            memcpy(address + j + 15, &month, 1);
            memcpy(address + j + 22, &month, 1);

            day = tm->tm_mday;
            memcpy(address + j + 16, &day, 1);
            memcpy(address + j + 23, &day, 1);

            hr = tm->tm_hour;
            memcpy(address + j + 17, &hr, 1);
            memcpy(address + j + 24, &hr, 1);

            min = tm->tm_min;
            memcpy(address + j + 18, &min, 1);
            memcpy(address + j + 25, &min, 1);

            sec = tm->tm_sec;
            memcpy(address + j + 19, &sec, 1);
            memcpy(address + j + 26, &sec, 1);

            strncpy(address+j+27, copy_file, 31);

            memcpy(address+j+58, &end1, 4);
            memcpy(address+j+62, &end2, 2);

            return 0;
        }
    }
    return 0;
}

int diskfix(int argc, char* argv[]){
    printf("Part 5 was not implemented.\n");
    return 0;
}

int main(int argc, char* argv[]){
    #if defined(PART1)
        diskinfo(argc, argv);
    #elif defined(PART2)
        disklist(argc, argv);
    #elif defined(PART3)
        diskget(argc, argv);
    # elif defined(PART4)
        diskput(argc, argv);
    #elif defined(PART5)
        diskfix(argc, argv);
    #else
    #   error "PART[12345] must be defined"
    #endif
        return 0;
}