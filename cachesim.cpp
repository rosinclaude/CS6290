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
 * Subroutine to search in the cache given as parameter. All parameters are addresses, except index_ and tag_to_search as they do not have to be modified
 * @cache The cache in which we should search for the tag
 * @valid_cache  Boolean holding the state of the cache: True -> The cache is full; False -> The cache is not full
 * @invalid_block In case the valid_cache boolean is false, Invalid block will hold the last empty index in the cache
 * @LRU_block_index Holds the index of the block that is the least recently used
 * @tag_found Boolean holding the state of the search.
 * @block_counter if the tag is found, this variable hold the block position of the tag we were looking for in the cache
 * @index_ The cache index deduced from the memory address given in by the CPU ()
 * @tag_to_search tag to search for in the cache
 */
 void search_in_cache(struct cache_struct *cache, bool *valid_cache, unsigned long int *invalid_block,
                        unsigned long int *LRU_block_index, bool *tag_found, unsigned long int *block_counter,
                        unsigned long int index_, unsigned long int tag_to_search){

    *tag_found = false;
    *valid_cache = true;
    *LRU_block_index = 0;
    *block_counter = 0;
    bool stop_loop = false;

    while (not stop_loop){
        if (cache->cache_lines[index_].blocks[*block_counter].valid_bit == 0) {
            *valid_cache = false;
            // if we find at least one block in this cache line that has the valid bit set to 0,
            // then the cache is not full yet.
            // Make sure that the LRU is not the last value accessed.
            if (*block_counter != cache->cache_lines[index_].last_accessed_block){
                *invalid_block = *block_counter;
            }
        } else {
            // if the valid bit is set for this line, compare the tag.
            *tag_found = (cache->cache_lines[index_].blocks[*block_counter].tag == tag_to_search);
            *LRU_block_index = (cache->cache_lines[index_].blocks[*block_counter].LRU < cache->cache_lines[index_].blocks[*LRU_block_index].LRU) ? *block_counter : *LRU_block_index;

        }
        *block_counter += 1;
        if ((*block_counter >= cache->nb_cache_blocks_per_line) or (*tag_found))
            stop_loop = true;
    }
    *block_counter -= 1;
 }


/**
 * Subroutine to read data from ram and setting data in the given cache parameters
 * @cache The address of the cache in which the data read should be put
 * @index_ The index in the cache in which the data should be put
 * @block_ the block number in the set (the index give the set number) in which we should put data
 * @tag the tag associated whith the data read.
 */
void read_ram_set_elements_in_cache(struct cache_struct *cache, unsigned long int index_,
                            unsigned long int block_, unsigned long int tag){

    cache->cache_lines[index_].blocks[block_].tag = tag;
    // Newly read data from ram, so the LRU is set to 1
    cache->cache_lines[index_].blocks[block_].LRU = 1;
    cache->cache_lines[index_].blocks[block_].valid_bit = 1;
    // No data to allocate
    // Since read from ram, it is the last accessed block.
    cache->cache_lines[index_].last_accessed_block = block_;
}

/**
 * Subroutine for setting the LRU index.
 * @cache The address of the cache in which the lru is extracted
 * @index_ The index in the cache (or the set number) in which the lru is located
 * @lru_index The address of the lru index we found in the given cache.
 */
void set_Least_Recently_used_index(struct cache_struct *cache, unsigned long int index_,
                    unsigned long int *lru_index){
    /** if the smallest value of the LRU in each block is the maximum LRU value,
        then reset the LRU value of every block except the last accessed one.
        Maybe randomly select one block that has the LRU set to 0??*/
    if (cache->cache_lines[index_].blocks[*lru_index].LRU == LRU_MAX_VALUE){
        // reset every LRU except the last accessed one.
        unsigned long int block_counter = 0;
        for (block_counter = 0; block_counter <
        cache->cache_lines[index_].last_accessed_block; block_counter++){
            cache->cache_lines[index_].blocks[block_counter].LRU = 0;
        }
        for (block_counter = cache->cache_lines[index_].last_accessed_block + 1;
         block_counter < cache->nb_cache_blocks_per_line; block_counter++){
            cache->cache_lines[index_].blocks[block_counter].LRU = 0;
        }
        cache->cache_lines[index_].blocks[cache->cache_lines[index_].last_accessed_block].LRU = 1;
        // Randomly select one number except the last accessed one
        time_t t;
        srand((unsigned) time(&t));
        *lru_index = (rand() % cache->nb_cache_blocks_per_line);
        while (*lru_index == cache->cache_lines[index_].last_accessed_block){
            *lru_index = (rand() % cache->nb_cache_blocks_per_line);
        }
    }
}

