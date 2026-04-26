#include <platform/platform.h>
#include <malloc.h>
#include <stdio.h>
#include "pcx.h"
#include "memman.h"

#include <stdarg.h>
#include <string.h>
#include "unaligned.h"

#define SHADE_STEPS 5
#define SHADE_PAL (SHADE_STEPS*256*(int)sizeof(pixel_t)*2)

void *get_palette_ptr=NULL;


void decomprimate_line_256(const char *src,char *trg,int linelen,int *srcstep)
  {
  const char *srcsave;

  srcsave=src;
  while (linelen--)
     {
     if (*src>=0xc0)
        {
        int i;
        i=*src++ & 0x3f;memset(trg,*src++,i);
        trg+=i;linelen-=i-1;
        }
     else
        *trg++=*src++;
     }
  *srcstep=src-srcsave;
  }
void decomprimate_line_hi(const char *src,pixel_t *trg,pixel_t *paleta,int linelen,int *srcstep)
  {
  const char *srcsave;

  srcsave=src;
  while (linelen--)
     {
     if (*src>=0xc0)
        {
        int i,j;
        i=(*src++) & 0x3f;
        for (j=0;j<i;j++) *trg++=paleta[(uint8_t)*src];
        src++;
        linelen-=i-1;
        }
     else
        *trg++=paleta[(uint8_t)*src++];
     }
  *srcstep=src-srcsave;
  }

void palette_shadow(const char *pal1,pixel_t pal2[][256],int tr,int tg,int tb)
  {
  int i,j;
  const char *bt;
  int r,g,b;
  pixel_t hi;

  for (j=0;j<SHADE_STEPS;j++)
     {
     bt=pal1;
     i=0;
     do
       {
       r=(tr+(*(bt++)-tr)*(3*SHADE_STEPS-3*j-1)/(3*SHADE_STEPS-1));
       g=(tg+(*(bt++)-tg)*(3*SHADE_STEPS-3*j-1)/(3*SHADE_STEPS-1));
       b=(tb+(*(bt++)-tb)*(3*SHADE_STEPS-3*j-1)/(3*SHADE_STEPS-1));
       hi=RGB888(r,g,b);
       pal2[j][i]=hi;
       }
    while (++i & 0xff);
    }
  for (j=0;j<SHADE_STEPS;j++)
     {
     bt=pal1;
     i=0;
     do
       {
       r=((*(bt++))*(SHADE_STEPS-j)/SHADE_STEPS);
       g=((*(bt++))*(SHADE_STEPS-j)/SHADE_STEPS);
       b=((*(bt++))*(SHADE_STEPS-j)/SHADE_STEPS);
       hi=RGB888(r,g,b);
       pal2[j+SHADE_STEPS][i]=hi;
       }
    while (++i & 0xff);
    }
  }


int load_pcx(const char *pcx,int32_t fsize,int conv_type,char **buffer, ... )
  //dale nasleduji int hodnoty poctu prechodu a R,G,B barvy
  {
  pixel_t paleta2[256];
  const char *paleta1;
  const char *ptr1;
  char *ptr4;
  pixel_t *ptr2;
  const char *ptr3;
  int i;
  PCXHEADER pcxdata;
  int xsize,ysize;


  if (pcx==0) return -1;
  paleta1=pcx+fsize-768;
  ptr1=paleta1;ptr2=paleta2;
  if (get_palette_ptr!=NULL)
     memcpy(get_palette_ptr,ptr1,768);
  for (i=0;i<256;i++)
     {
      int r = ptr1[0];
      int g = ptr1[1];
      int b = ptr1[2];
      *ptr2 = RGB888(r,g,b);
      ++ptr2;
      ptr1+=3;
     }

  memcpy(&pcxdata,pcx,sizeof(pcxdata));
  xsize=pcxdata.xmax-pcxdata.xmin+1;
  ysize=pcxdata.ymax-pcxdata.ymin+1;
  int sz = 0;
  switch (conv_type)
     {
     case A_8BIT: *buffer=(char *)getmem(sz = xsize*ysize+256*sizeof(pixel_t)+16);break;
     case A_16BIT_ZERO_TRANSP:conv_type = A_16BIT; paleta2[0] = BGSWITCHBIT;CASE_FALLTHROUGH;
     case A_16BIT: *buffer=(char *)getmem(sz = xsize*ysize*sizeof(pixel_t)+16);break;
     case A_FADE_PAL: *buffer=(char *)getmem(sz = xsize*ysize+SHADE_PAL+16);break;
     case A_8BIT_NOPAL: *buffer=(char *)getmem(sz = xsize*ysize+16);break;
     case A_NORMAL_PAL: *buffer=(char *)getmem(sz = xsize*ysize+16+768);break;
     default: return -2; //invalid type specificied
     }
  ptr4=*buffer;
  write_u16_unaligned(ptr4, xsize); ptr4 += 2;
  write_u16_unaligned(ptr4, ysize); ptr4 += 2;
  write_u16_unaligned(ptr4, conv_type == A_16BIT ? 32 : conv_type); ptr4 += 2;
  pcx+=sizeof(pcxdata);ptr3=pcx;
  if (conv_type==A_NORMAL_PAL)
     {
     memcpy(ptr4,paleta1,768);
     ptr4+=768;
     }
  if (conv_type==A_8BIT)
     {
     memcpy(ptr4,paleta2,256*sizeof(pixel_t));
     ptr4+=256*sizeof(pixel_t);
     }
  if (conv_type==A_FADE_PAL)
     {
     int tr,tg,tb;

     va_list lst;
     va_start(lst, buffer);
     tr=va_arg(lst,int);
     tg=va_arg(lst,int);
     tb=va_arg(lst,int);
     va_end(lst);
     palette_shadow(paleta1,(pixel_t (*)[256])ptr4,tr,tg,tb);
     ptr4+=SHADE_PAL;
     }
  ysize++;
  while (--ysize)
     {
     int step;
     if (conv_type==A_16BIT)
        {
        decomprimate_line_hi(ptr3,(pixel_t *)ptr4,paleta2,pcxdata.bytesperline,&step);
        ptr4+=sizeof(pixel_t)*xsize;
        }
     else
        {
        decomprimate_line_256(ptr3,ptr4,pcxdata.bytesperline,&step);
        ptr4+=xsize;
        }
     ptr3+=step;
     }
  return sz;

}
/*
int open_pcx(const char *filename,int type,char **buffer,...)
  {
  FILE *pcx;
  char *src;
  int32_t fsize;

  pcx=fopen_icase(filename,"rb");
  if (pcx==NULL) return -1;
  fseek(pcx,0,SEEK_END);
  fsize=ftell(pcx);
  fseek(pcx,0,SEEK_SET);
  src=(char *)getmem(fsize);
  fread(src,1,fsize,pcx);
  va_list lst;
  va_start(lst, buffer);
  int
  fsize=load_pcx(src,fsize,type,buffer,*((int *)&buffer+1),*((int *)&buffer+2),*((int *)&buffer+3));
  fclose(pcx);
  free(src);
  return fsize;
  }
*/
/*void initmode32b();

main()
  {
  char *buf;

  initmode32b();
  open_pcx("DESK.pcx",A_8BIT,&buf,0,0,0);
  put_picture(0,480-102,buf);
  showview(0,0,0,0);
  getchar();
  return 0;
  }


*/

