/***************************************************************************/
/*WDBPLTC.C-reads and plots compressed coastlines                          */
/*
       Retrieves the WDB II or WVS data from the direct access files.



	Authors :

	  Jan C. Depner                       James A. Hammack
	  U.S. Naval Oceanographic Office     NORDA
	  Information Systems Department      Code 117
	  Stennis Space Center, MS            Stennis Space Center, MS
	  39522-5001                          39529-5004

	C version : Jan C. Depner
	Date : 02-24-89

	Modifications for GCL : Jerry Landrum
	Date : 10 Dec 89

	NOTE : It should be intuitively obvious to the most casual observer
	       that I ain't no C programmer.  This is my first effort and,
	       hopefully, these routines will be good enough to get the job
	       done.  (JCD)

	Global variable definitions :

	PHYSIZ    - size of a physical record in the direct access file,
		    in bytes.
	g_bytbuf  - char array of 'PHYSIZ' length for i/o to the direct
		    access file.
	g_celbuf  - char array of 'PHYSIZ' length for i/o to the
		    direct access file (used to retrieve cell map).
	g_lunfil  - file handle for the direct access file.
	g_logrec  - length of a logical record in bytes.
	g_lperp   - logical records per physical record.
	g_offset  - number of logical records needed to store the bit
		    cell map + 1 .
	g_level   - wdbplt software and data file version.
	g_proj    - projection (6 chars + \000).
	g_stflag  - initialization flag for 'projct' and 'curve'.
	g_index   - cell index number for algorithm addressing, g_index =
		    integer (lat degrees + 90) * 360 + integer lon
		    degrees + 180 + 1 ; this is the logical record
		    address of the first data segment in any cell.
	g_addr    - current physical record address in bytes from beginning
		    of the file.
	g_paddr   - previous physical record address.
	g_lstat   - seek and read status variable.
	g_curpos  - current byte position within the physical record.
	g_fulrec  - full record value (g_logrec-4).
	g_slatdd  - latitude of southern boundary of area (degrees).
	g_nlatdd  - latitude of northern boundary of area (degrees).
	g_wlondd  - longitude of western boundary of area (degrees).
	g_elondd  - longitude of eastern boundary of area (degrees).
	g_scal    - scale factor.
	g_width   - width of area.
	g_height  - height of area.
	latray[512] - latitudes in degrees for one segment.
	lonray[512] - longitudes in degrees for one segment.

	Local variable definitions :

	ioff      - divisor for delta records (1 - WDB, 10 - WVS).
	i         - utility variable.
	j         - utility variable.
	col       - longitude index.
	go        - beginning value for longitude loop.
	stop      - ending value for longitude loop.
	inc       - increment for longitude loop.
	slat      - integer value of southern latitude of area (degrees).
	nlat      - integer value of northern latitude of area (degrees).
	wlon      - integer value of western longitude of area (degrees).
	elon      - integer value of eastern longitude of area (degrees).
	segcnt    - number of data points in the current segment.
	rank      - type of data of the current segment.
	cont      - segment continuation flag.
	cnt       - segment loop counter.
	gap       - every 'gap'th point will be plotted; largest allowed
		    value for gap is 45.
	latsec    - delta latitude in seconds or tenths of seconds.
	lonsec    - delta longitude in seconds or tenths of seconds.
	latoff    - latitude offset (seconds or tenths of seconds).
	lonoff    - longitude offset (seconds or tenths of seconds).
	conbyt    - byte position of continuation pointer within physical
		    record.
	eflag     - end of segment flag.
	first     - initialization flag.
	filenm    - full filename of data file.
	todeg     - conversion factor from deltas to degrees.
	lat       - latitude of current point (degrees +90.0).
	lon       - longitude of current point (degrees).
	dummy     - dummy pointer for modf function.

	Arguments:

	file   - 7 characters, 6 character file identifier & \000.
	slatd  - latitude degrees of southern boundary of area.
	slatm  - latitude minutes of southern boundary of area.
	nlatd  - latitude degrees of northern boundary of area.
	nlatm  - latitude minutes of northern boundary of area.
	wlond  - longitude degrees of western boundary of area.
	wlonm  - longitude minutes of western boundary of area.
	elond  - longitude degrees of eastern boundary of area.
	elonm  - longitude minutes of eastern boundary of area.
	scale  - scale factor in inches/degree.
	prjctn - 7 characters, 6 character projection id & \000.
	ranks  - char array containing color numbers for each rank.
	gapin  - every 'gapin'th point will be plotted; largest allowed
		 value for gapin is 45.

*****************************************************************************
*/
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <stdio.h>
#include <errno.h>
#include <math.h>

