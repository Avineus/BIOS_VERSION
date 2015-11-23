/*
 * BIOS Bin file Information especially for Insyde's BIOS bianry image
 * Compilation:
 * if O_LARGEFILE option is used then conpile with -DGNU_SOURCE option
 * gcc -D_GNU_SOURCE biosinfo.c
 */
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>


/* Reverse engineered */
#if 0 /* hexdump -C bin_file */
....
0060b000  24 42 56 44 54 24 00 00  00 24 00 00 00 24 4a 75  |$BVDT$...$...$Ju|
0060b010  6e 69 70 65 72 50 4c 35  2e 30 39 2e 30 34 2e 30  |niperPL5.09.04.0|
0060b020  30 2e 30 30 00 00 24 4d  6f 68 6f 6e 50 65 61 6b  |0.00..$MohonPeak|
0060b030  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
0060b040  24 30 35 2e 30 34 2e 30  34 00 00 00 00 00 00 00  |$05.04.04.......|
0060b050  00 00 00 00 00 00 00 00  00 00 00 00 00 00 00 00  |................|
0060b060  00 00 00 00 00 00 ff ff  ff ff ff ff ff ff ff ff  |................|
0060b070  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
*
0060b120  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff 24  |...............$|
0060b130  42 4d 45 24 00 b0 40 00  00 02 00 00 24 00 00 35  |BME$..@.....$..5|
0060b140  00 00 00 0b 00 24 00 ff  ff ff ff ff ff ff ff 24  |.....$.........$|
0060b150  5f 4d 53 43 5f 56 45 52  3d 40 06 24 52 44 41 54  |_MSC_VER=@.$RDAT|
0060b160  45 15 05 27 24 45 53 52  54 00 00 04 54 02 6c 5c  |E..'$ESRT...T.l\|
0060b170  ef 33 9d 29 41 80 42 d1  32 53 ed 1f bd 8b 01 00  |.3.)A.B.2S......|
0060b180  00 24 45 4e 44 4f 46 42  56 44 54 ff ff ff ff ff  |.$ENDOFBVDT.....|
0060b190  ff ff ff ff ff ff ff ff  ff ff ff ff ff ff ff ff  |................|
....
#endif /* 0*/

#define START_OF_WORD                       '$'
#define BIOS_VALUE_DATAFORMAT_TYPE_START    "BVDT"
#define BIOS_VALUE_DATAFORMAT_TYPE_END      "ENDOFBVDT"

#define PROG_OPTION     "f:"


extern char * program_invocation_name;

typedef struct bvdt_s {
    char str1[8];
    char str2[8];
    char name[32];
    char board[32];
    char x_ver[32];
} bvdt_t;

bvdt_t bios_info_g;

void usage(void)
{
    fprintf(stderr, "Usage: %s -f <bios bin>\r\n", program_invocation_name);    
}

int seek_to_bvdt(int fd) 
{
    int n = 0;
    unsigned long long i = 0;
    unsigned char buf[8];
    int c = 0;
    unsigned char check_pattern[8];
    int check = 0;

    do {
        n = read(fd, buf, 1);
        i+=n;
        if (n <= 0 ) {
            fprintf(stderr, "%s:%d read byte failed offset %#llx: %m\r\n", __FUNCTION__, __LINE__, i);
            break;
        }
        if (check) {
            check_pattern[c] = buf[0];
            if (c >= strlen(BIOS_VALUE_DATAFORMAT_TYPE_START)) {
                if (! strncmp(check_pattern, BIOS_VALUE_DATAFORMAT_TYPE_START, c)) {
                    /* Found */
                    return 0;
                }
                else {
                    check = 0;
                }
            }
            c++;
        }
        if(START_OF_WORD == buf[0]) {
            check = 1;
            c = 0;
        }
    } while(n);
    /* Not Found */
    return 1;
}

int read_bvdt(int fd) 
{
    int i = 0;
    int n = 0;
    int c = 0;
    char buf;
    char *off_ptr[5];

    off_ptr[0] = bios_info_g.str1;
    off_ptr[1] = bios_info_g.str2;
    off_ptr[2] = bios_info_g.name;
    off_ptr[3] = bios_info_g.board;
    off_ptr[4] = bios_info_g.x_ver;
    do {
        c = read(fd, &buf, 1);
        if (c <= 0 ) {
            fprintf(stderr, "%s:%d read byte failed: %m\r\n", __FUNCTION__, __LINE__);
            break;
        }
        off_ptr[i][n] = buf;
        n++;
        if (START_OF_WORD == buf) {
            i++;
            n = 0;
        }
        if (i>=5) {
            return 0;
        }
    } while(c);

    return 1;
}

int dump_bvdt(void) 
{
    int i = 0;
    char *off_ptr[5];

    off_ptr[0] = bios_info_g.str1;
    off_ptr[1] = bios_info_g.str2;
    off_ptr[2] = bios_info_g.name;
    off_ptr[3] = bios_info_g.board;
    off_ptr[4] = bios_info_g.x_ver;
    do {
        printf("%s\n", off_ptr[i]);
        i++;
    } while(i<5);

    return 0;
}

int main(int argc, char *argv[])
{
    int opt;
    int fd;
    int n;
    char *fname = NULL;
    char buf[256] = {0};

    while ((opt = getopt(argc, argv, PROG_OPTION)) != -1) { 
        switch(opt) {
            case 'f':
                fname = optarg; 
                break;
            default:
                usage();
                exit(1);
        }
    }
    if (NULL == fname) {
        usage();
        exit(1);
    }

    //fd = open(fname, O_RDONLY|O_LARGEFILE);
    fd = open(fname, O_RDONLY);
    if (fd < 0) {
        printf("open %s failed: %m\r\n", fname);
        exit(2);
    }

    if (seek_to_bvdt(fd)) {
        close(fd);
        exit(3);
    }

    if (read_bvdt(fd)) {
        close(fd);
        exit(4);
    }

    //dump_bvdt();
    printf("BIOS Model Name : %s\r\n", bios_info_g.board);
    printf("BIOS Version    : %s\r\n", bios_info_g.name);

    exit(0);
}
