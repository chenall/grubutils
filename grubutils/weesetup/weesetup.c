/*
 * Distributed as public domain
 * It is free as in both free beer and free speech.
 *
 * THIS SOFTWARE IS PROVIDED BY SvOlli ''AS IS'' AND ANY EXPRESS OR IMPLIED
 * WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF 
 * MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN
 * NO EVENT SHALL SvOlli BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
 * SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
 * TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR 
 * PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
 * LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
 * NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
 * SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 *
 * written by svolli at 2010-09-04,   http://svolli.org
 * Modify By chenall at 2011-01-28,   http://chenall.net
 * rewritten by svolli at 2011-12-03, http://svolli.org
 * Report bugs to website:  http://code.google.com/p/grubutils/issues
 */

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <getopt.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include "mbr.h"
#include "xdio.h"
#include "utils.h"

#define FLAG_UPDATE      (1<<0)
#define FLAG_FORCE       (1<<1)
#define FLAG_VERBOSE     (1<<2)
#define FLAG_DEBUG       (1<<3)
#define FLAG_SHOWSCRIPT  (1<<4)
#define FLAG_BACKUPMBR   (1<<5)
#define FLAG_RESTOREMBR  (1<<6)
#define FLAG_LISTDEVS    (1<<7)

#define SECTOR_SIZE      (0x200)
#define SECTOR_BIT      9
#define WEE_SECTORS	63
#define PARTTABLE_SIZE   (0x48)
#define WEE_SIGN         (0xCE1A02B0)
#define MAX_WEE_SIZE	(WEE_SECTORS<<SECTOR_BIT)
typedef unsigned char uchar;
typedef unsigned short uchar2;
typedef unsigned long uchar4;
struct fb_mbr
{
   uchar jmp_code;
   uchar jmp_ofs;
   uchar boot_code[0x1ab];
   uchar max_sec;		/* 0x1ad  */
   uchar2 lba;			/* 0x1ae  */
   uchar spt;			/* 0x1b0  */
   uchar heads;			/* 0x1b1  */
   uchar2 boot_base;		/* 0x1b2  */
   uchar4 fb_magic;		/* 0x1b4  */
   uchar mbr_table[0x46];	/* 0x1b8  */
   uchar2 end_magic;		/* 0x1fe  */
} PACK;


/*!
 \brief device handle

*/
typedef struct device_s {
   char *name;
   int  fd;
   xd_t *xd;
} device_t;


/*!
 \brief round up to next 0x200 boundry

 \param bytes value to round up
 \return int uprounded value
*/
inline int round2blocks( int bytes )
{
   bytes +=  (SECTOR_SIZE-1);
   bytes &= ~(SECTOR_SIZE-1);
   return bytes;
}


/*!
 \brief calculate blocks used by bytes

 \param bytes size in bytes
 \return int blocks used
*/
inline int bytes2blocks( int bytes )
{
   return round2blocks( bytes ) >> 9;
}


/*!
 \brief open a device either in path notation ("/dev/sda") or grub notation
        ("(hd0)")

 \param dev device handle
 \param name name of device
*/
static void open_dev( device_t *dev, char *name )
{
   dev->name = name;
   if( *name == '(' )
   {
      dev->xd = xd_open( name, 1, 1 );
      dev->fd = -1;
   }
   else
   {
      dev->fd = open( name, O_RDWR, 0666 );
      dev->xd = (xd_t*)0;
   }
   if( (dev->fd < 0) && !(dev->xd) )
   {
      fprintf( stderr, "open_dev failed for %s: %s\n",
               dev->name, strerror(errno) );
      exit(1);
   }
}


/*!
 \brief close opened device

 \param dev device handle
*/
static void close_dev( device_t *dev )
{
   if( dev->xd )
   {
      xd_close( dev->xd );
      dev->xd = (xd_t*)0;
   }

   if( dev->fd >= 0 )
   {
      close( dev->fd );
      dev->fd = -1;
   }

   dev->name = (char*)0;
}


