#!/usr/bin/env python3
#NAME: Luca Matsumoto, John Goodlad
#EMAIL: lucamatsumoto@gmail.com, JohnGoodlad@ucla.edu
#ID: 204726167, 404675483


import sys
import csv

isConsistent = True

def printWithStatus(message, file):
    global isConsistent
    isConsistent = False
    print(message, file=sys.stdout)


class Info:
    def __init__(self, block_type, block_num, inode_num, offset):
        if block_type == '':
            blockStr = 'BLOCK'
        else:
            blockStr = ' BLOCK'
        self.block_type = block_type + blockStr
        self.block_num = block_num
        self.inode_num = inode_num
        self.offset = offset
        #helper class to help with storing block information

class SuperBlock:
    def __init__(self, field):
        self.total_blocks = int(field[1])
        self.total_inodes = int(field[2])
        self.block_size = int(field[3])
        self.inode_size = int(field[4])
        self.blocks_per_group = int(field[5])
        self.inodes_per_group = int(field[6])
        self.first_non_reserved_inode = int(field[7])

class Group:
    def __init__(self, field):
        self.group_num = int(field[1])
        self.total_num_of_blocks = int(field[2])
        self.total_num_of_inodes = int(field[3])
        self.num_free_blocks = int(field[4])
        self.num_free_inodes = int(field[5])
        self.bitmap = int(field[6])
        self.imap = int(field[7])
        self.first_block = int(field[8])

class Inode:
    def read_blocks(self, field):
        list_blocks = []
        if field[2] != 's':
            for i in range(12, 24):
                list_blocks.append(int(field[i]))
        return list_blocks

    def read_pointers(self, field):
        list_pointers = []
        if field[2] != 's':
            for i in range(24, 27):
                list_pointers.append(int(field[i]))
        return list_pointers

    def __init__(self, field):
        self.inode_num = int(field[1])
        self.file_type = (field[2])
        self.mode = field[3]
        self.owner = int(field[4])
        self.group = int(field[5])
        self.link_count = int(field[6])
        self.change_time = field[7]
        self.mod_time = field[8]
        self.access_time = field[9]
        self.file_size = int(field[10])
        self.block_num = int(field[11])
        self.list_blocks = self.read_blocks(field)
        self.list_pointers = self.read_pointers(field)


class Dirent:
    def __init__(self, field):
        self.parent_inode_num = int(field[1])
        self.offset = int(field[2])
        self.inode_num = int(field[3])
        self.entry_length = int(field[4])
        self.name_length = int(field[5])
        self.name = str(field[6])

class Indirect:
    def __init__(self, field):
        self.inode_num = int(field[1])
        self.indirection_level = int(field[2])
        self.offset = int(field[3])
        self.indirect_block_num = int(field[4])
        self.ref_block_num = int(field[5])

def unreferenced_audit(list_free_blocks, block_referenced_dict, start, end):
    for block in range(0, end):
        if start <= block <= end:
            if block not in block_referenced_dict and block not in list_free_blocks:
                printWithStatus('UNREFERENCED BLOCK {}'.format(block), file=sys.stdout)

def allocated_audit(list_free_blocks, block_referenced_dict, start, end):
    for block in range(0, end):
        if start <= block <= end:
            if block in block_referenced_dict and block in list_free_blocks:
                printWithStatus('ALLOCATED BLOCK {} ON FREELIST'.format(block), file=sys.stdout)


def duplicate_audit(list_free_blocks, block_referenced_dict, start, end):
    for block in range(0, end):
        if start <= block <= end:
            if block in block_referenced_dict:
                info = block_referenced_dict[block]
                if len(info) > 1:
                    for i in range(len(info)):
                        block_info = info[i]
                        printWithStatus('DUPLICATE {} {} IN INODE {} AT OFFSET {}'.format(block_info.block_type, block_info.block_num, block_info.inode_num, block_info.offset), file=sys.stdout)


