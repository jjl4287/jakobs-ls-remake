# jakobs-ls-remake

This is a simple implementation of the `ls` command in C using low-level systems programming techniques. The `list` program can be used to list the contents of a directory, including hidden files and directories.

## Usage

To use the `list` program, simply compile the `list.c` file using a C compiler and run the resulting executable. By default, the program will list the contents of the current directory. You can specify a different directory to list by passing its path as a command line argument:

```./list /path/to/directory```


## Options
The list program supports the following options:

    -r: Reverse the order of the list.
    -a: Include hidden files and directories in the list.
    -i: Display the inode number of each file in the list.
    -l: Display the list in long format, including file permissions, owner, group, size, and modification time.
To use an option, simply include it as a command line argument when running the list program. For example, to list the contents of the current directory in reverse order, you would run:

```./list -r```

To list the contents of a specific directory in long format, including hidden files and directories, you would run:

```./list -la /path/to/directory``

Note that options can be combined, so you can use -al instead of -a -l.