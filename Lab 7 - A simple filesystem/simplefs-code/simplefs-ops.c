#include "simplefs-ops.h"
extern struct filehandle_t file_handle_array[MAX_OPEN_FILES]; // Array for storing opened files

int simplefs_create(char *filename){
    /*
	    Create file with name `filename` from disk
	*/
	// we have to go over the inodes and check if the filename already exists
	for(int i =0; i<NUM_INODES; i++){
		struct inode_t inode;
		simplefs_readInode(i,&inode);
		if(inode.status == INODE_IN_USE && strcmp(inode.name,filename) == 0){
			return -1;
		}
	}
	// now that we know that this file doesn't already exist
	// so we create a new inode block for the file
	int ret_inode = simplefs_allocInode();
	if(ret_inode == -1)
		return -1;

	struct inode_t inode;
	simplefs_readInode(ret_inode,&inode);
	//read into structure, modify and send it back
	inode.file_size = 0;
	inode.status = INODE_IN_USE;
	memset(inode.name, 0, MAX_NAME_STRLEN);
	strncpy(inode.name, filename, MAX_NAME_STRLEN-1);
	simplefs_writeInode(ret_inode, &inode);
    return 0;
}


void simplefs_delete(char *filename){
    /*
	    delete file with name `filename` from disk
	*/
	int found = -1;
	for(int i=0; i<NUM_INODE_BLOCKS; i++){
		struct inode_t inode;
		simplefs_readInode(i,&inode);
		if(inode.status == INODE_IN_USE && strcmp(inode.name, filename) == 0){
			found = i;
			break;
		}
	}

	if(found == -1){
		return;
	}
	else{
		struct inode_t inode;
		simplefs_readInode(found,&inode);
		// for the blocks referenced by this inode, we need to free them one by one
		for(int i = 0; i<MAX_FILE_SIZE; i++){  //max file size is in blocks
			if(inode.direct_blocks[i] != -1){
				simplefs_freeDataBlock(inode.direct_blocks[i]); // -1 if free and block number if used
			}
		}
		simplefs_freeInode(found);
	}
}

int simplefs_open(char *filename){
    /*
	    open file with name `filename`
	*/
	int found = -1;
	struct inode_t inode;
	for(int i = 0; i<NUM_INODE_BLOCKS; i++){
		simplefs_readInode(i, &inode);
		if(inode.status == INODE_IN_USE && strcmp(inode.name, filename) == 0){
			found = i;
			break;
		}
	}
	if(found == -1)
    	return -1;
	
	// struct inode_t inode;
	// simplefs_readInode(found, &inode);
	
	// iterates through the open files and if it is unopened we set it to this opened file
	// this is basically the global open file table
	int handle = -1;
	for(int i = 0; i<MAX_OPEN_FILES; i++){
		if(file_handle_array[i].inode_number < 0){
			handle = i;
			file_handle_array[i].inode_number = found;
			file_handle_array[i].offset = 0;
			break;
		}
	}
	return handle;
}

void simplefs_close(int file_handle){
    /*
	    close file pointed by `file_handle`
	*/
	file_handle_array[file_handle].inode_number = -1; // basically empty
	file_handle_array[file_handle].offset = 0;
}

