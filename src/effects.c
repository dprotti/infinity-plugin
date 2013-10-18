#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include <effects.h>
#include <gettext.h>
#include <assert.h>

static t_effect effects[100];
static gint32 nb_effects = 0;
static gboolean initialized = FALSE;

void effects_save_effect (t_effect *effect)
{
	gchar datafile[256], path[256];
	FILE *f;
	gint32 i;
    
	g_assert (effect);
	/** TODO Remove hardcoded path */
	f = fopen ("/usr/share/audacious/infinite_states","a");
	if (f == NULL) {
		g_warning (_("Could not be opened file %s for saving effects\n"),
			   datafile);
		return ;
	}
	for (i = 0; i < sizeof(t_effect); i++) 
		fputc (*((byte*)effect+i), f);
	fclose (f);
}

void effects_load_effects (void)
{
	gchar path[255];
	gchar datafile[255];
	FILE *f;
	gint32 finished = 0;
	gint32 i, b, c, d, e;

	f = fopen ("/usr/share/audacious/infinite_states","r");
	if (f == NULL) {
		g_warning (_("Could not be opened file %s for loading effects\n"),
			   datafile);
		return ;
	}
	while (!finished) {
		byte* ptr_effect = (byte*)&effects[nb_effects];
		for (i=0;i < sizeof(t_effect); i+=4) {
			b=fgetc(f);
			if (b!=EOF) {
				c = fgetc(f);
				assert(c != EOF);
				d = fgetc(f);
				assert(d != EOF);
				e = fgetc(f);
				assert(e != EOF);
#if BYTE_ORDER == BIG_ENDIAN
				ptr_effect[i]=(byte)e;
				ptr_effect[i+1]=(byte)d;
				ptr_effect[i+2]=(byte)c;
				ptr_effect[i+3]=(byte)b;
#else
				ptr_effect[i]=(byte)b;
				ptr_effect[i+1]=(byte)c;
				ptr_effect[i+2]=(byte)d;
				ptr_effect[i+3]=(byte)e;
#endif
			} else
				finished=1;
		}
		nb_effects+=1;
	}
	nb_effects -= 1;
	fclose (f);
}

void effects_load_random_effect (t_effect *effect)
{
	gint32 trash;

	if (!initialized) {
		srand (trash);
		initialized = TRUE;
	}
	if (nb_effects > 0) {
		gint32 num_effect = rand() % nb_effects;
		gint32 i;
        
		for (i = 0; i < sizeof(t_effect); i++) {
			*((byte*)effect+i) = *((byte*)(&effects[num_effect])+i);
		}
	}
}
