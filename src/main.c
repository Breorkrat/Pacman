#include "raylib.h"
#include <stdlib.h>

#include "resource_dir.h"	// utility header for SearchAndSetResourceDir

#define SPRITE_SIZE 8
#define TILE_SIZE 16

// Bitmasking
#define NORTH 1
#define EAST 2
#define SOUTH 4
#define WEST 8

typedef struct {
  int width;
  int height;
  char name[30];
} Window;

typedef struct {
  int width;
  int height;
  int* map;
} MapInfo;

// Bitfield for connecting textures
typedef struct {
  unsigned char n   : 1;
  unsigned char ne  : 1;
  unsigned char e   : 1;
  unsigned char se  : 1;
  unsigned char s   : 1;
  unsigned char sw  : 1;
  unsigned char w   : 1;
  unsigned char nw  : 1;
} NeighborFlags;

typedef enum {WALL = 1, PELLET, SPELLET, SPACE} Cell;
typedef enum {OUT_CORNER = 0, HORIZONTAL, VERTICAL, IN_CORNER, EMPTY} WallQuadrant;

int getMapCell(MapInfo* map, int x, int y){
  if (x < 0 || x >= map->width || y < 0 || y >= map->height) return 0;
  return map->map[y * map->width + x];
}

void setMapCell(MapInfo* map, int x, int y, int value){
  if (x >= 0 && x < map->width && y >= 0 && y < map->height)
    map->map[y * map->width + x] = value;
}

NeighborFlags getNeighbors(MapInfo* map, int x, int y){
  NeighborFlags nflags = {0};
  if (getMapCell(map, x, y-1) == WALL)   nflags.n = 1;
  if (getMapCell(map, x+1, y-1) == WALL) nflags.ne = 1;
  if (getMapCell(map, x+1, y) == WALL)   nflags.e = 1;
  if (getMapCell(map, x+1, y+1) == WALL) nflags.se = 1;
  if (getMapCell(map, x, y+1) == WALL)   nflags.s = 1;
  if (getMapCell(map, x-1, y+1) == WALL) nflags.sw = 1;
  if (getMapCell(map, x-1, y) == WALL)   nflags.w = 1;
  if (getMapCell(map, x-1, y-1) == WALL) nflags.nw = 1;
  return nflags;
}

void drawWallSubtile(NeighborFlags nf, int qX, int qY, Texture2D texture, Vector2 screenPos){
  // Flags for neighbors horizontally, vertically or diagonally
  unsigned char h = 0, v = 0, d = 0;

  // Pick the 3 neighbors of the current quadrant
  if (!qX) {
    if (!qY) {h = nf.w; v = nf.n; d = nf.nw;} // For example, x=y=0 is the top left quadrant, selects w horizontally, n vertically and nw diagonally
    else     {h = nf.w; v = nf.s; d = nf.sw;}
  } else {
    if (!qY) {h = nf.e; v = nf.n; d = nf.ne;}
    else     {h = nf.e; v = nf.s; d = nf.se;}
  }
  
  // Selects what sprite to be drawn
  WallQuadrant idx = EMPTY;
  if (!h) {
    if (!v)      idx = OUT_CORNER;
    else         idx = VERTICAL;
  } else if (!v) idx = HORIZONTAL;
  else if (!d)   idx = IN_CORNER;
  
  // If empty, it's not needed to draw anything
  if (idx == EMPTY) return;

  Rectangle src = { (float)idx * SPRITE_SIZE, 0, (float)SPRITE_SIZE, (float)SPRITE_SIZE };
  // Mirrors the sprite if not on the top left
  if (qX == 1) src.width *= -1;
  if (qY == 1) src.height *= -1;

  Rectangle dest = { screenPos.x + qX * (TILE_SIZE/2), screenPos.y + qY * (TILE_SIZE/2), TILE_SIZE/2, TILE_SIZE/2 };
  DrawTexturePro(texture, src, dest, (Vector2){0,0}, 0, WHITE);
}

