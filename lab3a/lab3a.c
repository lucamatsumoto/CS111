#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "ext2_fs.h"
#include <sys/types.h>
#include <fcntl.h>
#include <time.h>
#include <stdint.h>
int mountFd = -1;

#define SB_OFFSET 1024
struct ext2_super_block sb;
struct ext2_inode inode;
struct ext2_group_desc group;
struct ext2_dir_entry dir;
uint32_t blockSize;
uint32_t inodeSize;


void print_errors(char* errors) {
    if(strcmp(errors, "pread") == 0){
        fprintf(stderr, "%s\n", "Error with pread");
        exit(1);
    }
} //just to be safe

char* format_time(uint32_t time) {
    char* formattedDate = malloc(sizeof(char)*32); //random value
    time_t rawtime = time;
    struct tm* info = gmtime(&rawtime);
    strftime(formattedDate, 32, "%m/%d/%y %H:%M:%S", info);
    return formattedDate;
}//referenced from: https://www.tutorialspoint.com/c_standard_library/c_function_gmtime.htm

void scan_free_block_entries(uint32_t freeBlockBitmap, uint32_t numBlocks) {
    uint32_t i = 0;
    for(; i < numBlocks; i++){
        int bit = i & 7; //mask with 7 to get bit number *check with John
        uint8_t byte = i >> 3; //right shift 3 to convert to byte -> refer to ext2 docs
        unsigned char read;
        if(pread(mountFd, &read, sizeof(uint8_t), freeBlockBitmap*blockSize+byte)< 0){
            print_errors("pread");
        }
        int freeCheck = ((read >> bit) & 1);
        if(freeCheck == 0){
            fprintf(stdout, "BFREE,%d\n", i+1); //1 indexed
        }
    }
}// bit of 1 means "used" and 0 "free/available" from ext2 documentation

void scan_free_inode_entries(uint32_t freeInodeBitmap, uint32_t numInodes){
    uint32_t i = 0;
    for(; i < numInodes; i++){
        int bit = i & 7;
        uint8_t byte = i >> 3;
        unsigned char read;
        if(pread(mountFd, &read, sizeof(uint8_t), freeInodeBitmap*blockSize+byte)< 0){
            print_errors("pread");
        }
        int freeCheck = ((read >> bit) & 1);
        if(freeCheck == 0){
            fprintf(stdout, "IFREE,%d\n", i+1);
        }
    }
}//according to ext2 documentation, bit meanings are similar to block entries

void print_superblock_summary() {
    if(pread(mountFd, &sb, sizeof(struct ext2_super_block), SB_OFFSET) < 0){
        print_errors("pread");
    }
    uint32_t numBlocks = sb.s_blocks_count;
    uint32_t numInodes = sb.s_inodes_count;
    blockSize = EXT2_MIN_BLOCK_SIZE << sb.s_log_block_size;
    inodeSize = sb.s_inode_size;
    uint32_t blocksPerGroup = sb.s_blocks_per_group;
    uint32_t inodesPerGroup = sb.s_inodes_per_group;
    uint32_t firstNonReserved = sb.s_first_ino;
    fprintf(stdout, "SUPERBLOCK,%u,%u,%u,%u,%u,%u,%u\n", numBlocks, numInodes, blockSize, inodeSize, blocksPerGroup, inodesPerGroup, firstNonReserved);
}


void printDirectoryEntries(uint32_t starting, uint32_t parentNum)
{

    uint32_t current = starting;
    while(current < starting + blockSize)
    {
        if(pread(mountFd, &dir, sizeof(struct ext2_dir_entry), current) < 0) print_errors("pread");
        uint32_t parent = parentNum, logical = current - starting, inodeNumber = dir.inode, entryLength = dir.rec_len, nameLength = dir.name_len;
        current += entryLength;
        if(inodeNumber == 0)
            continue;
        fprintf(stdout, "DIRENT,%u,%u,%u,%u,%u,'", parent, logical, inodeNumber, entryLength, nameLength);
        for(uint32_t i = 0; i < nameLength; i++)
            fprintf(stdout, "%c", dir.name[i]);
        fprintf(stdout, "'\n");
    }

}

