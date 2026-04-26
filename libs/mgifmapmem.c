#include <platform/platform.h>
#include <libs/bgraph.h>
#include <stdio.h>
#include "types.h"
#include "memman.h"
#include "mem.h"
#include "mgifmem.h"
#include <platform/sound.h>

#include "strlite.h"




static pixel_t paleta[256];

static pixel_t *picture;
static pixel_t *anim_render_buffer;
static void *sound;

static inline pixel_t avg_pixels(pixel_t a, pixel_t b) {
    return BLEND_PIXELS(a, b);
}

static void StretchImageHQ(pixel_t *src, pixel_t *trg, int32_t linelen, char full)
  {
  word xs=((word *)src)[0],ys=((word *)src)[1];

  pixel_t *src_row = (pixel_t *)((char *)src + 6);
  pixel_t *trg_row = trg;

  for(int y = 0; y < ys; ++y) {

      for (int x = 0; x < xs; ++x) {
          pixel_t n = src_row[x];
          trg_row[2*x] = n;
      }

      trg_row += linelen;

/*
      if (y+1 < ys ) {
          for (int x = 0; x < xs; ++x) {
              pixel_t n1 = src_row[x];
              pixel_t n2 = src_row[x+xs];
              pixel_t n3 = y > 0?src_row[x-xs]:n1;
              pixel_t n4 = y < (ys-2)?src_row[x+2*xs]:n2;
              pixel_t n5 = avg_pixels(n1, n2);
              pixel_t n6 = avg_pixels(n3, n4);
              pixel_t n7 = avg_pixels(n5, n6);
              trg_row[2*x] = n7;
              trg_row[2*x+1] = n7;
          }
      }
*/
      trg_row += linelen;

      src_row += xs;
  }

  trg_row = trg;

  for (int y = 0; y < ys; ++y) {
      for (int x = 0; x < xs-1; ++x) {
          pixel_t n1 = trg_row[2*x];
          pixel_t n2 = trg_row[2*x+2];
          pixel_t n3 = x > 0?trg_row[2*x-2]:n1;
          pixel_t n4 = x < (xs-2)?trg_row[2*x+2]:n2;
          pixel_t n5 = avg_pixels(n1, n2);
          pixel_t n6 = avg_pixels(n3, n4);
          pixel_t n7 = avg_pixels(n5, n6);
          trg_row[2*x+1] = n7;
      }
      trg_row += 2*linelen;
  }

  trg_row = trg;

  for (int y = 0; y < ys-1; ++y) {
      for (int x = 0; x < 2*xs; ++x) {
          pixel_t n1 = trg_row[x];
          pixel_t n2 = trg_row[x+2*linelen];
          pixel_t n3 = y > 1?trg_row[x-2*linelen]:n1;
          pixel_t n4 = y < (ys-2)?trg_row[x+4*linelen]:n2;
          pixel_t n5 = avg_pixels(n1, n2);
          pixel_t n6 = avg_pixels(n3, n4);
          pixel_t n7 = avg_pixels(n5, n6);
          trg_row[x+linelen] = avg_pixels(n7,0);
      }
      trg_row += 2*linelen;
  }


  }

static TSTR_LIST titles;

static void show_title(int frame)
{
    int y,yt,h;
    static int lasty = 478;
    char *c;

    if (frame<2) lasty=478;
    if (frame >= str_count(titles)) return;
    if (titles[frame]==NULL) return;
    h=text_height(titles[frame])+2;
    yt=y=479-2*h;
    curcolor=0;
    bar32(0,lasty,639,479);
    c=strchr(titles[frame],'\n');
    if (c!=NULL) *c=0;
    set_aligned_position(320,y,1,0,titles[frame]);
    outtext(titles[frame]);
    if (c!=NULL)
      {
      *c='\n';c++;y+=h;
      set_aligned_position(320,y,1,0,c);
      outtext(c);
      }
    if (lasty<yt) y=lasty;else y=yt;
    showview(0,y,640,480-y);
    lasty=yt;
}



static void PlayMGFFile(const void *file, MGIF_PROC proc,int ypos,char full)
  {
  int32_t scr_linelen2 = GetScreenPitch();
  mgif_install_proc(proc);
  sound=PrepareVideoSound(22050,226*1024);
  picture=getmem(6+320*180*sizeof(pixel_t));
  ((word *)picture)[0]=320;
  ((word *)picture)[1]=180;
  ((word *)picture)[2]=15;
  memset((char *)picture+6,0,320*180*sizeof(pixel_t));
  anim_render_buffer=(pixel_t *)((char *)picture+6);
  file=open_mgif(file);
  if (file==NULL) return;
  MGIF_HEADER_T *hdr =(MGIF_HEADER_T *)file;
  hdr->accnums[0] = hdr->accnums[1] = 0;
  hdr->sound_write_pos = 65536;
  char f = 1;
  while (f)
	{
      f=mgif_play(file);
      StretchImageHQ(picture, GetScreenAdr()+ypos*scr_linelen2, scr_linelen2,full);
      showview(0,ypos,0,360);
      if (titles) show_title(hdr->cur_frame);
      if (game_display_is_quit_requested()) {
          break;
      } else if (_bios_keybrd(_KEYBRD_READY)) {
          _bios_keybrd(_KEYBRD_READ);
          break;
	  }
	}
  close_mgif(file);
  DoneVideoSound(sound);
  free(picture);
  }


void show_full_lfb12e(void *target,const void *buff,const void *paleta);
void show_delta_lfb12e(void *target,const void *buff,const void *paleta);
void show_delta_lfb12e_dx(void *target,void *buff,void *paleta);
void show_full_lfb12e_dx(void *target,void *buff,void *paleta);




void BigPlayProc(MGIF_HEADER_T *hdr,int act,const void *data,int csize)
  {
  switch (act)
     {
     case MGIF_LZW:
     case MGIF_COPY:show_full_lfb12e(anim_render_buffer,data,paleta);break;
     case MGIF_DELTA:show_delta_lfb12e(anim_render_buffer,data,paleta);break;
     case MGIF_PAL:{const word *src16=(const word *)data;int i;for(i=0;i<256;i++)paleta[i]=rgb555to32(src16[i]);}break;
	 case MGIF_SOUND:
	   while (LoadNextVideoFrame(sound,data,csize,hdr->ampl_table,hdr->accnums,&hdr->sound_write_pos)==0);
     }
  }

void play_animation(const char *filename,char mode,int posy,char sound)
  {
  size_t sz;
  void *mgf=map_file_to_memory(file_icase_find(filename), &sz);
  change_music(NULL);
  if (mgf==NULL) return;
  game_display_disable_crt_effect_temporary(1);
  PlayMGFFile(mgf,BigPlayProc,posy,mode & 0x80);
  game_display_disable_crt_effect_temporary(0);
  unmap_file(mgf, sz);
  }

void set_title_list(char **title_list)
  {
  titles=title_list;
  }

void set_play_attribs(void *screen,char rdraw,char bm,char colr64)
  {

  }
