/* code to receive timestamp data from the new test board via a serial
   interface.

   reasonably stable code with hardware svn-16    17.12.2016chk

   usage:
   readevents4 [-U devicefile ]
               [-g gatetime]
	       [-a outmode]
		   [-s outfile]
	       [-A ]
	       [-T value]
	       [-Y] [-X] [-v]

   options:

   -U devicename      device name of the serial interface; default is 
                      /dev/ttyACM0

   -g gatetime:       sets the gate time in milliseconds; if gatetime==0, the
                      gate never stops. Default is gatetime = 1000msec, the 
		      maximum is 65535.

   -a outmode :       Defines output mode. Defaults currently to 0. Currently
                      implemented output modes are:
		      0: raw event code patterns are delivered as 32 bit
		         hexadecimal patterns (i.e., 8 characters) 
		         separated by newlines. Exact structure of
			 such an event: see card description.
		      1: event patterns with a consolidated timing
		         are given out as 64 bit entities. Format:
			 most significant 45 bits contain timing info
			 in multiples of 2 nsec, least significant
			 4 bits contain the detector pattern.

		      2: output consolidated timing info as 64 bit patterns
		         as in option 1, but as hex text.
		      3: hexdump- like pattern of the input data for debugging
                      4: like 0, but in binary mode. Basically saves raw data
		         from timestamp device.
   -s outfile		output filename
   -A               sets absolute time mode 
   -T value         sets TTL levels for value !=0, or NIM for value==0 (default)
   -Y               show dummy events. Dummy events allow a timing extension
                    and are emitted by the hardware about every 140 ms if there
		    are no external events.
   -X               exchange 32bit words in timing info for compatibility with
                    old readevents code in output mode 1 and 2...
   -v               increases verbosity of return text

   Signals:
   SIGTERM:   terminates the acquisition process in a graceful way (disabling
              gate, and inhibiting data acquisition.
   SIGPIPE:   terminates the accquisition gracefully without further
              messages on stderr.


*/

#include <termios.h>
#include <unistd.h>
#include <stdio.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/time.h>
#include <sys/types.h>
#include <string.h>
#include <signal.h>

#include <unistd.h>
#include <inttypes.h>


#define DEFAULT_GATETIME 0 /* ACQUISITION TIME */
#define NOEVENT_TIMEOUT 200 /* timeout for no events in milliseconds */
#define READ_TIMEOUT 5 /* timeout for a single read in ms */
#define DEFAULTDEVICENAME "/dev/ttyACM0"

#define LOC_BUFSIZ (1<<18)      /* buffer size in 32bit words */
#define FNAMELENGTH 200      /* length of file name buffers */
#define FNAMFORMAT "%200s"   /* for sscanf of filenames */
#define CMDLENGTH 500        /* command */

#define DEFAULT_OUTMODE 1


/* some global variables */
int outmode=DEFAULT_OUTMODE;


/* error handling */
char *errormessage[] = {
    "No error.",
    "Missing command string", /* 1 */
    "Error opening device file",
    "cannot parse baud rate",
    "illegal baud rate", 
    "error in select call", /* 5 */
    "timeout while waiting for response",
    "illegal gatetime value",
    "specified outmode out of range",
    "Error reading time",
    "Status read returned unexpected text", /* 10 */
    "Device FIFO overflow",
    "Error or timeout in status read attempt",
};
int emsg(int code) {
    fprintf(stderr,"%s\n",errormessage[code]);
    return code;
};

/* timeout stuff */
struct timeval timeout; /* default timeout  */

/* signal handlers */

/* handler for termination */
volatile int terminateflag;
void termsig_handler(int sig) {
    switch (sig) {
	case SIGTERM: case SIGKILL:
	    //fprintf(stderr,"got hit by a term signal!\n");
	    terminateflag=1;
	    break;
	case SIGPIPE: 
	    /* stop acquisition */
	    //fprintf(stderr,"readevents:got sigpipe\n");
	    terminateflag=1;
    }
}

