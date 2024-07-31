#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/errno.h>
#include <linux/random.h>
#include <linux/slab.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

static const char *quotes[] = {
  "\"Experience is the name everyone gives to their mistakes\" - Oscar Wilde\n",
  "\"If, at first, you do not succeed, call it version 1.0\" - Khayri R.R. Woulfe\n",
  "\"It is never too late to be what you might have been.\" - George Eliot\n"
  "\"Success is not final, failure is not fatal: it is the courage to continue that counts.\" - Winston S. Churchill\n",
  "\"The truth is, unless you let go, unless you forgive yourself, unless you forgive the situation, unless you realize that the situation is over, you cannot move forward.\" - Steve Maraboli\n",
  "\"It's only after you've stepped outside your comfort zone that you begin to change, grow, and transform.\" - Roy T. Bennett\n",
  "\"The most difficult thing is the decision to act, the rest is merely tenacity.\" - Amelia Earhart\n",
  "\"The only person you are destined to becom is the person you decide to be.\" - Ralph Waldo Emerson\n",
  "\"It does not matter how slowly you go as long as you do not stop.\" - Confucius\n",
  "\"It's not the years in your life that count. It's the life in your years.\" - Abraham Lincoln\n",
  "\"A person who never made a mistake never tried anything new.\" - Albert Einstein\n",
  "\"Challenges are what make life interesting and overcoming them is what makes life meaningful.\" - Joshua J. Marine\n",
  "\"When I let go of what I am, I become what I might be.\" - Lao Tzu\n",
  "\"Happiness is not something readymade. It comes from your own actions.\" - Dalai Lama\n",
  "\"Go confidently in the direction of your dreams. Live the life you have imagined.\" - Henry David Thoreau\n",
  "\"The best revenge is massive success.\" - Frank Sinatra\n",
  "\"I've learned that people will forget what you said, people will forget what you did, but people will never forget how you made them feel.\" - Maya Angelou\n",
  "\"Your time is limited, so don't waste it living someone else's life.\" - Steve Jobs\n",
  "\"Limitations live only in our minds. But if we use our imaginations, our possibilities become limitless.\" - Jamie Paolinetti\n",
  "\"You become what you believe.\" - Oprah Winfrey\n"
};


// copy a random quote to the user buffer and return how many bytes were copied 
static ssize_t read_quotes(struct file *file, char __user *user_buffer, size_t size, loff_t *offset)
{
  // offset is used so only one quote is read per call
  if (*offset > 0) return 0;
  unsigned int* index = (int*)kmalloc(sizeof(unsigned int), 0);
  while (index == NULL)
  {
    index = (int*)kmalloc(sizeof(unsigned int), 0);
  }

  // get the quote and then send it to user_buffer
  get_random_bytes(index, sizeof(int));
 
  *index = (*index) % 19;
  size_t len = min(strlen(quotes[*index]), size);
  if (len <= 0) return 0;
  
  if (copy_to_user(user_buffer, quotes[*index], len))
    return -EFAULT;

  *offset += 1;

  return len;
};


static int open_quotes(struct inode *inode, struct file *file)
{
  // no write allowed
  if (file->f_mode & FMODE_WRITE) return -EPERM;
  
  return 0;
};

static int close_quotes(struct inode *inode, struct file *file)
{
  return 0;
};

// file operations structure
static const struct file_operations fops = {
  .owner = THIS_MODULE,
  .read = read_quotes,
  .open = open_quotes,
  .release = close_quotes
};

// device structure
static struct miscdevice inspiration_device = {
  .name = "inspiration",
  .minor = MISC_DYNAMIC_MINOR,
  .fops = &fops,
  .mode = 0444
};


// initialize the device
static int __init inspiration_init(void)
{
  int rv = misc_register(&inspiration_device);
  if (rv)
    return -rv;

  return 0;
}

static void __exit inspiration_exit(void)
{
  misc_deregister(&inspiration_device);
}

module_init(inspiration_init);
module_exit(inspiration_exit);
