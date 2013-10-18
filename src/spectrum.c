#include <math.h>
#include <xmms/plugin.h>
/*#include "libxmms/util.h"*/
#include <SDL/SDL.h>

#define wrap(a) ( a < 0 ? 0 : ( a > 255 ? 255 : a )) 
#define PI 3.14159
 
#define WIDTH 400
#define HEIGHT 300


#define INTERPOLATION 0          //0 or 1 to double resolution.
#define NB_FCT 6
#define NB_PALETTES 5

#include "affiche.h"

#define PERIODE_COLORS 64
#define PERIODE_EFFECTS 128

#define VIDEO_FLAGS (SDL_HWSURFACE|SDL_HWPALETTE)

#define BASS_EXT_MEMORY 100

typedef guchar byte;

static void sanalyzer_init(void);

static void sanalyzer_cleanup(void);
static void sanalyzer_playback_start(void);
static void sanalyzer_playback_stop(void);
static void sanalyzer_render_pcm(gint16 data[2][512]);
static void sanalyzer_render_freq(gint16 data[2][256]);

/*
typedef struct coord {
  int x,y;
} coord;

typedef struct complex {
  float x,y;
} complex;

typedef struct interpol {
  coord c;  //coordinates of the top left pixel.
  int w1,w2,w3,w4;
} interpol;

typedef struct effect {
  int addr_effect;
  int f1,f2,f3;
  int x_curve;
  int p_speed,p_rotation;
  int color,old_color;
  int curve_color;
  int spectral_color;
  int mode_spectre;
  int spectral_shift;
  int flash;
} effect;
*/
struct bass_info {
  //int mem[SIZE_BASS_MEM];
  gint32 max_recent;
  gint32 max_old;
  gint32 time_last_max;
  gint32 min_recent;
  gint32 min_old;
  gint32 time_last_min;
  gint32 activated;
} bass_info;



interpol vector_field[NB_FCT][WIDTH*HEIGHT];

effect current_effect;

gint32 t=0;

VisPlugin sanalyzer_vp =
{
	NULL,
	NULL,

	0,
	"Simple spectrum analyzer",
	1,
	1,		
	sanalyzer_init,
	sanalyzer_cleanup,
	NULL,
	NULL, 
	NULL,
	sanalyzer_playback_start,
	sanalyzer_playback_stop,
	sanalyzer_render_pcm,
	sanalyzer_render_freq
};

VisPlugin *get_vplugin_info(void)
{
	return &sanalyzer_vp;
}