/*!
 \brief read from device

 \param dev device handle
 \param buf buffer for read data
 \param sectors number of sectors to read
*/
static void read_dev( device_t *dev, void *buf, int sectors )
{
   int i;
   long offset;
   errno = 0;
   if( dev->xd )
   {
      offset = dev->xd->ofs;
      i = xd_read( dev->xd, buf, sectors );
   }
   else
   {
      offset = lseek( dev->fd, 0, SEEK_CUR );
      i = read( dev->fd, buf, SECTOR_SIZE * sectors );
      i = ( i != (SECTOR_SIZE * sectors) );
   }
   if( i )
   {
      fprintf( stderr, "read_dev failed for %s at pos %ld, size %d: %s\n",
               dev->name, offset, sectors, strerror(errno) );
      exit(1);
   }
}


/*!
 \brief write device

 \param dev device handle
 \param buf buffer with data to write
 \param sectors number of sectors to write
*/
static void write_dev( device_t *dev, void *buf, int sectors )
{
   int i;
   long offset;
   if( dev->xd )
   {
      offset = dev->xd->ofs;
      i = xd_write( dev->xd, buf, sectors );
   }
   else
   {
      offset = lseek( dev->fd, 0, SEEK_CUR );
      i = write( dev->fd, buf, SECTOR_SIZE * sectors );
      i = ( i != (SECTOR_SIZE * sectors) );
   }
   if( i )
   {
      fprintf( stderr, "write_dev failed for %s at pos %ld, size %d\n",
               dev->name, offset, sectors );
      exit(1);
   }
}


/*!
 \brief seek to a specific sector

 \param dev device handle
 \param ofs sector to seek to
*/
static void seek_dev( device_t *dev, unsigned long ofs )
{
   int i;
   long offset;
   if( dev->xd )
   {
      offset = ofs;
      i = xd_seek( dev->xd, ofs );
   }
   else
   {
      offset = lseek( dev->fd, ofs * SECTOR_SIZE, SEEK_SET );
      i = (offset != ofs * SECTOR_SIZE);
      offset /= SECTOR_SIZE;
   }
   if( i )
   {
      fprintf( stderr, "seek_dev failed for %s at pos %ld\n",
               dev->name, offset );
      exit(1);
   }
}


/*!
 \brief print out a list of all devices in grub notation

*/
void list_devs (void)
{
   int i;
   char name[16];

   for (i = 0; i < MAX_DISKS; i++)
   {
      xd_t *xd;

      sprintf (name, "(hd%d)", i);
      xd = xd_open ( name, 1, 1 );
      if (xd)
      {
         unsigned long size;
         int s;
         char c;
         struct fb_mbr m;

         size = xd_size (xd);
         if (size == XD_INVALID_SIZE)
            goto quit;

         if (xd_read (xd, (char *) &m, 1))
            goto quit;

         if (size >= (3 << 20))
         {
            s = (size + (1 << 20)) >> 21;
            c = 'g';
         }
         else
         {
            s = (size + (1 << 10)) >> 11;
            c = 'm';
         }

         printf ("%s: %lu (%d%c)\n", name, size, s, c);

quit:
         xd_close (xd);
      }
   }
}


/*!
 \brief read a file to memory

 \param filename name of file to read
 \param buf buffer to read data to
 \param size maximum size of data
 \return int data read (if buffer is 0 size of file is returned and no data is read)
*/
static int read_file( const char *filename, char *buf, int size )
{
   int fd       = 0;
   int dataread = 0;
   int chunk    = 0;
   fd = open( filename, O_RDONLY | O_BINARY );
   if( fd < 0 )
   {
      fprintf( stderr, "error opening file \"%s\" for reading: %s", filename,
               strerror( errno ) );
      exit( 2 );
   }
   
   if( buf )
   {
      while( dataread < size )
      {
         chunk = read( fd, buf, size - dataread );
         if( chunk == 0 )
         {
            /* file is shorter than reserved buffer */
            break;
         }
         if( chunk < 0 )
         {
            fprintf( stderr, "error reading file %s at %d: %s\n", filename,
                     dataread, strerror(errno) );
            exit( 2 );
         }
         dataread += chunk;
         buf      += chunk;
      }
   }
   else
   {
      dataread = lseek( fd, 0, SEEK_END );
   }
   close( fd );
   return dataread;
}


