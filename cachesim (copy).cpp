#include "cachesim.hpp"
#include <math.h>
#include <stdlib.h>
#include <time.h>

// Cache declaration
struct victim_cache_struct victim_cache;
struct cache_struct l1_cache;
struct cache_struct l2_cache;
struct cache_mask_struct l1_cache_mask;
struct cache_mask_struct l2_cache_mask;
/**
 * Subroutine for initializing the cache. You many add and initialize any global or heap
 * variables as needed.
 * XXX: You're responsible for completing this routine
 *
 * @c1 The total number of bytes for data storage in L1 is 2^c1
 * @b1 The size of L1's blocks in bytes: 2^b1-byte blocks.
 * @s1 The number of blocks in each set of L1: 2^s1 blocks per set.
 * @v Victim Cache's total number of blocks (blocks are of size 2^b1).
 *    v is in [0, 4].
 * @c2 The total number of bytes for data storage in L2 is 2^c2
 * @b2 The size of L2's blocks in bytes: 2^b2-byte blocks.
 * @s2 The number of blocks in each set of L2: 2^s2 blocks per set.
 * Note: c2 >= c1, b2 >= b1 and s2 >= s1.
 */
void setup_cache(uint64_t c1, uint64_t b1, uint64_t s1, uint64_t v,
                 uint64_t c2, uint64_t b2, uint64_t s2) {

    unsigned long int i = 0;
    unsigned long int j = 0;
    unsigned long int data_size = pow(2, b1);

    victim_cache.nb_victim_cache_lines = v;
    if (v > 0){
        // Victim cache Initialisation. Start with initialising victim cache line
        victim_cache.victim_cache_lines = (struct victim_cache_line_struct *) calloc(v, sizeof(struct victim_cache_line_struct));
        victim_cache.nb_victim_cache_blocks_per_line = 1;
        victim_cache.nb_bytes_per_data_block = data_size;
        //Initialise each victim cache block and set defaults values
        for (i = 0; i < v; i++){
            victim_cache.victim_cache_lines[i].victim_cache_block = (struct victim_cache_block_struct *) calloc(1, sizeof(struct victim_cache_block_struct));
            // victim_cache.victim_cache_lines[i].victim_cache_block->data = (char *) calloc(data_size, sizeof(char));
            /** Calloc initialise everything to 0 or null. But making sure is better. */
            victim_cache.victim_cache_lines[i].victim_cache_block->writable = 1;
        }
    }

    // L1 Cache initialization
    unsigned long int index_length = pow(2, c1-b1-s1);
    l1_cache.cache_lines = (struct cache_line_struct *) calloc (index_length, sizeof(struct cache_line_struct));
    l1_cache.nb_cache_lines = index_length;

    unsigned long int N = pow(2, s1);
    l1_cache.nb_cache_blocks_per_line = N;
    l1_cache.nb_bytes_per_data_block = data_size;
    //Blocks initialization
    for(i=0; i < index_length; i++){
        l1_cache.cache_lines[i].blocks = (struct block_struct *) calloc(N, sizeof(struct block_struct));
        // structure initialization
        for (j = 0; j < N; j++){
            // At this stage, data will never be used. So no initialisation.
            // l1_cache.cache_lines[i].blocks[j].data = (char *) calloc(data_size, sizeof(char));
            l1_cache.cache_lines[i].blocks[j].dirty_bit = 0;
            l1_cache.cache_lines[i].blocks[j].valid_bit = 0;
            l1_cache.cache_lines[i].blocks[j].LRU = 0;
        }
    }
    // Compute l1 cache mask values
    l1_cache_mask.tag_mask = ((unsigned long int)pow(2, 64 - c1 + s1) - 1) << (c1 - s1);
    l1_cache_mask.index_mask = (index_length - 1) << b1; //pow(2, c1-b1-s1) - 1;
    l1_cache_mask.offset_mask = data_size - 1; //pow(2, b1) -1;
    l1_cache_mask.tag_mask_bit_length = 64 - c1 + s1;
    l1_cache_mask.index_mask_bit_length = c1-b1-s1;
    l1_cache_mask.offset_mask_bit_length = b1;


    // L2 Cache initialization
    index_length = pow(2, c2-b2-s2);
    l2_cache.cache_lines = (struct cache_line_struct *) calloc (index_length, sizeof(struct cache_line_struct));
    l2_cache.nb_cache_lines = index_length;
    data_size = pow(2, b2);
    N = pow(2, s2);
    l2_cache.nb_cache_blocks_per_line = N;
    l2_cache.nb_bytes_per_data_block = data_size;
    //Blocks initialization
    for(i=0; i < index_length; i++){
        l2_cache.cache_lines[i].blocks = (struct block_struct *) calloc(N, sizeof(struct block_struct));
        // structure initialization
        for (j = 0; j < N; j++){
            // l2_cache.cache_lines[i].blocks[j].data = (char *) calloc(data_size, sizeof(char));
            l2_cache.cache_lines[i].blocks[j].dirty_bit = 0;
            l2_cache.cache_lines[i].blocks[j].valid_bit = 0;
            l2_cache.cache_lines[i].blocks[j].LRU = 0;
        }
    }

    // Compute l2 cache mask values
    l2_cache_mask.tag_mask = ((unsigned long int) pow(2, 64 - c2 + s2) - 1)  << (c2 - s2);
    l2_cache_mask.index_mask = (index_length - 1) << b2 ; //pow(2, c2-b2-s2) - 1;
    l2_cache_mask.offset_mask = data_size - 1; //pow(2, b2) -1;
    l2_cache_mask.tag_mask_bit_length = 64 - c2 + s2;
    l2_cache_mask.index_mask_bit_length = c2-b2-s2;
    l2_cache_mask.offset_mask_bit_length = b2;
}
/**
 * Subroutine that simulates the cache one trace event at a time.
 * XXX: You're responsible for completing this routine
 *
 * @type The type of event, can be READ or WRITE.
 * @arg  The target memory address
 * @p_stats Pointer to the statistics structure
 */
