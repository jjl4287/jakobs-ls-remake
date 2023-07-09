////////////////////////////////////////////////////////////////////////////////

/*
 * l - an implementation of ls with -a, -l, -i, -r support
 *     along with use of ioctl to determine semi-proper spacing
 *
 *     known issues:
 *          - no dynamic spacing on -l
 *          - missing all other options for ls
 *          - does not print archives etc. red
 *          - does not list what links link to
 *          - does not list downwards and then across with spacing
 *     
 *     written by:
 *         - Jakob Langtry
 */

#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/types.h> 
#include <sys/stat.h>
#include <sys/ioctl.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <time.h>
#include <pwd.h>
#include <grp.h>
#include <limits.h>
#include <termios.h>

void list(char[], int);       // prints the list
char *pstring(struct stat *); // permissions string
char *gstring(struct stat *); // user and group string
int color = 7; // global color variable, set to white
int bold  = 0; // global variable to set boldness
int wide  = 0; // global variable for width
int winsize(); // grabs winsize for normal ls

/*
 * main - takes command line arguments given and
 *        decides what to pass to the list function for listing.
 *        Default behavior - passes "." directory.
 *        Anything else is passed as an array of chars called drname,
 *        which will allow opendir to find it.
 */
int main(int argc, char **argv)
{
    int optflag = 0;  // octal flag to AND for option functionality
    int opt    = -1;  // variable to store getopt() return
    while ((opt = getopt(argc, argv, "rail")) != -1)
    {
        switch (opt)  // increments octal value for options
        {
            case 'r': // -r is given
                optflag++;
                break;
            case 'a': // -a is given
                optflag = optflag + 2;
                break;
            case 'i': // -i is given
                optflag = optflag + 4;
                break;
            case 'l': // -l is given
                optflag = optflag + 8;
                break;
            default:  // nothing is given
                break;
        }
    }

    // when there are more arguments than options, take them
    // as directory names to pass
    if (optind < argc)
    {
        while ( optind < argc)
        {
            printf("\033[00;37m%s: \n", argv[optind]); // prints argument
            list(argv[optind], optflag);               // passes argument
            printf("\n");                              // formatting work
            optind++;
        }
    }
    else // else pass current working directory
    {
        list(".", optflag); // default behavior
    }
    if (wide)
    {
        printf("\n");
    } 
}

/*
 * list - takes args passed from main, opens directory pointer
 * prints error if need be, reads directory, prints contents,
 * and then closes directory
 */
void list(char drname[], int optflag)
{
    struct stat fstat;
    struct dirent **dirbuf;      // directory data struct
    DIR    *dirfd;               // directory pointer struct
    int     n = 0;               // scandir return int
    int termwidth;               // terminal width integer
    char    *path;               // /absolute/path/to/file string

    termwidth = winsize();       // sets termwidth

    // debug command to see if termwidth is correct
    // printf("%d\n\n\n", termwidth);

    dirfd = opendir(drname);     // opens the directory

    // use of scandir to list and sort directory
    n = scandir(drname, &dirbuf, NULL, alphasort);

    if (dirfd == NULL)           // checks for error
    {
        perror("Error");         // gives correct error code
        exit(EXIT_FAILURE);      // exits
    }
    else if (n == -1) 
    {
        perror("Scandir");       // gives correct error code
        exit(EXIT_FAILURE);      // exits
    }
    else // if no error
    {
        
        for (int i = 0; n--; i++) 
        {
            color = 7;           // resetting color to white
            bold  = 0;           // resetting bold to white mode


            // if reverse option is given list in reverse
            if ((optflag & 1) == 1)
            {
                i = n;
            }
            // uses and checks realpath command
            path = realpath(drname, NULL);
            if ( ! path )
            {
                perror("realpath");
            }

            // debug command
            // fprintf (stderr, "path: %s\n", path);
            // fprintf(stderr, "dirbuf: %s\n", dirbuf[i]->d_name);
            
            // appends the name of the entry to the 
            // path from realpath
            strcat(path, "/");
            strcat(path, dirbuf[i]->d_name);

            // debug command
            // fprintf (stderr, "path2: %s\n", path);

            // if this lstat is ! 0 there has been an error
            if ( lstat(path, &fstat))
            {
                perror("lstat Error");
            }

            // while we arent using the string it returns
            // we are using the color variable it sets
            pstring(&fstat);

            // check whether should be printed bold or not 
            if (color != 7)
            {
                bold = 1;
            }

            // if option -a is given
            if ((optflag&2) == 2)
            {   
                // if -i is given
                if ((optflag&4) == 4)
                {
                    // print inode number
                    printf("\033[00;37m%7lu ", fstat.st_ino);
                }
                // if -l is given
                if ((optflag&8) == 8)
                {
                    // print the many attributes of -l
                    printf("\033[00;37m%s ", pstring(&fstat));

                    // this determines if we are 64 bit or 32 bit
                    #if defined(__LP64__) || defined(_LP64)
                        printf("%3lu ", fstat.st_nlink);
                    #else
                        printf("%3u ",  fstat.st_nlink);
                    #endif

                    // prints the rest after the 32-64 bit dependancy
                    printf("%-17s %10lu %.12s ",
                    gstring(&fstat), fstat.st_size,
                    4+ctime(&fstat.st_mtime));

                    // print normal ls at the end
                    printf("\033[0%d;3%dm%s  ",
                    bold, color, dirbuf[i]->d_name);
                    printf("\n");
                }
                // print newline if listing exceeds termwidth for normal ls
                else if ((wide=((wide+strlen(dirbuf[i]->d_name))+2))>=termwidth)
                {
                    printf("\n");
                    printf("\033[0%d;3%dm%s  ",
                    bold, color, dirbuf[i]->d_name);

                    // reset width counter
                    wide = wide - wide;
                }
                else
                {
                    // print normal ls at the end
                    printf("\033[0%d;3%dm%s  ",
                    bold, color, dirbuf[i]->d_name);
                }
            }
            else if ((dirbuf[i]->d_name[0]) != 46) // if -a isnt given
            {
                // if -i is given
                if ((optflag&4) == 4)
                {
                    // print inode number
                    printf("\033[00;37m%7lu ",  fstat.st_ino);
                }
                // if -l is given
                if ((optflag&8) == 8)
                {
                    // print the many attributes of -l
                    printf("\033[00;37m%s ", pstring(&fstat));

                    // this determines if we are 64 bit or 32 bit
                    #if defined(__LP64__) || defined(_LP64)
                        printf("%3lu ", fstat.st_nlink);
                    #else
                        printf("%3u ",  fstat.st_nlink);
                    #endif

                    // prints the rest after the 32-64 bit dependancy
                    printf("%-17s %10lu %.12s ",
                    gstring(&fstat), fstat.st_size,
                    4+ctime(&fstat.st_mtime));

                    // print normal ls at the end
                    printf("\033[0%d;3%dm%s  ",
                    bold, color, dirbuf[i]->d_name);
                    printf("\n");
                }
                // print newline if listing exceeds termwidth for normal ls
                else if ((wide=((wide+strlen(dirbuf[i]->d_name))+2))>=termwidth)
                {
                    printf("\n");
                    printf("\033[0%d;3%dm%s  ",
                    bold, color, dirbuf[i]->d_name);

                    // reset width counter
                    wide = wide - wide;
                }
                else
                {
                    // print normal ls at the end
                    printf("\033[0%d;3%dm%s  ",
                    bold, color, dirbuf[i]->d_name);
                }
            }
            free(dirbuf[i]);
        }
        free (dirbuf);
    }
    closedir(dirfd); // close the directory
}

