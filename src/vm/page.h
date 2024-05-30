#ifndef VM_PAGE_H 

#define VM_PAGE_H 

 

#include "filesys/off_t.h" 

#include "threads/vaddr.h" 

#include "filesys/file.h" 

 

bool add_lazy_file_page(struct file *file, off_t ofs, uint8_t *upage, 

                        size_t read_bytes, size_t zero_bytes, bool writable); 

bool add_zero_page(uint8_t *upage, bool writable); 

void page_fault_handler(struct intr_frame *f); 

bool load_page_from_file(struct page *p); 

bool allocate_zero_page(struct page *p); 

struct page *page_replacement_algorithm(void); 

void write_page_to_swap(struct page *p); 

 

#endif /* vm/page.h */ 