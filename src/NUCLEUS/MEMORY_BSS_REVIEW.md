# MEMORY.ASM BSS segment review

## Layout (current)

| Symbol                  | Size    | Purpose |
|-------------------------|---------|---------|
| Start_of_free_memory    | 4 (.DS.l 1) | Start of TPA (program end). Set in Init_memory from basepage; never read outside MEMORY.ASM. |
| Size_of_free_memory     | 4 (.DS.l 1) | Current "free" memory size. Updated on every alloc/free/shrink. |
| Memlist_end             | 4 (.DS.l 1) | Pointer to first byte past the last 12-byte entry in Memory_list. |
| Memory_list             | 12000 (3*Max_memblocks) | Array of 12-byte entries. Max 1000 entries. |

## External references

- **Size_of_free_memory**: Read in TEST.ASM (Total_size), STATUS.ASM (status line "Size of free memory" and "Total size" calculation). Must remain and be maintained so status screen and tests see a sensible value.

- **Memory_list**, **Memlist_end**: Read only in STATUS.ASM by `Show_memory`. It walks from Memory_list to Memlist_end, step 12 bytes, and for each entry with bit 7 of byte 8 clear (allocated) prints start, length, type, and subfile. So any malloc-based reimplementation must either:
  - Keep a buffer with the same 12-byte layout and update it on every alloc/free/shrink so Show_memory still works, or
  - Replace Show_memory with a C/diagnostic version and drop the list.

## Entry layout (12 bytes)

- 0–3: Start of block (.l)
- 4–7: Length of block (.l)
- 8:    File type (.b), bit 7 = 0 allocated / 1 free
- 9:    Priority (.b)
- 10–11: Subfile number (.w)

## Recommendations for SDL_malloc reimplementation

1. **Start_of_free_memory**: Can be dropped from the public API and BSS; no external references. If you keep a BSS for compatibility, set it to 0 or leave unused.

2. **Size_of_free_memory**: Keep. Initialize to a large value (e.g. 16 MB) in Init_memory; subtract on allocate, add on free/shrink. Use a single global so both C and ASM can read it if needed.

3. **Memlist_end**: Keep if Show_memory stays in ASM. Must point to (Memory_list + 12 * number_of_entries). C code should update it whenever the list changes.

4. **Memory_list**: Keep same size and layout if Show_memory is unchanged. C implementation can own the buffer (e.g. static uint8_t memory_list_buf[12*1000]) and write big-endian values so 68k STATUS.ASM reads correct data. Rebuild the list from internal tracking on every alloc/free/shrink, or provide a refresh function called before Show_memory (only possible if Show_memory is C or calls into C first).

5. **Future compacting**: When you add a compacting allocator, you can reintroduce a single contiguous pool, coalescing, and priority-based eviction; the same BSS symbols and 12-byte entry format can be preserved so STATUS.ASM and callers remain unchanged.

## Using the C reimplementation (memory.c)

- **Build**: Compile `memory.c` with the rest of the C port and link so that the memory symbols are provided by C instead of MEMORY.ASM. Do not link MEMORY.ASM in that build, or the BSS/definitions will conflict.
- **Calling from 68k**: If 68k code still runs (e.g. under an emulator) and must call these routines, add small ASM trampolines that move d0/d1/d2/a0/a1 into C calling convention and jump to the C functions. The C API matches the MEMORY.ASM comments (Init_memory(basepage), Allocate_memory(size) → ptr, etc.).
- **Errors**: The original uses ERROR/DI_Error on failure. The C routines return NULL on alloc failure and no-op on free/shrink when the pointer is unknown. A thin ASM or C wrapper can call DI_Error when the C function returns NULL.
