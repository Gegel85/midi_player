#include <string.h>
#include <SFML/Graphics.h>
#include <fcntl.h>
#include <stdbool.h>
#include <header.h>
#include <malloc.h>
#include <errno.h>
#include <limits.h>
#include <concatf.h>
#include <stdlib.h>
#include <dirent.h>
#include <time.h>
#include <ctype.h>

Sprite	*loadConfig(char *path)
{
	Sprite	*array = malloc(sizeof(*array));
	int	len = 1;
	bool	found = false;
	void	*buff;
	char	buffer[PATH_MAX];

	memset(array, 0, sizeof(*array));
	for (int i = 0; ; i++) {
		found = false;
		if (!configs[i].extension) {
			array[len - 1].path = configs[i].path;
			array[len - 1].sprite = sfSprite_create();
			sprintf(buffer, "%s%s", path, configs[i].path);
			array[len - 1].texture = sfTexture_createFromFile(buffer, NULL);
			array[len - 1].extensions = NULL;
			array[len - 1].nbOfExtensions = 0;
			sfSprite_setTexture(array[len - 1].sprite, array[len - 1].texture, sfFalse);
			break;
		} else
			for (int j = 0; j < len - 1; j++) {
				if (strcmp(array[j].path, configs[i].path) == 0) {
					found = true;
					buff = realloc(array[j].extensions, ++array[j].nbOfExtensions * sizeof(*array[j].extensions));
					if (!buff) {
						for (int j = 0; j < len; i++) {
							sfSprite_destroy(array[i].sprite);
							sfTexture_destroy(array[i].texture);
							free(array[i].extensions);
						}
						free(array);
						return (NULL);
					}
					array[j].extensions = buff;
					array[j].extensions[array[j].nbOfExtensions - 1] = configs[i].extension;
					break;
				}
			}
		if (!found) {
			buff = realloc(array, ++len * sizeof(*array));
			if (!buff) {
				for (int j = 0; j < len - 1; i++) {
					sfSprite_destroy(array[i].sprite);
					sfTexture_destroy(array[i].texture);
					free(array[i].extensions);
				}
				free(array);
				return (NULL);
			}
			array = buff;
			array[len - 2].path = configs[i].path;
			array[len - 2].sprite = sfSprite_create();
			sprintf(buffer, "%s%s", path, configs[i].path);
			array[len - 2].texture = sfTexture_createFromFile(buffer, NULL);
			array[len - 2].extensions = malloc(sizeof(*array[len - 2].extensions));
			array[len - 2].nbOfExtensions = 1;
			if (!array[len - 2].extensions) {
				for (int j = 0; j < len - 1; i++) {
					sfSprite_destroy(array[i].sprite);
					sfTexture_destroy(array[i].texture);
					free(array[i].extensions);
				}
				free(array);
				return (NULL);
			}
			*array[len - 2].extensions = configs[i].extension;
			sfSprite_setTexture(array[len - 2].sprite, array[len - 2].texture, sfFalse);
			memset(&array[len - 1], 0, sizeof(*array));
		}
	}
	return (array);
}

int	my_strcmp(char *str1, char *str2)
{
	for (int i = 0; str1[i]; i++)
		if (tolower(str1[i]) != tolower(str2[i]))
			return (str1[i] - str2[i]);
	return (str1[strlen(str1)] - str2[strlen(str1)]);
}

float	getWordSize(sfUint32 *string, int start, int end, sfFont *font, int size)
{
	float	result = 0;

	for (int i = start; string[i] && i < end; i++)
		result += sfFont_getGlyph(font, string[i], size, false, 0).advance;
	return (result);
}

int	getWordPosition(sfUint32 *string, float position, sfFont *font, int size)
{
	float	result = 0;

	for (int i = 0; string[i]; i++) {
		if (result >= position)
			return (i);
		result += sfFont_getGlyph(font, string[i], size, false, 0).advance;
	}
	return (strlen_unicode(string));
}

