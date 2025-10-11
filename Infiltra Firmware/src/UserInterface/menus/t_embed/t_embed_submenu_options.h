#ifndef T_EMBED_SUBMENU_OPTIONS_H
#define T_EMBED_SUBMENU_OPTIONS_H

#include <TFT_eSPI.h>
#include <Arduino.h>
#include "../submenu_options.h"

// T-Embed specific submenu option functions
void initTEmbedSubmenuOptions(TFT_eSPI* display);
void drawTEmbedSubmenuOptions();
void drawTEmbedSubmenuOption(int index, const char* text, bool selected, int y_position);

// T-Embed optimized positioning and sizing
#define T_EMBED_SUBMENU_START_Y 50
#define T_EMBED_SUBMENU_ITEM_HEIGHT 20
#define T_EMBED_SUBMENU_TEXT_X 15
#define T_EMBED_SUBMENU_MAX_VISIBLE 12  // More items visible on 320px height

// T-Embed specific option layout functions
void setTEmbedSubmenuType(SubmenuType type);
void nextTEmbedSubmenuOption();
void previousTEmbedSubmenuOption();
int getTEmbedSubmenuOptionIndex();
String getTEmbedSubmenuOptionText();

// T-Embed display utilities for submenus
void drawTEmbedSubmenuScrollIndicator(int current, int total);
void drawTEmbedSubmenuSelection(int y_position);
void clearTEmbedSubmenuSelection(int y_position);

#endif