/* Author: Karla Mejia

This code is provided here solely for educational and portfolio purposes.  
No permission is granted to copy, modify, or redistribute this code.  

*/

/*
This code creates the character device driver for the runners list in the kernel. It receives input via the userspace file
(runner_userspace.c) and uses the arguments received from there to decide how to modify the list, whether it be printing, adding,
or removing runners from the list. Each runner will have their information (lane, bib number, name, school, qualifier time, 
record time) saved in a linked list in the kernel.
*/

#include <linux/init.h>
#include <linux/module.h>
#include <linux/kernel.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/uaccess.h>
#include <linux/mutex.h>
#include <linux/device.h>
#include <linux/list.h>

MODULE_LICENSE("GPL");
MODULE_AUTHOR("Karla Mejia");
MODULE_DESCRIPTION("Runned Linked List Character Device");
MODULE_VERSION("1.1");

// allowing up to 100 characters for runner's name and school
#define LEN_NAME 100 
#define LEN_SCHOOL 100

// node structure that will hold info about each runner
struct runner {
    int lane;
    int bib_num;
    char name[LEN_NAME];
    char school[LEN_SCHOOL];

    // kernel doesn't support floats, so we'll deal with that later with scaling..
    int qual_time; 
    int record_time;

    struct list_head list; // allows struct to be used as a kernel list
};

static LIST_HEAD(list_of_runners); // declaring and initializing head
static DEFINE_MUTEX(lock); // defining locking mechanism for mutual exclusion

static int major; // major number/integer parameter
static struct class *runner_class; // will be used to make /dev/runners