/**
 * Subroutine to search the victim cache.
 * @v_cache Victim cache in which we should search for the tag
 * @writable_index contains the address of the first index in the victim cache where we can write data
 * @block_counter contains the block number  in which we foumd the tag in the victim cache.
 * @tag_found boolean to tell if we found the tag in the victim cache or not
 * @victim_cache_tag_sent tag to search for in the victim cache.
 */
void search_in_vcache(struct victim_cache_struct v_cache,
                    unsigned long int *writable_index,
                    unsigned long int *block_counter, bool *tag_found,
                    unsigned long int victim_cache_tag_sent){

    *block_counter = 0;
    *tag_found = false;
    bool stop_loop = false;
    while (not stop_loop){
        if (v_cache.victim_cache_lines[*block_counter].victim_cache_block->tag
         == victim_cache_tag_sent){
            *tag_found = true;
        }
        if (*writable_index == WRITABLE)
            *writable_index =
            (v_cache.victim_cache_lines[*block_counter].victim_cache_block->writable
             == 1)? *block_counter : *writable_index;
        *block_counter += 1;
        if ((*block_counter >= v_cache.nb_victim_cache_lines) or (*tag_found)){
            stop_loop = true;
        }
    }
    *block_counter -= 1;
}

/**
 * Subroutine to exchange data between the victim cache and the l1 cache.
 * @type The type of access to the cache (r or w)
 * @victim_cache the victim cache in which we should change data
 * @vc_tag_f_index the index of the tag found in the victim cache
 * @cache Mostly the l1 cache, or the cache used to change data with the corresponding victim cache
 * @index_c The index in the cache given by the processor
 * @cache_lru The least recently used index of the l1 cache
 * @cache_mask The cache mask structure associated with the cache
 * @tag_searched The tag searched in L1
 */
void exchange_vc_and_l1c_els(char type, struct victim_cache_struct *v_cache,
                unsigned long int vc_tag_f_index, struct cache_struct *cache,
                unsigned long int index_c, unsigned long int cache_lru,
                struct cache_mask_struct cache_mask, unsigned long int tag_searched){

    unsigned long int cache_tag_temp = cache->cache_lines[index_c].blocks[cache_lru].tag;
    // Updating cache
    read_ram_set_elements_in_cache(cache, index_c, cache_lru, tag_searched);
    if (type == READ){
        cache->cache_lines[index_c].blocks[cache_lru].dirty_bit = 0;
    } else {
        if (type == WRITE){
            cache->cache_lines[index_c].blocks[cache_lru].dirty_bit = 1;
        }
    }
    //cache->cache_lines[index_c].blocks[cache_lru].tag = tag_sent_l1; //(vc_tmp_tag^index_sent_l1) >> l1_cache_mask.index_mask_bit_length; // Should be equal to tag_sent_l1
    // Updating Victim cache.
    v_cache->victim_cache_lines[vc_tag_f_index].victim_cache_block->tag = ((cache_tag_temp << cache_mask.index_mask_bit_length) | index_c);
    v_cache->victim_cache_lines[vc_tag_f_index].victim_cache_block->writable = 0;
}