def block_consistency_audit(super_block, group, list_free_blocks, list_free_inodes, list_indirect):
    indirection_level = {1: 'INDIRECT', 2: 'DOUBLE INDIRECT', 3: 'TRIPLE INDIRECT'} #map of levels to text for output
    logical_offsets = {1: 12, 2: (12 + 256), 3: (12 + 256 + 256**2)} #dict of associated logical offsets for each block

    block_referenced_dict = {} #create a dictionary of block numbers to a list of associated info objects
    valid_block_start = group.first_block + super_block.inode_size * group.total_num_of_inodes / super_block.block_size #calculate where the first non-reserved block is

    for inode in list_free_inodes:
        logical_offset = 0
        for block in inode.list_blocks:
            if block != 0:
                if block < 0 or block > (super_block.total_blocks-1):
                    printWithStatus('INVALID BLOCK {} IN INODE {} AT OFFSET {}'.format(block, inode.inode_num, logical_offset), file=sys.stdout)
                if block < valid_block_start:
                    printWithStatus('RESERVED BLOCK {} IN INODE {} AT OFFSET {}'.format(block, inode.inode_num, logical_offset), file=sys.stdout)
                elif block in block_referenced_dict and block != 0:
                    block_referenced_dict[block].append(Info('', block, inode.inode_num, logical_offset))
                else:
                    block_referenced_dict[block] = [Info('', block, inode.inode_num, logical_offset)]
                #handle duplicates for later
            logical_offset = logical_offset + 1

        for i in range(len(inode.list_pointers)):
            level = indirection_level[i+1]
            logical_offset = logical_offsets[i+1] #one indexed
            if inode.list_pointers[i] != 0:
                if inode.list_pointers[i] < 0 or inode.list_pointers[i]> (super_block.total_blocks-1) : #can't be at total block amount
                    printWithStatus('INVALID {} BLOCK {} IN INODE {} AT OFFSET {}'.format(level, inode.list_pointers[i], inode.inode_num, logical_offset), file=sys.stdout)
                if inode.list_pointers[i] < valid_block_start:
                    printWithStatus('RESERVED {} BLOCK {} IN INODE {} AT OFFSET {}'.format(level, inode.list_pointers[i], inode.inode_num, logical_offset), file=sys.stdout)
                elif inode.list_pointers[i] in block_referenced_dict:
                    block_referenced_dict[inode.list_pointers[i]].append(Info(level, inode.list_pointers[i], inode.inode_num, logical_offset))
                else:
                    block_referenced_dict[inode.list_pointers[i]] = [Info(level, inode.list_pointers[i], inode.inode_num, logical_offset)]


    for i in range(len(list_indirect)):
        level = indirection_level[list_indirect[i].indirection_level]
        logical_offset = list_indirect[i].offset
        if list_indirect[i].ref_block_num != 0:
            if list_indirect[i].ref_block_num < 0 or list_indirect[i].ref_block_num > (super_block.total_blocks-1):
                printWithStatus('INVALID {} BLOCK {} IN INODE {} AT OFFSET {}'.format(level, list_indirect[i].ref_block_num, list_indirect[i].inode_num, logical_offset), file=sys.stdout)
            if list_indirect[i].ref_block_num < valid_block_start:
                printWithStatus('INVALID {} BLOCK {} IN INODE {} AT OFFSET {}'.format(level, list_indirect[i].ref_block_num, list_indirect[i].inode_num, logical_offset), file=sys.stdout)
            elif list_indirect[i].ref_block_num in block_referenced_dict:
                block_referenced_dict[list_indirect[i].ref_block_num].append(Info(level, list_indirect[i].ref_block_num, list_indirect[i].inode_num, logical_offset))
            else:
                block_referenced_dict[list_indirect[i].ref_block_num] = [Info(level, list_indirect[i].ref_block_num, list_indirect[i].inode_num, logical_offset)]

    unreferenced_audit(list_free_blocks, block_referenced_dict, valid_block_start, super_block.total_blocks - 1)
    allocated_audit(list_free_blocks, block_referenced_dict, valid_block_start, super_block.total_blocks - 1)
    duplicate_audit(list_free_blocks, block_referenced_dict, valid_block_start, super_block.total_blocks - 1) #test this edge case later/confirm with John
                    

#invalid block is one whose number is less than zero or greater than the highest block
#reserved block is one that could not legally be allocated to any file

def get_allocated_inode_nums(inode_list):
    inode_num_from_list = []
    for i in range(len(inode_list)):
        inode_num_from_list.append(inode_list[i].inode_num)
    return inode_num_from_list

def inode_allocation_audit(inode_list, list_free_inodes, super_block, inode_num_from_list):
    for inode in inode_list:
        if inode.inode_num in list_free_inodes:
            printWithStatus('ALLOCATED INODE {} ON FREELIST'.format(inode.inode_num), file=sys.stdout)

    for inode in range(super_block.first_non_reserved_inode, super_block.total_inodes):
        if inode not in inode_num_from_list and inode not in list_free_inodes:
            printWithStatus('UNALLOCATED INODE {} NOT ON FREELIST'.format(inode), file=sys.stdout)
    #think about using a set 

def check_invalid_directory(dirent, last_inode, inode_num_list):
    if dirent.inode_num< 1 or dirent.inode_num > last_inode:
        printWithStatus('DIRECTORY INODE {} NAME {} INVALID INODE {}'.format(dirent.parent_inode_num, dirent.name, dirent.inode_num), file=sys.stdout)
        return False
    elif dirent.inode_num not in inode_num_list:
        printWithStatus('DIRECTORY INODE {} NAME {} UNALLOCATED INODE {}'.format(dirent.parent_inode_num, dirent.name, dirent.inode_num), file=sys.stdout)
        return False
    else:
        return True

