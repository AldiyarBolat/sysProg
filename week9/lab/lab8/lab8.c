#include <linux/init.h>
#include <linux/module.h>
#include <linux/kthread.h>
#include <linux/sched.h>
#include <linux/delay.h>
#include <linux/spinlock.h>
#include <linux/random.h>
#include <linux/timer.h>
#include <linux/jiffies.h>

MODULE_LICENSE("LAB8");

#define TAG "lab8"
#define DELAY 23
#define ARR_SZ 1000000

static struct task_struct *t1;
static struct task_struct *t2;
static struct task_struct *t3;

int numbers[ARR_SZ];
int before[ARR_SZ];
unsigned long threadtime;

int L[ARR_SZ]; //temp arrays
int R[ARR_SZ]; //temp

DEFINE_SPINLOCK(lock);
DEFINE_SPINLOCK(lock2);

static int counter = 0;

void merge(int l, int m, int r)
{
    int i, j, k;
    int n1 = m - l + 1;
    int n2 = r - m;

    /* Copy data to temp arrays L[] and R[] */
    for (i = 0; i < n1; i++)
        L[i] = numbers[l + i];
    for (j = 0; j < n2; j++)
        R[j] = numbers[m + 1 + j];

    /* Merge the temp arrays back into arr[l..r]*/
    i = 0; // Initial index of first subarray
    j = 0; // Initial index of second subarray
    k = l; // Initial index of merged subarray
    while (i < n1 && j < n2)
    {
        if (L[i] <= R[j])
        {
            numbers[k] = L[i];
            i++;
        }
        else
        {
            numbers[k] = R[j];
            j++;
        }
        k++;
    }

    /* Copy the remaining elements of L[], if there 
       are any */
    while (i < n1)
    {
        numbers[k] = L[i];
        i++;
        k++;
    }

    /* Copy the remaining elements of R[], if there 
       are any */
    while (j < n2)
    {
        numbers[k] = R[j];
        j++;
        k++;
    }
}

/* l is for left index and r is right index of the 
   sub-array of arr to be sorted */
void mergeSort(int l, int r)
{
    if (l < r)
    {
        // Same as (l+r)/2, but avoids overflow for
        // large l and h
        int m = l + (r - l) / 2;

        // Sort first and second halves
        mergeSort(l, m);
        mergeSort(m + 1, r);

        merge(l, m, r);
    }
}

static int threadMethod(void *unused)
{
    while (counter < 10)
    {
        int start = 0;
        spin_lock(&lock);
        start = jiffies;
        printk(KERN_INFO "I'm %s", current->comm);
        mergeSort(counter * 100000, counter * 100000 + 99999);
        counter += 1;
        if (counter >= 2)
        {
            printk(KERN_INFO "I'm %s and want to merge", current->comm);
            merge(0, counter * 100000 - 99999, counter * 100000);
            printk(KERN_INFO "I'm %s completed merging", current->comm);
        }
        threadtime += jiffies_to_msecs(jiffies - start);
        spin_unlock(&lock);

        msleep(DELAY);
    }
    return 0;
}

void createThreads(void)
{
    spin_lock(&lock2);

    t1 = kthread_run(threadMethod, NULL, "mythread1");
    t2 = kthread_run(threadMethod, NULL, "mythread2");
    t3 = kthread_run(threadMethod, NULL, "mythread3");
    spin_unlock(&lock2);

    //kthread_stop(t1);
    //kthread_stop(t2);
}

int init_module(void)
{
    int i;
    int rand;
    unsigned long start = 0;
    unsigned long stop = 0;
    unsigned long time1 = 0;

    threadtime = 0;

    for (i = 0; i < ARR_SZ; i++)
    {
        get_random_bytes(&rand, sizeof(rand));
        rand %= ARR_SZ;
        numbers[i] = rand + ARR_SZ;
        before[i] = rand + ARR_SZ;
    }
    printk(KERN_INFO "START %s\n", TAG);
    start = jiffies;
    createThreads();
    //mergeSort(0, ARR_SZ - 1);
    msleep(1000);
    stop = jiffies;
    time1 = jiffies_to_msecs(stop - start);
    for (i = 0; i < 100; i++)
    {
        printk(KERN_INFO "%d at pos %d, before was %d", 
               numbers[i * 10000], i * 10000, before[i * 10000]);
    }

    printk(KERN_INFO "Time passed: %lu \n", time1);
    printk(KERN_INFO "Thread time passed: %lu \n", threadtime);
    return 0;
}

void cleanup_module(void)
{
    printk(KERN_INFO "END %s\n", TAG);
}