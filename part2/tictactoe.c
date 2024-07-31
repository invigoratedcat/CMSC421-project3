#include <linux/kernel.h>
#include <linux/module.h>
#include <linux/errno.h>
#include <linux/slab.h>
#include <linux/fs.h>
#include <linux/miscdevice.h>
#include <linux/random.h>
#include <linux/string.h>

MODULE_LICENSE("GPL");

static const char *board_top = "- - A B C\n  --------\n";
static const char *BAR = " | ", *SPACE = " ";
static const char *RESET = "RESET", *TURN = "TURN", *BOARD = "BOARD";
static char board[3][3];
static int scores[8];
static char *command;
static int status; // -1 = bad move, 0 = success, 1 = player won, 2 = CPU won 

// handle CPU move and return board status
static ssize_t read_ttt(struct file *file, char __user *user_buffer, size_t size, loff_t *offset )
{
  char *rows = NULL, *prefix = NULL, *MSG = NULL;
  int i = 0, j = 0, k = 0, rows_len = 31 + strlen(board_top);
  if (*offset != 0) return 0; 

  // handle RESET command
  if (strncmp(command, RESET, sizeof(char)*5) == 0)
  {
    char *OK = "OK\n";
    if (copy_to_user(user_buffer, OK, strlen(OK)))
      return -EFAULT;
    
    *offset = 1;
    return size;
  }
  if (strncmp(command, TURN, sizeof(char)*4) == 0)
  {
    // an illegal move was made by the player
    if (status == -1)
    {
      MSG = "That is an illegal move. Please try again.\n";
      status = 0;
    }
    // the player won
    else if (status == 1)
    {
      MSG = "Congratulations! You have won!\n";
    }
    // the CPU won
    else if (status == 2)
    {
      MSG = "You have lost...\n";
    }
    // a draw occurred
    else if (status == 3)
    {
      MSG = "A draw has occurred.\n";
    }
  }
  if (strncmp(command, BOARD, sizeof(char)*5) == 0
      || strncmp(command, TURN, sizeof(char)*4) == 0)
  {
    // display board
    rows = (char*)kmalloc(sizeof(char)*rows_len, 0);
    while (rows == NULL) rows = (char*)kmalloc(sizeof(char)*rows_len, 0);
    rows = strcat(rows, board_top);
  
    prefix = (char*)kmalloc(sizeof(char)*6, 0);
    while (prefix == NULL) prefix = (char*)kmalloc(sizeof(char)*5, 0);

    i = 0;
    for (; i < 3; i++)
    {
      // construct "i | " where i is the row number
      prefix[0] = i + 49;
      prefix[1] = '\0';
      prefix = strcat(prefix, BAR);
      rows = strcat(rows, prefix);

      // construct rest of row string
      j = 0;
      for (; j < 3; j++)
      {
	rows[strlen(rows)] = board[i][j];
	rows[strlen(rows) + 1] = '\0';
	rows = strcat(rows, SPACE);
      }
      const char n = '\n';
      rows = strcat(rows, &n);

      k = 0;
      for (; k < strlen(prefix); k++)
	prefix[k] = '\0';
    }

    // append MSG if there is one
    if (MSG != NULL)
    {
      unsigned int new_size = strlen(rows) + strlen(MSG);
      rows = (char*)krealloc(rows, sizeof(char)*new_size, 0);
      while (rows == NULL)
	rows = (char*)krealloc(rows, sizeof(char)*new_size, 0); 

      rows = strcat(rows, MSG);
    }
    
    if (copy_to_user(user_buffer, rows, strlen(rows)))
    {
      kfree(rows);
      kfree(prefix);    
      return -EFAULT;
    }
  
    kfree(rows);
    kfree(prefix);
  }
  
  *offset = 1;
  return size;
}

