#include	<stdio.h>
#include	<sys/types.h>
#include	<dirent.h>
#include	<sys/stat.h>
#include	<pwd.h>
#include	<grp.h>
#include    <string.h>
#include	<unistd.h>
#include	<stdlib.h>
#include	<fcntl.h>

#define BUFFERSIZE      4096
#define COPYMODE        0644


void open_dir(char[], char[]);
void copy_file(char *, char[]);
void oops(char *, char *);

int main(int ac, char *av[]){
	if ( ac == 2 )
		open_dir( ".", av[1]);
	if ( ac == 3)
		open_dir( av[1], av[2]);
    return 0;
}		
						
/*******************************************
 *	list files in directory called dirname */
 void open_dir( char dirname[], char tardir[] ){
	DIR		*dir_ptr;		/* the directory */
	struct dirent	*direntp;	/* each entry	 */
    printf("IN open_dir: dirname: %s | tardir: %s\n", dirname, tardir);
	if ( ( dir_ptr = opendir( dirname ) ) == NULL )
		fprintf(stderr,"ls1: cannot open %s\n", dirname);
	else {
		while ( ( direntp = readdir( dir_ptr ) ) != NULL ) {
            printf("IN open_dir: dirfile: %s | tardir: %s\n", direntp->d_name, tardir);
			copy_file( direntp->d_name, tardir);}
		closedir(dir_ptr);
	}
}

/********************************************
 *  copys file								*/
void copy_file( char *filename, char dirname[]){
    
		
		int     in_fd, out_fd, n_chars;
        char    buf[BUFFERSIZE];
		DIR		*dir_ptr;		/* the directory */
		struct dirent	*direntp;	/* each entry	 */      						
						/* open files	*/
        if ( (in_fd=open(filename, O_RDONLY)) == -1 )
                oops("Cannot open ", filename);
		if ( ( dir_ptr = opendir( dirname ) ) == NULL )
		fprintf(stderr,"ls1: cannot open %s\n", dirname);
		else {
			if ( (out_fd=creat( filename, COPYMODE)) == -1 )
                oops( dirname, filename);
		}
						/* copy files	*/

        while ( (n_chars = read(in_fd , buf, BUFFERSIZE)) > 0 )
                if ( write( out_fd, buf, n_chars ) != n_chars )
                        oops("Write error to ", filename);
	if ( n_chars == -1 )
			oops("Read error from ", filename);

						/* close files	*/

        if ( close(in_fd) == -1 || close(out_fd) == -1 )
                oops("Error closing files","");
}
void oops(char *s1, char *s2)
{
        fprintf(stderr,"Error: %s ", s1);
        perror(s2);
        exit(1);
}