void dL0(int inodeOffSet, int parentNum)
{
    int ib;
    for(uint32_t j = 0; j < 12; j++)
    {
        if(pread(mountFd, &ib, 4, inodeOffSet + 40 + j * 4) < 0) print_errors("pread");
        if(ib != 0)
        {
            int curOffset = blockSize * ib;
            printDirectoryEntries(curOffset, parentNum);
        }   
    }
}
void dL1(int inodeOffSet, int parentNum)
{
    int indir1, ib;
    if(pread(mountFd, &indir1, 4, inodeOffSet + 40 + 48) < 0) print_errors("pread");
    if(indir1 != 0)
    {
        for(uint32_t j = 0; j < blockSize /4; j++)
        {
            if(pread(mountFd,&ib, 4, blockSize * indir1 + j * 4) < 0) print_errors("pread");
            int curOffset = blockSize * ib;
            if(ib != 0)
            {
                printDirectoryEntries(curOffset, parentNum);           
            }   
        }
    }
}
void dL2(int inodeOffSet, int parentNum)
{
    int indir2, indir1, ib;
    if(pread(mountFd, &indir2, 4, inodeOffSet + 40 + 52) < 0) print_errors("pread");
    if(indir2 != 0)
    {
        for(uint32_t k = 0; k < blockSize /4; k++)
        {
            if(pread(mountFd, &indir1, 4, indir2 * blockSize + k * 4) < 0) print_errors("pread");
            if(indir1 != 0)
            {
                for(uint32_t j = 0; j < blockSize /4; j++)
                {
                    
                    if(pread(mountFd, &ib, 4, blockSize * indir1 + j * 4) < 0) print_errors("pread");
                    int curOffset = blockSize * ib;
                    if(ib != 0)
                        printDirectoryEntries(curOffset, parentNum);
                }
            }
        }
    }
}
void dL3(int inodeOffSet, int parentNum)
{
    int indir3, indir2, indir1, ib;
    if(pread(mountFd,&indir3, 4, inodeOffSet + 40 + 56) < 0) print_errors("pread");
    if(indir3 != 0)
    {
        for(uint32_t m = 0; m < blockSize / 4; m++)
        {
            if(pread(mountFd,&indir2, 4, indir3 * blockSize + m * 4) < 0) print_errors("pread");
            if(indir2 != 0)
            {
                for(uint32_t k = 0; k < blockSize /4; k++)
                {
                    if(pread(mountFd,&indir1, 4, indir2 * blockSize + k * 4) < 0) print_errors("pread");
                    if(indir1 != 0)
                    {
                        for(uint32_t j = 0; j < blockSize /4; j++)
                        {
                            
                            if(pread(mountFd, &ib, 4, blockSize * indir1 + j * 4) < 0) print_errors("pread");
                            int curOffset = blockSize * ib;
                            if(ib != 0)
                                printDirectoryEntries(curOffset, parentNum);
                        }
                    }
                }
            }
        }
    }
}

void print_inode_Directory_Summary(struct ext2_inode in, uint32_t offset, int num)
{
        //LOL my turn for unused parameter
        (void) in;
        dL0(offset, num);
        dL1(offset, num);
        dL2(offset, num);
        dL3(offset, num);
}

