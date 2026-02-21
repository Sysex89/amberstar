#ifndef AMBERSTAR_MEMORY_H
#define AMBERSTAR_MEMORY_H

#include <stdint.h>

extern void  *Memory_list;
extern void  *Memlist_end;
extern void  *Start_of_free_memory;
extern uint32_t Size_of_free_memory;

void Init_memory(void *basepage);
void Next_generation(void);

void *Allocate_memory(uint32_t size);
void *Reallocate_memory(uint8_t file_type, uint16_t subfile);
void *File_allocate(uint8_t file_type, uint32_t size, uint16_t subfile);

void Kill_memory(void *ptr);
void Free_memory(void *ptr);
void Shrink_memory(uint32_t new_size, void *ptr);

#endif