/**
 * Subroutine for l1 write back in l2. It consist in searching for the tag in l2 and do some operations depending on the result
 * @p_stats  address of the Structure for statitics
 * @level1_c address of level 1 cache structure
 * @level2_c address of level 2 cache structure
 * @level1_c_mask level1 cache mask structure
 * @level2_c_mask level2 cache mask strucutre
 * @level1_lru least recently used block number (in the set) in level 1 cache
 * @level1_index Index for level 1 cache sent by the CPU
 */
 void write_back_level_1_cache(struct cache_stats_t* p_stats, struct cache_struct* level1_c,
                struct cache_struct* level2_c, struct cache_mask_struct level1_c_mask,
                struct cache_mask_struct level2_c_mask, unsigned long int level1_lru,
                unsigned long int level1_index){
    // Updating stats
    p_stats->write_back_l1 += 1;
    p_stats->accesses_l2 += 1; // L1 Write back means we access l2 cache.
    p_stats->writes += 1; // Write back in L2 is a write.

    bool valid_l2_cache = true;
    bool l1_lru_tag_found_in_l2 = false;
    unsigned long int invalid_l2_block = 0;
    unsigned long int l2_lru_block_index = 0;
    unsigned long int l2_block_counter = 0;
    // Setting the index and the tag for L2 cache
    unsigned long int l1_lru_tag = level1_c->cache_lines[level1_index].blocks[level1_lru].tag;
    unsigned long int pseudo_mem_address = l1_lru_tag << (level1_c_mask.index_mask_bit_length + level1_c_mask.offset_mask_bit_length);
    pseudo_mem_address = pseudo_mem_address | (level1_index << level1_c_mask.offset_mask_bit_length);
    unsigned long int l1_lru_index_in_l2 = (pseudo_mem_address & level2_c_mask.index_mask) >> level2_c_mask.offset_mask_bit_length;
    unsigned long int l1_lru_tag_in_l2 = (pseudo_mem_address & level2_c_mask.tag_mask) >> (level2_c_mask.index_mask_bit_length + level2_c_mask.offset_mask_bit_length);
    // Searching for the right tag at the right index in l2
    search_in_cache(level2_c, &valid_l2_cache, &invalid_l2_block, &l2_lru_block_index,
    &l1_lru_tag_found_in_l2, &l2_block_counter, l1_lru_index_in_l2, l1_lru_tag_in_l2);

    if (l1_lru_tag_found_in_l2){
        // The l1 LRU tag is found in l2. We should update its value
        level2_c->cache_lines[l1_lru_index_in_l2].blocks[l2_block_counter].LRU =
        (level2_c->cache_lines[l1_lru_index_in_l2].blocks[l2_block_counter].LRU ==
        LRU_MAX_VALUE)? LRU_MAX_VALUE :
        level2_c->cache_lines[l1_lru_index_in_l2].blocks[l2_block_counter].LRU + 1;
        // When write back from l1, it means that this data has not been saved in memory.
        // Therefore, it should be marked as dirty. L2 will write it back in ram at the right time
        level2_c->cache_lines[l1_lru_index_in_l2].blocks[l2_block_counter].dirty_bit = 1;
        level2_c->cache_lines[l1_lru_index_in_l2].blocks[l2_block_counter].valid_bit = 1;
        level2_c->cache_lines[l1_lru_index_in_l2].last_accessed_block = l2_block_counter;
    } else {
        // Tag not found. Write miss ?
        // Updating stats
        p_stats->write_misses_l2 += 1;
        if ((not l1_lru_tag_found_in_l2) and (not valid_l2_cache)){
            // Tag not found but found some empty space in l2. Just write it back. (By the way, it is a write miss? seems like)
            // Writing in empty space located at invalid_l2_block.
            read_ram_set_elements_in_cache(level2_c, l1_lru_index_in_l2, invalid_l2_block, l1_lru_tag_in_l2);
            level2_c->cache_lines[l1_lru_index_in_l2].blocks[invalid_l2_block].dirty_bit = 1;
        } else {
            if ((not l1_lru_tag_found_in_l2) and (valid_l2_cache)){
                // Tag not found, no empty space. Setting the level 2 cache LRU
                set_Least_Recently_used_index(level2_c, l1_lru_index_in_l2, &l2_lru_block_index);
                // Testing if l2_lru has the dirty bit set.
                if (level2_c->cache_lines[l1_lru_index_in_l2].blocks[l2_lru_block_index].dirty_bit == 1){
                    // Dirty bit set, L2 write back in ram.
                    p_stats->write_back_l2 += 1;
                }
                read_ram_set_elements_in_cache(level2_c, l1_lru_index_in_l2, l2_lru_block_index, l1_lru_tag_in_l2);
                level2_c->cache_lines[l1_lru_index_in_l2].blocks[l2_lru_block_index].dirty_bit = 1;
            }
        }
    }
 }

/**
 * Subroutine to put the least recently used element of l1 in the victim cache.
 * @P_stats Address of the statistic structure
 * @v_cache Address of the victim cache in which we should modify elements
 * @victim_cache_writable_index index in which we should write the data in the victim cache
 * @level1_lru_index least recently used tag in level 1 cache
 * @index_l1 Index sent by the CPU, used in level 1 cache
 * @cache Level 1 cache corresponding to the index sent
 * @level1_c_mask cache mask corresponding to level 1 cache
 */