#define PHYSIZ 3072

#define SIGN_OF(x) ((x)<0.0 ? -1 : 1)

float *ufmatrix(); /* allocates memory for float matrix */
unsigned char *ucmatrix(); /* allocates memory for character matrix */
void wdbpltc_free(); /* frees all buffers */
#ifdef DEBUG
int debug=1;
#else
int debug=0;
#endif
unsigned char *g_bytbuf,*g_celbuf;
short g_lunfil,g_logrec,g_lperp,g_offset,g_level;
char g_proj[6+1], g_stflag;
long g_index,g_addr,g_paddr,g_lstat,g_curpos,g_fulrec;
float g_slatdd,g_nlatdd,g_wlondd,g_elondd,g_scal,g_height,g_width;
float *latray,*lonray;

void wdbplt(file, slatd, slatm, nlatd, nlatm, wlond, wlonm, elond, elonm,
	  scale, ranks, gapin)
char file[], ranks[], gapin;
float slatd, slatm, nlatd, nlatm, wlond, wlonm, elond, elonm, scale;
{
  unsigned char celchk();
  void nxtrec(), movpos(), pltpro();

  short ioff, i, j, col, go, stop, inc, slat, nlat, wlon, elon, segcnt,
    rank, cont, cnt, gap, latsec, lonsec;
    long latoff, lonoff, conbyt;
  char eflag, first, *dirfil;
  float todeg, lat=0., lon=0.;
  double dummy;
  int ii,jj;
  char filspc[40];

  if(debug)
  {
    printf("WDBPLT file %s, s %f n %f w %f e %f\n",
      file,slatd,nlatd,wlond,elond);
    printf("gapin %hd %c\n",gapin,gapin);
    for(ii=0;ii<7;ii++)
    {
      jj=ranks[ii];
      printf(" %d ",jj);
    }
    printf("\n");
  }
/* get memory for working arrays */
  lonray=ufmatrix(1,512);
  latray=ufmatrix(1,512);
  g_bytbuf = ucmatrix(1,PHYSIZ);
  if((g_celbuf=ucmatrix(1,PHYSIZ))==NULL)
  {
    gstat("Error allocating memory in WDBPLTC");
    return;
  }
/*Initialize variables, open file and read first record*/
  first = 1;
  eflag = 0;
  g_stflag = 1;
  g_scal=scale;
  g_lunfil = open(file,O_RDONLY);
  if (g_lunfil==-1)
  {
    gstat("Coastline file not found");
    wdbpltc_free();
    return;
  }
  g_lstat = lseek(g_lunfil,0L,0);
  g_lstat = read(g_lunfil,g_bytbuf,PHYSIZ);
  g_logrec = g_bytbuf[3];
  g_fulrec = g_logrec - 4;
  g_level = g_bytbuf[4];
  ioff = g_bytbuf[5];
  todeg = 3600.0 * ioff;
  g_offset = 64799 / (g_logrec*8) + 2;
  g_lperp = PHYSIZ / g_logrec;
/*Adjust gapin if neccessary*/
  gap = gapin;
  if (gap>45)
  {
    printf("Plot gap is too large!    ");
    printf("It has been reset to 45.\n");
    gap = 45;
  }
  if (gap<=0)
  {
    printf("Plot gap is 0 or negative!    ");
    printf("It has been reset to 1.\n");
    gap = 1;
  }
/*Compute latitude and longitude in degrees and adjust for 180 crossing*/
  g_slatdd = slatd+(slatm/60.0)*SIGN_OF(slatd);
  g_nlatdd = nlatd+(nlatm/60.0)*SIGN_OF(nlatd);
  g_wlondd = wlond+(wlonm/60.0)*SIGN_OF(wlond);
  g_elondd = elond+(elonm/60.0)*SIGN_OF(elond);
  if (g_elondd<=g_wlondd) g_elondd = g_elondd + 360.0;
/*Compute start and end integer values for retrieval loop and adjust
if neccessary*/
  slat = g_slatdd + 90.0;
  nlat = g_nlatdd + 90.0;
  wlon = g_wlondd;
  elon = g_elondd;

  if (modf((double)g_nlatdd,&dummy)==0.0) nlat--;
  if (modf((double)g_elondd,&dummy)==0.0) elon--;
  if (g_wlondd<0.0 && modf((double)g_wlondd,&dummy)!=0.0) wlon--;
  if (g_elondd<0.0 && modf((double)g_elondd,&dummy)!=0.0) elon--;
/*Latitude loop*/
  for (i=slat; i<=nlat; i++)
  {
/*Change direction of retrieval to minimize movement on mechanical
plotters*/
    if (i%2==0)
    {
      go = wlon;
      stop = elon;
      inc = 1;
    }
    else
    {
      go = elon;
      stop = wlon;
      inc = -1;
    }
/*Longitude loop*/
    for (j=go; j*inc<=stop*inc; j+=inc)
    {
/*Use latitude and longitude loop counters to compute index into
direct access data base */
      col = j%360;
      if (col<-180) col = col + 360;
      if (col>=180) col = col - 360;
      col = col +181;
      g_index = i*360L + col + g_offset;
/*Check cell map to see if data is available in 'g_index' cell*/
      if (celchk(&first))
      {
/*Compute physical record address, read record and save as previous address*/
	eflag = 0;
	g_addr = ((g_index-1)/g_lperp)*PHYSIZ;
	if (g_addr!=g_paddr)
	{
	  g_lstat = lseek(g_lunfil,g_addr,SEEK_SET);
	  g_lstat = read(g_lunfil,g_bytbuf,PHYSIZ);
	}
	g_paddr = g_addr;
/*Compute byte position within physical record*/
	g_curpos = ((g_index-1)%g_lperp)*g_logrec;
/*If not at end of segment, process the record*/
	while (!eflag)
	{
/*Get first two bytes of header and break out count and
continuation bit.*/
	  segcnt = (g_bytbuf[g_curpos]%128)*4 + g_bytbuf[g_curpos+1]/64 + 1;
	  cont = g_bytbuf[g_curpos]/128;
/*If this is a continuation record get offsets from the second byte.*/
	  if (cont)
	  {
	    latoff = ((g_bytbuf[g_curpos+1]%64)/8)*65536L;
	    lonoff = (g_bytbuf[g_curpos+1]%8)*65536L;
	  }
/*If this is an initial record set the offsets to zero and
get the rank from the second byte.*/
	  else
	  {
	    latoff = 0;
	    lonoff = 0;
	    rank = g_bytbuf[g_curpos+1]%64;
	  }
/*Update the current byte position and get a new record if neccessary.*/
	  movpos();
/*Compute the rest of the latitude offset.*/
	  latoff += g_bytbuf[g_curpos]*256L + g_bytbuf[g_curpos+1];
	  movpos();
/*Compute the rest of the longitude offset.*/
	  lonoff += g_bytbuf[g_curpos]*256L + g_bytbuf[g_curpos+1];
/*If this is a continuation record, bias the lat and lon offsets
and compute the position.*/
	  if (cont)
	  {
	    latoff -= 262144;
	    lonoff -= 262144;
	    lat += (float)latoff/todeg;
	    lon += (float)lonoff/todeg;
	  }
/*Else, compute the position.*/
	  else
	  {
	    lat = (float)i + (float)latoff/todeg;
	    lon = (float)j + (float)lonoff/todeg;
	  }
/*Update the current byte position.*/
	  g_curpos += 2;
/*Get the continuation pointer.*/
	  conbyt = ((g_index-1)%g_lperp)*g_logrec+g_fulrec;
/*If there is no continuation pointer or the byte position
is not at the position pointed to by the continuation pointer,
process the segment data.*/
	  if (g_bytbuf[conbyt]==0 || (g_curpos+1)%g_logrec<=
	    g_bytbuf[conbyt])
	  {
/*If at the end of the logical record, get the next record in the chain.*/
	    if (g_curpos%g_logrec==g_fulrec && g_bytbuf[conbyt]==0)
	      nxtrec();
/*If the rank is to be plotted call the plot routine.*/
	    if (ranks[rank]) pltpro(lat,lon,rank,&cont,ranks[rank],gap);
/*If the end of the segment has been reached, set the endflag */
	    if((g_curpos+1)%g_logrec==g_bytbuf[conbyt]) eflag=1;
/*Process the segment.*/
	    for (cnt=2; cnt<=segcnt; cnt++)
	    {
/*Compute the position from the delta record.*/
	      latsec = g_bytbuf[g_curpos] - 128;
	      lat += (float)latsec/todeg;
	      lonsec = g_bytbuf[g_curpos+1] - 128;
	      lon += (float)lonsec/todeg;
/*Call the plotting routine.*/
	      if (ranks[rank]) pltpro(lat,lon,rank,&cont,ranks[rank],gap);
	      g_curpos += 2;
	      conbyt = ((g_index-1)%g_lperp)*g_logrec+g_fulrec;
/*If the end of the segment has been reached, set the end flag
and break out of for loop.*/
	      if ((g_curpos+1)%g_logrec==g_bytbuf[conbyt])
	      {
		eflag = 1;
		break;
	      }
	      else if (g_curpos%g_logrec==g_fulrec) nxtrec();
	    }
	  }
/*break out of while loop if at the end of the segment.*/
	  else break;
	} /* end while */
      } /* end if */
/* test for operator abort */
      if(uabort())
      {
/*Call the plot routine to flush the buffers.*/
	pltpro(999.0,999.0,64,&cont,1,gap);
	wdbpltc_free();
	first=1;
	return;
      }
    } /* end for */
  }  /* end for */
/*Call the plot routine to flush the buffers.*/
  pltpro(999.0,999.0,64,&cont,1,gap);
/* free local working memory */
  wdbpltc_free();
  first=1;
  return;
}
/**************************************************************************/
/* WDBPLTC_FREE- frees buffers                                            */
/**************************************************************************/
void wdbpltc_free()
{

  ufree((char*)lonray);
  ufree((char*)latray);
  ufree(g_bytbuf);
  ufree(g_celbuf);
  return;
}
/*
*****************************************************************************
	Function celchk

	Checks for data in a given one-degree cell, reads bit map.


	Variable definitions :

	caddr  - current physical record address in bytes from beginning
		 of the file (cell map address).
	pcaddr - previous physical record address (cell map address).
	ndxpos - bit position within the 64800 bit cell map for the cell
		 pointed to by 'g_index'.
	bytpos - byte position within the cell map of the 'g_index' cell.
	bitpos - bit position within the 'bytpos' byte of the 'g_index' cell
		 bit.
	chk    - logical value returned (true if there is data in the
		 'g_index' cell).

	Arguments:

	first  - logical value that is set true the first time the cell map
		 is accessed.

*/
unsigned char celchk(first)
short *first;
{
  unsigned char test_bit();

  static long caddr, pcaddr = 0;
  long ndxpos;
  short bytpos, bitpos;
  unsigned char chk;
/*Compute the physical address of the 'g_index' cell bit.*/
  caddr = (((g_index+g_logrec*8)-(g_offset+1))/(PHYSIZ*8))*PHYSIZ;
/*If this is the first access or the physical address has changed since
the last access, read a new physical record.*/
  if (*first || pcaddr!=caddr)
  {
    g_lstat = lseek(g_lunfil,caddr,SEEK_SET);
    g_lstat = read(g_lunfil,g_celbuf,PHYSIZ);
  }
/*Set the previous address to the current one, set 'first' false.*/
  pcaddr = caddr;
  *first = 0;
/*Compute the 'g_index' position within the physical record.*/
  ndxpos = ((g_index+g_logrec*8)-(g_offset+1))%(PHYSIZ*8);
/*Compute the byte and bit positions.*/
  bytpos = ndxpos/8;
  bitpos = 7-ndxpos%8;
/*Test the 'g_index' bit and return.*/
  chk = test_bit(g_celbuf[bytpos],bitpos);
  return (chk);
}

