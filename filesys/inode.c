#include "filesys/inode.h"
#include <list.h>
#include <debug.h>
#include <round.h>
#include <string.h>
#include "filesys/filesys.h"
#include "filesys/free-map.h"
#include "threads/malloc.h"
#include "filesys/cache.h"
#include "threads/thread.h"

/* Identifies an inode. */
#define INODE_MAGIC 0x494e4f44

/* On-disk inode.
   Must be exactly DISK_SECTOR_SIZE bytes long. */
// struct inode_disk
//   {
//     // disk_sector_t start;                /* First data sector. */
//     off_t length;                       /* File size in bytes. */
//     unsigned magic;                     /* Magic number. */
//     disk_sector_t start[124];                /* First data sector. */
//     // disk_sector_t unused[123];               /* Not used. */
//
//     disk_sector_t* indirect; //Pointer has same size with uint32_t
//     disk_sector_t** doubly_indirect; //Pointer has same size with  uint32_t
//
//
//     // uint32_t unused[125];               /* Not used. */
//   };

/* Returns the number of sectors to allocate for an inode SIZE
   bytes long. */
static inline size_t
bytes_to_sectors (off_t size)
{
  return DIV_ROUND_UP (size, DISK_SECTOR_SIZE);
}

/* In-memory inode. */
struct inode
  {
    struct list_elem elem;              /* Element in inode list. */
    disk_sector_t sector;               /* Sector number of disk location. */
    int open_cnt;                       /* Number of openers. */
    bool removed;                       /* True if deleted, false otherwise. */
    int deny_write_cnt;                 /* 0: writes ok, >0: deny writes. */
    struct inode_disk data;             /* Inode content. */
  };

/* Returns the disk sector that contains byte offset POS within
   INODE.
   Returns -1 if INODE does not contain data for a byte at offset
   POS. */
static disk_sector_t
byte_to_sector (const struct inode *inode, off_t pos)
{
  ASSERT (inode != NULL);
  inode_grow(inode->sector,&inode->data,pos);
  size_t currentsectors = bytes_to_sectors(inode->data.length);
  size_t targetsector = bytes_to_sectors(pos);
  // printf("Targetsector %u | pos %u\n",targetsector,pos);
  if(pos==0)
  {
    return *(inode->data.start);
  }
  else if(currentsectors<=124)
  {
    // printf("\nNow less than 124!!!\nReturn : %u | target sector is %u with position %u\n Because start is %u\n",*(inode->data.start+targetsector-1),targetsector,pos,*(inode->data.start));
    return *(inode->data.start+targetsector-1);
  }
  else if (currentsectors<=DISK_SECTOR_SIZE)
  {
    // printf("\nNow less than 512!!!\n\n");
    // printf("Hmm %u | before %u | start %p |inode %p\n",*(inode->data.indirect+targetsector-1)
    // ,*(inode->data.indirect+targetsector-2),inode->data.indirect,inode);
    return *(inode->data.indirect+targetsector-1);
  }
  else if(currentsectors<=DISK_SECTOR_SIZE*DISK_SECTOR_SIZE)
  {
    PANIC("test case please not here\n");
  }
  else
    PANIC("File size should be smaller than disk size(%d Bytes)\n",DISK_SECTOR_SIZE*disk_size(filesys_disk));
  printf("\nNot implemented!!!\n\n");
  return -1;
  /*if (pos < inode->data.length)
    return inode->data.start + pos / DISK_SECTOR_SIZE;
  else
    return -1;*/
}

/* List of open inodes, so that opening a single inode twice
   returns the same `struct inode'. */
static struct list open_inodes;

/* Initializes the inode module. */
void
inode_init (void)
{
  list_init (&open_inodes);
}