// handle commands
static ssize_t write_ttt(struct file *file, const char __user *user_buffer, size_t size, loff_t *offset)
{
  int move_count = 0, i = 0, j = 0;
  if (*offset != 0) return 0;

  if (copy_from_user(command, user_buffer, 10))
    return -EFAULT;

  
  //clear command - reset board
  if (strncmp(command, RESET, sizeof(char)*5) == 0)
  {
    i = 0;
    for (; i < 3; i++)
    {
      j = 0;
      for (; j < 3; j++)
      {
	board[i][j] = '-';
      }
    }

    i = 0;
    for (; i < 8; i++)
    {
      scores[i] = 0;
    }
    status = 0;
  }
  // TURN command
  else if (strncmp(command, TURN, sizeof(char)*4) == 0)
  {
    int row = command[7] - 49;
    int col = command[5] - 65;
    if (board[row][col] == '-')
    {
      board[row][col] = 'X';

      // update row/col/diagonal scores
      scores[row]++;
      scores[col + 3]++;
      if (row != 1 && col != 1)
      {
	if (row > col || col > row)
	  scores[6]++;
	else
	  scores[7]++;
      }
      
      status = 0;

      // check if player won
      if (scores[row] == 3 || scores[col + 3] == 3 || scores[6] == 3
	  || scores[7] == 3)
      {
	status = 1;
	*offset = 1;
	return size;
      }
      
      struct point {
	int row;
	int col;
      };

      // find possible moves
      struct point moves[9];
      i = 0;
      for (; i < 3; i++)
      {
	j = 0;
	for (; j < 3; j++)
	{
	  if (board[i][j] == '-')
	  {
	    moves[move_count].row = i;
	    moves[move_count].col = j;
	    move_count++;
	  }
	}
      }

      // no possible moves means there has been a draw
      if (move_count == 0)
      {
	status = 3;
	*offset = 1;
	return size;
      }

      // generate CPU move
      unsigned int *move = (unsigned int*)kmalloc(sizeof(unsigned int), 0);

      while (move == NULL) move = (unsigned int*)kmalloc(sizeof(unsigned int), 0);
      get_random_bytes(move, sizeof(int));
      *move = *move % move_count;

      // CPU performs move
      int cpu_row = moves[*move].row, cpu_col = moves[*move].col; 
      board[cpu_row][cpu_col] = 'O';

      scores[cpu_row]--;
      scores[cpu_col + 3]--;
      if (row != 1 && col != 1)
      {
	if (row > col || col > row)
	{
	  scores[6]--;
	}
	else
	{
	  scores[7]--;
	}
      }
      
      // check if CPU won
      if (scores[row] == -3 || scores[col + 3] == -3 || scores[6] == -3
	  || scores[7] == -3)
      {
	status = 2;
	*offset = 1;
	return size;
      }
      
    }
    else
    {
      // illegal move
      status = -1;
    }
  }
  
  
  *offset = 1;
  return size;
}


static int open_ttt(struct inode *inode, struct file *file)
{
  return 0;
}

static int close_ttt(struct inode *inode, struct file *file)
{
  return 0;
}

static const struct file_operations file_ops = {
  .owner = THIS_MODULE,
  .read = read_ttt,
  .write = write_ttt,
  .open = open_ttt,
  .release = close_ttt
};

static struct miscdevice tictactoe_device = {
  .name = "tictactoe",
  .minor = MISC_DYNAMIC_MINOR,
  .fops = &file_ops,
  .mode = 0666,
};

static int __init tictactoe_init(void)
{
  int i = 0;
  int result = misc_register(&tictactoe_device);
  if (result)
    return -result;

  command = (char*)kmalloc(sizeof(char)*10, 0);
  while (command == NULL) command = (char*)kmalloc(sizeof(char)*10, 0);

  for (; i < 8; i++)
    scores[i] = 0;
  
  status = 0;
  return 0;
}

static void __exit tictactoe_exit(void)
{
  misc_deregister(&tictactoe_device);
}

module_init(tictactoe_init);
module_exit(tictactoe_exit);
