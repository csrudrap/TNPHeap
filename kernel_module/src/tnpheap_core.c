//////////////////////////////////////////////////////////////////////
//                             North Carolina State University
//
//
//
//                             Copyright 2016
//
////////////////////////////////////////////////////////////////////////
//
// This program is free software; you can redistribute it and/or modify it
// under the terms and conditions of the GNU General Public License,
// version 2, as published by the Free Software Foundation.
//
// This program is distributed in the hope it will be useful, but WITHOUT
// ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
// FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License for
// more details.
//
// You should have received a copy of the GNU General Public License along with
// this program; if not, write to the Free Software Foundation, Inc.,
// 51 Franklin St - Fifth Floor, Boston, MA 02110-1301 USA.
//
////////////////////////////////////////////////////////////////////////
//
//   Author:  Hung-Wei Tseng
//
//   Description:
//     Skeleton of NPHeap Pseudo Device
//
////////////////////////////////////////////////////////////////////////

#include "tnpheap_ioctl.h"

#include <asm/uaccess.h>
#include <linux/slab.h>
#include <linux/kernel.h>
#include <linux/errno.h>
#include <linux/mm.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/module.h>
#include <linux/moduleparam.h>
#include <linux/poll.h>
#include <linux/mutex.h>
#include <linux/time.h>

// Project 2: Chinmay Rudrapatna, csrudrap; Rakshit Holkal Ravishankar, rhravish; Shishir Nagendra, sbnagend;

// Keep a global mutex variable.
// Write lock and unlock functions that will operate on this variable.
DEFINE_MUTEX(global_lock);

DEFINE_MUTEX(transaction_lock);
__u64 transaction_num = 0;
struct miscdevice tnpheap_dev;

struct node {
    struct tnpheap_cmd *cmd;
    struct node *next;
};

struct node *head;
struct node *tail;

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To create a new object for a given offset.
*/
struct node *create_obj(__u64 offset)
{
    // If head is NULL, create a new node and make head and tail point to it.
    // Otherwise, add to tail and advance tail.
    if (head == NULL)
    {
        head = (struct node *) kmalloc(sizeof(struct node), GFP_KERNEL);
        head->next = NULL;
        head->cmd = (struct tnpheap_cmd *) kmalloc(sizeof(struct tnpheap_cmd), GFP_KERNEL);
        head->cmd->data = NULL;
        head->cmd->version = 0;
        head->cmd->offset = offset;
        tail = head;
        return head;
    }
    else
    {
        struct node *new_node = (struct node *) kmalloc(sizeof(struct node), GFP_KERNEL);
        tail->next = new_node;
        tail = new_node;
        new_node->next = NULL;
        new_node->cmd = (struct tnpheap_cmd *) kmalloc(sizeof(struct tnpheap_cmd), GFP_KERNEL);
        new_node->cmd->version = 0;
        new_node->cmd->offset = offset;
        return new_node;
    }
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To find the object for a given offset.
*/
struct node *find_obj(__u64 offset)
{
    struct node *temp = head;
    while (temp != NULL && (temp->cmd->offset) != offset)
    {
        temp = temp->next;
    }
    return temp;
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To find the version for a given offset.
*/
__u64 tnpheap_get_version(struct tnpheap_cmd __user *user_cmd)
{
    // Access global linked list for each object, get node->cmd->version and return it.
    struct tnpheap_cmd cmd;
    if (!copy_from_user(&cmd, user_cmd, sizeof(cmd)))
    {
        struct node *obj;
        if ((obj = find_obj(cmd.offset)) != NULL)
        {
            return obj->cmd->version;
        }
        else 
        {
            printk("Object not found for offset %llu\n", cmd.offset);            
        }
    }    
    return -1;
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: Increments the transaction number everytime this function is called. 
*/
__u64 tnpheap_start_tx(struct tnpheap_cmd __user *user_cmd)
{
    struct tnpheap_cmd cmd;
    __u64 ret=0;
    if (!copy_from_user(&cmd, user_cmd, sizeof(cmd)))
    {
        return -1;
    }
    mutex_lock(&transaction_lock);
    ret = transaction_num++;
    mutex_unlock(&transaction_lock);
    return ret;
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To increase the version number for a given offset.
*/
__u64 tnpheap_commit(struct tnpheap_cmd __user *user_cmd)
{
    struct tnpheap_cmd cmd;
    if (!copy_from_user(&cmd, user_cmd, sizeof(cmd)))
    {
        // Access global linked list for each object
        struct node *obj;
        obj = find_obj(cmd.offset);
        if (obj == NULL)
        {
            obj = create_obj(cmd.offset);
        }
        if (obj)
        {
            (obj->cmd->version)++;
            return 0;
        }
    }
    return -1;
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To lock.
*/
void tnpheap_lock(struct tnpheap_cmd __user *user_cmd)
{
    mutex_lock(&global_lock);
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To unlock.
*/
void tnpheap_unlock(struct tnpheap_cmd __user *user_cmd)
{
    mutex_unlock(&global_lock);
}

__u64 tnpheap_ioctl(struct file *filp, unsigned int cmd,
                                unsigned long arg)
// Add lines for lock and unlock.
{
    switch (cmd) {
    case TNPHEAP_IOCTL_START_TX:
        return tnpheap_start_tx((void __user *) arg);
    case TNPHEAP_IOCTL_GET_VERSION:
        return tnpheap_get_version((void __user *) arg);
    case TNPHEAP_IOCTL_COMMIT:
        return tnpheap_commit((void __user *) arg);
    case 1:
        tnpheap_lock((void __user *) arg);
    case 0:
        tnpheap_unlock((void __user *) arg);
    default:
        return -ENOTTY;
    }
}

static const struct file_operations tnpheap_fops = {
    .owner                = THIS_MODULE,
    .unlocked_ioctl       = tnpheap_ioctl,
};

struct miscdevice tnpheap_dev = {
    .minor = MISC_DYNAMIC_MINOR,
    .name = "tnpheap",
    .fops = &tnpheap_fops,
};

static int __init tnpheap_module_init(void)
{
    int ret = 0;
    if ((ret = misc_register(&tnpheap_dev)))
        printk(KERN_ERR "Unable to register \"npheap\" misc device\n");
    else
        printk(KERN_ERR "\"npheap\" misc device installed\n");
    return 1;
}

static void __exit tnpheap_module_exit(void)
{
    misc_deregister(&tnpheap_dev);
    return;
}

MODULE_AUTHOR("Hung-Wei Tseng <htseng3@ncsu.edu>");
MODULE_LICENSE("GPL");
MODULE_VERSION("0.1");
module_init(tnpheap_module_init);
module_exit(tnpheap_module_exit);
