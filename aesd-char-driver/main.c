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
    PDEBUG("%s\n", __func__);
    /**
     * TODO: handle release
     */
    return 0;
}

ssize_t aesd_read(struct file *filp, char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = 0;
    struct aesd_dev* aesd_dev_ptr = (struct aesd_dev*)filp->private_data;


    PDEBUG("read %zu bytes with offset %lld\n",count,*f_pos);

    struct aesd_buffer_entry*   buffer_entry_ptr = NULL;
    size_t                      buffer_entry_off = 0;

    PDEBUG("Lookiong for offset %lld\n", *f_pos);
    buffer_entry_ptr = aesd_circular_buffer_find_entry_offset_for_fpos(
        &aesd_dev_ptr->circular_buffer,
        *f_pos,
        &buffer_entry_off);

    if (buffer_entry_ptr != NULL)
    {
        retval = buffer_entry_ptr->size - buffer_entry_off;
        PDEBUG("Entry with offset %llu has been found. It's size is %zu. Reading %zu bytes\n", *f_pos, buffer_entry_ptr->size, retval);
        copy_to_user(buf, buffer_entry_ptr->buffptr, retval);
    }
    else
    {
        PDEBUG("No entry with offset %lld.\n", *f_pos);
    }

    *f_pos += retval;

    PDEBUG("Read returning %zu. New fpos %lld.\n", retval, *f_pos);

    return retval;
}

ssize_t aesd_write(struct file *filp, const char __user *buf, size_t count,
                loff_t *f_pos)
{
    ssize_t retval = -ENOMEM;
    struct aesd_buffer_entry entry;
    char* bufferptr = NULL;
    struct aesd_dev* aesd_dev_ptr = (struct aesd_dev*)filp->private_data;


    PDEBUG("write %zu bytes with offset %lld\n",count,*f_pos);

    bufferptr = kmalloc(count, GFP_KERNEL);
    copy_from_user(bufferptr, buf, count);
    entry.buffptr = bufferptr;
    entry.size = count;
    bufferptr = (char*) aesd_circular_buffer_add_entry(&aesd_dev_ptr->circular_buffer, &entry);
    PDEBUG("circular_buffer->in_off = %zu; circular_buffer->out_off = %zu", aesd_dev_ptr->circular_buffer.in_offs, aesd_dev_ptr->circular_buffer.out_offs);

    if (bufferptr)
    {
        kfree(bufferptr);
    }

    retval = count;
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



int aesd_init_module(void)
{
    PDEBUG("%s\n", __func__);

    dev_t dev = 0;
    int result;
    result = alloc_chrdev_region(&dev, aesd_minor, 1,
            "aesdchar");
    aesd_major = MAJOR(dev);
    if (result < 0) {
        printk(KERN_WARNING "Can't get major %d\n", aesd_major);
        return result;
    }
    memset(&aesd_device,0,sizeof(struct aesd_dev));

    aesd_circular_buffer_init(&aesd_device.circular_buffer);

    result = aesd_setup_cdev(&aesd_device);

    if( result ) {
        unregister_chrdev_region(dev, 1);
    }
    return result;

}

void aesd_cleanup_module(void)
{
    PDEBUG("%s\n", __func__);

    dev_t devno = MKDEV(aesd_major, aesd_minor);

    cdev_del(&aesd_device.cdev);

    uint8_t index = 0;
    struct aesd_buffer_entry *entry = NULL;
    AESD_CIRCULAR_BUFFER_FOREACH(entry, &aesd_device.circular_buffer, index) {
        kfree(entry->buffptr);
    }

    unregister_chrdev_region(devno, 1);
}



module_init(aesd_init_module);
module_exit(aesd_cleanup_module);