bool
inode_grow(disk_sector_t sector, struct inode_disk* disk_inode, off_t length)
{
  //If don't need to grow
  if(disk_inode->length>=length)
    return true;

  //If file size is bigger than disk size
  if(length>DISK_SECTOR_SIZE*disk_size(filesys_disk)) // 아직 metadata 용량 못뺌
  {
    PANIC("File size should be smaller than disk size(%d Bytes)\n",DISK_SECTOR_SIZE*disk_size(filesys_disk));
    return false;
  }
  size_t neededsectors = bytes_to_sectors(length);
  size_t currentsectors = bytes_to_sectors(disk_inode->length);
  int i;
  // printf("length %u | need %u | current %u | currlen %u\n"
  // ,length,neededsectors,currentsectors,disk_inode->length);
  // disk_sector_t* zerobuffer = calloc(1,DISK_SECTOR_SIZE);
  static char zeros[DISK_SECTOR_SIZE];

  if(neededsectors<=124)
  {
    // Use direct pointer
    for (i=currentsectors;i<neededsectors;i++)
    {
      if (!free_map_allocate (1, disk_inode->start+i))
        PANIC("Fail to allocate free map.\n"); // Fail to allocate
      else
        buffer_cache_write(filesys_disk,disk_inode->start[i],zeros,0,DISK_SECTOR_SIZE);// Success to allocate -> make all value zero
    }
  }
  else if (neededsectors<=DISK_SECTOR_SIZE)
  {
    //Use indirect pointer
    if(disk_inode->indirect==NULL)
    {
      disk_inode->indirect = (disk_sector_t*)calloc(DISK_SECTOR_SIZE,sizeof (disk_sector_t));
      memcpy(disk_inode->indirect,disk_inode->start,currentsectors*(sizeof (disk_sector_t)));
    }
    for (i=currentsectors;i<neededsectors;i++)
    {
      if (!free_map_allocate (1, disk_inode->indirect+i))
        PANIC("Fail to allocate free map.\n"); // Fail to allocate
      else
        buffer_cache_write(filesys_disk,*(disk_inode->indirect+i),zeros,0,DISK_SECTOR_SIZE);// Success to allocate -> make all value zero

    }


  }
  else if (neededsectors<=DISK_SECTOR_SIZE*DISK_SECTOR_SIZE)
  {

    //Use doubly-indirect pointer
    if(disk_inode->doubly_indirect==NULL)
    {
      disk_inode->doubly_indirect = (disk_sector_t*)calloc(DISK_SECTOR_SIZE,sizeof (disk_sector_t*));
      *disk_inode->doubly_indirect = disk_inode->indirect;
    }
    for (i=currentsectors;i<neededsectors;i++)
    {
      disk_sector_t* doubly = *(disk_inode->doubly_indirect)+(i/DISK_SECTOR_SIZE);
      if(*doubly==NULL)
      {
        doubly = (disk_sector_t*)calloc(DISK_SECTOR_SIZE,sizeof (disk_sector_t));
      }
      if (!free_map_allocate (1, doubly+(i%DISK_SECTOR_SIZE)))
        PANIC("Fail to allocate free map.\n"); // Fail to allocate
      else
        buffer_cache_write(filesys_disk,*(doubly+(i%DISK_SECTOR_SIZE)),zeros,0,DISK_SECTOR_SIZE);// Success to allocate -> make all value zero

    }
  }
  else
  {
    NOT_REACHED();
    PANIC("File size should be smaller than disk size(%d Bytes)\n",DISK_SECTOR_SIZE*disk_size(filesys_disk));
    return false;
  }
  disk_inode->length = length;
  // printf("inode grow from %d to %d sectors!\n",currentsectors,neededsectors);
  buffer_cache_write(filesys_disk, sector, disk_inode,0,DISK_SECTOR_SIZE);
  return true;

}


/* Initializes an inode with LENGTH bytes of data and
   writes the new inode to sector SECTOR on the file system
   disk.
   Returns true if successful.
   Returns false if memory or disk allocation fails. */
bool
inode_create (disk_sector_t sector, off_t length)
{
  struct inode_disk *disk_inode = NULL;
  bool success = false;

  ASSERT (length >= 0);

  /* If this assertion fails, the inode structure is not exactly
     one sector in size, and you should fix that. */
  ASSERT (sizeof *disk_inode == DISK_SECTOR_SIZE);

  disk_inode = calloc (1, sizeof *disk_inode);
  if (disk_inode != NULL)
    {
      size_t sectors = bytes_to_sectors (length);
      disk_inode->length = 0;
      disk_inode->magic = INODE_MAGIC;

      if (inode_grow(sector,disk_inode,length))
        {
          buffer_cache_write(filesys_disk, sector, disk_inode,0,DISK_SECTOR_SIZE);
          success = true;
        }


      free (disk_inode);
    }
  return success;
}

/* Reads an inode from SECTOR
   and returns a `struct inode' that contains it.
   Returns a null pointer if memory allocation fails. */
