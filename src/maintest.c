#include "xmms/plugin.h"

#include <SDL/SDL.h>
#include <SDL/SDL_thread.h>

#include "main.h"
#include "renderer.h"

gint32 playing, finished, reset, closed;
gint32 refreshed;
gint16 pcm_data[2][512];
gint16 freq_data[2][256];
gint32 pcm_count = 0, freq_count = 0;
gint32 tr;

int main(int argc, char *argv[])
{
	finished = FALSE;
	closed = FALSE;
	/* mutex=SDL_CreateMutex(); */
	/* FIXME check if there is MMX support here */
	init_renderer();
	renderer();
	return 0;
}