int ll_to_bin(unsigned long long n)
{
  int c, k;
  printf("%lld in binary is:\n", n);
  for (c = 63; c >= 0; c--)
  {
    k = n >> c; 
    if (k & 1) printf("1");
    else printf("0");
  }
  printf("\n");
  return 0;
}


int main(int argc, char *argv[]){

    int opt; /* option parsing */
    int fh;  /* device file handle */
    int cnt=0;    
	struct termios config;
    fd_set fds;   /* file descriptors for select call */
    int maxhandle;
    uint32_t lbuf[LOC_BUFSIZ]; /* input buffer in 32bit */
    uint64_t targetbuf[LOC_BUFSIZ]; /* target buffer */
    char * buf2;  /* input buffer in char **/
    int i, retval;
    char command[CMDLENGTH];  /* command line to be read in */
    char devicefilename[FNAMELENGTH] = DEFAULTDEVICENAME;
	char outfilename[FNAMELENGTH];
    int j,k;
    char asciistring[18]; /* keeps empty string */
    char c;
    int hangover=0; /* initial offset */
    uint32_t tsraw, tsold;
    uint64_t loffset, tl, tl2;
    int timemode=0; /* absolute time mode (default: no) */
    int gatetime = DEFAULT_GATETIME; /* integration time */
    int threshold = 0; /* 0: nim, other value: ttl */
    int emptycalls = 0; /* number of empty read calls */
    int showdummies=0; /* on if dummy events are to be shown on mode -a 1,2 */
    struct timeval timerequest_pointer; /*  structure to hold time request  */
    int swapoption =0; /* for compatibility with old code */
    int verbosity=0; /* for debug texts */

    buf2= (char *)lbuf; /* pointer recast */

    /* parse option(s) */
    opterr=0; /* be quiet when there are no options */
    while ((opt=getopt(argc, argv, "U:g:a:AT:YXv")) != EOF) {
	switch (opt) {
	case 'U': /* parse device file */
	    if (1!=sscanf(optarg,FNAMFORMAT,devicefilename)) 
		return -emsg(2);
	    break;
	    
	case 'g': /* set the gate time */
	    if (1!=sscanf(optarg,"%i",&gatetime)) return -emsg(2);
	    if ((gatetime<0) || (gatetime>0xffff)) return -emsg(7);
	    break;
	case 'a': /* set output mode */
	    sscanf(optarg,"%d",&outmode);
	    if ((outmode<0)||(outmode>4)) return -emsg(8);
	    break;
	case 's': /* set output file name */
	    if (1!=sscanf(optarg,FNAMFORMAT,outfilename)) 
		return -emsg(2);
	    break;

	case 'A': /* absolute time mode */
	    timemode=1;
	    break;
	case 'T': /* set threshold to TTL/NIM */
	    sscanf(optarg,"%d",&threshold);
	    break;
	case 'Y': /* sow dummy events */
	    showdummies=1;
	    break;
	case 'X': /* swapoption */
	    swapoption=1;
	    break;
	case 'v': /* verbosity increase */
	    verbosity++;
	    break;
	}
    }

    
    /* prepare setup command */
    sprintf(command,"*RST;%s;time %i;timestamp;counts?\r\n",
	    threshold?"TTL":"NIM",
	    gatetime);
    
    /* open device */
    fh=open(devicefilename,O_NOCTTY|O_RDWR);
    if (fh==-1) return -emsg(2);

	// printf("fh: %d\n", fh);
    
    maxhandle=fh+1;  /* for select call */
    
    //setbuf(stdin,NULL);
    
    /* set terminal parameters */
    cfmakeraw(&config);
    config.c_iflag = config.c_iflag | IGNBRK;
    config.c_oflag = (config.c_oflag );
    /* mental note to whenever this is read: disabling the CRTSCTS flag
       made the random storage of the send data go away, and transmit
       data after a write immediately.... */
    config.c_cflag = (config.c_cflag & ~CRTSCTS)|CLOCAL|CREAD;

    //config.c_cc[VTIME] =0; config.c_cc[VMIN]=0;
         
    tcsetattr(fh,TCSANOW, &config);
    
    tcflush(fh,TCIOFLUSH); /* empty buffers */

    /* output printing for outmode 3*/
    j=0; strcpy(asciistring,"................");


    /* send command */
    write(fh,command,strlen(command));
    /* prepare buffer pointer for overflow */
    hangover=0; loffset=0LL; tsold=0;

    /* take absolute time if option is set */
    if (timemode) {
	if (gettimeofday(&timerequest_pointer, NULL)) {
	    return -emsg(9);
	}
	loffset = timerequest_pointer.tv_sec; loffset *= 1000000;
	loffset += timerequest_pointer.tv_usec;
	loffset = (loffset*500) << 19;
    }

    /* install termination signal handlers */
    terminateflag=0;
    signal(SIGTERM, &termsig_handler);
    signal(SIGKILL, &termsig_handler);
    signal(SIGPIPE, &termsig_handler);

    /* prepare inpkt flush command */
    strcpy(command,"INPKT\r\n");

    for(;;) {
	/* set timeout */

	timeout.tv_sec=READ_TIMEOUT / 1000;
	timeout.tv_usec=(READ_TIMEOUT % 1000)*1000;
	FD_ZERO(&fds);
	FD_SET(fh,&fds); /* watch for stuff from serial */
	retval = select(maxhandle, &fds, NULL, NULL, &timeout);
	if (terminateflag) break;
	if (retval<0) return -emsg(5);
	if (retval==0) { /* we got a timeout */
	    emptycalls++;
		// printf("timeout %d\n", emptycalls);

	    // if (emptycalls > (NOEVENT_TIMEOUT/READ_TIMEOUT)) {
		// 	printf("timeout with no events\n");
		// 	break; /* timeout with no events */
	    // }
	    /* force a flush of the FX2 FIFO with an inpkt command */
	    write(fh,command,strlen(command));
	    continue; /* wait for stuff to arrive */
	}
	emptycalls=0; /* reset timeout counter if we got something */ 
	
	/* serial->stdout handler */
	if (FD_ISSET(fh,&fds)) {

	    cnt=read(fh, &buf2[hangover], sizeof(lbuf)-hangover);
	    if (cnt<0) break; /* we got an error */
	    if (cnt==0) { 
		continue; /* can this happen ? */
	    } else {
		cnt=cnt+hangover;
		switch(outmode) {
		case 0: /* output of raw data in hex mode */


		    for (j=0; j<cnt/4; j++) {

			fprintf(stdout,"%08x\n",lbuf[j]);
		    }
		    hangover=cnt%4; 
		    if (hangover) lbuf[0]=lbuf[j]; /* safe hangover correct */
		    
		    break;
		case 1: /* consolidated output in binary mode - fast code */
		    k=0;
		    for (j=0; j<cnt/4; j++) {
			tsraw=lbuf[j];
			if ((tsold ^ tsraw) & 0x80000000) {
			    if ((tsraw & 0x80000000)==0) {
				loffset += 0x400000000000LL;
			    }
			}
			tsold=tsraw;
			if (tsraw &0x10) {
			    if (!showdummies) continue; /* don't log dummies */
			}
			tl = loffset 
			    + ((uint64_t)(tsraw &0xffffffe0)<<14) 
			    + (tsraw & 0x1f);
			if (swapoption) {
			    tl2=tl<<32L;
			    tl = (tl>>32L) | tl2;
			}
			targetbuf[k++]=tl;
		    }
		    hangover=cnt%4; 
		    if (hangover) lbuf[0]=lbuf[j]; /* safe hangover correct */
			FILE *fp = fopen(outfilename, "wb");
		    fwrite(targetbuf, sizeof(int64_t),k,fp);
			ll_to_bin(targetbuf[k]);

		    break;

		case 2: /* consolidated output in hex mode */
		    for (j=0; j<cnt/4; j++) {
			tsraw=lbuf[j];
			if ((tsold ^ tsraw) & 0x80000000) {
			    if ((tsraw & 0x80000000)==0) {
				loffset += 0x400000000000LL;
			    }
			}
			tsold=tsraw;
			if (tsraw &0x10) {
			    if (!showdummies) continue; /* don't log dummies */
			}
			tl = loffset 
			    + ((uint64_t)(tsraw &0xffffffe0)<<14) 
			    + (tsraw & 0x1f);
			if (swapoption) {
			    tl2=tl<<32L;
			    tl = (tl>>32L) | tl2;
			}
			fprintf(stdout,"%016llx\n",(unsigned long long)tl);
		    }
		    hangover=cnt%4; 
		    if (hangover) lbuf[0]=lbuf[j]; /* safe hangover correct */
		    
		    break;

		case 3:
		    /* write stuff as hexdump */
		    for (i=0; i<cnt; i++) {
			c = buf2[i];
			fprintf(stdout, "%02x ", c&0xff);
			if (j==7) fprintf(stdout, " ");
			asciistring[j++] = ((c>31) && (c<127))?c:'.';
			if (j==16) {
			    fprintf(stdout, " %s\n",asciistring);
			    j=0;
			}
		    }
		    break;
		case 4: /* output raw input data into a file */
		    fwrite(lbuf,sizeof(uint32_t), cnt/4, stdout);
		    hangover=cnt%4; 
		    if (hangover) lbuf[0]=lbuf[j]; /* safe hangover correct */
		    break;
		}	
	    }
	}	
    }
    
    /* honor terminate flag */
    if (terminateflag) {
	strcpy(command, "*rst\r\n"); 
	write(fh,command,strlen(command));
	//printf("terminated..\n");
    }
    if (verbosity>0) {
      fprintf(stderr,"termflag: %d, cnt: %d, empty: %d\n",
	      terminateflag, cnt, emptycalls);
    }
    
    /* benignly end the timestamp device and return status */
    strcpy(command, "abort;STATUS?\r\n"); write(fh,command,strlen(command));
    for(;;) {
	/* set timeout */
	timeout.tv_sec=READ_TIMEOUT / 1000;
	timeout.tv_usec=(READ_TIMEOUT % 1000)*1000;
	FD_ZERO(&fds);
	FD_SET(fh,&fds); /* watch for stuff from serial */
	retval = select(maxhandle, &fds, NULL, NULL, &timeout);
	if (terminateflag) break;
	if (retval<0) return -emsg(12);
	if (retval==0) break; /* we got a timeout */
	if (FD_ISSET(fh,&fds)) {
	  cnt=read(fh, buf2, sizeof(lbuf));
	  // some consistency check if the answer is ok
	  if ((cnt==7)&&(buf2[4]==' ')&&(buf2[5]=='\r')&&(buf2[6]=='\n')) {
	      j=strtol(buf2, NULL, 0); // contains status word
	    if (verbosity>0) fprintf(stderr,"Status word: 0x%04x\n",j);
	    if (j&0x10) return -emsg(11); // overflow detected
	  } else {
	    if (verbosity>0) 
	      fprintf(stderr,"unspecified return, character cnt: %d\n",cnt);
	    return -emsg(10);

	  }
	}
    }


    /* close remaining buffer if needed */
    switch (outmode) {
    case 0: case 1: case 2:/* most cases */
	break;
    case 3:
	/* print residual text */
	if (j>0) {
	    do {
		fprintf(stdout, "   ");if (j==7) fprintf(stdout, " ");
		asciistring[j++] = ' ';
	    } while (j<16);
	    fprintf(stdout, " %s\n",asciistring);
	}	
	fprintf(stdout,"\n");
	break;
    }
    close(fh);
    return 0;
}