struct inode *
inode_open (disk_sector_t sector)
{
  struct list_elem *e;
  struct inode *inode;

  /* Check whether this inode is already open. */
  for (e = list_begin (&open_inodes); e != list_end (&open_inodes);
       e = list_next (e))
    {
      inode = list_entry (e, struct inode, elem);
      if (inode->sector == sector)
        {
          inode_reopen (inode);
          return inode;
        }
    }

  /* Allocate memory. */
  inode = malloc (sizeof *inode);
  if (inode == NULL)
    return NULL;

  /* Initialize. */
  list_push_front (&open_inodes, &inode->elem);
  inode->sector = sector;
  inode->open_cnt = 1;
  inode->deny_write_cnt = 0;
  inode->removed = false;
  // disk_read (filesys_disk, inode->sector, &inode->data);
  struct buffer_cache_entry *bce = buffer_cache_check(filesys_disk, inode->sector,false);
  // PANIC("bce %p",bce);
  if(bce!=NULL&&bce->buffer!=NULL)
    {
      lock_acquire(&bce->entry_lock);
      bce->readers++;
      lock_release(&bce->entry_lock);
      memcpy(&inode->data,bce->buffer,DISK_SECTOR_SIZE);
      lock_acquire(&bce->entry_lock);
      bce->readers--;
      if(bce->readers==0)
        cond_broadcast(&bce->entry_cond,&bce->entry_lock);
      lock_release(&bce->entry_lock);
    }
  return inode;
}

/* Reopens and returns INODE. */
struct inode *
inode_reopen (struct inode *inode)
{
  if (inode != NULL)
    inode->open_cnt++;
  return inode;
}

/* Returns INODE's inode number. */
disk_sector_t
inode_get_inumber (const struct inode *inode)
{
  return inode->sector;
}

/* Closes INODE and writes it to disk.
   If this was the last reference to INODE, frees its memory.
   If INODE was also a removed inode, frees its blocks. */
void
inode_close (struct inode *inode)
{
  /* Ignore null pointer. */
  if (inode == NULL)
    return;

  /* Release resources if this was the last opener. */
  if (--inode->open_cnt == 0)
    {
      /* Remove from inode list and release lock. */
      list_remove (&inode->elem);

      /* Deallocate blocks if removed. */
      if (inode->removed)
        {
          int i;
          free_map_release (inode->sector, 1);

          size_t currentsectors = bytes_to_sectors(inode->data.length);
          if(currentsectors<=124)
          {

            for (i=0;i<currentsectors;i++)
            {
              free_map_release (inode->data.start[i],
                                1);
            }

          }
          else if(currentsectors<=DISK_SECTOR_SIZE)
          {
            for (i=0;i<currentsectors;i++)
            {
              free_map_release(*(inode->data.indirect+i),1);
            }

          }
          else if(currentsectors<=DISK_SECTOR_SIZE*DISK_SECTOR_SIZE)
          {
            for (i=0;i<currentsectors;i++)
            {
              disk_sector_t* doubly = *(inode->data.doubly_indirect)+(i/DISK_SECTOR_SIZE);
              free_map_release(*(doubly+(i%DISK_SECTOR_SIZE)),1);
            }
          }
          else
          {
            PANIC("NOT HERE\n");
          }
          // free_map_release (inode->data.start,
          //                   bytes_to_sectors (inode->data.length));
        }

      free (inode);
    }
}

/* Marks INODE to be deleted when it is closed by the last caller who
   has it open. */
void
inode_remove (struct inode *inode)
{
  ASSERT (inode != NULL);
  inode->removed = true;
}

/* Reads SIZE bytes from INODE into BUFFER, starting at position OFFSET.
   Returns the number of bytes actually read, which may be less
   than SIZE if an error occurs or end of file is reached. */
off_t
inode_read_at (struct inode *inode, void *buffer_, off_t size, off_t offset)
{
  uint8_t *buffer = buffer_;
  off_t bytes_read = 0;
  // uint8_t *bounce = NULL;

  while (size > 0)
    {
      /* Disk sector to read, starting byte offset within sector. */
      // disk_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % DISK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - offset;
      int sector_left = DISK_SECTOR_SIZE - sector_ofs;
      int min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually copy out of this sector. */
      int chunk_size = size < min_left ? size : min_left;
      if (chunk_size <= 0)
        break;
      disk_sector_t sector_idx = byte_to_sector (inode, offset+chunk_size);

      if (sector_ofs == 0 && chunk_size == DISK_SECTOR_SIZE)
        {
          /* Read full sector directly into caller's buffer. */
          struct buffer_cache_entry *bce = buffer_cache_check(filesys_disk, sector_idx,true);
          if(bce!=NULL&&bce->buffer!=NULL)
          {
            lock_acquire(&bce->entry_lock);
            bce->readers++;
            lock_release(&bce->entry_lock);
            memcpy(buffer+bytes_read,bce->buffer,DISK_SECTOR_SIZE);
            lock_acquire(&bce->entry_lock);
            bce->readers--;
            if(bce->readers==0)
              cond_broadcast(&bce->entry_cond,&bce->entry_lock);
            lock_release(&bce->entry_lock);
          }
        }
      else
        {
          /* Read sector into bounce buffer, then partially copy
             into caller's buffer. */
          struct buffer_cache_entry *bce = buffer_cache_check(filesys_disk, sector_idx,true);
          if(bce!=NULL&&bce->buffer!=NULL)
          {
            lock_acquire(&bce->entry_lock);
            bce->readers++;
            lock_release(&bce->entry_lock);


            memcpy (buffer + bytes_read, bce->buffer + sector_ofs, chunk_size);
            lock_acquire(&bce->entry_lock);
            bce->readers--;
            if(bce->readers==0)
              cond_broadcast(&bce->entry_cond,&bce->entry_lock);
            lock_release(&bce->entry_lock);

          }
        }

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_read += chunk_size;
    }
  // free (bounce);

  return bytes_read;
}

