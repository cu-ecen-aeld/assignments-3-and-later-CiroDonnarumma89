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
#include <linux/init.h>
#include <linux/printk.h>
#include <linux/types.h>
#include <linux/cdev.h>
#include <linux/fs.h> // file_operations
#include <linux/uaccess.h>	/* copy_*_user */
#include "aesdchar.h"
int aesd_major =   0; // use dynamic major
int aesd_minor =   0;

MODULE_AUTHOR("Ciro Donnarumma");
MODULE_LICENSE("Dual BSD/GPL");

struct aesd_dev aesd_device;

int     aesd_open(struct inode *inode, struct file *filp);
int     aesd_release(struct inode *inode, struct file *filp);
ssize_t aesd_read(struct file *filp, char __user *buf, size_t count, loff_t *f_pos);
ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count, loff_t *f_pos);
int     aesd_init_module(void);
void    aesd_cleanup_module(void);


int aesd_open(struct inode *inode, struct file *filp)
{
    PDEBUG("%s\n", __func__);
    filp->private_data = &aesd_device;
    return 0;
}

int aesd_release(struct inode *inode, struct file *filp)
{
    struct aesd_dev* aesd_dev_ptr = NULL;

    PDEBUG("%s\n", __func__);

    aesd_dev_ptr = (struct aesd_dev*)(filp->private_data);

    mutex_lock(&(aesd_dev_ptr->lock));
    kfree(aesd_dev_ptr->staging_entry.buffptr);
    memset(&(aesd_dev_ptr->staging_entry), 0x00, sizeof(aesd_dev_ptr->staging_entry));
    mutex_unlock(&aesd_dev_ptr->lock);

    return 0;
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
        retval = buffer_entry_ptr->size - buffer_entry_off;
        if(copy_to_user(buf, buffer_entry_ptr->buffptr, retval) > 0)
        {
            PDEBUG("Error in copy to user\n");
            retval = -EFAULT;
        }
    }

    mutex_unlock(&aesd_dev_ptr->lock);

    if (retval > 0)
    {
        *f_pos += retval;
    }

    PDEBUG("Read returning %zu. New fpos %lld.\n", retval, *f_pos);

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

    bufferptr    = kmalloc((count + aesd_dev_ptr->staging_entry.size), GFP_KERNEL);

    if (bufferptr != NULL)
    {
        memcpy(bufferptr, aesd_dev_ptr->staging_entry.buffptr, aesd_dev_ptr->staging_entry.size);
        if (copy_from_user((bufferptr + aesd_dev_ptr->staging_entry.size), buf, count) > 0)
        {
            PDEBUG("Error in copy from user\n");
            retval = -EFAULT;
        }
        else
        {
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
struct file_operations aesd_fops = {
    .owner =    THIS_MODULE,
    .read =     aesd_read,
    .write =    aesd_write,
    .open =     aesd_open,
    .release =  aesd_release,
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