int simplefs_read(int file_handle, char *buf, int nbytes){
    /*
	    read `nbytes` of data into `buf` from file pointed by `file_handle` starting at current offset
	*/
	struct filehandle_t *fh = &(file_handle_array[file_handle]);

	// get the inode corresponding to the file
	if(fh->inode_number < 0){
		return -1;
	}
	struct inode_t inode;
	simplefs_readInode(fh->inode_number, &inode);

	if(inode.status == INODE_FREE){ // obviously shouldn't be free
		return -1;
	}

	if(fh->offset + nbytes > inode.file_size){
		return -1;
	}

	if(nbytes <= 0){
		return -1;
	}

	// simply count the number of blocks, atmost 4 atleast 0.
	int num_blks = 0;
	for (int i = 0; i < MAX_FILE_SIZE; i++)
	{
		if(inode.direct_blocks[i] != -1){
			num_blks++;
		}
	}

	int blk = fh->offset / BLOCKSIZE; //current block using offset
	int off_in_blk = fh->offset % BLOCKSIZE; // offset inside the block
	int bytes_left_to_read = nbytes; // input se

	while((blk < num_blks) && (bytes_left_to_read > 0)){ //start from current block
		char blk_data[BLOCKSIZE];
		simplefs_readDataBlock(inode.direct_blocks[blk], blk_data);
		if(off_in_blk + bytes_left_to_read > BLOCKSIZE){
			memcpy(buf + (nbytes - bytes_left_to_read), blk_data + off_in_blk, BLOCKSIZE - off_in_blk);
			blk++;
			bytes_left_to_read -= (BLOCKSIZE - off_in_blk);
			off_in_blk = 0;
		}
		else{
			memcpy(buf + (nbytes - bytes_left_to_read), blk_data + off_in_blk, bytes_left_to_read);
			bytes_left_to_read = 0;
			break;
		}
	}


    return 0;
}


int simplefs_write(int file_handle, char *buf, int nbytes){
    /*
	    write `nbytes` of data from `buf` to file pointed by `file_handle` starting at current offset
	*/

	// first we obtain the file_array
	struct filehandle_t *fh = &file_handle_array[file_handle];

	//get the inode now
	if(fh->inode_number < 0){
		return -1;
	}
	struct inode_t inode;
	simplefs_readInode(fh->inode_number,&inode);

	//some simple checks
	if(inode.status == INODE_FREE){ // obviously shouldn't be free
		return -1;
	}

	if(fh->offset + nbytes > MAX_FILE_SIZE * BLOCKSIZE){ //number of blocks * blocksize
		return -1;
	}

	if(nbytes <= 0){ 
		return -1;
	}

	// get the number of blocks!
	int num_blks = 0;
	for (int i = 0; i < MAX_FILE_SIZE; i++)
	{
		if(inode.direct_blocks[i] != -1){
			num_blks++;
		}
	}

	// now write logic very similarly to read logic
	int blk = fh->offset / BLOCKSIZE; //current block using offset
	int off_in_blk = fh->offset % BLOCKSIZE; // offset inside the block
	int bytes_left_to_write = nbytes; // input se

	while((blk < num_blks) && (bytes_left_to_write > 0)){ //start from current block
		char blk_data[BLOCKSIZE];
		simplefs_readDataBlock(inode.direct_blocks[blk], blk_data);
		if(off_in_blk + bytes_left_to_write > BLOCKSIZE){
			memcpy(blk_data + off_in_blk, buf + (nbytes - bytes_left_to_write), BLOCKSIZE - off_in_blk);
			simplefs_writeDataBlock(inode.direct_blocks[blk], blk_data);
			blk++;
			bytes_left_to_write -= (BLOCKSIZE - off_in_blk);		
			off_in_blk = 0;
		}
		else{
			memcpy(blk_data + off_in_blk, buf + (nbytes - bytes_left_to_write), bytes_left_to_write);
			simplefs_writeDataBlock(inode.direct_blocks[blk], blk_data);
			bytes_left_to_write = 0;
			break;
		}
	}

	inode.file_size = (fh->offset + nbytes > inode.file_size) ? (fh->offset + nbytes) : inode.file_size;
	simplefs_writeInode(fh->inode_number, &inode);
	// this update also needs to be done
    return 0;
}


int simplefs_seek(int file_handle, int nseek){
    /*
	   increase `file_handle` offset by `nseek`
	*/
	struct filehandle_t* fh = &(file_handle_array[file_handle]); // get the fh structure
	if(fh->inode_number < 0){
		return -1;
	}
	struct inode_t inode;
	simplefs_readInode(fh->inode_number, &inode); //obtain the inode number
	if(inode.status == INODE_FREE){
		return -1;
	}

	if(nseek < 0){
		fh->offset = (fh->offset + nseek > 0) ? fh->offset + nseek : 0;
	}
	else{
		fh->offset = (fh->offset + nseek < inode.file_size) ? (fh->offset + nseek) : (inode.file_size);
	}
    return 0;
}