FileInfos	*getDirEntries(char *path)
{
	DIR		*dirstream = opendir(path);
	FileInfos	*entries;
	struct dirent	*entry = NULL;
	int		len = 1;
	void		*buff;
	FileInfos	buffer;

	if (!dirstream) {
		printf("Error: %s: %s\n", path, strerror(errno));
		return (NULL);
	}
	entries = malloc(sizeof(*entries));
	entries->isEnd = true;
	for (entry = readdir(dirstream); entry; entry = readdir(dirstream)) {
		entries[len - 1].name = strdup(entry->d_name);
		entries[len - 1].isEnd = false;
		buff = concatf("%s/%s", path, entries[len - 1].name);
		stat(buff, &entries[len - 1].stats);
		buff = realloc(entries, ++len * sizeof(*entries));
		if (!buff) {
			free(entries);
			return (NULL);
		}
		entries = buff;
		entries[len - 1].isEnd = true;
	}
	for (int i = len - 1; i >= 0; i--)
		for (int j = 1; j < i; j++)
			if (
				((entries[j].stats.st_mode & S_IFMT) == S_IFDIR && (entries[j - 1].stats.st_mode & S_IFMT) != S_IFDIR) ||
				(
					(
						((entries[j].stats.st_mode & S_IFMT) != S_IFDIR && (entries[j - 1].stats.st_mode & S_IFMT) != S_IFDIR) ||
						((entries[j].stats.st_mode & S_IFMT) == S_IFDIR && (entries[j - 1].stats.st_mode & S_IFMT) == S_IFDIR)
					) &&
					strcmp(entries[j - 1].name, entries[j].name) > 0
				)
			) {
				buffer = entries[j - 1];
				entries[j - 1] = entries[j];
				entries[j] = buffer;
			}
	return (entries);
}