/* Writes SIZE bytes from BUFFER into INODE, starting at OFFSET.
   Returns the number of bytes actually written, which may be
   less than SIZE if end of file is reached or an error occurs.
   (Normally a write at end of file would extend the inode, but
   growth is not yet implemented.) */
off_t
inode_write_at (struct inode *inode, const void *buffer_, off_t size,
                off_t offset)
{

  const uint8_t *buffer = buffer_;
  off_t bytes_written = 0;
  uint8_t *bounce = NULL;

  if (inode->deny_write_cnt)
    return 0;

  while (size > 0)
    {
      // printf("In Inode size = %d\n",size);
      /* Sector to write, starting byte offset within sector. */

      // disk_sector_t sector_idx = byte_to_sector (inode, offset);
      int sector_ofs = offset % DISK_SECTOR_SIZE;

      /* Bytes left in inode, bytes left in sector, lesser of the two. */
      off_t inode_left = inode_length (inode) - sector_ofs;
      // if(strcmp(thread_current()->name,"main")!=0)

      // off_t inode_left = inode_length (inode) - offset;
      if(inode_left<=0)
      {
        int growth = size < DISK_SECTOR_SIZE ? size : DISK_SECTOR_SIZE;
        // if(strcmp(thread_current()->name,"main")!=0)
        //   printf("GROWTH! %d\n",growth);
        inode_grow(inode->sector,&inode->data,inode->data.length+growth);
        inode_left = inode_length (inode) - sector_ofs;

      }
      uint32_t sector_left = DISK_SECTOR_SIZE - sector_ofs;
      uint32_t min_left = inode_left < sector_left ? inode_left : sector_left;

      /* Number of bytes to actually write into this sector. */
      uint32_t chunk_size = size < min_left ? size : min_left;

      disk_sector_t sector_idx = byte_to_sector (inode, offset+chunk_size);
      // if(strcmp(thread_current()->name,"main")!=0)
      // {
      //   printf("inode_length= %u | sectorofs =%u\n",inode_length(inode),sector_ofs);
      //   printf("sector index = %u | first = %u | off = %d | length = %u\n",sector_idx,inode->data.start[0],offset+chunk_size,inode->data.length);
      // }
      if (chunk_size <= 0)
      {
        // if(strcmp(thread_current()->name,"main")!=0)
        //   printf("\n\nchunk_size is zero...\n\nsize=%d | min_left=%d|inode_left=%d|sector_left=%d\n",size,min_left,inode_left,sector_left);
        break;
      }
      // if(strcmp(thread_current()->name,"main")!=0)
      //   printf("WRITE : sector_idx = %u |chunk_size = %d | size = %d | ofs = %d | inode_left = %d\n",sector_idx,chunk_size,size,sector_ofs,inode_left);

      buffer_cache_write(filesys_disk, sector_idx, buffer+bytes_written,sector_ofs,chunk_size);
      // if(strcmp(thread_current()->name,"main")!=0)
      //   printf("%u = length |sector %u | file %u| chunk %u|offset %u\n",inode->data.length,sector_idx,inode->sector,chunk_size,offset);

      /* Advance. */
      size -= chunk_size;
      offset += chunk_size;
      bytes_written += chunk_size;
      // printf("SIZE = %d | off = %d | written = %d\n",size,offset,bytes_written);
    }
  // free (bounce);

  return bytes_written;
}

/* Disables writes to INODE.
   May be called at most once per inode opener. */
void
inode_deny_write (struct inode *inode)
{
  inode->deny_write_cnt++;
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
}

/* Re-enables writes to INODE.
   Must be called once by each inode opener who has called
   inode_deny_write() on the inode, before closing the inode. */
void
inode_allow_write (struct inode *inode)
{
  ASSERT (inode->deny_write_cnt > 0);
  ASSERT (inode->deny_write_cnt <= inode->open_cnt);
  inode->deny_write_cnt--;
}

/* Returns the length, in bytes, of INODE's data. */
off_t
inode_length (const struct inode *inode)
{
  return inode->data.length;
}