/*!
 \brief write a chunk of memory to a file

 \param filename name of file to write to
 \param buf data to write
 \param size size of data
 \return int bytes written
*/
static int write_file( const char *filename, char *buf, int size )
{
   int fd          = 0;
   int datawritten = 0;
   int chunk       = 0;
   fd = open( filename, O_WRONLY | O_BINARY | O_CREAT | O_TRUNC, 0666 );
   if( fd < 0 )
   {
      fprintf( stderr, "error opening file \"%s\" for writing: %s", filename,
               strerror( errno ) );
      exit( 2 );
   }

   while( datawritten < size )
   {
      chunk = write( fd, buf, size - datawritten );
      if( chunk <= 0 )
      {
         fprintf( stderr, "error writing file %s at %d: %s\n", filename,
                  datawritten, strerror(errno) );
         exit( 2 );
      }
      datawritten += chunk;
      buf         += chunk;
   }
   close( fd );
   return datawritten;
}


/*!
 \brief check if an mbr is suited for installation

 \param mbr data of mbr
 \param sectors_needed number of sectors needed for installation
 \return int 0=ok, else fail
*/
int check_mbr_data(char *mbr, unsigned long sectors_needed)
{
   if (*(unsigned short *)(mbr + 510) != 0xAA55 ||
       *(unsigned long *)(mbr + 0x1B4) == 0x46424246 ||
       memcmp(mbr + 0x200,"EFI PART",8) == 0)
   {
      fprintf(stderr, "not supported MBR.\n");
      return 1;
   }

   int n;
   unsigned long ofs = 0xFFFFFFFF;
   for (n=0x1BE;n<0x1FE;n+=16)
   {
      if (mbr[n+4])
      {
         if (ofs>valueat(mbr[n],8,unsigned long))
            ofs=valueat(mbr[n],8,unsigned long);
      }
   }
   if (ofs < sectors_needed)
   {
      fprintf( stderr,"Not enough room to install mbr\n" );
      return 1;
   }
   return 0;
}


/*!
 \brief write the backup on second sector back to first one

 \param name name of device containing wee
 \return int 0=ok, else fail
*/
int restore_mbr( char *name, int install_flag )
{
   device_t dev = { 0, -1, 0 };
   int retval = 1;
   char *wee_data = 0;

   wee_data = (char*)malloc( 0x400 );

   open_dev( &dev, name );
   seek_dev( &dev, 0 );
   read_dev( &dev, wee_data, 1 );
   seek_dev( &dev, 1 );
   read_dev( &dev, wee_data + 0x200, 1 );
   if( *(unsigned long *)(wee_data + 0x86) != WEE_SIGN )
   {
      fprintf( stderr, "\"%s\" does not contain a wee mbr\n", name );
      goto quit;
   }

   if( memcmp( wee_data + 0x1B8, wee_data + 0x3B8, PARTTABLE_SIZE ) )
   {
      if( install_flag & FLAG_FORCE )
      {
         fprintf( stderr, "partition tables differ, using primary\n" );
         memcpy( wee_data + 0x3B8, wee_data + 0x1B8, PARTTABLE_SIZE );
      }
      else
      {
         fprintf( stderr, "partition tables differ, using -f to force (keep first)\n" );
         goto quit;
      }
   }
   seek_dev( &dev, 0 );
   write_dev( &dev, wee_data + 0x200, 1 );
   close_dev( &dev );

   retval = 0;
quit:
   if( wee_data != (char*)wee63_mbr )
   {
      free( wee_data );
   }
   return retval;
}