complex fct(complex a,gint32 n,gint32 p1,gint32 p2)   //p1 et p2:0-4 
{
  complex b,b1,b2;
  gfloat fact;
  gfloat an;
  gfloat circle_size;
  gfloat speed;
  gfloat co,si;

  a.x-=WIDTH/2;
  a.y-=HEIGHT/2;
  
  switch (n) {
  
  case 0:
    an=0.025*(p1-2)+0.002;
    co=cos(an);
    si=sin(an);
    circle_size=HEIGHT*0.25;
    speed=2000+p2*500;
    b.x=(co*a.x-si*a.y);
    b.y=(si*a.x+co*a.y);
    fact=-(sqrt(b.x*b.x+b.y*b.y)-circle_size)/speed+1;
    b.x=(b.x*fact);
    b.y=(b.y*fact);
    break;
  
  case 1:
    an=0.015*(p1-2)+0.002;
    co=cos(an);
    si=sin(an);
    circle_size=HEIGHT*0.45;
    speed=4000+p2*1000;
    b.x=(co*a.x-si*a.y);
    b.y=(si*a.x+co*a.y);
    fact=(sqrt(b.x*b.x+b.y*b.y)-circle_size)/speed+1;
    b.x=(b.x*fact);
    b.y=(b.y*fact);
    break;
  
  case 2:
    an=0.002;
    co=cos(an);
    si=sin(an);
    circle_size=HEIGHT*0.25;
    speed=400+p2*100;
    b.x=(co*a.x-si*a.y);
    b.y=(si*a.x+co*a.y);  
    fact=-(sqrt(b.x*b.x+b.y*b.y)-circle_size)/speed+1;
    b.x=(b.x*fact);
    b.y=(b.y*fact);
    break;
    
  case 3:
    an=(sin(sqrt(a.x*a.x+a.y*a.y)/20)/20)+0.002;
    co=cos(an);
    si=sin(an);
    circle_size=HEIGHT*0.25;
    speed=4000;
    b.x=(co*a.x-si*a.y);
    b.y=(si*a.x+co*a.y);
    fact=-(sqrt(b.x*b.x+b.y*b.y)-circle_size)/speed+1;
    b.x=(b.x*fact);
    b.y=(b.y*fact);
    break;
    
  case 4:
    an=0.002;
    co=cos(an);
    si=sin(an); 
    circle_size=HEIGHT*0.25;
    speed=sin(sqrt(a.x*a.x+a.y*a.y)/5)*3000+4000;
    b.x=(co*a.x-si*a.y);
    b.y=(si*a.x+co*a.y);
    fact=-(sqrt(b.x*b.x+b.y*b.y)-circle_size)/speed+1;
    b.x=(b.x*fact);
    b.y=(b.y*fact);    
    break;

  case 5:
    b.x=a.x*1.0;
    b.y=a.y*1.0;

  }  
 
  b.x+=WIDTH/2;
  b.y+=HEIGHT/2;  
  if (b.x<0)
    b.x=0;
  if (b.y<0)
    b.y=0;
  if (b.x>WIDTH-1)
    b.x=WIDTH-1;
  if (b.y>HEIGHT-1)
    b.y=HEIGHT-1;     
  
  return b;
}


void generate_sector(gint32 g,gint32 f,gint32 p1,gint32 p2,gint32 debut,gint32 step)
{
  coord c;
  complex a;
  gfloat fpy;
  gint32 rw,lw,add;
  gint32 fin=debut+step;
  gint32 prop_transmitted=249;
  gint32 i,j;

  if (fin>HEIGHT)
    fin=HEIGHT;
  for (c.y=debut;c.y<fin;c.y++)
    for (c.x=0;c.x<WIDTH;c.x++) {
      a.x=(gfloat)c.x;
      a.y=(gfloat)c.y;
      a=fct(a,f,p1,p2);
      add=c.x+c.y*WIDTH;
      vector_field[g][add].c.x=(gint32)(a.x);
      vector_field[g][add].c.y=(gint32)(a.y);
      fpy=a.y-floor(a.y);
      rw=(gint32)((a.x-floor(a.x))*prop_transmitted);
      lw=prop_transmitted-rw;
      vector_field[g][add].w4=(gint32)(fpy*rw);
      vector_field[g][add].w2=rw-vector_field[g][add].w4;
      vector_field[g][add].w3=(gint32)(fpy*lw);
      vector_field[g][add].w1=lw-vector_field[g][add].w3; 
    }
}
  
void generate_vector_field() 
{
  gint32 f;
  gint32 i,p1,p2;
  
  for (f=0;f<NB_FCT;f++) {
    p1=2;
    p2=2;
    for (i=0;i<HEIGHT;i+=10)
      generate_sector(f,f,p1,p2,i,10);
  }
}

static void sanalyzer_init(void)
{

  generate_vector_field();
  generate_colors();

}


static void sanalyzer_cleanup(void)
{
  SDL_Quit();
}


static void sanalyzer_playback_start(void)
{
  init_sdl();
  generate_vector_field();
}


static void sanalyzer_playback_stop(void)
{
  SDL_Quit();
}