void put_l1_el_in_vc(struct cache_stats_t* p_stats, struct victim_cache_struct *v_cache,
    unsigned long int *victim_cache_writable_index, unsigned long int level1_lru_index,
    unsigned long int index_l1, struct cache_struct cache, struct cache_mask_struct level1_c_mask){
    // moving L1 LRU to victim cache
    if (v_cache->nb_victim_cache_lines > 0){
        unsigned long int l1_tag_tmp = cache.cache_lines[index_l1].blocks[level1_lru_index].tag;
        // If the index is still the default value, then the first value inserted in the cache is the index 0
        *victim_cache_writable_index = (*victim_cache_writable_index == WRITABLE)? 0: *victim_cache_writable_index;
        v_cache->victim_cache_lines[*victim_cache_writable_index].victim_cache_block->tag =
        ((l1_tag_tmp << level1_c_mask.index_mask_bit_length) | index_l1);
        v_cache->victim_cache_lines[*victim_cache_writable_index].victim_cache_block->writable = 0;
        p_stats->accesses_vc += 1;
    }
}

/**
 * Subroutine to copy the element with the right tag from cache l2 to cache l1
 * @level1_c address of the level 1 cache
 * @level2_c address of the level 2 cache
 * @index_l1 Index sent by the CPU used in l1 cache (The cpu send an address from which the index is extracted)
 * @index_l2 Index sent by the CPU used in l2 cache (The cpu send an address from which the index is extracted)
 * @block_index_l1 block of interest from the index sent in level1 cache
 * @block_index_l2 block of interest from the index sent in level2 cache
 * @tag_l1 Tag extracted from address sent by the CPU and used in cache l1
 */
 void copy_tag_found_in_l2_to_l1_cache(struct cache_struct *level1_c, struct cache_struct *level2_c,
                    unsigned long int index_l1, unsigned long int index_l2,
                    unsigned long int block_index_l1, unsigned long int block_index_l2,
                    unsigned long int tag_l1){
    // No stats to update here.
    // Setting the tag to an invalid place
    level1_c->cache_lines[index_l1].blocks[block_index_l1].tag =
    tag_l1;
    // Setting the dirty bit (Just copy without l2 write back)
    level1_c->cache_lines[index_l1].blocks[block_index_l1].dirty_bit
     = level2_c->cache_lines[index_l2].blocks[block_index_l2].dirty_bit;
    // Set the new place as valid
    level1_c->cache_lines[index_l1].blocks[block_index_l2].valid_bit = 1;
    // No data to copy
    // Newly accessed data. Increment the LRU value
    level1_c->cache_lines[index_l1].blocks[block_index_l2].LRU = 1;
    // Set this new place as the last accessed
    level1_c->cache_lines[index_l1].last_accessed_block = block_index_l1;
    // Increment the LRU value of this block in l2 since it was accessed.
    level2_c->cache_lines[index_l2].blocks[block_index_l2].LRU =
    (level2_c->cache_lines[index_l2].blocks[block_index_l2].LRU ==
     LRU_MAX_VALUE)? LRU_MAX_VALUE :
     level2_c->cache_lines[index_l2].blocks[block_index_l2].LRU + 1;;
    // Set block counter as the last accessed block in l2
    level2_c->cache_lines[index_l2].last_accessed_block =
    block_index_l2;

 }

/**
 * Subroutine used to write back in l2, and move element in the victim cache in case we are searching in cache l2 and the cache l1 is valid
 * @level1_c Address of the level 1 cache
 * @level2_c Address of the level 2 cache
 * @v_cache Address of the victim cache
 * @level1_c_mask memory Address bit masks corresonding to the l1_cache
 * @level2_c_mask memory Address bit masks corresonding to the l2 cache
 * @level1_lru least recently used block index in cache l1
 * @level1_index Memory index sent by the CPU to the cache l1
 * @level2_index Memory index sent by the CPU to the cache l2
 * @tag_found_in_l2 boolean used to determine if the tag we were looking for has been found in l2 cache
 * @level2_lru least recently used block index in cache l2
 * @p_stats Address of the statistic structure
 * @tag_block_in_l2 block index corresponding to the tag we were looking for in l2 cache
 * @v_cache_writable_index Index of a block where we can erase data in the victim cache
 * @tag_sent_l1 Memory tag sent by the CPU to cache l1
 */