/*!
 \brief write the content of the wee script to stdout

 \param filename name of wee containing script (0: internal)
 \return int 0=ok, else fail
*/
int show_script( char *filename )
{
   device_t dev = { 0, -1, 0 };
   int retval = 1;
   int   wee_size = sizeof(wee63_mbr);
   char *wee_data = (char*)wee63_mbr;
   int i = 0;

   if( filename )
   {
      wee_size = 0x10000;
      wee_data = (char*)malloc( wee_size );
      open_dev( &dev, filename );
      if(dev.xd)
      {
         for( i = 0; i < 0x80; ++i )
         {
            char *p = wee_data + (i * SECTOR_SIZE);
            seek_dev( &dev, i );
            read_dev( &dev, p, 1 );
         }
         close_dev( &dev );
      }
      else
      {
         close_dev( &dev );
         memset( wee_data, 0, wee_size );
         wee_size = read_file( filename, wee_data, wee_size );
      }
   }

   i = *(unsigned long *)(wee_data + 0x84) & 0xFFFF;
   if ((*(unsigned long *)(wee_data + 0x86) != WEE_SIGN) ||
       (*(unsigned long *)(wee_data +    i) != WEE_SIGN))
   {
      fprintf( stderr, "\"%s\" does not conain a wee mbr\n", filename );
      goto quit;
   }

   i += 0x10;
   puts( wee_data + i );
   fflush( stdout );

   retval = 0;
quit:
   if( wee_data != (char*)wee63_mbr )
   {
      free( wee_data );
   }
   return retval;
}


/*!
 \brief write out help message

*/
void help( const char *name )
{
   printf("weesetup v2.0\n"
          "\nUsage:\n %s [options]\n\nOptions:\n"
          " --help,\t-h\tshow this help\n"
          " --verbose,\t-v\tenable verbose mode\n"
          " --device=dd,\t-d dd\tuse device 'dd' for installation (mandatory)\n"
          " --wee=ww,\t-w ww\tuse extern wee 'ww'\n"
          " --script=ss,\t-s ss\tuse script file 'ss'\n"
          " --showscript,\t-S\twrite wee script to stdout\n"
          " --saveold=oo,\t-o oo\tsave read data to 'oo'\n"
          " --grldr=gg,\t-g gg\tuse wee to load grldr 'gg' installed\n\t\t\tin unparitioned space after wee\n"
          " --force,\t-f\tforce install\n"
          " --update,\t-u\tupdate\n"
          " --backupmbr,\t-b\tbackup mbr to second sector\n"
          " --restorembr,\t-r\trestore backup mbr from second sector\n"
          " --list,\t-l\tList all disks in system and exit\n"
          "\nReport bugs to website:\n https://code.google.com/p/grubutils/issues\n"
          "\nThanks:\n"
          " wee63.mbr (minigrub for mbr by tinybit)\n\t\thttp://bbs.znpc.net/viewthread.php?tid=5838\n"
          " wee63setup.c by SvOlli, xdio by bean\n", name );
}