void sanalyzer_render_freq(gint16 data[2][256])
{
  gint32 bass=0;
  gint32 i;
  gint32 step=10;

  for (i=0;i<step;i++)
    bass+=(data[0][i]>>4)+(data[1][i]>>4);

  bass=bass/step/2;

  if (bass>bass_info.max_recent)
    bass_info.max_recent=bass;
  
  if (bass<bass_info.min_recent)
    bass_info.min_recent=bass;

  if (t-bass_info.time_last_max>BASS_EXT_MEMORY) {
    bass_info.max_old=bass_info.max_recent;
    bass_info.max_recent=0;
    bass_info.time_last_max=t;
  }

  if (t-bass_info.time_last_min>BASS_EXT_MEMORY) {
    bass_info.min_old=bass_info.min_recent;
    bass_info.min_recent=0;
    bass_info.time_last_min=t;
  }

  if (bass>(bass_info.max_old*6+bass_info.min_old*4)/10 && bass_info.activated==0) {
    if (current_effect.flash)
      flash(255,t);
    bass_info.activated=1;
  }
  
  if (bass<(bass_info.max_old*4+bass_info.min_old*6)/10 && bass_info.activated==1)
    bass_info.activated=0;
}

void sanalyzer_render_pcm(gint16 data[2][512])
{
  SDL_Event event;
  Uint8 *keystate;
  gint32 son=0;
  gint32 trace=0;
  gfloat x,y,v,vr;
  gint32 i;
  gint32 t_last_palette=0;

  while ( SDL_PollEvent(&event) ) {  
    if ( event.type == SDL_QUIT ) 
      SDL_Quit();
    if (event.type == SDL_KEYDOWN) {
      gchar *st;
      st=SDL_GetKeyName(event.key.keysym.sym);
      if (!strcmp(st,"a"))
	current_effect.curve_color=wrap(current_effect.curve_color-32);
      if (!strcmp(st,"z"))
	current_effect.curve_color=wrap(current_effect.curve_color+32);
      if (!strcmp(st,"q"))
	current_effect.spectral_color=wrap(current_effect.spectral_color-32);
      if (!strcmp(st,"s"))
	current_effect.spectral_color=wrap(current_effect.spectral_color+32);
      if (!strcmp(st,"w"))
	current_effect.mode_spectre=(current_effect.mode_spectre+1)%5;
      if (!strcmp(st,"x"))
	current_effect.flash=(current_effect.flash+1)%2;
    }    
  }

  keystate = SDL_GetKeyState(NULL);
  if ( keystate[SDLK_ESCAPE] )
    ToggleFullScreen();
  for (i=0;i<10;i++)
    if ( keystate[SDLK_F1+i])
	 current_effect.addr_effect=i%NB_FCT;
  if (keystate[SDLK_F11]) { 
    current_effect.color=(current_effect.color-1)%NB_PALETTES;
    t_last_palette=0;
  }
  if (keystate[SDLK_F12]) {  
    current_effect.color=(current_effect.color+1)%NB_PALETTES;
    t_last_palette=0;
  }
  if (keystate[SDLK_e]) {  
    current_effect.spectral_shift=(current_effect.spectral_shift-10)%HEIGHT;
  }
  if (keystate[SDLK_r]) {  
    current_effect.spectral_shift=(current_effect.spectral_shift+10)%HEIGHT;
   }
  
  if (t_last_palette<8) 
    change_color(current_effect.color,(current_effect.color+1)%NB_PALETTES,(t_last_palette+1)*32);
  /*
  if (t%PERIODE_EFFECTS==0) {
    // while (current_effect.f1==current_effect.f2) {
      //   current_effect.f2=current_effect.f1;
    //current_effect.f1=rand()%NB_TYPE_FCT;
      //}
    //current_effect.p_rotation=2;
    //current_effect.p_speed=rand()%5;
    current_effect.addr_effect=(current_effect.addr_effect+1)%NB_FCT;
    }*/
  /*
  generate_sector(1-current_effect.addr_effect,current_effect.f1,
		  current_effect.p_rotation,current_effect.p_speed,
		  (t%PERIODE_EFFECTS)*(HEIGHT/PERIODE_EFFECTS+1),
		  (HEIGHT/PERIODE_EFFECTS+1));
  */
  affiche(current_effect.addr_effect,vector_field);  
 
  spectral(data,current_effect);

  curve(&current_effect);

  t+=1;
  t_last_palette+=1;
}