/*
*****************************************************************************
	Function test_bit

	Checks for bit set in an unsigned char.

	Variable definitions :

	mask  - char array of bit masks

*/
unsigned char test_bit(byte, bitpos)
unsigned char byte;
short bitpos;
{
  static unsigned char mask[9] = {0x1,0x2,0x4,0x8,0x10,0x20,0x40,0x80};
  return (byte&mask[bitpos]);
}

/*
*****************************************************************************
	Function movpos

	Updates current position pointer and checks for end of record.

*/
void movpos(void)
{
  void nxtrec();
  g_curpos += 2;
  if (g_curpos%g_logrec==g_fulrec) nxtrec();
  return ;
}

/*
*****************************************************************************
	Function nxtrec

	Reads next record in overflow chain.

*/
void nxtrec(void)
{
/*Compute the index number for the next logical record in the chain.*/
  g_index = g_bytbuf[g_curpos+1]*65536L + g_bytbuf[g_curpos+2]*256L +
  g_bytbuf[g_curpos+3];
  g_addr = ((g_index-1)/g_lperp)*PHYSIZ;
/*If the physical record has changed since the last access, read a new
physical record.*/
  if (g_addr!=g_paddr)
  {
    g_lstat = lseek(g_lunfil,g_addr,SEEK_SET);
    g_lstat = read(g_lunfil,g_bytbuf,PHYSIZ);
  }
/*Set the previous physical address to the current one, and compute the
current byte position for the new record.*/
  g_paddr = g_addr;
  g_curpos = ((g_index-1)%g_lperp)*g_logrec;
  return ;
}