/*!
 \brief main routine

 \param argc number of arguments
 \param argv[] list of arguments
 \return int 0=ok, else fail
*/
int main( int argc, char *argv[] )
{
   device_t dev = { 0, -1, 0 };

   int  retval             = 1;
   int  opt;
   int  i;
   int  fs;
   char *device_name       = 0; // name of the device to install to
   char *saveold_filename  = 0;

   int  data_size          = MAX_WEE_SIZE; // size of each of these buffers
   unsigned data_sectors  = 0;
   char *read_data         = 0; // buffer for data read from hd
   char *write_data        = 0; // buffer for data created for writing to hd
   char *reread_data       = 0; // buffer for verification of written data

   int  wee_size           = sizeof(wee63_mbr);
   char *wee_filename      = 0;

   int  grldr_size         = 0;
   int  grldr_offset       = 0;
   char *grldr_filename    = 0;

   char *script_filename   = 0;
   char *script_data       = 0;

   int  install_flag	   = 0;
   unsigned short verbose = 0;
   
   struct option long_options[] = {
   {"backupmbr",  0, 0, 'b'},
   {"device",     1, 0, 'd'},
   {"dump",       0, 0, 'D'},
   {"force",      0, 0, 'f'},
   {"grldr",      1, 0, 'g'},
   {"help",       0, 0, 'h'},
   {"list",       0, 0, 'l'},
   {"saveold",    1, 0, 'o'},
   {"restorembr", 0, 0, 'r'},
   {"script",     1, 0, 's'},
   {"showscript", 0, 0, 'S'},
   {"verbose",    0, 0, 'v'},
   {"wee",        1, 0, 'w'},
   };
   
   if (argc == 1)
   {
      help( argv[0] );
      return 0;
   }
   while ((opt = getopt_long(argc, argv, "bd:Dfg:hlo:rs:Suvw:",long_options,0)) != -1)
   {
      switch (opt)
      {
      case 'b':
         install_flag |= FLAG_BACKUPMBR;
         break;
      case 'd':
         device_name = optarg;
         break;
      case 'D':
         install_flag |= FLAG_DEBUG;
         break;
      case 'f':
         install_flag |= FLAG_FORCE;
         break;
      case 'g':
         grldr_filename = optarg;
         break;
      case 'h':
         help( argv[0] );
         return 0;
      case 'l':
         install_flag |= FLAG_LISTDEVS;
         break;
      case 'o':
         saveold_filename = optarg;
         break;
      case 'u':
         install_flag |= FLAG_UPDATE;
         break;
      case 'r':
         install_flag |= FLAG_RESTOREMBR;
         break;
      case 's':
         script_filename = optarg;
         break;
      case 'S':
         install_flag |= FLAG_SHOWSCRIPT;
         break;
      case 'v':
         verbose = 1;
         break;
      case 'w':
         wee_filename = optarg;
         break;
      default:
         help( argv[0] );
         return 1;
      }
   }
   
   /* step 1: sanity checks and setup */
   i = 0;
   if( install_flag & FLAG_SHOWSCRIPT ) i++;
   if( install_flag & FLAG_LISTDEVS   ) i++;
   if( install_flag & FLAG_RESTOREMBR ) i++;
   if( i > 1 )
   {
      fprintf( stderr, "show script, list devs and restore mbr are mutually exclusive.\n" );
      return 1;
   }

    if (!device_name)
    {
	if (argv[argc-1][0] == '(')
	    device_name = argv[argc-1];
    }
   if( install_flag & FLAG_SHOWSCRIPT )
   {
      if( device_name && wee_filename )
      {
         fprintf( stderr, "you can't use wee and device at the same time for showing script\n" );
         return 1;
      }
      if( device_name )
      {
         return show_script( device_name );
      }
      if( wee_filename )
      {
         return show_script( wee_filename );
      }
      return show_script( 0 );
   }

   if( install_flag & FLAG_LISTDEVS )
   {
      list_devs();
      return 0;
   }

   if( script_filename && grldr_filename )
   {
      fprintf( stderr, "you can't use script and grldr at the same time\n" );
      return 1;
   }

   if( wee_filename )
   {
      wee_size = read_file( wee_filename, 0, 0 );
   }

   if( grldr_filename )
   {
      grldr_size   = read_file( grldr_filename, 0, 0 );
      grldr_offset = round2blocks( wee_size );
      data_size += round2blocks( grldr_size );
   }

   data_sectors = bytes2blocks( data_size );
   read_data   = (char*)malloc( data_size );
   write_data  = (char*)malloc( data_size );
   reread_data = (char*)malloc( data_size );
   if( !read_data || !write_data || !reread_data )
   {
      fprintf( stderr,"error allocating memory!" );
      exit(1);
   }
   memset( read_data,   0, data_size );
   memset( write_data,  0, data_size );
   memset( reread_data, 0, data_size );
    if(verbose)
   {
      printf( "need %d bytes (%d sectors) for installation\n",
              data_size, data_sectors );
   }

   /* step 2: read replacement data */
   if( wee_filename )
   {
      if(verbose)
      {
         printf( "reading wee from \"%s\" (%d bytes)\n", wee_filename, wee_size );
      }
      if( read_file( wee_filename, write_data, data_size ) != wee_size )
      {
         fprintf( stderr, "error reading wee mbr \"%s\"\n", wee_filename );
         goto quit;
      }
   }
   else
   {
      if(verbose)
      {
         printf( "using internal wee (%d bytes)\n", wee_size );
      }
      memcpy( write_data, wee63_mbr, wee_size );
   }

   i = *(unsigned long *)(write_data + 0x84) & 0xFFFF;
   if ((*(unsigned long *)(write_data + 0x86) != WEE_SIGN) ||
       (*(unsigned long *)(write_data +    i) != WEE_SIGN))
   {
      fprintf( stderr, "\"%s\" does not conain a wee mbr\n", wee_filename );
      goto quit;
   }

   i += 0x10;	//wee63 script offset;
   script_data = write_data + i;

   if( script_filename )
   {
      int max_script_size = (MAX_WEE_SIZE - i);
      int script_size = read_file( script_filename, 0, 0 );
      if( script_size >= max_script_size )
      {
         fprintf( stderr, "script \"%s\" does not fit in memory: size=%d, available=%d",
                  script_filename, script_size, max_script_size );
         goto quit;
      }
      if(verbose)
      {
         printf( "reading script from \"%s\" (%d bytes)\n", script_filename, script_size );
      }
      read_file( script_filename, script_data, script_size );
      //terminate 
      memset(script_data + script_size,0,MAX_WEE_SIZE - i - script_size);
   }

   if( grldr_filename )
   {
      if(verbose)
      {
         printf( "reading grldr from \"%s\" (%d bytes)\n", grldr_filename, grldr_size );
      }

      if( read_file( grldr_filename, write_data + grldr_offset, grldr_size ) != grldr_size )
      {
         fprintf( stderr, "error reading grldr \"%s\"\n", grldr_filename );
         goto quit;
      }
      sprintf( script_data, "(hd0)%d+%d",
               bytes2blocks( grldr_offset ), bytes2blocks( grldr_size ) );
       if(verbose)
      {
         printf( "wee script for loading grldr: \"%s\"\n", script_data );
      }
   }
   
   if( !device_name )
   {
	if( saveold_filename )
	{
	    if(verbose)
	    {
		printf( "saving data to \"%s\"\n", saveold_filename );
	    }

	    write_file( saveold_filename, write_data, MAX_WEE_SIZE );
	    goto quit;
	}

      fprintf( stderr, "you must specify a device name using the --device= option\n" );
      return 1;
   }

   if( install_flag & FLAG_RESTOREMBR )
   {
      return restore_mbr( device_name, install_flag );
   }

   /* step 3: read data from device */
    if(verbose)
   {
      printf( "reading current data from \"%s\"", device_name );
   }
   open_dev( &dev, device_name );
   for( i = 0; i < data_sectors; ++i )
   {
      char *p = read_data + (i<<SECTOR_BIT);
      seek_dev( &dev, i );
      read_dev( &dev, p, 1 );
       if(verbose)
      {
         putchar( '.' );
         fflush( stdout );
      }
   }
   close_dev( &dev );
    if(verbose)
   {
      putchar( '\n' );
      fflush( stdout );
   }

   /* step 4: analyse read data and integrate into new data */
   if (*(unsigned long *)(read_data + 0x86) == WEE_SIGN && !(install_flag & FLAG_UPDATE))
   {
      fprintf( stderr, "wee already installed, use -u option!\n" );
      goto quit;
   }

   fs = get_fstype( (unsigned char*)read_data );

   if( fs == FST_MBR_BAD )
   {
      if( install_flag & FLAG_FORCE )
      {
         fs = FST_MBR;
      }
      else
      {
         fprintf( stderr, "bad partition table, use -f option to force install.\n" );
         goto quit;
      }
   }

   if( fs != FST_MBR || check_mbr_data(read_data, bytes2blocks(data_size)) )
   {
      goto quit;
   }

   memcpy(write_data + 0x1B8 , read_data + 0x1B8, PARTTABLE_SIZE);

   if( install_flag & FLAG_BACKUPMBR )
   {
       if(verbose)
      {
         printf( "copying original boot sector to second sector\n" );
      }
      memcpy( write_data + 0x200, read_data, 0x200 );
   }
   else
   {
      memcpy(write_data + 0x3B8, read_data + 0x1B8, PARTTABLE_SIZE);
   }

   if( memcmp(write_data + 0x1B8, read_data + 0x1B8, PARTTABLE_SIZE) )
   {
      fprintf(stderr, "error: Partition table\n" );
      goto quit;
   }

   /* step 5: save read data */
   if( saveold_filename )
   {
       if(verbose)
      {
         printf( "saving read data to \"%s\"\n", saveold_filename );
      }

      write_file( saveold_filename, read_data, data_size );
   }

    write_file( "backup.mbr",   read_data,   data_size );
   /* step 6: write new data and reread */
    if(verbose)
   {
      printf( "writing new data to \"%s\"", device_name );
   }
   open_dev( &dev, device_name );
   char *p;
   int j = 0;
   for(p =write_data,i = 0; i < data_sectors; ++i )
   {
      if (memcmp(p,read_data + (i<<SECTOR_BIT),SECTOR_SIZE))
      {
	seek_dev( &dev, i );
	write_dev( &dev, p, 1 );
	if(verbose)
	{
	    putchar( '.' );
	    fflush( stdout );
	}
	++j;
      }
      p += SECTOR_SIZE;
   }
    if(verbose)
   {
      printf("\n%d sectors changed.\n",j);
      fflush( stdout );
   }

    if(verbose)
   {
      printf( "rereading written data for verification from \"%s\"", device_name );
   }
   for( i = 0; i < data_sectors; ++i )
   {
      char *p = reread_data + (i<<SECTOR_BIT);
      seek_dev( &dev, i );
      read_dev( &dev, p, 1 );
       if(verbose)
      {
         putchar( '.' );
         fflush( stdout );
      }
   }
   close_dev( &dev );
    if(verbose)
   {
      putchar( '\n' );
      fflush( stdout );
   }

   if( install_flag & FLAG_DEBUG )
   {
       if(verbose)
      {
         printf( "creating dump files for debug purposes\n" );
      }
      write_file( "dump.read_data",   read_data,   data_size );
      write_file( "dump.write_data",  write_data,  data_size );
      write_file( "dump.reread_data", reread_data, data_size );
   }

   /* step 7: compare read to written data and rewind if error */
   if( memcmp( reread_data, write_data, data_size ) )
   {
      fprintf( stderr, "writing new data failed\ntrying to rewrite original data... " );
      fflush( stderr );
      open_dev( &dev, device_name );
      for( i = 0; i < data_sectors; ++i )
      {
         char *p = read_data + (i<<SECTOR_BIT);
         seek_dev( &dev, i );
         write_dev( &dev, p, 1 );
      }

      for( i = 0; i < data_sectors; ++i )
      {
         char *p = reread_data + (i<<SECTOR_BIT);
         seek_dev( &dev, i );
         read_dev( &dev, p, 1 );
      }
      close_dev( &dev );

      if( memcmp( reread_data, read_data, data_size ) )
      {
         fprintf( stderr, "fail!\n" );
      }
      else
      {
         fprintf( stderr, "success.\n" );
      }
   }

   retval = 0;

   /* step 8: cleanup */
quit:
   if( read_data )
   {
      free( read_data );
   }
   if( write_data )
   {
      free( write_data );
   }
   if( reread_data )
   {
      free( reread_data );
   }

   return retval;
}