void indirectL1(uint32_t inodeNumber, uint32_t blockNumber)
{
    uint32_t blockOffset = blockNumber * blockSize;
    uint32_t blockValue;
    for(uint32_t i = 0; i < blockSize / 4; i++)
    {
        if(pread(mountFd, &blockValue, sizeof(blockValue), blockOffset + i * 4) < 0) print_errors("pread");
        if(blockValue != 0)
            fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 1, 12 + i, blockNumber, blockValue);
    }
}
void indirectL2(uint32_t inodeNumber, uint32_t blockNumberOfIndirectL2)
{
    uint32_t blockOffsetL1Indirect = blockNumberOfIndirectL2 * blockSize, blockOffset;
    uint32_t blockValueL1Indirect, blockValue;
    for(uint32_t i = 0; i < blockSize / 4; i++)
    {
        if(pread(mountFd, &blockValueL1Indirect, sizeof(blockValue), blockOffsetL1Indirect + i * 4) < 0) print_errors("pread");

        blockOffset = blockValueL1Indirect * blockSize;
        if(blockValueL1Indirect != 0)
        {
            fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 2, 12 + blockSize / 4 + i * blockSize / 4, blockNumberOfIndirectL2, blockValueL1Indirect);
            for(uint32_t j = 0; j < blockSize / 4; j++)
            {
                if(pread(mountFd, &blockValue, sizeof(blockValue), blockOffset + j * 4) < 0) print_errors("pread");
                if(blockValue != 0)
                    fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 1, 12 + blockSize / 4 + i * blockSize / 4 + j, blockValueL1Indirect, blockValue);
            }
        }
    }
}
void indirectL3(uint32_t inodeNumber, uint32_t blockNumberOfIndirectL3)
{
    uint32_t blockOffsetL3 = blockNumberOfIndirectL3 * blockSize;
    uint32_t blocksSeenThusFar = 12 + blockSize / 4 + blockSize / 4 * blockSize / 4;
    for(uint32_t i = 0; i < blockSize / 4; i++)
    {
        //Read each entry in L3
        uint32_t blockOffsetL2, blockNumberIndirectL2;
        if(pread(mountFd, &blockNumberIndirectL2, sizeof(blockNumberIndirectL2), blockOffsetL3 + i * 4) < 0) print_errors("pread");
        blockOffsetL2 = blockNumberIndirectL2 * blockSize;
        if(blockNumberIndirectL2 != 0)
        {
            fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 3, blocksSeenThusFar + i * (blockSize * blockSize / 8), blockNumberOfIndirectL3, blockNumberIndirectL2);
            for(uint32_t j = 0; j < blockSize / 4; j++)
            {
                //Read each entry in L2
                uint32_t blockOffsetL1, blockNumberIndirectL1;
                if(pread(mountFd, &blockNumberIndirectL1, sizeof(blockNumberIndirectL1), blockOffsetL2 + j * 4) < 0) print_errors("pread");
                blockOffsetL1 = blockNumberIndirectL1 * blockSize;
                if(blockNumberIndirectL1 != 0)
                {
                    fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 2, blocksSeenThusFar + i * (blockSize * blockSize / 8) + j * (blockSize / 4), blockNumberIndirectL2, blockNumberIndirectL1);
                    for(uint32_t k = 0; k < blockSize / 4; k++)
                    {
                        uint32_t dataBlockNumber;
                        if(pread(mountFd, &dataBlockNumber, sizeof(dataBlockNumber), blockOffsetL1 + k * 4) < 0) print_errors("pread");
                        if(dataBlockNumber != 0)
                            fprintf(stdout, "INDIRECT,%u,%u,%u,%u,%u\n", inodeNumber, 1, blocksSeenThusFar + i *(blockSize * blockSize / 8) + j * (blockSize / 4) + k, blockNumberIndirectL1, dataBlockNumber);

                    }
                }
            }
        }
    }
}