// program used to write to /dev/runners
static ssize_t runners_write(struct file *filp, const char __user *buf,
                             size_t len, loff_t *f_pos)
{
    // buffers for arguments from user, separating command and arguments
    char kernel_buffer[100]; 
    char cmd[16]; 

    // data extracted from buffer
    int lane, bib;
    char name[LEN_NAME], school[LEN_SCHOOL];

    struct runner *runner_node, *temp_node;

    // checking if length of input is valid, must fit into buffer
    if (len >= sizeof(kernel_buffer))
        return -EINVAL; // invalid argument error

    // copying user buffer to kernel buffer
    if (copy_from_user(kernel_buffer, buf, len) != 0)
        return -EFAULT; // invalid memory error
    kernel_buffer[len] = '\0'; // terminating string manually, now we're ready to manipulate it

    // extracts first keyword in buffer, which will be the command (ADD, REMOVE, PRINT)
    sscanf(kernel_buffer, "%s", cmd);

    // enforcing mutual exclusion to prevent from manipulating driver at the same time
    mutex_lock(&lock);

    // ADD command, where we add a runner to the linked list
    // ADD lane, bib, name, school name, qual time, record time
    if (strcmp(cmd, "ADD") == 0) {
        // since kernel can't deal with floats, we'll parse the seconds and milliseconds separately
        int qual_time_s, qual_time_ms;
        int record_time_s, record_time_ms;

        // first ensuring user gave all proper arguments for adding, then extracts each
        if (sscanf(kernel_buffer, "ADD %d %d %s %s %d.%d %d.%d",
                &lane, &bib, name, school,
                &qual_time_s, &qual_time_ms, // separating seconds and milliseconds
                &record_time_s, &record_time_ms) == 8) {

            // allocates memory in kernel for runner
            runner_node = kmalloc(sizeof(*runner_node), GFP_KERNEL);

            // saving data from buffer into node in kernel
            runner_node->lane = lane;
            runner_node->bib_num = bib;
            strcpy(runner_node->name, name);
            strcpy(runner_node->school, school);

            // putting together time as a full integer to be parsed easier later, since kernel
            // cannot save floats
            runner_node->qual_time = qual_time_s * 100 + qual_time_ms; // 10.01 will be saved as 1001
            runner_node->record_time = record_time_s * 100 + record_time_ms; // same method as qual_time

            
            INIT_LIST_HEAD(&runner_node->list); // initializes node, prepare to be added
            list_add_tail(&runner_node->list, &list_of_runners); // adds node to list

            // prints runner data when runners_write() is used
            printk(KERN_INFO "[runners] Added Runner: Lane: %d | Bib: %d | Name: %s | School: %s | "
                            "Qualifier Time: %d.%02d | Personal Record: %d.%02d\n",
                runner_node->lane, runner_node->bib_num, runner_node->name, runner_node->school,
                runner_node->qual_time / 100, // separates seconds
                runner_node->qual_time % 100, // separates milliseconds
                runner_node->record_time / 100, 
                runner_node->record_time % 100);
        }
    }


    // REMOVE command, where we remove the runner using the bib number (used to ID runners)
    else if (strcmp(cmd, "REMOVE") == 0) {
        if (sscanf(kernel_buffer, "REMOVE %d", &bib) == 1) {
            list_for_each_entry_safe(runner_node, temp_node, &list_of_runners, list) {
                // iterates through the linkedlist and looks for the runner with the bib we want to remove
                if (runner_node->bib_num == bib) {
                    // once node with bib found, deletes it
                    list_del(&runner_node->list);

                    // prints data about runner just removed in same format as before
                    printk(KERN_INFO "[runners] Removed Runner: Lane: %d | Bib: %d | Name: %s | School: %s | "
                            "Qualifier Time: %d.%02d | Personal Record: %d.%02d\n",
                        runner_node->lane, runner_node->bib_num, runner_node->name, runner_node->school, 
                        runner_node->qual_time / 100, runner_node->qual_time % 100, 
                        runner_node->record_time / 100, runner_node->record_time % 100);
                    
                    kfree(runner_node); // frees memory of node just removed
                    break; // stops iterating
                }
            }
        }
    }

    // PRINT command, where it prints all runners in the linked list
    else if (strcmp(cmd, "PRINT") == 0) {
        printk(KERN_INFO "[runners] Printing entire list of runners:\n");
        list_for_each_entry(runner_node, &list_of_runners, list) {
            printk(KERN_INFO "[runners] Lane: %d | Bib: %d | Name: %s | School: %s | "
                            "Qualifier Time: %d.%02d | Personal Record: %d.%02d\n",
                runner_node->lane, runner_node->bib_num, runner_node->name, runner_node->school, 
                runner_node->qual_time / 100, runner_node->qual_time % 100, 
                runner_node->record_time / 100, runner_node->record_time % 100);
        }
        printk(KERN_INFO "[runners] End of list.\n");
    }

    // unlocks driver to allow it to be modified again once previous modification done
    mutex_unlock(&lock);

    return len; // returns number of bytes written
}

// configures what function to write when writing to device
static struct file_operations fops = {
    .owner = THIS_MODULE,
    .write = runners_write,
};

// initializes and loads module (with insmod)
static int __init runners_init(void)
{
    major = register_chrdev(0, "runners", &fops); // assigning driver to device

    runner_class = class_create("runners_class"); // creates device class

    // checks if class created properly, otherwise prints an error
    if (IS_ERR(runner_class))
        pr_err("[runners] Could not create device class.\n");
        return PTR_ERR(runner_class);

    // creates /dev/runners with no parent and no additional data
    device_create(runner_class, NULL, MKDEV(major, 0), NULL, "runners");
    printk(KERN_INFO "[runners] /dev/runners loaded and ready.\n");
    return 0;
}

// unloads module (with rmmod)
static void __exit runners_exit(void)
{
    struct runner *runner_node, *temp_node;

    // ensures mutual exclusion while list being modified
    mutex_lock(&lock);

    // goes through entire linked list to delete and free memory of any nodes present
    list_for_each_entry_safe(runner_node, temp_node, &list_of_runners, list) {
        list_del(&runner_node->list);
        kfree(runner_node);
    }

    mutex_unlock(&lock);

    // destroy device + class, unregisters major number
    device_destroy(runner_class, MKDEV(major, 0)); 
    class_destroy(runner_class);
    unregister_chrdev(major, "runners");
    printk(KERN_INFO "[runners] Module was unloaded.\n");
}

module_init(runners_init); // for insmod
module_exit(runners_exit); // for rmmod
// :)