/*
*****************************************************************************
	Function pltpro

	Plots the data retrieved by wdbplt.

	Variable definitions :

	flag   - set if segment has gone outside of plotting area.
	npts   - number of points in the current segment prior to 'simple'.
	prank  - previous rank value.
	count  - number of points after 'simple'.
	savlat - variable used to save the 512th latitude value if the
		 segment is larger than 512 points.
	savlon - variable used to save the 512th longitude value if the
		 segment is larger than 512 points.
	latray - array of latitude values.
	lonray - array of longitude values.

	ipen   - pseudo pen-up/pen-down value, set based on flag and cont.
	ylat   - latitude in range -90 to +90.
	i      - utility variable.

	Arguments:

	lat    - latitude of current point (degrees +90.0)
	lon    - longitude of current point (degrees)
	rank   - rank of data point.
	cont   - segment continuation flag.
	penclr - number of color to be used for this rank.
	gap    - every 'gap'th point will be plotted; largest allowed
		 value for gap is 45

*/
void pltpro(lat, lon, rank, cont, penclr, gap)
float lat, lon;
short rank, *cont, gap;
char penclr;
{
  void simple(), projct(), curve();

  static char color, flag = 1;
  static short npts = -1, prank = 0, count;
  static float savlat, savlon;
  char ipen;
  float ylat;
  if(debug) printf("PLTPRO:lat %f lon %f rank %hd gap %hd cont %hd\n",
    lat,lon,rank,gap,*cont);
/*Adjust latitude back to -90 to +90 range.*/
  ylat = lat - 90.0;
/*Check for beginning of line.*/
  if (flag && *cont) ipen = 2;
  else ipen = 3;
/*If this is beginning of a line or rank has changed or 512 points have
been stored, plot the line.*/
  if (ipen==3 || rank!=prank || npts==511)
  {
/*Make sure there are at least two points in the arrays.*/
    if (npts>0)
    {
/*Store the count and simplify the arrays if requested.*/
      count = npts;
      simple(&count,gap);
/*Save the last lat and lon if the segment is larger than 512 points.*/
      if (npts==511)
      {
	savlat = latray[511];
	savlon = lonray[511];
      }
/*Plot the data points.*/
      curve(lonray,latray,count,color);
    }
/*If rank has changed, change colors*/
    if(rank != prank) color = penclr;
/*For line larger than 512 points, store the 512th point in the
beginning of the next line and set the counter.*/
    if(npts==511 && *cont)
    {
      latray[0] = savlat;
      lonray[0] = savlon;
      npts = 0;
    }
    else npts = -1;
  }
/*If the current point is within the area, store it and set the flag.*/
  if (ylat>=g_slatdd && ylat<=g_nlatdd && lon>=g_wlondd &&
    lon<=g_elondd)
  {
    npts++;
    latray[npts] = ylat;
    lonray[npts] = lon;
    flag = 1;
  }
  else flag = 0;
  *cont = -1;
  prank = rank;
/*If this is the last point read for this area, reset the 'projct'
initialization flag.*/
  if (rank==64) g_stflag = 1;
  return ;
}