void print_inode_summary(uint32_t firstBlockInode, uint32_t numInodes, uint32_t freeInodeBitmap) {
    //Hack to compile clean, did you want this parameter Luca?
    (void) firstBlockInode;



    uint32_t i = 0;
    uint32_t j = 0;
    char fileType = '?';
    //printf("here");
    for(; i < numInodes; i++){
        //NOTE: CHECK TO SEE IF ALLOCATED!!!!
        int bit = i & 7;
        uint8_t byte = i >> 3;
        unsigned char read;
        if(pread(mountFd, &read, sizeof(uint8_t), freeInodeBitmap*blockSize+byte)< 0){
            print_errors("pread");
        }
        int freeCheck = ((read >> bit) & 1);
        if(freeCheck == 0)
           (void)i;// continue;
        //ALLOCATED
        //uint32_t inodeOffset = 1024 +  (firstBlockInode - 1) * (blockSize) + i * sizeof(struct ext2_inode);
        if(pread(mountFd, &inode, inodeSize, 1024 +  (firstBlockInode - 1) * (blockSize) + i * sizeof(struct ext2_inode)) < 0){
            print_errors("pread");
        }//offset is wherever the imap starts * blockSize + the inode number we are currently on
        if(inode.i_mode == 0 || inode.i_links_count == 0){
            continue;
        }
        if((inode.i_mode & 0xF000) == 0xA000){
            fileType = 's';
        }
        else if((inode.i_mode & 0xF000) == 0x8000){
            fileType = 'f';
        }
        else if((inode.i_mode & 0xF000) == 0x4000){
            fileType = 'd';
            //We note that this is where there is a directory....
            print_inode_Directory_Summary(inode, 1024 +  (firstBlockInode - 1) * (blockSize) + i * sizeof(struct ext2_inode), i + 1);
        }
        uint16_t imode = inode.i_mode & 0xFFF; ///& 0xFFF gives lower 12 bits https://cboard.cprogramming.com/cplusplus-programming/98577-how-do-i-get-lower-12-bits-32-bit-int.html
        uint16_t owner = inode.i_uid;
        uint16_t group = inode.i_gid;
        uint16_t linksCount = inode.i_links_count;
        char* changeDate = format_time(inode.i_ctime);
        char* modificationDate = format_time(inode.i_mtime);
        char* accessDate = format_time(inode.i_atime);
        uint32_t fileSize = inode.i_size;
        uint32_t numBlocks = inode.i_blocks; //should contain same value as the i_blocks field according to spec
        fprintf(stdout, "INODE,%d,%c,%o,%u,%u,%u,%s,%s,%s,%d,%d", i + 1, fileType, imode, owner, group, linksCount, changeDate, modificationDate, accessDate, fileSize, numBlocks);
        if(!(fileType == 's' && fileSize < 60))
            for(j = 0; j < 15; j++){
                fprintf(stdout, ",%u", inode.i_block[j]);
            }
        else
            fprintf(stdout, ",%u", inode.i_block[0]);
        fprintf(stdout,"\n");

        if(!(fileType == 's' && fileSize < 60))
        {
                //LETS DEAL WITH INDIRECTS
                //uint32_t indirectL1BlockNum = 0;
                if(inode.i_block[12] != 0)
                    indirectL1(i + 1, inode.i_block[12]);
                if(inode.i_block[13] != 0)
                    indirectL2(i + 1, inode.i_block[13]);
                if(inode.i_block[14] != 0)
                    indirectL3(i + 1, inode.i_block[14]);


        }
    }
    
} //first non-reserved inode is 11 -> check later

void print_group_summary() {
    if(pread(mountFd, &group, sizeof(struct ext2_group_desc), SB_OFFSET + sizeof(struct ext2_super_block)) < 0){
        print_errors("pread");
    } // according to the documentation, the block group descriptor table starts right after the superblock
    uint32_t numBlocks = sb.s_blocks_count;
    uint32_t numInodes = sb.s_inodes_count;
    uint16_t numFreeBlocks = group.bg_free_blocks_count;
    uint16_t numFreeInodes = group.bg_free_inodes_count;
    uint32_t freeBlockBitmap = group.bg_block_bitmap;
    uint32_t freeInodeBitmap = group.bg_inode_bitmap;
    uint32_t firstBlockInode = group.bg_inode_table;
    fprintf(stdout, "GROUP,0,%u,%u,%u,%u,%u,%u,%u\n", numBlocks, numInodes, numFreeBlocks, numFreeInodes, freeBlockBitmap, freeInodeBitmap, firstBlockInode);//according to spec, we can assume that there will be only one group
    scan_free_block_entries(freeBlockBitmap, numBlocks);
    scan_free_inode_entries(freeInodeBitmap, numInodes);
    print_inode_summary(firstBlockInode, numInodes, freeInodeBitmap);
}

int main(int argc, char** argv){
    if(argc != 2){
        fprintf(stderr, "%s\n", "Incorrect arguments");
        exit(1);
    }
    if((mountFd = open(argv[1], O_RDONLY)) < 0)
    {
        fprintf(stderr, "%s\n", "Could not mount" );
        exit(2);
    }
    print_superblock_summary();
    print_group_summary();
}
