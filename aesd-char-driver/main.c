/**
 * @file aesdchar.c
 * @brief Functions and data related to the AESD char driver implementation
 *
 * Based on the implementation of the "scull" device driver, found in
 * Linux Device Drivers example code.
 *
 * @author Dan Walkes
 * @date 2019-10-22
 * @copyright Copyright (c) 2019
 *
 */

#include <linux/module.h>
#include <linux/slab.h>
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/uaccess.h>	/* copy_*_user */
#include "aesdchar.h"
#include "aesd_ioctl.h"

int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Ciro Donnarumma");
MODULE_LICENSE("Dual BSD/GPL");

#ifdef MIN
#undef MIN
#endif
#define MIN(a, b) ((a) < (b) ? (a) : (b))

struct aesd_dev aesd_device;

int     aesd_open(struct inode *inode, struct file *filp);
int     aesd_release(struct inode *inode, struct file *filp);
loff_t  aesd_llseek(struct file *, loff_t, int);
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
long    aesd_ioctl(struct file * filp, unsigned int cmd, unsigned long arg);
int     aesd_init_module(void);
void    aesd_cleanup_module(void);


int aesd_open(struct inode *inode, struct file *filp)
{
    filp->private_data = &aesd_device;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    return 0;
}

loff_t aesd_llseek(struct file *filp, loff_t off, int whence)
{
    loff_t           retval       = -EINVAL;
    struct aesd_dev* aesd_dev_ptr = NULL;

    aesd_dev_ptr = (struct aesd_dev*)(filp->private_data);

    PDEBUG("aesd_llseek: off = %llu, whence = %d", off, whence);

    mutex_lock(&aesd_dev_ptr->lock);
    retval = fixed_size_llseek(filp, off, whence, aesd_circular_buffer_size(&aesd_dev_ptr->circular_buffer));
    mutex_unlock(&aesd_dev_ptr->lock);

    return retval;
}


ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t                     retval           = 0;
    struct aesd_dev*            aesd_dev_ptr     = NULL;
    struct aesd_buffer_entry*   buffer_entry_ptr = NULL;
    size_t                      buffer_entry_off = 0;

    PDEBUG("read %zu bytes with offset %lld\n",count,*f_pos);

    aesd_dev_ptr = (struct aesd_dev*)(filp->private_data);

    mutex_lock(&aesd_dev_ptr->lock);

    buffer_entry_ptr = aesd_circular_buffer_find_entry_offset_for_fpos(&aesd_dev_ptr->circular_buffer, *f_pos, &buffer_entry_off);
    if (buffer_entry_ptr != NULL)
    {
        retval = MIN(count, buffer_entry_ptr->size - buffer_entry_off);
        if(copy_to_user(buf, buffer_entry_ptr->buffptr + buffer_entry_off, retval) > 0)
        {
            PDEBUG("Error in copy to user");
            retval = -EFAULT;
        }
    }


    if (retval > 0)
    {
        *f_pos += retval;
    }

    mutex_unlock(&aesd_dev_ptr->lock);

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos)
{
    ssize_t retval                        = -ENOMEM;
    char*                    bufferptr    = NULL;
    struct aesd_dev*         aesd_dev_ptr = NULL;

    PDEBUG("write %zu bytes with offset %lld\n",count,*f_pos);

    aesd_dev_ptr = (struct aesd_dev*)filp->private_data;

    mutex_lock(&aesd_dev_ptr->lock);

    bufferptr = kmalloc((count + aesd_dev_ptr->staging_entry.size), GFP_KERNEL);

    if (bufferptr != NULL)
    {
        memcpy(bufferptr, aesd_dev_ptr->staging_entry.buffptr, aesd_dev_ptr->staging_entry.size);
        if (copy_from_user((bufferptr + aesd_dev_ptr->staging_entry.size), buf, count) > 0)
        {
            PDEBUG("Error in copy from user\n");
            kfree(bufferptr);
            retval = -EFAULT;
        }
        else
        {
            kfree(aesd_dev_ptr->staging_entry.buffptr);
            aesd_dev_ptr->staging_entry.buffptr = bufferptr;
            aesd_dev_ptr->staging_entry.size += count;
            if (aesd_dev_ptr->staging_entry.buffptr[aesd_dev_ptr->staging_entry.size - 1] == '\n')
            {
                bufferptr = (char*) aesd_circular_buffer_add_entry(&aesd_dev_ptr->circular_buffer, &(aesd_dev_ptr->staging_entry));
                kfree(bufferptr);
                memset(&(aesd_dev_ptr->staging_entry), 0x00, sizeof(aesd_dev_ptr->staging_entry));
            }
            retval = count;
        }
    }

    mutex_unlock(&aesd_dev_ptr->lock);

    return retval;
}


/**
 * Asdjust the file offset (f_pos) parameter of @param filp based on the location specified by
 * @param write_cmd (the zero referenced command to locate)
 * and @param write_cmd_offset (the zero referenced offset into the command)
 * @return 0 if successful, negative if error occurred:
 *      -ERESTARTSYS if mutex could not be obtained
 *      -EINVAL if write_command or write_cmd_offset wasout of range
 */
static long aesd_adjust_file_offset(struct file *filp, unsigned int write_cmd, unsigned int write_cmd_offset)
{
    PDEBUG("adjust_file_offset to cmd %d with offset %d", write_cmd, write_cmd_offset);
    return -EINVAL;
}

long aesd_ioctl(struct file * filp, unsigned int cmd, unsigned long arg)
{
    long retval = -EINVAL;

    switch (cmd)
    {
    case AESDCHAR_IOCSEEKTO:
        struct aesd_seekto seekto;
        if (copy_from_user(&seekto, (const void __user*)arg, sizeof(seekto)) != 0)
        {
            retval = -EFAULT;
        }
        else
        {
            retval = aesd_adjust_file_offset(filp, seekto.write_cmd, seekto.write_cmd_offset);
        }
        break;
    default:
        retval = -EINVAL;
        break;
    }

    return retval;
}


struct file_operations aesd_fops = {
    .owner =            THIS_MODULE,
    .llseek =           aesd_llseek,
    .read =             aesd_read,
    .write =            aesd_write,
    .unlocked_ioctl =   aesd_ioctl,
    .open =             aesd_open,
    .release =          aesd_release,
};


static int aesd_setup_cdev(struct aesd_dev *dev)
{
    int err, devno = MKDEV(aesd_major, aesd_minor);

    cdev_init(&dev->cdev, &aesd_fops);
    dev->cdev.owner = THIS_MODULE;
    dev->cdev.ops = &aesd_fops;
    err = cdev_add (&dev->cdev, devno, 1);
    if (err) {
        printk(KERN_ERR "Error %d adding aesd cdev\n", err);
    }
    return err;
}



int __init aesd_init_module(void)
{
    PDEBUG("%s\n", __func__);

    int    result   = 0;
    dev_t  dev      = 0;

    result = alloc_chrdev_region(&dev, aesd_minor, 1, "aesdchar");
    aesd_major = MAJOR(dev);

    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    aesd_circular_buffer_init(&aesd_device.circular_buffer);
    mutex_init(&aesd_device.lock);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }

    return result;
}

void __exit aesd_cleanup_module(void)
{
    PDEBUG("%s\n", __func__);

    struct aesd_buffer_entry* entry = NULL;
    dev_t                     devno = MKDEV(aesd_major, aesd_minor);
    uint8_t                   index = 0;

    cdev_del(&aesd_device.cdev);
    mutex_destroy(&aesd_device.lock);

    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, index) {
        kfree(entry->buffptr);
    }

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
