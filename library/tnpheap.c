#include <npheap/tnpheap_ioctl.h>
#include <npheap/npheap.h>
#include <npheap.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <sys/mman.h>
#include <unistd.h>
#include <signal.h>
#include <malloc.h>
#include <string.h>
#define TNPHEAP_IOCTL_LOCK 0
#define TNPHEAP_IOCTL_UNLOCK 1
//#define DEBUG 0
#ifdef DEBUG 
#define DEBUG_PRINT printf 
#else
#define DEBUG_PRINT dummy_print 
#endif

struct node {
    struct tnpheap_cmd *cmd;
    struct node *next;
};

static struct node *head;
static struct node *tail;

// Project 2: Chinmay Rudrapatna, csrudrap; Rakshit Holkal Ravishankar, rhravish; Shishir Nagendra, sbnagend;

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: Dummy function used when DEBUG is not defined.
*/
void dummy_print(char *str, ...)
{
    int local = 0;
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To find the version from the kernel for a given offset.
*/
__u64 tnpheap_get_version(int npheap_dev, int tnpheap_dev, __u64 offset)
{
    // Calls the kernel get_version() function.
    // Return error if the object doesnt exist. 
    DEBUG_PRINT("tnpheap_get_version function entered.\n");
    struct tnpheap_cmd cmd;
    cmd.offset = offset;
    __u64 version = ioctl(tnpheap_dev, TNPHEAP_IOCTL_GET_VERSION, &cmd);
    DEBUG_PRINT("For offset %lld, version returned by the kernel is %lld\n", offset, version);
    return version;
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To find a local object for a given offset.
*/
struct node *find_obj(__u64 offset)
{
    DEBUG_PRINT("find_obj function entered.\n");
    struct node *temp = head;
    while (temp != NULL && (temp->cmd->offset) != offset)
    {   
        temp = temp->next;
    }
    return temp;
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To create a new local object for a given offset.
*/
struct node *create_local_obj(__u64 offset, __u64 version, __u64 size)
{
    // If head is NULL, create a new node and make head and tail point to it.
    // Otherwise, add to tail and advance tail.
    DEBUG_PRINT("create_local_obj function entered.\n");
    __u64 aligned_size = ((size + getpagesize() - 1) / getpagesize()) * getpagesize();
    if (head == NULL)
    {
        head = (struct node *) malloc(sizeof(struct node));
        head->next = NULL;
        head->cmd = (struct tnpheap_cmd *) malloc(sizeof(struct tnpheap_cmd));
        head->cmd->data = malloc(aligned_size);
        head->cmd->size = aligned_size;
        head->cmd->version = version;
        head->cmd->offset = offset;
        tail = head;
        return head;
    }
    else
    {
        struct node *new_node;
        new_node = (struct node*) malloc(sizeof(struct node));
        tail->next = new_node;
        tail = new_node;
        new_node->next = NULL;
        new_node->cmd = (struct tnpheap_cmd *) malloc(sizeof(struct tnpheap_cmd));
        new_node->cmd->data = malloc(aligned_size);
        new_node->cmd->size = aligned_size;
        new_node->cmd->offset = offset;
        new_node->cmd->version = version;
        return new_node;
    }
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To handle a given signal.
*/
int tnpheap_handler(int sig, siginfo_t *si)
{
    DEBUG_PRINT("handler function entered: Error is: %d\n", si->si_errno);
    return 0;
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To allocate a local object and return a pointer back to the user.
*/
void *tnpheap_alloc(int npheap_dev, int tnpheap_dev, __u64 offset, __u64 size)
{
    // Write to the local buffer.
    // Buffer will be global linked list in this file. This should be similar to npheap_alloc of the kernel space, but here.
    // Get version and store it in the struct.
    DEBUG_PRINT("tnpheap_alloc function entered.\n");
    struct node *local_data;
    __u64 version = tnpheap_get_version(npheap_dev, tnpheap_dev, offset);
    DEBUG_PRINT("Get version returned %lld for offset %lld\n", version, offset);
    if ((local_data = find_obj(offset)) == NULL)
    {
        local_data = create_local_obj(offset, version, size);
    }
    // local_data will be of type void *
    return local_data->cmd->data;
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To get a new transaction number from the kernel.
*/
__u64 tnpheap_start_tx(int npheap_dev, int tnpheap_dev)
{
    // Global transaction number.
    // Calls the kernel start_tx function.
    head = NULL;
    tail = NULL;
    DEBUG_PRINT("tnpheap_start_tx function entered.\n");
    return ioctl(tnpheap_dev, TNPHEAP_IOCTL_START_TX, NULL);
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To free the local buffer of user data.
*/
void nullify_local_data()
{
    DEBUG_PRINT("nullify_local_data function entered.\n");
    struct node *temp = head;
    while (temp != NULL)
    {
        free(temp->cmd->data);
        temp = temp->next;
    }
}

/*
*   Authors: Chinmay Rudrapatna, Rakshit Holkal Ravishankar, Shishir Nagendra
*   Purpose: To copy the local data into the kernel virtual address space and commit.
*/
int tnpheap_commit(int npheap_dev, int tnpheap_dev)
{
    // Acquire lock.
    // Check version of all objects.
    // If versions are all the same:
        // Call mmap and do an npheap_alloc
        // Update the version numbers in the kernel.
    // Else:
        // Abort this transaction. 
    // Unlock.
    
    DEBUG_PRINT("tnpheap_commit function entered.\n");
    ioctl(tnpheap_dev, 1, NULL);
    struct node *temp = head;
    while (temp != NULL)
    {
        if (temp->cmd->version == tnpheap_get_version(npheap_dev, tnpheap_dev, temp->cmd->offset))
        {
            temp = temp->next;
        }
        else
        {
            // Abort even if one mismatch occurs.
            // Free up local buffers, set head and tail to NULL.
            nullify_local_data();
            ioctl(tnpheap_dev, 0, NULL);
            return -1;
        }
    }    
    // Can now be committed.
    temp = head;
    DEBUG_PRINT("temp is %p\n", temp);
    
    while (temp != NULL)
    {
        char *mapped_data;
        __u64 size = temp->cmd->size;
        __u64 offset = temp->cmd->offset;
        DEBUG_PRINT("Calling npheap_alloc for offset %lld and version %d\n", offset, temp->cmd->version);
        __u64 npheap_size = npheap_getsize(npheap_dev, offset);
        DEBUG_PRINT("GETSIZE::::::: %llu\n", npheap_size);
        if (npheap_size != 0 && npheap_size != size) 
        {
            if (npheap_delete(npheap_dev, offset) != npheap_size)
            {
                DEBUG_PRINT("Delete worked\n");
            }
            else
            {
                DEBUG_PRINT("Delete returned non-zero!! Aborting.\n");
                nullify_local_data();
                ioctl(tnpheap_dev, 0, NULL);
                return -5;
            }  
        }
        if ((mapped_data = (char *) npheap_alloc(npheap_dev, offset, size)) == NULL)
        {
            // Unlock.
            nullify_local_data();
            ioctl(tnpheap_dev, 0, NULL);
            return -3;  // Let's say 3 means npheap_alloc failed.
        }
        // Increment version number by calling commit in tnpheap.
        if (ioctl(tnpheap_dev, TNPHEAP_IOCTL_COMMIT, temp->cmd) == -1)
        {
            // Unlock.
            nullify_local_data();
            ioctl(tnpheap_dev, 0, NULL); 
            return -4;  // Let's say 4 means commit failed.
        }
        // Copy data from local object to mapped_data.
        DEBUG_PRINT("DATA: %s\n", temp->cmd->data); 
        memcpy(mapped_data, temp->cmd->data, size); 
        temp = temp->next;
    }
    // Unlock.
    ioctl(tnpheap_dev, 0, NULL);
    nullify_local_data();
    DEBUG_PRINT("End of commit\n");
    return 0;
}