char	*exploreFile(char *path, sfFont *font, Sprite *sprites)
{
	sfVideoMode	mode = {600, 400, 32};
	sfRectangleShape*rect = sfRectangleShape_create();
	sfEvent		event;
	sfRenderWindow	*window = sfRenderWindow_create(mode, "Choose a path", sfClose, NULL);
	float		start = 0;
	int		selected = 0;
	char		*buff;
	void		*buf;
	char		realPath[PATH_MAX + 1];
	char		buffer[PATH_MAX + 2];
	sfUint32	bufferUnicode[PATH_MAX + 1];
	FileInfos	*direntry;
	bool		displayed = false;
	int		scrollBarPressed = 0;
	sfUint32	displayedPath[PATH_MAX + 2];
	int		len;
	int		menuSelected = -1;
	unsigned int	cursorPos = 0;
	sfVector2i	selectedText = {0, 0};
	sfText		*text = sfText_create();
	int		leftButtonIsPressed = 0;

	if (!window || !text || !rect) {
		if (window)
			sfRenderWindow_destroy(window);
		if (rect)
			sfRectangleShape_destroy(rect);
		if (text)
			sfText_destroy(text);
		return (NULL);
	}
	sfRenderWindow_setFramerateLimit(window, 60);
	sfText_setFont(text, font);
	memset(displayedPath, 0, sizeof(displayedPath));
	if (!realpath(path, realPath)) {
		buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
		printf("Error: %s: %s\n", realPath, strerror(errno));
		dispMsg("Error", buf, 0);
		free(buf);
		realpath(".", realPath);
	}
	direntry = getDirEntries(realPath);
	if (!direntry && errno != 0) {
		buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
		printf("Error: %s: %s\n", realPath, strerror(errno));
		dispMsg("Error", buf, 0);
		free(buf);
		realpath(".", realPath);
		direntry = getDirEntries(realPath);
	}
	if (!direntry) {
		sfRectangleShape_destroy(rect);
		sfRenderWindow_destroy(window);
		return (NULL);
	}
	sfText_setCharacterSize(text, 15);
	for (len = 0; !direntry[len].isEnd; len++);
	while (sfRenderWindow_isOpen(window)) {
		sfRectangleShape_setOutlineThickness(rect, 1);
		sfRectangleShape_setOutlineColor(rect, (sfColor){0, 0, 0, 255});
		sfRenderWindow_clear(window, (sfColor){200, 200, 200, 255});
		while (sfRenderWindow_pollEvent(window, &event)) {
			if (event.type == sfEvtClosed)
				sfRenderWindow_close(window);
			else if (event.type == sfEvtMouseWheelScrolled) {
				if (len < 15) continue;
				if (start - event.mouseWheelScroll.delta / 2 < 0)
					start = 0;
				else if (start - event.mouseWheelScroll.delta / 2 >= len - 15)
					start = len - 15;
				else
					start -= event.mouseWheelScroll.delta / 2;
			} else if (event.type == sfEvtMouseMoved && leftButtonIsPressed) {
				if (menuSelected == 1) {
					selectedText.y = getWordPosition(displayedPath, event.mouseMove.x - 30, font, 15);
					cursorPos = selectedText.y;
				}
				if (scrollBarPressed) {
					start = (float)((scrollBarPressed - event.mouseMove.y + 20) * (len - 15) * len) / (6000 - 300 * len);
					if (start < 0)
						start = 0;
					else if (start > len - 15)
						start = len - 15;
				}
			} else if ((event.type == sfEvtMouseButtonReleased && event.mouseButton.button == sfMouseLeft) || event.type == sfEvtMouseLeft) {
				leftButtonIsPressed--;
				scrollBarPressed = false;
				if (leftButtonIsPressed < 0)
					leftButtonIsPressed = 0;
			} else if (event.type == sfEvtMouseButtonPressed && event.mouseButton.button == sfMouseLeft) {
				leftButtonIsPressed++;
				if (
					event.mouseButton.x >= 569 &&
					event.mouseButton.x <= 580 &&
					event.mouseButton.y >= (float)start * 300 / (len - 15) - ((float)start / (len - 15)) * 6000 / len + 20 &&
					event.mouseButton.y <= (float)start * 300 / (len - 15) - ((float)start / (len - 15)) * 6000 / len + 20 + 6000 / len + 1
					)
					scrollBarPressed = event.mouseButton.y - ((float)start * 300 / (len - 15) - ((float)start / (len - 15)) * 6000 / len + 20);
				else if (event.mouseButton.x < 380 && event.mouseButton.x > 20 && event.mouseButton.y > 330 && event.mouseButton.y < 370) {
					if (menuSelected == 1) {
						selectedText.x = getWordPosition(displayedPath, event.mouseButton.x - 30, font, 15);
						selectedText.y = selectedText.x;
						cursorPos = selectedText.x;
					} else {
						menuSelected = 1;
						selectedText.x = 0;
						selectedText.y = strlen_unicode(displayedPath);
					}
				} else if (event.mouseButton.x > 20 && event.mouseButton.x < 570 && event.mouseButton.y > 20 && event.mouseButton.y < 320 && event.mouseButton.y < (len + 1) * 20) {
					if (menuSelected == 0 && (int)((float)event.mouseButton.y / 20 + start - 1) == selected) {
						displayedPath[0] = '\0';
						if ((direntry[selected].stats.st_mode & S_IFMT) == S_IFDIR) {
							buff = strdup(realPath);
							path = concatf("%s/%s", realPath, direntry[selected].name);
							if (!realpath(path, realPath)) {
								free(path);
								buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
								printf("Error: %s: %s\n", realPath, strerror(errno));
								dispMsg("Error", buf, 0);
								free(buf);
								strcpy(realPath, buff);
								free(buff);
								continue;
							}
							free(path);
							buf = getDirEntries(realPath);
							if (!buf) {
								buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
								printf("Error: %s: %s\n", realPath, strerror(errno));
								dispMsg("Error", buf, 0);
								free(buf);
								strcpy(realPath, buff);
								free(buff);
								continue;
							}
							for (len = 0; !direntry[len].isEnd; len++)
								free(direntry[len].name);
							free(direntry);
							direntry = buf;
							free(buff);
							for (len = 0; !direntry[len].isEnd; len++);
							selected = 0;
							start = 0;
							menuSelected = -1;
							if (!direntry) {
								sfRectangleShape_destroy(rect);
								sfRenderWindow_destroy(window);
								return (NULL);
							}
							continue;
						} else {
							path = concatf("%s/%s", realPath, direntry[selected].name);
							for (len = 0; !direntry[len].isEnd; len++)
								free(direntry[len].name);
							free(direntry);
							sfRectangleShape_destroy(rect);
							sfRenderWindow_destroy(window);
							return (path);
						}
					}
					menuSelected = 0;
					selected = (float)event.mouseButton.y / 20 + start - 1;
					convertStringToUnicode(direntry[selected].name, displayedPath);
				} else
					menuSelected = -1;
			} else if (event.type == sfEvtTextEntered) {
				if (event.text.unicode >= ' ' && strlen_unicode(displayedPath) < PATH_MAX - 1 && event.text.unicode != 127) {
					for (int i = selectedText.x > selectedText.y ? selectedText.y : selectedText.x; displayedPath[i + abs(selectedText.x - selectedText.y) - 1]; i++)
						displayedPath[i] = displayedPath[i + abs(selectedText.x - selectedText.y)];
					for (int i = strlen_unicode(displayedPath); i >= (int)cursorPos; i--)
						displayedPath[i + 1] = displayedPath[i];
					displayedPath[cursorPos++] = event.text.unicode;
					selectedText.x = cursorPos;
					selectedText.y = cursorPos;
				}
			} else if (event.type == sfEvtKeyPressed) {
				if (event.key.code == sfKeyA) {
					if (menuSelected == 1 && event.key.control) {
						selectedText.x = 0;
						selectedText.y = strlen_unicode(displayedPath);
						cursorPos = selectedText.y;
					}
				} else if (event.key.code == sfKeyUp) {
					if (menuSelected != 0) {
						menuSelected = 0;
						if (start > selected)
							start = selected;
						else if (start + 15 < selected)
							start = selected - 14;
						continue;
					}
					if (selected <= 0) {
						selected = len - 1;
						start = selected >= 15 ? selected - 14 : 0;
					} else {
						selected -= 1;
						if (start > selected)
							start = selected;
					}
				} else if (event.key.code == sfKeyDown) {
					if (menuSelected != 0) {
						menuSelected = 0;
						if (start > selected)
							start = selected;
						else if (start + 15 < selected)
							start = selected - 14;
						continue;
					}
					if (selected >= len - 1) {
						selected = 0;
						start = 0;
					} else {
						selected += 1;
						if (start + 14 <= selected)
							start = selected - 14;
					}
					convertStringToUnicode(direntry[selected].name, displayedPath);
				} else if (event.key.code == sfKeyLeft) {
					if (menuSelected == 1) {
						if (cursorPos > 0)
							cursorPos -= 1;
						if (!event.key.shift)
							selectedText.x = cursorPos;
						selectedText.y = cursorPos;
					} else if (menuSelected == 0) {
						if (start > selected)
							start = selected;
						else if (start + 15 < selected)
							start = selected - 14;
						convertStringToUnicode(direntry[selected].name, displayedPath);
					}
				} else if (event.key.code == sfKeyRight) {
					if (menuSelected == 1) {
						if (cursorPos < strlen_unicode(displayedPath))
							cursorPos += 1;
						if (!event.key.shift)
							selectedText.x = cursorPos;
						selectedText.y = cursorPos;
					} else if (menuSelected == 0) {
						if (start > selected)
							start = selected;
						else if (start + 15 < selected)
							start = selected - 14;
						convertStringToUnicode(direntry[selected].name, displayedPath);
					}
				} else if (event.key.code == sfKeyHome) {
					if (menuSelected == 1) {
						cursorPos = 0;
						if (!event.key.shift)
							selectedText.x = cursorPos;
						selectedText.y = cursorPos;
					}
				} else if (event.key.code == sfKeyEnd) {
					if (menuSelected == 1) {
						cursorPos = strlen_unicode(displayedPath);
						if (!event.key.shift)
							selectedText.x = cursorPos;
						selectedText.y = cursorPos;
					}
				} else if (event.key.code == sfKeyPageUp) {
					if (menuSelected == 0) {
						if (selected <= 15) {
							selected = 0;
							start = 0;
						} else {
							selected -= 15;
							start = selected;
						}
						convertStringToUnicode(direntry[selected].name, displayedPath);
					} else {
						if (start <= 15)
							start = 0;
						else
							start -= 15;
					}
				} else if (event.key.code == sfKeyPageDown) {
					if (menuSelected == 0) {
						if (selected >= len - 15) {
							selected = len - 1;
							start = len - 15;
						} else {
							selected += 15;
							start = selected - 14;
						}
						convertStringToUnicode(direntry[selected].name, displayedPath);
					} else {
						if (start >= len - 30)
							start = len - 15;
						else
							start += 15;
					}
				} else if (event.key.code == sfKeySpace) {
					if (menuSelected == -1)
						menuSelected = 0;
					if (menuSelected == 0) {
						if (start > selected)
							start = selected;
						else if (start + 15 < selected)
							start = selected - 14;
						convertStringToUnicode(direntry[selected].name, displayedPath);
					}
				}
					#ifndef sfKeyBackspace
				else if (event.key.code == sfKeyBack) {
					#else
					else if (event.key.code == sfKeyBackspace) {
					#endif
					if (menuSelected == 1 && selectedText.x != selectedText.y) {
						for (int i = selectedText.x > selectedText.y ? selectedText.y : selectedText.x; displayedPath[i + abs(selectedText.x - selectedText.y) - 1]; i++)
							displayedPath[i] = displayedPath[i + abs(selectedText.x - selectedText.y)];
						selectedText.x = 0;
						selectedText.y = 0;
					} else if (menuSelected == 1 && cursorPos) {
						for (int i = --cursorPos; i == 0 || displayedPath[i - 1]; i++)
							displayedPath[i] = displayedPath[i + 1];
					}
				} else if (event.key.code == sfKeyDelete) {
					if (menuSelected == 1 && selectedText.x != selectedText.y) {
						for (int i = selectedText.x > selectedText.y ? selectedText.y : selectedText.x; displayedPath[i + abs(selectedText.x - selectedText.y) - 1]; i++)
							displayedPath[i] = displayedPath[i + abs(selectedText.x - selectedText.y)];
						selectedText.x = 0;
						selectedText.y = 0;
					} else if (menuSelected == 1 && cursorPos < strlen_unicode(displayedPath)) {
						for (int i = cursorPos; i == 0 || displayedPath[i - 1]; i++)
							displayedPath[i] = displayedPath[i + 1];
					}
				}
					#ifndef sfKeyEnter
				else if (event.key.code == sfKeyReturn) {
					#else
					else if (event.key.code == sfKeyEnter) {
					#endif
					struct stat	stats;

					if (menuSelected == 0) {
						path = concatf("%s/%s", realPath, direntry[selected].name);
						stats = direntry[selected].stats;
					} else if (menuSelected == 1) {
						#if defined _WIN32 || defined __WIN32 || defined __WIN32__
						if (strlen_unicode(displayedPath) < 2 || displayedPath[1] != ':')
							path = concatf("%s\\%s", realPath, convertUnicodeToString(displayedPath, buffer));
						#else
						if (displayedPath[0] != '/')
							path = concatf("%s/%s", realPath, convertUnicodeToString(displayedPath, buffer));
							#endif
						else
							path = strdup(convertUnicodeToString(displayedPath, buffer));
						if (stat(path, &stats) < 0) {
							buf = concatf("Error: %s: %s\n", path, strerror(errno));
							printf("Error: %s: %s\n", path, strerror(errno));
							dispMsg("Error", buf, 0);
							free(path);
							free(buf);
							continue;
						}
					}
					if (!realpath(path, realPath)) {
						free(path);
						buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
						printf("Error: %s: %s\n", realPath, strerror(errno));
						dispMsg("Error", buf, 0);
						free(buf);
						strcpy(realPath, buff);
						free(buff);
						continue;
					}
					convertStringToUnicode(path, displayedPath);
					selectedText.x = 0;
					selectedText.y = menuSelected == 1 ? strlen_unicode(displayedPath) : 0;
					cursorPos = selectedText.y;
					if ((stats.st_mode & S_IFMT) == S_IFDIR) {
						buff = strdup(realPath);
						free(path);
						buf = getDirEntries(realPath);
						if (!buf) {
							buf = concatf("Error: %s: %s\n", realPath, strerror(errno));
							printf("Error: %s: %s\n", realPath, strerror(errno));
							dispMsg("Error", buf, 0);
							free(buf);
							strcpy(realPath, buff);
							free(buff);
							continue;
						}
						for (len = 0; !direntry[len].isEnd; len++)
							free(direntry[len].name);
						free(direntry);
						direntry = buf;
						free(buff);
						for (len = 0; !direntry[len].isEnd; len++);
						selected = 0;
						start = 0;
						if (!direntry) {
							sfRectangleShape_destroy(rect);
							sfRenderWindow_destroy(window);
							return (NULL);
						}
					} else {
						for (len = 0; !direntry[len].isEnd; len++)
							free(direntry[len].name);
						free(direntry);
						sfRectangleShape_destroy(rect);
						sfRenderWindow_destroy(window);
						return (path);
					}
				}
			}
		}
		sfText_setColor(text, (sfColor){0, 0, 0, 255});
		sfRectangleShape_setPosition(rect, (sfVector2f){20, 20});
		sfRectangleShape_setSize(rect, (sfVector2f){560, 300});
		sfRectangleShape_setFillColor(rect, (sfColor){125, 125, 125, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		for (int i = start; !direntry[i].isEnd && i - start < 15; i++) {
			displayed = false;
			if (menuSelected == 0 && selected == i) {
				sfText_setColor(text, (sfColor){255, 255, 255, 255});
				sfRectangleShape_setFillColor(rect, (sfColor){0, 0, 255, 255});
				sfRectangleShape_setPosition(rect, (sfVector2f){20, 20 + (i - start) * 20});
				sfRectangleShape_setSize(rect, (sfVector2f){560, 20});
				sfRenderWindow_drawRectangleShape(window, rect, NULL);
			} else if (menuSelected == 0 && selected + 1 == i)
				sfText_setColor(text, (sfColor){0, 0, 0, 255});
			for (int j = 0; (j == 0 || sprites[j - 1].extensions) && !displayed; j++)
				for (int k = 0; !k || sprites[j].nbOfExtensions > k; k++) {
					if (!sprites[j].extensions) {
						sfSprite_setPosition(sprites[j].sprite, (sfVector2f){24, 22 + (i - start) * 20});
						sfRenderWindow_drawSprite(window, sprites[j].sprite, NULL);
						displayed = true;
					} else if (strcmp(sprites[j].extensions[k], "folder") == 0 && (direntry[i].stats.st_mode & S_IFMT) == S_IFDIR) {
						sfSprite_setPosition(sprites[j].sprite, (sfVector2f){24, 22 + (i - start) * 20});
						sfRenderWindow_drawSprite(window, sprites[j].sprite, NULL);
						displayed = true;
					} else if (
						strlen(direntry[i].name) >= strlen(sprites[j].extensions[k]) &&
						my_strcmp(
							&direntry[i].name[
								strlen(direntry[i].name) - strlen(sprites[j].extensions[k])
							],
							sprites[j].extensions[k]
						) == 0
						) {
						sfSprite_setPosition(sprites[j].sprite, (sfVector2f){24, 22 + (i - start) * 20});
						sfRenderWindow_drawSprite(window, sprites[j].sprite, NULL);
						displayed = true;
					}
				}
			sfText_setUnicodeString(text, convertStringToUnicode(direntry[i].name, bufferUnicode));
			sfText_setPosition(text, (sfVector2f){40, 20 + (i - start) * 20});
			sfRenderWindow_drawText(window, text, NULL);
		}
		sfText_setColor(text, (sfColor){0, 0, 0, 255});
		sfRectangleShape_setOutlineThickness(rect, 0);
		sfRectangleShape_setPosition(rect, (sfVector2f){0, 0});
		sfRectangleShape_setSize(rect, (sfVector2f){600, 20});
		sfRectangleShape_setFillColor(rect, (sfColor){200, 200, 200, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){0, 320});
		sfRectangleShape_setSize(rect, (sfVector2f){600, 80});
		sfRectangleShape_setFillColor(rect, (sfColor){200, 200, 200, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){20, 330});
		sfRectangleShape_setSize(rect, (sfVector2f){360, 30});
		sfRectangleShape_setFillColor(rect, (sfColor){255, 255, 255, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){580, 20});
		sfRectangleShape_setSize(rect, (sfVector2f){20, 300});
		sfRectangleShape_setFillColor(rect, (sfColor){200, 200, 200, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){569, 20});
		sfRectangleShape_setSize(rect, (sfVector2f){11, 300});
		sfRectangleShape_setFillColor(rect, (sfColor){75, 75, 75, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfText_setUnicodeString(text, displayedPath);
		sfText_setPosition(text, (sfVector2f){30, 335});
		if (menuSelected == 1) {
			if (selectedText.x > selectedText.y) {
				sfRectangleShape_setPosition(rect, (sfVector2f){getWordSize(displayedPath, 0, selectedText.y, font, 15) + 30, 335});
				sfRectangleShape_setSize(rect, (sfVector2f){getWordSize(displayedPath, selectedText.y, selectedText.x, font, 15), 20});
			} else {
				sfRectangleShape_setPosition(rect, (sfVector2f){getWordSize(displayedPath, 0, selectedText.x, font, 15) + 30, 335});
				sfRectangleShape_setSize(rect, (sfVector2f){getWordSize(displayedPath, selectedText.x, selectedText.y, font, 15), 20});
			}
			sfRectangleShape_setFillColor(rect, (sfColor){0, 0, 255, 255});
			sfRenderWindow_drawRectangleShape(window, rect, NULL);
			if (time(NULL) % 2) {
				sfRectangleShape_setOutlineThickness(rect, 1);
				sfRectangleShape_setPosition(rect, (sfVector2f){getWordSize(displayedPath, 0, cursorPos, font, 15) + 30, 335});
				sfRectangleShape_setSize(rect, (sfVector2f){0, 20});
				sfRectangleShape_setFillColor(rect, (sfColor){0, 0, 0, 255});
				sfRenderWindow_drawRectangleShape(window, rect, NULL);
				sfRectangleShape_setOutlineThickness(rect, 0);
			}
		}
		sfRenderWindow_drawText(window, text, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){380, 320});
		sfRectangleShape_setSize(rect, (sfVector2f){220, 80});
		sfRectangleShape_setFillColor(rect, (sfColor){200, 200, 200, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		if (len > 15) {
			sfRectangleShape_setPosition(rect, (sfVector2f){570, (float)(300 * start * len - 6000 * start) / ((len - 15) * len) + 20});
			sfRectangleShape_setSize(rect, (sfVector2f){10, 6000 / len + 1});
			sfRectangleShape_setFillColor(rect, (sfColor){255, 255, 255, 255});
			sfRenderWindow_drawRectangleShape(window, rect, NULL);
		}
		sfRectangleShape_setOutlineThickness(rect, 1);
		sfRectangleShape_setPosition(rect, (sfVector2f){20, 330});
		sfRectangleShape_setSize(rect, (sfVector2f){360, 30});
		sfRectangleShape_setFillColor(rect, (sfColor){255, 255, 255, 0});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){20, 20});
		sfRectangleShape_setSize(rect, (sfVector2f){560, 300});
		sfRectangleShape_setFillColor(rect, (sfColor){200, 200, 200, 0});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfText_setColor(text, (sfColor){0, 0, 0, 255});
		sfText_setUnicodeString(text, convertStringToUnicode(realPath, bufferUnicode));
		sfText_setPosition(text, (sfVector2f){10, 0});
		sfRectangleShape_setPosition(rect, (sfVector2f){400, 330});
		sfRectangleShape_setSize(rect, (sfVector2f){60, 30});
		sfRectangleShape_setFillColor(rect, (sfColor){255, 255, 255, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfText_setColor(text, (sfColor){0, 0, 0, 255});
		sfText_setString(text, "Open");
		sfText_setPosition(text, (sfVector2f){410, 335});
		sfRenderWindow_drawText(window, text, NULL);
		sfRectangleShape_setPosition(rect, (sfVector2f){470, 330});
		sfRectangleShape_setSize(rect, (sfVector2f){60, 30});
		sfRectangleShape_setFillColor(rect, (sfColor){255, 255, 255, 255});
		sfRenderWindow_drawRectangleShape(window, rect, NULL);
		sfText_setColor(text, (sfColor){0, 0, 0, 255});
		sfText_setString(text, "Cancel");
		sfText_setPosition(text, (sfVector2f){475, 335});
		sfRenderWindow_drawText(window, text, NULL);
		sfRenderWindow_display(window);
	}
	sfRectangleShape_destroy(rect);
	sfRenderWindow_destroy(window);
	for (len = 0; !direntry[len].isEnd; len++)
		free(direntry[len].name);
	free(direntry);
	return (NULL);
}
