#include "vm/page.h" 

#include "threads/malloc.h" 

#include "userprog/pagedir.h" 

#include "threads/vaddr.h" 

#include "threads/thread.h" 

#include "threads/synch.h" 

#include "devices/block.h" 

 

/* Các khai báo khóa và biến điều kiện */ 

struct lock page_fault_lock; 

struct condition page_fault_cond; 

 

void 

vm_init (void)  

{ 

  lock_init(&page_fault_lock); 

  cond_init(&page_fault_cond); 

} 

 

bool 

add_lazy_file_page(struct file *file, off_t ofs, uint8_t *upage, 

                   size_t read_bytes, size_t zero_bytes, bool writable) 

{ 

  struct thread *t = thread_current(); 

 

  struct page *p = malloc(sizeof(struct page)); 

  if (p == NULL) 

    return false; 

 

  p->file = file; 

  p->ofs = ofs; 

  p->upage = upage; 

  p->read_bytes = read_bytes; 

  p->zero_bytes = zero_bytes; 

  p->writable = writable; 

 

  lock_acquire(&t->spt_lock); 

  bool success = hash_insert(&t->supplemental_page_table, &p->hash_elem) == NULL; 

  lock_release(&t->spt_lock); 

 

  return success; 

} 

 

bool 

add_zero_page(uint8_t *upage, bool writable) 

{ 

  struct thread *t = thread_current(); 

 

  struct page *p = malloc(sizeof(struct page)); 

  if (p == NULL) 

    return false; 

 

  p->file = NULL; 

  p->ofs = 0; 

  p->upage = upage; 

  p->read_bytes = 0; 

  p->zero_bytes = PGSIZE; 

  p->writable = writable; 

 

  lock_acquire(&t->spt_lock); 

  bool success = hash_insert(&t->supplemental_page_table, &p->hash_elem) == NULL; 

  lock_release(&t->spt_lock); 

 

  return success; 

} 

 

void 

page_fault_handler(struct intr_frame *f) 

{ 

  void *fault_addr = intr_frame_fault_addr(f); 

  struct thread *t = thread_current(); 

 

  lock_acquire(&page_fault_lock); 

 

  struct page *p = page_lookup(fault_addr); 

  if (p == NULL)  

  { 

    lock_release(&page_fault_lock); 

    return; 

  } 

 

  if (p->file != NULL)  

  { 

    if (!load_page_from_file(p))  

    { 

      lock_release(&page_fault_lock); 

      return; 

    } 

  }  

  else  

  { 

    if (!allocate_zero_page(p))  

    { 

      lock_release(&page_fault_lock); 

      return; 

    } 

  } 

 

  lock_release(&page_fault_lock); 

  cond_broadcast(&page_fault_cond, &page_fault_lock); 

} 

 

bool 

load_page_from_file(struct page *p) 

{ 

  struct thread *t = thread_current(); 

 

  void *kpage = allocate_frame(PAL_USER); 

  if (kpage == NULL) 

    return false; 

 

  file_seek(p->file, p->ofs); 

  if (file_read(p->file, kpage, p->read_bytes) != (int) p->read_bytes)  

  { 

    free_frame(kpage); 

    return false; 

  } 

  memset(kpage + p->read_bytes, 0, p->zero_bytes); 

 

  if (!install_page(p->upage, kpage, p->writable))  

  { 

    free_frame(kpage); 

    return false; 

  } 

 

  return true; 

} 

 

bool 

allocate_zero_page(struct page *p) 

{ 

  void *kpage = allocate_frame(PAL_USER | PAL_ZERO); 

  if (kpage == NULL) 

    return false; 

 

  if (!install_page(p->upage, kpage, p->writable))  

  { 

    free_frame(kpage); 

    return false; 

  } 

 

  return true; 

} 

 

struct page * 

page_replacement_algorithm(void) 

{ 

  struct thread *t = thread_current(); 

  struct list_elem *e = list_begin(&t->pages); 

 

  while (true)  

  { 

    struct page *p = list_entry(e, struct page, elem); 

 

    if (!p->accessed)  

    { 

      if (!p->dirty)  

      { 

        list_remove(&p->elem); 

        return p; 

      }  

      else  

      { 

        write_page_to_swap(p); 

        list_remove(&p->elem); 

        return p; 

      } 

    }  

    else  

    { 

      p->accessed = false; 

      e = list_next(e); 

      if (e == list_end(&t->pages)) 

        e = list_begin(&t->pages); 

    } 

  } 

} 

 

void 

write_page_to_swap(struct page *p) 

{ 

  void *kpage = pagedir_get_page(thread_current()->pagedir, p->upage); 

  swap_out(kpage); 

  p->dirty = false; 

} 