/*
 * pstring - returns permission string
 * in human readable form. 
 */
char *pstring(struct stat * srcstr){
	char *dststr=malloc(11);
	strcpy(dststr,"----------");

	// the most manual thing I think ive ever done
    // but this ands the flags to see the human
    // readable permissions
	mode_t mode = srcstr->st_mode;
	if (S_IRUSR & mode)
        dststr[1]='r';
	if (S_IWUSR & mode)
        dststr[2]='w';
	if (S_IXUSR & mode)
    {
        dststr[3]='x';
        color = 2;
    }
	if (S_IRGRP & mode)
        dststr[4]='r';
	if (S_IWGRP & mode)
        dststr[5]='w';
	if (S_IXGRP & mode)
    {
        dststr[6]='x';
        color = 2;
    }
	if (S_IROTH & mode)
        dststr[7]='r';
	if (S_IWOTH & mode)
        dststr[8]='w';
	if (S_IXOTH & mode)
    {
        dststr[9]='x';
        color = 2;
    }
	if (S_ISDIR(mode))
    {
        dststr[0]='d';
        color = 4;
    }
    if (S_ISLNK(mode))
    {
        dststr[0]='l';
        color = 6;
    }

	return (dststr);
}

/*
 * gstring - totally did not change all function names
 * so that i could call this one gstring. 
 * will return string with spaced out user and group id
 */
char *gstring(struct stat * srcstr)
{
    // use library functions to get user and group id's
    struct passwd *usrptr = getpwuid(srcstr->st_uid);
    if (usrptr == NULL) // give errors
    {
        exit(EXIT_FAILURE);
    }
    struct group *grpptr = getgrgid(srcstr->st_gid);
    if (grpptr == NULL) // give errors
    {
        exit(EXIT_FAILURE);
    }

    // finding some way to combine these things into one
    // result string was difficult, because i remember
    // the use of sprintf but didnt know how to form it
    // into one result - i ended up being pretty smart about 
    // it i think.

    // add two lengths (plus two spaces)
    int size = strlen(usrptr->pw_name) + strlen(grpptr->gr_name) + 2;
    // and throw them into newly sized str
	char *dststr = (char*)malloc(size); 
	sprintf(dststr, "%s %s", usrptr->pw_name, grpptr->gr_name);
	return (dststr);
}

/*
 * winsize - a function to use an ioctl call to determine winsize for
 *           normal ls functionality
 *           
 *           the link for where i got this code from (i tried the ioctl
 *           man page and such but couldnt find anything) will be in my
 *           journal, because its way longer than 80 characters
 */
int winsize()
{
    struct winsize termwidth;
    int               termfd;

    // opens tty file for reading
    if ((termfd = open("/dev/tty", O_RDWR|O_NOCTTY)) == -1)
    {
        perror("open dev/tty");
        exit(EXIT_FAILURE);
    }

    // calls ioctl to read winsize
    if (ioctl(termfd, TIOCGWINSZ, &termwidth) == -1)
    {
        perror("ioctl");
        exit(EXIT_FAILURE);
    }

    // returns winsize
    return(termwidth.ws_col);
}