void write_back_l1_move_to_vc_copy_tag_found_in_l2(struct cache_struct *level1_c,
            struct cache_struct *level2_c, struct victim_cache_struct *v_cache,
            struct cache_mask_struct level1_c_mask,struct cache_mask_struct level2_c_mask,
            unsigned long int *level1_lru, unsigned long int level1_index,
            unsigned long int level2_index, bool tag_found_in_l2, unsigned long int *level2_lru,
            struct cache_stats_t* p_stats, unsigned long int tag_block_in_l2,
            unsigned long int *v_cache_writable_index, unsigned long int tag_sent_l1){

    // Setting the LRU in l1 cache
    set_Least_Recently_used_index(level1_c, level1_index, level1_lru);
    if (level1_c->cache_lines[level1_index].blocks[*level1_lru].dirty_bit == 1){
        // Dirty bit is set. Write back in l2
        if (tag_found_in_l2){
            // Make sure the LRU in cache l2 is not the element we try to access.
            // If it is the case, find the second LRU and set its value to lower than
            // the first LRU we have in cache l2. The write back routine will take care of the rest
            if (*level2_lru == tag_block_in_l2){
                // Finding the second LRU that is not the last block accessed
                bool stop_loop = false;
                unsigned long int second_l2_lru = 0;
                unsigned long int counter = 0;
                while (not stop_loop){
                    if ((level2_c->cache_lines[level2_index].blocks[counter].LRU < level2_c->cache_lines[level2_index].blocks[second_l2_lru].LRU)
                     and (counter != tag_block_in_l2)){
                     // Make sure that the LRU is not the last value accessed.
                        if (counter != level2_c->cache_lines[level2_index].last_accessed_block){
                            second_l2_lru = counter;
                        }
                    }
                    counter += 1;
                    if (counter >= level2_c->nb_cache_blocks_per_line){
                        stop_loop = true;
                    }
                }
                // Making the second LRU less than the first LRU
                if (level2_c->cache_lines[level2_index].blocks[tag_block_in_l2].LRU > 0){
                    level2_c->cache_lines[level2_index].blocks[second_l2_lru].LRU = level2_c->cache_lines[level2_index].blocks[tag_block_in_l2].LRU - 1;
                }else{
                    level2_c->cache_lines[level2_index].blocks[second_l2_lru].LRU = 0;
                    level2_c->cache_lines[level2_index].blocks[tag_block_in_l2].LRU = 1;
                }
            }
        }
        // The LRU has the diry bit set.l1 write back in l2
        write_back_level_1_cache(p_stats, level1_c, level2_c,
        level1_c_mask, level2_c_mask, *level1_lru, level1_index);

    }

    put_l1_el_in_vc(p_stats, v_cache, v_cache_writable_index, *level1_lru,
    level1_index, *level1_c, level1_c_mask);
     if (tag_found_in_l2){
        // Then we should copy data from l2 to l1 LRU block index.
        copy_tag_found_in_l2_to_l1_cache(level1_c, level2_c, level1_index,
            level2_index, *level1_lru, tag_block_in_l2, tag_sent_l1);
     }
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
        // Total reads
        p_stats->reads += 1;
    } else {
        // Total writes
        if (type == WRITE){
            p_stats->writes += 1;
        }
    }


    unsigned long int block_counter = 0;
    bool valid_l1_cache = true;
    unsigned long int invalid_l1_block = 0;
    bool tag_found_in_l1 = false;
    /** Search in L1 first. */
    unsigned long int l1_LRU_block_index = 0;
    // First step, get the index.
    unsigned long int index_sent_l1 = (arg & l1_cache_mask.index_mask) >>
    l1_cache_mask.offset_mask_bit_length;
    // Second step, get the tag.
    unsigned long int tag_sent_l1 = (arg & l1_cache_mask.tag_mask) >>
    (l1_cache_mask.offset_mask_bit_length + l1_cache_mask.index_mask_bit_length);
    //Search for the tag in the cache line index_sent.
    search_in_cache(&l1_cache, &valid_l1_cache, &invalid_l1_block,
    &l1_LRU_block_index, &tag_found_in_l1, &block_counter, index_sent_l1,
    tag_sent_l1);
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
    if (tag_found_in_l1) {
        /** Eight bits LRU. Either add 1 or set the max value. */
        l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU =
        (l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU ==
        LRU_MAX_VALUE)? LRU_MAX_VALUE :
        l1_cache.cache_lines[index_sent_l1].blocks[block_counter].LRU + 1;
        // Set the last accessed block in l1
        l1_cache.cache_lines[index_sent_l1].last_accessed_block = block_counter;
        // If write, set the dirty bit
        if (type == WRITE){
            l1_cache.cache_lines[index_sent_l1].blocks[block_counter].dirty_bit = 1;
        }
    } else {
        /** First outcome: The cache line is not full (cache not valid),
            and tag is not found in l1.
            * Cache l1 not full, no need for the victim cache.
            * valid_l1_cache is false
            * tag_found_in_l1 is false
            * invalid_l1_block has one invalid value we could change.

            - read or write miss for cache l1
            - next step, search in cache l2
        */
        if ((not tag_found_in_l1) and (not valid_l1_cache)){

            if (type == READ){
                p_stats->read_misses_l1 += 1;
            } else {
                if (type == WRITE){
                    p_stats->write_misses_l1 += 1;
                }
            }

            /** Searching in L2 cache */
            p_stats->accesses_l2 += 1;
            block_counter = 0;
            bool valid_l2_cache = true;
            unsigned long int invalid_l2_block = 0;
            bool tag_found_in_l2 = false;
            /** Searching in L2. */
            unsigned long int l2_LRU_block_index = 0;
            // First step, get the index.
            unsigned long int index_sent_l2 = (arg & l2_cache_mask.index_mask) >> l2_cache_mask.offset_mask_bit_length;
            // Second step, get the tag.
            unsigned long int tag_sent_l2 = (arg & l2_cache_mask.tag_mask) >> (l2_cache_mask.offset_mask_bit_length + l2_cache_mask.index_mask_bit_length);
            // Searching
            search_in_cache(&l2_cache, &valid_l2_cache, &invalid_l2_block,
            &l2_LRU_block_index, &tag_found_in_l2, &block_counter, index_sent_l2,
            tag_sent_l2);

            /** Possible outcomes:
                - The tag is found, then we should move data in l1
                - The tag is not found, :
                    * cache is invalid find an invalid block and set data ?
                    * cache is valid then we should search for the LRU and replace it (Should we move data in l1 too ??)
            */

            /** 1- The tag is found in cache l2.
                Copy data from l2 to l1. Remember: here, l1_cache is not full yet.
            */
            if (tag_found_in_l2){
                copy_tag_found_in_l2_to_l1_cache(&l1_cache, &l2_cache, index_sent_l1,
                index_sent_l2, invalid_l1_block, block_counter, tag_sent_l1);
            }

            /** 2- The tag is not found in l2, and the l2 cache is not full.
                L1 and  L2 cache are not full yet. There are invalid place in both */
            if ((not tag_found_in_l2) and (not valid_l2_cache)){
                // Read data from ram and place it in l2 cache
                read_ram_set_elements_in_cache(&l2_cache, index_sent_l2,
                invalid_l2_block, tag_sent_l2);
                // Also set data in l1 cache
                read_ram_set_elements_in_cache(&l1_cache, index_sent_l1,
                invalid_l1_block, tag_sent_l1);
                // stats
                if (type == READ){
                    p_stats->read_misses_l2 += 1;
                } else {
                    if (type == WRITE){
                        p_stats->write_misses_l2 += 1;
                        // If write, only set dirty bit in l1, since data will be write back from l1 to l2 if it is not used.
                        l1_cache.cache_lines[index_sent_l1].blocks[invalid_l1_block].dirty_bit = 1;
                        /** If the teacher tell us to set the dirty bit in both, then uncomment the following line */
                        // l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].dirty_bit = 1;
                    }
                }
            }

            /** 3- The tag is not found in l2 and the l2 cache is full (l1 still have some place left),
            every valid bit is set. Search for the LRU and replace it LRU is given by the l2_LRU_block_index */
            if ((not tag_found_in_l2) and (valid_l2_cache)){
                // Setting the LRU value
                set_Least_Recently_used_index(&l2_cache, index_sent_l2, &l2_LRU_block_index);
                // Updating stats if the LRU has the dirty bit set.
                if (l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].dirty_bit == 1){
                    p_stats->write_back_l2 += 1;
                }
                // Read data from ram and place it in l2 cache
                read_ram_set_elements_in_cache(&l2_cache, index_sent_l2,
                l2_LRU_block_index, tag_sent_l2);
                // Also set data in l1 cache
                read_ram_set_elements_in_cache(&l1_cache, index_sent_l1,
                invalid_l1_block, tag_sent_l1);
                // stats
                if (type == READ){
                    p_stats->read_misses_l2 += 1;
                } else {
                    if (type == WRITE){
                        p_stats->write_misses_l2 += 1;
                        // If write, only set dirty bit in l1, since data will be write back from l1 to l2 if it is not used.
                        l1_cache.cache_lines[index_sent_l1].blocks[invalid_l1_block].dirty_bit = 1;
                        /** If the teacher tell us to set the dirty bit in both, then uncomment the following line */
                        // l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].dirty_bit = 1;
                    }
                }
            }

        } else {
            /** l1 cache is full, we should search for the tag in the victim cache first */
            if ((not tag_found_in_l1) and (valid_l1_cache)){
                // Setting stats
                if (type == READ){
                    p_stats->read_misses_l1 += 1;
                } else {
                    if (type == WRITE){
                        p_stats->write_misses_l1 += 1;
                    }
                }

                unsigned long int victim_cache_writable_index = WRITABLE;
                bool tag_found_in_vc = false;
                // Searching in victim cache.
                if (victim_cache.nb_victim_cache_lines > 0){
                    // Updating stats
                    p_stats->accesses_vc += 1;
                    // Searching in the victim cache
                    block_counter = 0;
                    // Victim cache tag is longer than l1 tag. If not, we could have two same tags in the victim cache as the index would not be present, unlike l1
                    // Victim cache tag is compose of the index appended at the end of the l1 tags.
                    // Victim cache tag should hold information on the index.
                    unsigned long int victim_cache_tag_sent =
                    ((tag_sent_l1 << l1_cache_mask.index_mask_bit_length) | index_sent_l1);

                    search_in_vcache(victim_cache, &victim_cache_writable_index,
                     &block_counter, &tag_found_in_vc, victim_cache_tag_sent);

                    if (tag_found_in_vc) {
                     /** The tag is found in the victim cache. We should search for the LRU in the l1 cache  and replace it.
                         The l1 LRU is already known. */
                        // Updating stats
                        p_stats->victim_hits += 1;
                        // Setting the lru for l1
                        set_Least_Recently_used_index(&l1_cache, index_sent_l1, &l1_LRU_block_index);

                        // Test if the LRU has the dirty bit set.
                        if (l1_cache.cache_lines[index_sent_l1].blocks[l1_LRU_block_index].dirty_bit == 0){
                            // If there is no dirty bits set, then we should directly
                            // exchange data between the l1_cache and the victim cache
                            exchange_vc_and_l1c_els(type, &victim_cache, block_counter,
                            &l1_cache, index_sent_l1, l1_LRU_block_index, l1_cache_mask, tag_sent_l1);
                        }else{
                            // The LRU has the diry bit set.l1 write back in l2
                            write_back_level_1_cache(p_stats, &l1_cache, &l2_cache,
                            l1_cache_mask, l2_cache_mask, l1_LRU_block_index, index_sent_l1);
                            // Exchanging data between the l1 and victim cache
                            exchange_vc_and_l1c_els(type, &victim_cache, block_counter,
                            &l1_cache, index_sent_l1, l1_LRU_block_index, l1_cache_mask, tag_sent_l1);
                        }
                    }
                }
                 /** Tag neither found in victim cache nor in l1. Searching in l2.
                  (If victim cache exist and tag found, then cannot take the following branch)*/
                if (not tag_found_in_vc){
                    // Updating Stats
                    p_stats->accesses_l2 += 1;

                    /** Searching in L2 cache */
                    block_counter = 0;
                    bool valid_l2_cache = true;
                    unsigned long int invalid_l2_block = 0;
                    bool tag_found_in_l2 = false;
                    /** Searching in L2. */
                    unsigned long int l2_LRU_block_index = 0;
                    // First step, get the index.
                    unsigned long int index_sent_l2 = (arg & l2_cache_mask.index_mask) >> l2_cache_mask.offset_mask_bit_length;
                    // Second step, get the tag.
                    unsigned long int tag_sent_l2 = (arg & l2_cache_mask.tag_mask) >> (l2_cache_mask.offset_mask_bit_length + l2_cache_mask.index_mask_bit_length);
                    // Searching
                    search_in_cache(&l2_cache, &valid_l2_cache, &invalid_l2_block,
                    &l2_LRU_block_index, &tag_found_in_l2, &block_counter, index_sent_l2,
                    tag_sent_l2);

                    if (tag_found_in_l2){
                        write_back_l1_move_to_vc_copy_tag_found_in_l2(&l1_cache, &l2_cache, &victim_cache,
                        l1_cache_mask, l2_cache_mask, &l1_LRU_block_index, index_sent_l1, index_sent_l2,
                        tag_found_in_l2, &l2_LRU_block_index, p_stats, block_counter,
                        &victim_cache_writable_index, tag_sent_l1);
                    }

                    // 2- The tag is not found, and the l2 cache is not full.
                    if (not tag_found_in_l2){
                        // if the l2 cache is not valid (valid_l2_cache = False), we will be looking for an empty space in l2 to create data.
                        // If there is no empty space left (last empty space used by the write back),
                        // we will be using the LRU
                        // In case the l2 cache is valid (full), we will be directly using the second lru.
                        write_back_l1_move_to_vc_copy_tag_found_in_l2(&l1_cache, &l2_cache, &victim_cache,
                        l1_cache_mask, l2_cache_mask, &l1_LRU_block_index, index_sent_l1, index_sent_l2,
                        tag_found_in_l2, &l2_LRU_block_index, p_stats, block_counter,
                        &victim_cache_writable_index, tag_sent_l1);

                        // There should be and empty place in cache l2, but we have to test if the write back had not already
                        // Written data at the invalid place.
                        if (l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].valid_bit != 0){
                            // The write back has already take this place (invalid_l2_block)
                            // Then, we should search for the LRU in l2 or for another invalid_place.
                            block_counter = 0;
                            invalid_l2_block = 0;
                            l2_LRU_block_index = 0;
                            bool stop_loop = false;
                            // Finding a new invalid place, or the new LRU that will be replaced.
                            while(not stop_loop){
                                if (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].valid_bit == 0){
                                    stop_loop = true;
                                    invalid_l2_block = block_counter;
                                }
                                if (l2_cache.cache_lines[index_sent_l2].blocks[block_counter].LRU <
                                l2_cache.cache_lines[index_sent_l2].blocks[l2_LRU_block_index].LRU){
                                    // Make sure that the LRU is not the last value accessed.
                                    if (block_counter != l2_cache.cache_lines[index_sent_l2].last_accessed_block){
                                        l2_LRU_block_index = block_counter;
                                    }
                                }
                                block_counter += 1;
                                if (block_counter < l2_cache.nb_cache_blocks_per_line){
                                    stop_loop = true;
                                }
                            }
                            // If the invalid block we found has the valid bit set, this means there are no empty place in
                            // The l2 cache, we should use the LRU instead.
                            if (l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].valid_bit == 1){
                                invalid_l2_block = l2_LRU_block_index;
                            }
                        }
                        // Read data from ram and place it in l2 cache
                        read_ram_set_elements_in_cache(&l2_cache, index_sent_l2,
                         invalid_l2_block, tag_sent_l2);
                        // Also set data in l1 cache. The l1 block is full, and the LRU has already be written in the VC.
                        read_ram_set_elements_in_cache(&l1_cache, index_sent_l1,
                        l1_LRU_block_index, tag_sent_l1);
                        // stats
                        if (type == READ){
                            p_stats->read_misses_l2 += 1;
                        } else {
                            if (type == WRITE){
                                p_stats->write_misses_l2 += 1;
                                // If write, only set dirty bit in l1, since data will be write back from l1 to l2 if it is not used.
                                l1_cache.cache_lines[index_sent_l1].blocks[invalid_l1_block].dirty_bit = 1;
                                /** If the teacher tell us to set the dirty bit in both, then uncomment the following line */
                                // l2_cache.cache_lines[index_sent_l2].blocks[invalid_l2_block].dirty_bit = 1;
                            }
                        }
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