// Loads map from text file. Returns 1 on fail
void loadMap(MapInfo* mapInfo, char* filename){
  char* fileText = LoadFileText(filename);

  int tempWidth = 0;
  // Determina dimensões
  for (int i = 0; fileText[i] != '\0'; i++) {
    // Ignore Window's \n\r
    if (fileText[i] == '\r') continue;
    if (fileText[i] == '\n'){
      // Uses the largest line as Width
      if (tempWidth > mapInfo->width) mapInfo->width = tempWidth;
      tempWidth = 0;
      mapInfo->height++;
    } else 
      tempWidth++;
  }
  // If tempWidth isn't 0, line didn't end with a newline, so it's processed here
  if (tempWidth > 0) {
    if (tempWidth > mapInfo->width) mapInfo->width = tempWidth;
    mapInfo->height++;
  }


  mapInfo->map = (int*)calloc(mapInfo->width*mapInfo->height, sizeof(int));

  // Process text into map data
  int x = 0, y = 0;
  for (int i = 0; fileText[i] != '\0'; i++) {
    if (fileText[i] == '\r') continue; // Ignore Windows's \n\r
    if (fileText[i] == '\n') {
        x = 0; y++;
        continue;
    }
    switch(fileText[i]){
      case '#': setMapCell(mapInfo, x, y, 1); break;
      case '.': setMapCell(mapInfo, x, y, 2); break;
      case 'o': setMapCell(mapInfo, x, y, 3); break;
      case ' ': setMapCell(mapInfo, x, y, 4); break;
      default: setMapCell(mapInfo, x, y, 0);
    }
    x++;
  }
  UnloadFileText(fileText);
}

int main ()
{
	// Tell the window to use vsync and work on high DPI displays
	SetConfigFlags(FLAG_VSYNC_HINT | FLAG_WINDOW_HIGHDPI);
  MapInfo mapInfo = {0};
  loadMap(&mapInfo, "./levels/test.txt");

  Window window = {mapInfo.width, mapInfo.height, "Pacman"};

  InitWindow(window.width, window.height, window.name);
  SetTargetFPS(60);

  // Load spritesheet, removing background
  Image spriteSheetImg = LoadImage("resources/spritesheet.png");
  ImageColorReplace(&spriteSheetImg, (Color){255, 0, 255, 255}, BLANK);
  ImageColorReplace(&spriteSheetImg, (Color){16, 16, 16, 255}, BLANK);
  Texture2D spriteSheet = LoadTextureFromImage(spriteSheetImg);
  UnloadImage(spriteSheetImg);

  SetTextureFilter(spriteSheet, TEXTURE_FILTER_POINT);


  SetWindowSize(mapInfo.width*TILE_SIZE, mapInfo.height*TILE_SIZE);

  while (!WindowShouldClose()) {
    BeginDrawing();
    ClearBackground(BLACK);

    for (int y = 0; y < mapInfo.height; y++){
      for (int x = 0; x < mapInfo.width; x++){
        Cell cell = getMapCell(&mapInfo, x, y);
        
        Rectangle dest = {x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE};
        switch (cell) {
          case WALL: {
            Vector2 pos = { (float)x * TILE_SIZE, (float)y * TILE_SIZE };
            NeighborFlags nf = getNeighbors(&mapInfo, x, y);
            // Draw the 4 quadrants of the wall
            drawWallSubtile(nf, 0, 0, spriteSheet, pos);
            drawWallSubtile(nf, 1, 0, spriteSheet, pos);
            drawWallSubtile(nf, 0, 1, spriteSheet, pos);
            drawWallSubtile(nf, 1, 1, spriteSheet, pos);
            break;
          }

          case PELLET:  {
            Rectangle source = {0, SPRITE_SIZE, SPRITE_SIZE, SPRITE_SIZE};
            DrawTexturePro(spriteSheet, source, dest, (Vector2){0,0}, 0.0f, WHITE);
            break;
          }

          case SPELLET: {
            Rectangle source = {SPRITE_SIZE, SPRITE_SIZE, SPRITE_SIZE, SPRITE_SIZE};
            DrawTexturePro(spriteSheet, source, dest, (Vector2){0,0}, 0.0f, WHITE);
            break;
          }

          case SPACE: break;

          default: DrawRectangle(x*TILE_SIZE, y*TILE_SIZE, TILE_SIZE, TILE_SIZE, RED);
        }
      }
    }
    //DrawFPS(10, 10);
    EndDrawing();
  }
  free(mapInfo.map);
  CloseWindow();
  return 0;
}
