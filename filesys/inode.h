#ifndef FILESYS_INODE_H
#define FILESYS_INODE_H

#include <stdbool.h>
#include "filesys/off_t.h"
#include "devices/disk.h"

struct bitmap;
struct inode_disk
  {
    // disk_sector_t start;                /* First data sector. */
    off_t length;                       /* File size in bytes. */
    unsigned magic;                     /* Magic number. */
    disk_sector_t start[124];                /* First data sector. */
    // disk_sector_t unused[123];               /* Not used. */

    disk_sector_t* indirect; //Pointer has same size with uint32_t
    disk_sector_t** doubly_indirect; //Pointer has same size with  uint32_t


    // uint32_t unused[125];               /* Not used. */
  };

void inode_init (void);
bool inode_grow(disk_sector_t sector, struct inode_disk* disk_inode, off_t length);
bool inode_create (disk_sector_t, off_t);
struct inode *inode_open (disk_sector_t);
struct inode *inode_reopen (struct inode *);
disk_sector_t inode_get_inumber (const struct inode *);
void inode_close (struct inode *);
void inode_remove (struct inode *);
off_t inode_read_at (struct inode *, void *, off_t size, off_t offset);
off_t inode_write_at (struct inode *, const void *, off_t size, off_t offset);
void inode_deny_write (struct inode *);
void inode_allow_write (struct inode *);
off_t inode_length (const struct inode *);


#endif /* filesys/inode.h */