void cache_access(char type, uint64_t arg, cache_stats_t* p_stats) {

    // Access in cache
    p_stats->accesses += 1;
    if (type == READ){
        p_stats->reads += 1;
    } else {
        // Maybe Write in cache ?
        if (type == WRITE){
            p_stats->writes += 1;
        }
    }


    unsigned int block_counter = 0;
    bool valid_cache_l1 = true;
    unsigned long int invalid_l1_block = 0;
    bool tag_found = false;
    bool stop_loop = false;
    /** Search in L1 first. */
    unsigned long int l1_LRU_block_index = 0;
    // First step, get the index.
    unsigned long int index_sent_l1 = (arg & l1_cache_mask.index_mask) >> l1_cache_mask.offset_mask_bit_length;
    // Second step, get the tag.
    unsigned long int tag_sent_l1 = (arg & l1_cache_mask.tag_mask) >> (l1_cache_mask.offset_mask_bit_length + l1_cache_mask.index_mask_bit_length);
    //Search for the tag in the cache line index_sent.
    while (not stop_loop){
        if (l1_cache.cache_lines[index_sent_l1].blocks[block_counter].valid_bit == 0) {
            valid_cache_l1 = false;
            // if we find at least one block in this cache line that has the valid bit set to 0,
            // then the cache is not full yet.
            invalid_l1_block = block_counter;
        } else {
            // if the valid bit is set for this line, compare the tag.
            tag_found = (l1_cache.cache_lines[index_sent_l1].blocks[block_counter].tag == tag_sent_l1);
            l1_LRU_block_index = (l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU < l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].LRU) ? block_counter : l1_LRU_block_index;

        }
        block_counter += 1;
        if ((block_counter >= l1_cache.nb_cache_blocks_per_line) or (tag_found))
            stop_loop = true;
    }
    /**
     Finish searching in cache l1. Three possible outcomes:
        1- For that particular index, the cache is not valid and we did not find the tag. Therefore, valid_cache is false and we know at least one block (invalid_l1_block)
            that we can modify. The next step here is to search for element in l2. The cache line is not full, therefore, no need for victim cache
        2- For that index, the cache is valid but we did not found the right tag. Therefore, consult the victim cache procceed accordingly
        3- For that index, the cache is maybe valid or invalid, but in any case, we found the valid line and the right tag.
    */

    /** Third possibility: The tag is found in the l1 cache, at the cache line
        This means:
            - tag_found_in_l1 = true
            - block_counter -1 = block containing the right set of information
            - valid_bit must be 1
        Next, check if read or write and set values accordingly:
            - dirty-bit if write
            - write stats
            + reads stats
            -+ LRU
    */
    if (tag_found) {
        //tag found in l1 cache.
        block_counter -= 1;
        /** Eight bits LRU */
        l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU = (l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU == LRU_MAX_VALUE)? LRU_MAX_VALUE : l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU + 1;
        l1_cache.cache_lines[index_sent_l1].last_accessed_block = block_counter;
        // Maybe Write in cache ?
        if (type == WRITE){
            l1_cache.cache_lines[index_sent_l1].blocks[block_counter].dirty_bit = 1;
        }
    } else {    // Tag not found in l1

        /** First outcome: The cache line is not full (cache not valid), and tag not found.
            * Cache l1 not full, no need for the victim cache.
            * valid_l1_cache is false
            * tag_found_in_l1 is false
            * invalid_l1_block has one invalid value we could change.

            - read or write miss for cache l1
            - next step, search in cache l2
        */
        //tag not found in l1 and l1 cache not full
        //if ((not tag_found) and (not valid_cache)){
        if (not valid_cache_l1){

            if (type == READ){
                p_stats->read_misses_l1 += 1;
            } else {
                if (type == WRITE){
                    p_stats->write_misses_l1 += 1;
                }
            }

            // Searching in l2 cache. Same procedure as searching in cache l1
            p_stats->accesses_l2 += 1;
            block_counter = 0;
            bool valid_cache_l2 = true;
            unsigned long int invalid_l2_block = 0; // Used for replacement if the cache is invalid.
            tag_found = false;
            stop_loop = false;
            /** Searching in L2. */
            unsigned long int l2_LRU_block_index = 0;
            // First step, get the index.
            unsigned long int index_sent_l2 = (arg & l2_cache_mask.index_mask) >> l2_cache_mask.offset_mask_bit_length;
            // Second step, get the tag.
            unsigned long int tag_sent_l2 = (arg & l2_cache_mask.tag_mask) >> (l2_cache_mask.offset_mask_bit_length + l2_cache_mask.index_mask_bit_length);
            // Searching
            while (not stop_loop) {
                if (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].valid_bit == 0) {
                    valid_cache_l2 = false;
                    // if we find at least one block in this cache line that has the valid bit set to 0,
                    // then the cache is not full yet.
                    invalid_l2_block = block_counter;
                } else {
                    // if the valid bit is set for this line, compare the tag.
                    tag_found = (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].tag == tag_sent_l2);
                    l2_LRU_block_index = (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU < l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU) ? block_counter : l2_LRU_block_index;
                }
                block_counter += 1;
                if ((block_counter >= l2_cache.nb_cache_blocks_per_line) or (tag_found))
                    stop_loop = true;
            }
            /** Possible outcomes:
                - The tag is found, then we should move data in l1
                - The tag is not found, :
                    * cache is invalid find an invalid block and set data ?
                    * cache is valid then we should search for the LRU and replace it (Should we move data in l1 too ??)
            */

            // 1- The tag is found in cache l2. Remove data in cache l2 and put it in cache l1 no worries since cache l1 is no full.
            if (tag_found){
                // bloc in l2 is put in l1, even if the dirty bit is set. we do not care. Remove block from l2 ??? I think no.
                // Since it is said if B does not exist in L2 anymore.
                block_counter -= 1;
                l1_cache.cache_lines[index_sent_l1].blocks[invalid_l1_block].tag = tag_sent_l1;
                l1_cache.cache_lines[index_sent_l1].blocks[invalid_l1_block].dirty_bit = l2_cache.cache_lines[index_sent_l2].blocks[block_counter].dirty_bit;
                l1_cache.cache_lines[index_sent_l1].blocks[invalid_l1_block].valid_bit = 1;
                //l1_cache.cache_lines[index_sent_l1].blocks[invalid_l1_block].data = l2_cache.cache_lines[index_sent_l2].blocks[block_counter].data;
                l1_cache.cache_lines[index_sent_l1].blocks[invalid_l1_block].LRU = 1;
                l1_cache.cache_lines[index_sent_l1].last_accessed_block = invalid_l1_block;
                // Removing block from l2.
                // l2_cache.cache_lines[index_sent_l2].blocks[block_counter].valid_bit = 0; // Do not remove
                l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU = (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU == LRU_MAX_VALUE)? LRU_MAX_VALUE : l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU + 1;;
                l2_cache.cache_lines[index_sent_l2].last_accessed_block = block_counter;
            }

            // 2- The tag is not found, and the l2 cache is not full.
            if ((not tag_found) and (not valid_cache_l2)){
                // Create data in an empty place in l2 // Should put this data in l1 ????
                l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].tag = tag_sent_l2;
                l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].LRU = 1;
                l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].valid_bit = 1;
                //l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].data = (char *) calloc(l2_cache.nb_bytes_per_data_block, sizeof(char));
                l2_cache.cache_lines[index_sent_l2].last_accessed_block = invalid_l2_block;
                // stats
                if (type == READ){
                    p_stats->read_misses_l2 += 1;
                } else {
                    if (type == WRITE){
                        p_stats->write_misses_l2 += 1;
                        l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].dirty_bit = 1;
                    }
                }
            }

            // 3- The tag is not found and the l2 cache is full, every valid bit is set. Search for the LRU and replace it
            // LRU is given by the l2_LRU_block_index
            if ((not tag_found) and (valid_cache_l2)){
                // if the smallest value of the LRU in each block is the maximum LRU value, then reset the LRU value of every block and
                // maybe randomly select one block ??
                if (l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU == LRU_MAX_VALUE){
                    // reset
                    for (block_counter = 0; block_counter < l2_cache.cache_lines[index_sent_l2].last_accessed_block; block_counter++){
                        l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU = 0;
                    }
                    for (block_counter = l2_cache.cache_lines[index_sent_l2].last_accessed_block + 1; block_counter < l2_cache.nb_cache_blocks_per_line; block_counter++){
                        l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU = 0;
                    }
                    l2_cache.cache_lines[index_sent_l2].blocks[l2_cache.cache_lines[index_sent_l2].last_accessed_block].LRU = 1;
                    /* Initialises random number generator */
                    time_t t;
                    srand((unsigned) time(&t));
                    l2_LRU_block_index = (rand() % l2_cache.nb_cache_blocks_per_line);
                    while (l2_LRU_block_index == l2_cache.cache_lines[index_sent_l2].last_accessed_block){
                        l2_LRU_block_index = (rand() % l2_cache.nb_cache_blocks_per_line);
                    }
                }
                // We have the LRU, we should test if the dirty_bit is set. If it is, then there should be a write back.
                // Next, erase the LRU and set the new infos
                if (l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit == 1){
                    p_stats->write_back_l2 += 1;
                }
                // setting infos read from memory
                if (type == READ){
                    p_stats->read_misses_l2 += 1;
                    l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit = 0;
                } else {
                    if (type == WRITE){
                        p_stats->write_misses_l2 += 1;
                        l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit = 1;
                    }
                }
                l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU = 1;
                l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].tag = tag_sent_l2;
                l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].valid_bit = 1;
                l2_cache.cache_lines[index_sent_l2].last_accessed_block = l2_LRU_block_index;

            }

        } else {
            // l1 cache is full, search the victim cache (Third outcome for l1)
            if (valid_cache_l1){
                if (type == READ){
                    p_stats->read_misses_l1 += 1;
                } else {
                    if (type == WRITE){
                        p_stats->write_misses_l1 += 1;
                    }
                }
                unsigned long int victim_cache_writable_index = WRITABLE;
                // Searching in victim cache.
                if (victim_cache.nb_victim_cache_lines > 0){
                    unsigned long int victim_cache_tag_sent = ((tag_sent_l1 << l1_cache_mask.index_mask_bit_length) | index_sent_l1);
                    block_counter = 0;
                    stop_loop = false;
                    tag_found = false;
                    p_stats->accesses_vc += 1;
                    while (not stop_loop){
                        if (victim_cache.victim_cache_lines[block_counter].victim_cache_block->tag == victim_cache_tag_sent){
                            tag_found = true;
                        }
                        if (victim_cache_writable_index == WRITABLE)
                            victim_cache_writable_index = (victim_cache.victim_cache_lines[block_counter].victim_cache_block->writable == 1)? block_counter : victim_cache_writable_index;
                        block_counter += 1;
                        if ((block_counter >= victim_cache.nb_victim_cache_lines) or (tag_found)){
                            stop_loop = true;
                        }
                    }
                    if (tag_found) {
                        // tag is found in victim cache. Search for LRU in l1 and replace it.
                        // We already know the LRU.
                        block_counter -= 1;
                        p_stats->victim_hits += 1;
                        if (l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].LRU == LRU_MAX_VALUE){
                        // reset
                            for (block_counter = 0; block_counter < l1_cache.cache_lines[index_sent_l1].last_accessed_block; block_counter++){
                                l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU = 0;
                            }
                            for (block_counter = l1_cache.cache_lines[index_sent_l1].last_accessed_block + 1; block_counter < l1_cache.nb_cache_blocks_per_line; block_counter++){
                                l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU = 0;
                            }
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_cache.cache_lines[index_sent_l1].last_accessed_block].LRU = 1;
                            /* Initialises random number generator */
                            time_t t;
                            srand((unsigned) time(&t));
                            l1_LRU_block_index = (rand() % l1_cache.nb_cache_blocks_per_line);
                            while (l1_LRU_block_index == l1_cache.cache_lines[index_sent_l1].last_accessed_block){
                                l1_LRU_block_index = (rand() % l1_cache.nb_cache_blocks_per_line);
                            }
                        }
                        // We found the LRU and now, we test if there are dirty bits sets.
                        // If no dirty bit, replace directly
                        if (l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit == 0){
                            unsigned long int l1_tag_tmp = l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].tag;
                            //unsigned long int vc_tmp_tag = victim_cache.victim_cache_lines[block_counter].victim_cache_block->tag;
                            //setting data
                            if (type == READ){
                                l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit = 0;
                            } else {
                                if (type == WRITE){
                                    l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit = 1;
                                }
                            }
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].valid_bit = 1;
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].LRU = 1;
                            l1_cache.cache_lines[index_sent_l1].last_accessed_block = l1_LRU_block_index;
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].tag = tag_sent_l1; //(vc_tmp_tag^index_sent_l1) >> l1_cache_mask.index_mask_bit_length; // Should be equal to tag_sent_l1

                            victim_cache.victim_cache_lines[block_counter].victim_cache_block->tag = ((l1_tag_tmp << l1_cache_mask.index_mask_bit_length) | index_sent_l1);
                            victim_cache.victim_cache_lines[block_counter].victim_cache_block->writable = 0;
                        }else{ /******************************** Search first is tag is present in l2 before LRU  Increase hits and miss accordingly*******************/
                            // l1 write back in l2
                            p_stats->write_back_l1 += 1;
                            // Write back in L2. Find the LRU or empty space in l2 and replace it.
                            // Searching for empty space and for the LRU
                            unsigned long int l2_LRU_block_index = 0;
                            unsigned long int l2_invalid_block = 0;
                            unsigned long int index_sent_l2 = (arg & l2_cache_mask.index_mask) >> l2_cache_mask.offset_mask_bit_length;
                            stop_loop = false;

                            //search for l1 lru tag in l2

                            if (l2_cache.cache_lines[index_sent_l2].blocks[l2_invalid_block].valid_bit == 0){
                                    stop_loop = true;
                            }
                            while (not stop_loop){
                                l2_LRU_block_index = (l2_cache.cache_lines[index_sent_l2].blocks[l2_invalid_block].LRU < l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU) ? l2_invalid_block : l2_LRU_block_index;
                                l2_invalid_block += 1;
                                if ((l2_invalid_block >= l2_cache.nb_cache_blocks_per_line) or (l2_cache.cache_lines[index_sent_l2].blocks[l2_invalid_block].valid_bit == 0)){
                                    stop_loop = true;
                                }
                            }

                            if (l2_invalid_block < l2_cache.nb_cache_blocks_per_line){
                                // Means we found an empty space.
                                l2_LRU_block_index = l2_invalid_block;
                            }
                            // Analyse l2 data.
                            if((l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].valid_bit == 1) and (l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit == 1)){
                                 // If valid bit and dirty bit then, write back l2
                                 p_stats->write_back_l2 += 1;
                            }
                            // Now can change data in l2 (writing l1 LRU to l2 LRU).
                            // Note B2 >= B1 --> bit_length(tag_l1 | index_l1) >= bit_length(tag_l2 | index_l2)
                            // Write back in L2, and in victim then, copy victim data in l1
                            unsigned long int l1_tag_tmp = l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].tag;
                            l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU = 1;
                            l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit = l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit;
                            l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].valid_bit = 1;
                            l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].tag = ((((l1_tag_tmp << l1_cache_mask.index_mask_bit_length) | index_sent_l1) << l1_cache_mask.offset_mask_bit_length) & l2_cache_mask.tag_mask) >> (l2_cache_mask.offset_mask_bit_length + l2_cache_mask.index_mask_bit_length);
                            l2_cache.cache_lines[index_sent_l2].last_accessed_block = l2_LRU_block_index;
                            // Writing data in victim cache and taking data from victim cache to l1
                            //unsigned long int vc_tmp_tag = victim_cache.victim_cache_lines[block_counter].victim_cache_block->tag;
                            //setting data
                            if (type == READ){
                                l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit = 0;
                            } else {
                                if (type == WRITE){
                                    l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit = 1;
                                }
                            }
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].valid_bit = 1;
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].LRU = 1;
                            l1_cache.cache_lines[index_sent_l1].last_accessed_block = l1_LRU_block_index;
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].tag = tag_sent_l1; //(vc_tmp_tag^index_sent_l1) >> l1_cache_mask.index_mask_bit_length; // Should be equal to tag_sent_l1

                            victim_cache.victim_cache_lines[block_counter].victim_cache_block->tag = ((l1_tag_tmp << l1_cache_mask.index_mask_bit_length) | index_sent_l1);
                            victim_cache.victim_cache_lines[block_counter].victim_cache_block->writable = 0;
                        }
                    }
                }
                 // Cache neither found in victim cache nor in l1. Now Searching in l2
                 // (If victim cache exist and tag found, then cannot take the following branch)
                if (not tag_found){
                    //Search in L2.
                    p_stats->accesses_l2 += 1;
                    block_counter = 0;
                    bool valid_cache_l2 = true;
                    unsigned long int invalid_l2_block = 0; // Used for replacement if the cache is invalid.
                    tag_found = false;
                    stop_loop = false;
                    /** Searching in L2. */
                    unsigned long int l2_LRU_block_index = 0;
                    // First step, get the index.
                    unsigned long int index_sent_l2 = (arg & l2_cache_mask.index_mask) >> l2_cache_mask.offset_mask_bit_length;
                    // Second step, get the tag.
                    unsigned long int tag_sent_l2 = (arg & l2_cache_mask.tag_mask) >> (l2_cache_mask.offset_mask_bit_length + l2_cache_mask.index_mask_bit_length);
                    // Searching
                    while (not stop_loop) {
                        if (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].valid_bit == 0) {
                            valid_cache_l2 = false;
                            // if we find at least one block in this cache line that has the valid bit set to 0,
                            // then the cache is not full yet.
                            invalid_l2_block = block_counter;
                        } else {
                            // if the valid bit is set for this line, compare the tag.
                            tag_found = (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].tag == tag_sent_l2);
                            l2_LRU_block_index = (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU < l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU) ? block_counter : l2_LRU_block_index;
                        }
                        block_counter += 1;
                        if ((block_counter >= l2_cache.nb_cache_blocks_per_line) or (tag_found))
                            stop_loop = true;
                    }
                    /** Possible outcomes:
                        - The tag is found, then we should move data in l1
                        - The tag is not found, :
                            * cache is invalid find an invalid block and set data ?
                            * cache is valid then we should search for the LRU and replace it (Should we move data in l1 too ??)
                    */

                    // 1- The tag is found in cache l2. Replacement policies should take place: replace in L1 and C.
                    if (tag_found){
                        // bloc in l2 is put in l1, even if the dirty bit is set. we do not care. Remove block from l2 ??? I think no.
                        // Since it is said if B does not exist in L2 anymore.
                        block_counter -= 1;
                        // Testing LRU value for cache l1
                        if (l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].LRU == LRU_MAX_VALUE){
                        // reset
                            for (block_counter = 0; block_counter < l1_cache.cache_lines[index_sent_l1].last_accessed_block; block_counter++){
                                l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU = 0;
                            }
                            for (block_counter = l1_cache.cache_lines[index_sent_l1].last_accessed_block + 1; block_counter < l1_cache.nb_cache_blocks_per_line; block_counter++){
                                l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU = 0;
                            }
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_cache.cache_lines[index_sent_l1].last_accessed_block].LRU = 1;
                            /* Initialises random number generator */
                            time_t t;
                            srand((unsigned) time(&t));
                            l1_LRU_block_index = (rand() % l1_cache.nb_cache_blocks_per_line);
                            while (l1_LRU_block_index == l1_cache.cache_lines[index_sent_l1].last_accessed_block){
                                l1_LRU_block_index = (rand() % l1_cache.nb_cache_blocks_per_line);
                            }
                        }
                        // We found the LRU and now, we test if there are dirty bits sets.
                        // If no dirty bit, move LRU to victim and move the data found in l2 in l1
                        if (l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit == 0){
                            // moving L1 LRU to victim cache
                            if (victim_cache.nb_victim_cache_lines > 0){
                                unsigned long int l1_tag_tmp = l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].tag;
                                // If the index is still the default value, then the first value inserted in the cache is the index 0
                                victim_cache_writable_index = (victim_cache_writable_index == WRITABLE)? 0: victim_cache_writable_index;
                                victim_cache.victim_cache_lines[victim_cache_writable_index].victim_cache_block->tag = ((l1_tag_tmp << l1_cache_mask.index_mask_bit_length) | index_sent_l1);
                                victim_cache.victim_cache_lines[victim_cache_writable_index].victim_cache_block->writable = 0;
                            }
                            // Moving data found in l2 in cache l1
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].tag = tag_sent_l1;
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit = l2_cache.cache_lines[index_sent_l2].blocks[block_counter].dirty_bit;
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].valid_bit = 1;
                            //l1_cache.cache_lines[index_sent_l1].blocks[invalid_l1_block].data = l2_cache.cache_lines[index_sent_l2].blocks[block_counter].data;
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].LRU = 1;
                            l1_cache.cache_lines[index_sent_l1].last_accessed_block = l1_LRU_block_index;
                            // Removing block from l2.
                            // l2_cache.cache_lines[index_sent_l2].blocks[block_counter].valid_bit = 0; // Do not remove
                            l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU = (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU == LRU_MAX_VALUE)? LRU_MAX_VALUE : l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU + 1;;
                            l2_cache.cache_lines[index_sent_l2].last_accessed_block = block_counter;

                        }else{  /******************************** Search first is tag is present in l2 before LRU  Increase hits and miss accordingly*******************/
                            //dirty bit set
                            // l1 write back in l2
                            p_stats->write_back_l1 += 1;
                            // Write back in L2. Find the LRU or empty space in l2 and replace it.
                            // Empty space in L2 is located in invalid_l2_block, provided that valid_cache_l2 is false
                            if (not valid_cache_l2){
                                // Means we found an empty space.
                                l2_LRU_block_index = invalid_l2_block;
                            }else{
                                //Means no empty space. Make sure the l2_lru does not correspond to the tag we are looking for
                                if (l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU == LRU_MAX_VALUE){
                                    // reset
                                    for (block_counter = 0; block_counter < l2_cache.cache_lines[index_sent_l2].last_accessed_block; block_counter++){
                                        l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU = 0;
                                    }
                                    for (block_counter = l2_cache.cache_lines[index_sent_l2].last_accessed_block + 1; block_counter < l2_cache.nb_cache_blocks_per_line; block_counter++){
                                        l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU = 0;
                                    }
                                    l2_cache.cache_lines[index_sent_l2].blocks[l2_cache.cache_lines[index_sent_l2].last_accessed_block].LRU = 1;
                                    /* Initialises random number generator */
                                    time_t t;
                                    srand((unsigned) time(&t));
                                    l2_LRU_block_index = (rand() % l2_cache.nb_cache_blocks_per_line);
                                    while ((l2_LRU_block_index == l2_cache.cache_lines[index_sent_l2].last_accessed_block) or (l2_LRU_block_index == block_counter)){
                                        l2_LRU_block_index = (rand() % l2_cache.nb_cache_blocks_per_line);
                                    }
                                }
                                // Make sure to find the second LRU in l2 if the first LRU is the address we want to access
                                if (block_counter == l2_LRU_block_index){
                                    stop_loop = false;
                                    invalid_l2_block = 0;
                                    while (not stop_loop){
                                        l2_LRU_block_index = (l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].LRU < l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU) ? invalid_l2_block : l2_LRU_block_index;
                                        invalid_l2_block += 1;
                                        if (invalid_l2_block == block_counter){
                                            invalid_l2_block += 1;
                                        }
                                        if (invalid_l2_block >= l2_cache.nb_cache_blocks_per_line){
                                            stop_loop = true;
                                        }
                                    }
                                }
                            }
                            // Analyse l2 data.
                            if((l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].valid_bit == 1) and (l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit == 1)){
                                 // If valid bit and dirty bit then, write back l2
                                 // cannot comes in if the cache is not valid
                                 p_stats->write_back_l2 += 1;
                            }
                            // Now can change data in l2 (writing l1 LRU to l2 LRU).
                            // Note B2 >= B1 --> bit_length(tag_l1 | index_l1) >= bit_length(tag_l2 | index_l2)
                            // Write back in L2,
                            unsigned long int l1_tag_tmp = l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].tag;
                            l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU = 1;
                            l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit = l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit;
                            l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].valid_bit = 1;
                            l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].tag = ((((l1_tag_tmp << l1_cache_mask.index_mask_bit_length) | index_sent_l1) << l1_cache_mask.offset_mask_bit_length) & l2_cache_mask.tag_mask) >> (l2_cache_mask.offset_mask_bit_length + l2_cache_mask.index_mask_bit_length);
                            l2_cache.cache_lines[index_sent_l2].last_accessed_block = l2_LRU_block_index;
                            // Writing data in victim cache
                            if (victim_cache.nb_victim_cache_lines > 0){
                                unsigned long int l1_tag_tmp = l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].tag;
                                // If the index is still the default value, then the first value inserted in the cache is the index 0
                                victim_cache_writable_index = (victim_cache_writable_index == WRITABLE)? 0: victim_cache_writable_index;
                                victim_cache.victim_cache_lines[victim_cache_writable_index].victim_cache_block->tag = ((l1_tag_tmp << l1_cache_mask.index_mask_bit_length) | index_sent_l1);
                                victim_cache.victim_cache_lines[victim_cache_writable_index].victim_cache_block->writable = 0;
                            }
                            // Moving data from L2 to cache l1
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].tag = tag_sent_l1;
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit = l2_cache.cache_lines[index_sent_l2].blocks[block_counter].dirty_bit;
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].valid_bit = 1;
                            //l1_cache.cache_lines[index_sent_l1].blocks[invalid_l1_block].data = l2_cache.cache_lines[index_sent_l2].blocks[block_counter].data;
                            l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].LRU = 1;
                            l1_cache.cache_lines[index_sent_l1].last_accessed_block = l1_LRU_block_index;
                            // Removing block from l2.
                            // l2_cache.cache_lines[index_sent_l2].blocks[block_counter].valid_bit = 0; // Do not remove
                            l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU = (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU == LRU_MAX_VALUE)? LRU_MAX_VALUE : l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU + 1;;
                            l2_cache.cache_lines[index_sent_l2].last_accessed_block = block_counter;
                        }
                    }

                    // 2- The tag is not found, and the l2 cache is not full.
                    if ((not tag_found) and (not valid_cache_l2)){
                        // Create data in an empty place in l2 // Should put this data in l1 ????
                        l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].tag = tag_sent_l2;
                        l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].LRU = 1;
                        l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].valid_bit = 1;
                        //l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].data = (char *) calloc(l2_cache.nb_bytes_per_data_block, sizeof(char));
                        l2_cache.cache_lines[index_sent_l2].last_accessed_block = invalid_l2_block;
                        // stats
                        if (type == READ){
                            p_stats->read_misses_l2 += 1;
                        } else {
                            if (type == WRITE){
                                p_stats->write_misses_l2 += 1;
                                l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].dirty_bit = 1;
                            }
                        }
                    }

                    // 3- The tag is not found and the l2 cache is full, every valid bit is set. Search for the LRU and replace it
                    // LRU is given by the l2_LRU_block_index
                    if ((not tag_found) and (valid_cache_l2)){
                        // if the smallest value of the LRU in each block is the maximum LRU value, then reset the LRU value of every block and
                        // maybe randomly select one block ??
                        if (l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU == LRU_MAX_VALUE){
                            // reset
                            for (block_counter = 0; block_counter < l2_cache.cache_lines[index_sent_l2].last_accessed_block; block_counter++){
                                l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU = 0;
                            }
                            for (block_counter = l2_cache.cache_lines[index_sent_l2].last_accessed_block + 1; block_counter < l2_cache.nb_cache_blocks_per_line; block_counter++){
                                l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU = 0;
                            }
                            l2_cache.cache_lines[index_sent_l2].blocks[l2_cache.cache_lines[index_sent_l2].last_accessed_block].LRU = 1;
                            /* Initialises random number generator */
                            time_t t;
                            srand((unsigned) time(&t));
                            l2_LRU_block_index = (rand() % l2_cache.nb_cache_blocks_per_line);
                            while (l2_LRU_block_index == l2_cache.cache_lines[index_sent_l2].last_accessed_block){
                                l2_LRU_block_index = (rand() % l2_cache.nb_cache_blocks_per_line);
                            }
                        }
                        // We have the LRU, we should test if the dirty_bit is se. If it is, then there should be a write back.
                        // Next, erase the LRU and set the new infos
                        if (l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit == 1){
                            p_stats->write_back_l2 += 1;
                        }
                        // setting infos read from memory
                        if (type == READ){
                            p_stats->read_misses_l2 += 1;
                            l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit = 0;
                        } else {
                            if (type == WRITE){
                                p_stats->write_misses_l2 += 1;
                                l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit = 1;
                            }
                        }
                        l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU = 1;
                        l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].tag = tag_sent_l2;
                        l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].valid_bit = 1;
                        l2_cache.cache_lines[index_sent_l2].last_accessed_block = l2_LRU_block_index;

                    }
                }
            }
        }
    }

}

/**
 * Subroutine for cleaning up any outstanding memory operations and calculating overall statistics
 * such as miss rate or average access time.
 * XXX: You're responsible for completing this routine
 *
 * @p_stats Pointer to the statistics structure
 */
void complete_cache(cache_stats_t *p_stats) {
}
