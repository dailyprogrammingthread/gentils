/* this header is temporary and stuff
 * I guess it's released under the MIT liscence
 * or something
 * Nov 19 16:20:13 EST 2012
 * 420 smoak moar
 */

#include "../shared.h"

/* the following should be in shared.h
 * or created by a Makefile
 */

#define VERSION "1.0"
#define LICENSE "MIT"

/*typedef for return types*/
typedef enum {
    NO_ERROR,
    ERROR_EOF,
    ERROR_MEM,
    ERROR_FILE,
} RESULT;

/*Sets the file position to just after a character 'c'*/
RESULT skip_over_char(char c, FILE * const fp)
{
    while(1) {
        int fc = fgetc(fp);
        if (fc == EOF) return ERROR_EOF;
        if (fc == c)   return NO_ERROR;
    }
}

RESULT process_passwd(void)
{
    uid_t euid = geteuid();
    FILE *passwd = fopen("/etc/passwd", "r");

    /*perhaps we're in an incomplete chroot or something*/
    if (!passwd)
        return ERROR_FILE;

    /*file position variables*/
    int start, end, len;

    /*temporary storage for user names and ids*/
    char *name = NULL;
    int   tempid;
    char id_found = 0;

    RESULT RETURN_STATUS = NO_ERROR;
    while (1)
    {
        /*record position of name*/
        start = ftell(passwd);

        /*move over ':', check for eof*/
        if (skip_over_char(':', passwd))
        {
            RETURN_STATUS = ERROR_EOF;
            break;
        }

        /* record current position - one char after the ':' character
         * then move back to string, calculate length */

        end = ftell(passwd) - 1;
        fseek(passwd, start, SEEK_SET);
        len = end - start;

        /*I image this always *should* be true*/
        if (len > 0)
        {
            /*reallocate space for string, including NULL byte*/
            if (!(name = realloc(name, len+1)))
            {
                RETURN_STATUS = ERROR_MEM;
                break; 
            }

            /*read string, if it fucks up return 1 I guess*/
            if (fread(name, 1, len, passwd) != len)
            {
                RETURN_STATUS = ERROR_FILE;
                break;
            }
            name[len] = 0;

            /*move back over two colons*/
            if (skip_over_char(':', passwd) || skip_over_char(':', passwd))
            {
                RETURN_STATUS = ERROR_EOF;
                break;
            }

            /*read and check the uid*/
            fscanf(passwd, "%d", &tempid);
            if (tempid == euid) id_found = 1;
        } 
        else {
            /*possible formatting error in passwd file*/
            RETURN_STATUS = ERROR_FILE;
            break;
        }
        
        /*if success, report the "user name" to stdout*/
        if (id_found)
        {
            printf("%s\n", name);
            RETURN_STATUS = NO_ERROR;
            break;
        } 
        
        /*skip to next line, check for EOF*/
        if (skip_over_char('\n', passwd))
        {
            RETURN_STATUS = ERROR_EOF;
            break;
        }
    } //while(1)

    if (name) free(name);
    return RETURN_STATUS;
}

void print_usage(char const * const progname)
{
    printf("Usage %s [OPTION]...\n", progname); 
    printf("Print the user name associated with the current effective user ID.\n");
    printf("Same as id -un.\n");
    printf("\n");
    printf("\t--help     display this help and exit\n");
    printf("\t--version  output version information and exit\n");
}

void print_version()
{
    printf("whoami (gentils) "VERSION"\n");
    printf("License "LICENSE"\n");
}

int find_argument(char const * const arg, char const * const pattern)
{
    if (strlen(arg) == strlen(pattern) + 2)
    {
        if (arg[0] == '-' && arg[1] == '-')
        {
            if (!strcmp(pattern, &arg[2]))
                return 1;
        }
    }
    return 0; 
}

int main(int argc, char *argv[])
{
    if (argc > 1) {
        char **arg = argv;
        while (*(++arg))
        {
            if (find_argument(*arg, "help")) {
                print_usage(argv[0]);
                return EXIT_SUCCESS;
            }
            else if (find_argument(*arg, "version")) {
                print_version();
                return EXIT_SUCCESS;
            }
            else {
                printf("%s: extra operand `%s’\n", argv[0], *arg);
            }
        }

        print_usage(argv[0]);
    }

    switch (process_passwd())
    {
        case NO_ERROR:
            return EXIT_SUCCESS;
        case ERROR_EOF:
            fprintf(stderr, "Premature EOF, check your /etc/passwd!\n");
            return EXIT_FAILURE;
        case ERROR_FILE:
            fprintf(stderr, "Error occured when reading /etc/passwd!\n");
            return EXIT_FAILURE;
        case ERROR_MEM:
            fprintf(stderr, "Encountered a memory error!\n");
            return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