/*
*****************************************************************************
	Function simple

	Nth point simplification routine.


	Variable definitions :

	i      - utility variable.
	ndx    - utility variable.

	Arguments:

	count  - number of values in the arrays.
	gap    - increment for simplification.

*****************************************************************************
*/
void simple(count, gap)
short *count, gap;
{
  register short i, ndx;
/*If the increment is not 1 and the number of points in the arrays
is greater than 10, simplify the arrays.*/
  if (gap>1 && *count>10)
  {
    for (i=ndx=0; i<=*count; i+=gap, ndx++)
    {
      latray[ndx] = latray[i];
      lonray[ndx] = lonray[i];
    }
    latray[ndx] = latray[*count];
    lonray[ndx] = lonray[*count];
/*Set the count to the new count.*/
    *count = ndx;
  }
  return ;
}

/**************************************************************************/
/* Module UTIL.C- Utility functions                                       */
/**************************************************************************/

#define ESC 27

/**************************************************************************/
/* UCMATRIX- allocates space for a character 2D array                          */
/**************************************************************************/
unsigned char *ucmatrix(int a, int b)
{
  unsigned char *m;
  m = (unsigned char *)malloc((unsigned) a*b*sizeof(char));
  if (!m) return (NULL);
  return (m);
}
/**************************************************************************/
/* UFMATRIX- allocates space for a float 2D array                          */
/**************************************************************************/
float *ufmatrix(int a, int b)
{
  float *m;
  m=(float *)malloc((unsigned) a*b*sizeof(float));
  if (!m) return ((float*)NULL);
  return (m);
}
/**************************************************************************/
/* UFREE- frees up memory                                                 */
/**************************************************************************/
int ufree(char *m)
{

  free(m);
  return (0);
}
/**************************************************************************/
/* UABORT- tests for operator abort                                       */
/***************************************************************************/
int uabort()
{
    return (0);
}
/**************************************************************************/
/* GSTAT - posts error messages
/***************************************************************************/
int gstat(char *error_msg)
{
    fprintf(stderr,"wdbpltc: %s\n", error_msg);
    return (0);
}
