#include <stdio.h>
#include <stdlib.h>
#include <glib.h>

#include <effects.h>
#include <assert.h>

#define EFFECTS_FILE	(DATADIR "/infinite_states")

static t_effect effects[100];
static gint32 nb_effects = 0;
static gboolean initialized = FALSE;
static gchar error_msg[256];

void effects_append_effect(t_effect *effect)
{
	FILE *f;
	gint32 i;
	gchar *personal_states = g_strconcat(g_get_home_dir(), "/infinite_states", NULL);

	g_return_if_fail(effect != NULL);

	f = fopen (personal_states, "a");
	if (f == NULL) {
		g_critical("Cannot open file '%s' for saving effects", personal_states);
		g_free(personal_states);
		return;
	}
	g_message("Infinity appended effect to '%s'", personal_states);
	g_free(personal_states);
	for (i = 0; i < sizeof(t_effect); i++)
		fputc(*((byte *)effect + i), f);
	fclose(f);
}

gboolean effects_load_effects(Player *player)
{
	FILE *f;
	gint32 finished = 0;
	gint32 i, b, c, d, e;

	g_return_val_if_fail(player != NULL, FALSE);

	f = fopen (EFFECTS_FILE, "r");
	if (f == NULL) {
		g_snprintf(error_msg, 256, "Cannot open file '%s' for loading effects",
			   EFFECTS_FILE);
		player->notify_critical_error(error_msg);
		return FALSE;
	}
	while (!finished) {
		byte *ptr_effect = (byte *)&effects[nb_effects];
		for (i = 0; i < sizeof(t_effect); i += 4) {
			b = fgetc(f);
			if (b != EOF) {
				c = fgetc(f);
				assert(c != EOF);
				d = fgetc(f);
				assert(d != EOF);
				e = fgetc(f);
				assert(e != EOF);
#if BYTE_ORDER == BIG_ENDIAN
				ptr_effect[i] = (byte)e;
				ptr_effect[i + 1] = (byte)d;
				ptr_effect[i + 2] = (byte)c;
				ptr_effect[i + 3] = (byte)b;
#else
				ptr_effect[i] = (byte)b;
				ptr_effect[i + 1] = (byte)c;
				ptr_effect[i + 2] = (byte)d;
				ptr_effect[i + 3] = (byte)e;
#endif
			} else {
				finished = 1;
			}
		}
		nb_effects++;
	}
	nb_effects--;
	fclose(f);
	return TRUE;
}

void effects_load_random_effect(t_effect *effect)
{
	gint32 trash;

	if (!initialized) {
		#pragma GCC diagnostic ignored "-Wmaybe-uninitialized"
		srand(trash);
		initialized = TRUE;
	}
	if (nb_effects > 0) {
		gint32 num_effect = rand() % nb_effects;
		gint32 i;

		for (i = 0; i < sizeof(t_effect); i++)
			*((byte *)effect + i) = *((byte *)(&effects[num_effect]) + i);
	}
}
