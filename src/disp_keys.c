#include <SFML/Graphics.h>
#include <stdbool.h>
#include "header.h"

void	displayPianoKeys(char playingNotes[MAX_CHANNELS][MAX_BUFFERS], sfRectangleShape *rec, sfRenderWindow *win)
{
	bool	drawOneBefore = false;
	float	last = 0;
	sfColor	color;

	sfRectangleShape_setOrigin(rec, (sfVector2f){0, 0});
	if (!rec)
		return;
	for (int i = 0; i < MAX_BUFFERS; i++) {
		switch (i % 12) {
		case 0:
		case 2:
		case 4:
		case 5:
		case 7:
		case 9:
		case 11:
			sfRectangleShape_setPosition(rec, (sfVector2f){last + 1, 960 - 80 * frect.height / 960});
			color = (sfColor){255, 255, 255, 255};
			for (int j = 0; j < MAX_CHANNELS; j++)
				if (playingNotes[j][i]) {
					color = channelColors[j];
					break;
				}
			sfRectangleShape_setFillColor(rec, color);
			sfRectangleShape_setSize(rec, (sfVector2f){NOTE_STEP - 2, 80 * frect.height / 960});
			sfRenderWindow_drawRectangleShape(win, rec, NULL);
			if (drawOneBefore) {
				sfRectangleShape_setPosition(rec, (sfVector2f){last - 6, 960 - 80 * frect.height / 960});
				color = (sfColor){0, 0, 0, 255};
				for (int j = 0; j < MAX_CHANNELS; j++)
					if (playingNotes[j][i - 1]) {
						color = (sfColor){
							channelColors[j].r * 0.5,
							channelColors[j].g * 0.5,
							channelColors[j].b * 0.5,
							channelColors[j].a
						};
						break;
					}
				sfRectangleShape_setFillColor(rec, color);
				sfRectangleShape_setSize(rec, (sfVector2f){(NOTE_STEP - 2) / 1.5, 40 * frect.height / 960});
				sfRenderWindow_drawRectangleShape(win, rec, NULL);
			}
			drawOneBefore = false;
			last += NOTE_STEP;
			break;
		case 1:
		case 3:
		case 6:
		case 8:
		case 10:
			drawOneBefore = true;
			break;
		}
	}
}