def directory_consistency_audit(list_dirent, super_block, inode_num_list, inode_list):
    inode_link_count_dict = {}
    for i in range(1, super_block.total_inodes+1):
        inode_link_count_dict[i] = 0

    for dirent in list_dirent:
        if check_invalid_directory(dirent, super_block.total_inodes, inode_num_list) == True:
            if dirent.inode_num in inode_link_count_dict:
                inode_link_count_dict[dirent.inode_num] += 1

    for inode in inode_list:
        if inode.inode_num in inode_link_count_dict:
            if inode.link_count != inode_link_count_dict[inode.inode_num]:
                printWithStatus('INODE {} HAS {} LINKS BUT LINKCOUNT IS {}'.format(inode.inode_num, inode_link_count_dict[inode.inode_num], inode.link_count), file=sys.stdout)

    for dirent in list_dirent:
        if dirent.name ==  "'.'" and dirent.parent_inode_num != dirent.inode_num:
            printWithStatus('DIRECTORY INODE {} NAME {} LINK TO INODE {} SHOULD BE {}'.format(dirent.parent_inode_num, dirent.name, dirent.inode_num, dirent.parent_inode_num), file=sys.stdout)
    
    baseDirEnts = list(filter(lambda x: x.parent_inode_num == 2 and x.name == "'..'", list_dirent))
    for dirent in baseDirEnts:
        if dirent.name == "'..'" and dirent.parent_inode_num != dirent.inode_num:
            printWithStatus('DIRECTORY INODE {} NAME {} LINK TO INODE {} SHOULD BE {}'.format(dirent.parent_inode_num, dirent.name, dirent.inode_num, dirent.parent_inode_num), file=sys.stdout)


    filteredDirEnt = list(filter(lambda x: x.parent_inode_num != 2 and x.name == "'..'", list_dirent))
    for direntInQuestion in list(filter(lambda x: x.name == "'..'", filteredDirEnt)):
        #print("Looking at inode# {} with name {} and number {}".format(direntInQuestion.parent_inode_num, direntInQuestion.name, direntInQuestion.inode_num), file=sys.stdout)
        possibleParents = list(filter(lambda x: x.inode_num == direntInQuestion.parent_inode_num and x.parent_inode_num != direntInQuestion.parent_inode_num, list_dirent))
        #print(len(possibleParents))
        for parentDirEnt in possibleParents:
            if direntInQuestion.inode_num != parentDirEnt.parent_inode_num:
                #print('check against pnum {} inode target {} name {}'.format(parentDirEnt.parent_inode_num, parentDirEnt.inode_num, parentDirEnt.name), file= sys.stdout)
                printWithStatus('DIRECTORY INODE {} NAME {} LINK TO INODE {} SHOULD BE {}'.format(direntInQuestion.parent_inode_num, direntInQuestion.name, direntInQuestion.inode_num, parentDirEnt.parent_inode_num), file=sys.stdout)
        

    #This would probably be the method you need to modify so that it supports the last part of directory checking -> checking for the correctness of '.' and '..'


def read_csv(filename):
    super_block = None
    group = None
    inode_list = []
    list_free_blocks = []
    list_free_inodes = []
    list_indirect = []
    list_dirent = []
    inode_num_list = []

    try:
            with open(filename) as csv_file:
                csv_reader = csv.reader(csv_file)
                for field in csv_reader:
                    if field[0] == 'SUPERBLOCK':
                        super_block = SuperBlock(field)
                    if field[0] == 'INODE':
                        inode_list.append(Inode(field))
                    if field[0] == 'GROUP':
                        group = Group(field)
                    if field[0] == 'IFREE':
                        list_free_inodes.append(int(field[1]))
                        # put the number of the free inode in the list
                    if field[0] == 'BFREE':
                        list_free_blocks.append(int(field[1]))
                        #put the number of the block in the list
                    if field[0] == 'INDIRECT':
                        list_indirect.append(Indirect(field))
                    if field[0] == 'DIRENT':
                        list_dirent.append(Dirent(field))


    except IOError:
        print('Unable to read file', file=sys.stderr)
        sys.exit(1)

    inode_num_list = get_allocated_inode_nums(inode_list) #get inode numbers of allocated inodes from list -> helps with inode_allocation_audit
    block_consistency_audit(super_block, group, list_free_blocks, inode_list, list_indirect)
    inode_allocation_audit(inode_list, list_free_inodes, super_block, inode_num_list)
    directory_consistency_audit(list_dirent, super_block, inode_num_list, inode_list)

def main():
    if len(sys.argv) != 2:
        print('Usage Error: ./lab3b fileName', file=sys.stderr)
        sys.exit(1)

    read_csv(sys.argv[1])
    if isConsistent:
        sys.exit(0)
    else:
        sys.exit(2)

if __name__ == '__main__':
    main()




#Python documentation from https://docs.python.org/2/library/csv.html (reading CSV files)
