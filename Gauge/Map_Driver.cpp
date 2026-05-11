#include "Map_Driver.h"
#include <SD_MMC.h>

#define TILE_SIZE 256
#define GRID_SIZE 3
#define TILE_BYTES (TILE_SIZE * TILE_SIZE * 2)

static lv_obj_t* tileImgs[GRID_SIZE][GRID_SIZE];
static uint8_t* tileBuffers[GRID_SIZE][GRID_SIZE];
static lv_img_dsc_t tileDsc[GRID_SIZE][GRID_SIZE];

static lv_obj_t* mapParent = NULL;

static int currentZoom = 16;
static int currentCenterX = 32363; //TODO Make gps?
static int currentCenterY = 21355;

bool loadTileToBuffer(const char* path, uint8_t* buffer)
{
  File f = SD_MMC.open(path, FILE_READ);

  if (!f) {
    Serial.print("Tile not found: ");
    Serial.println(path);
    return false;
  }

  size_t fileSize = f.size();

  if (fileSize == TILE_BYTES + 12) {
    f.seek(12); // converter added 12-byte header
  }
  else if (fileSize != TILE_BYTES) {
    Serial.print("Wrong tile size: ");
    Serial.print(path);
    Serial.print(" size=");
    Serial.println(fileSize);
    f.close();
    return false;
  }

  size_t bytesRead = f.read(buffer, TILE_BYTES);
  f.close();

  return bytesRead == TILE_BYTES;
}

bool Map_Init(lv_obj_t * parent)
{
  mapParent = parent;

  for (int row = 0; row < GRID_SIZE; row++) {
    for (int col = 0; col < GRID_SIZE; col++) {

      tileBuffers[row][col] = (uint8_t*)ps_malloc(TILE_BYTES);

      if (!tileBuffers[row][col]) {
        Serial.println("Failed to allocate tile buffer");
        return false;
      }

      tileDsc[row][col].header.always_zero = 0;
      tileDsc[row][col].header.w = TILE_SIZE;
      tileDsc[row][col].header.h = TILE_SIZE;
      tileDsc[row][col].header.cf = LV_IMG_CF_TRUE_COLOR;
      tileDsc[row][col].data_size = TILE_BYTES;
      tileDsc[row][col].data = tileBuffers[row][col];

      tileImgs[row][col] = lv_img_create(mapParent);
      lv_img_set_src(tileImgs[row][col], &tileDsc[row][col]);
      lv_obj_clear_flag(tileImgs[row][col], LV_OBJ_FLAG_SCROLLABLE);
    }
  }

  Serial.println("Map grid initialised");
  return true;
}

void Map_SetOffset(int offsetX, int offsetY)
{
  int screenCenterX = 240;
  int screenCenterY = 240;

  for (int row = 0; row < GRID_SIZE; row++) {
    for (int col = 0; col < GRID_SIZE; col++) {

      int drawX = screenCenterX + ((col - 1) * TILE_SIZE) - (TILE_SIZE / 2) - offsetX;
      int drawY = screenCenterY + ((row - 1) * TILE_SIZE) - (TILE_SIZE / 2) - offsetY;

      lv_obj_set_pos(tileImgs[row][col], drawX, drawY);
    }
  }
}

void Map_MovePixels(int dx, int dy)
{
  static int offsetX = 0;
  static int offsetY = 0;

  offsetX += dx;
  offsetY += dy;

  while (offsetX >= TILE_SIZE) {
    offsetX -= TILE_SIZE;
    currentCenterX += 1;
    Map_ShowTileGrid(currentZoom, currentCenterX, currentCenterY);
  }

  while (offsetX < 0) {
    offsetX += TILE_SIZE;
    currentCenterX -= 1;
    Map_ShowTileGrid(currentZoom, currentCenterX, currentCenterY);
  }

  while (offsetY >= TILE_SIZE) {
    offsetY -= TILE_SIZE;
    currentCenterY += 1;
    Map_ShowTileGrid(currentZoom, currentCenterX, currentCenterY);
  }

  while (offsetY < 0) {
    offsetY += TILE_SIZE;
    currentCenterY -= 1;
    Map_ShowTileGrid(currentZoom, currentCenterX, currentCenterY);
  }

  Map_SetOffset(offsetX, offsetY);
}

void Map_ShowTileGrid(int zoom, int centerX, int centerY)
{
  int screenCenterX = 240;
  int screenCenterY = 240;

  for (int row = 0; row < GRID_SIZE; row++) {
    for (int col = 0; col < GRID_SIZE; col++) {

      int tileX = centerX + (col - 1);
      int tileY = centerY + (row - 1);

      char path[80];
      snprintf(path, sizeof(path), "/tiles/%d/%d/%d.bin", zoom, tileX, tileY);

      if (loadTileToBuffer(path, tileBuffers[row][col])) {
        lv_img_set_src(tileImgs[row][col], &tileDsc[row][col]);
      }

      int drawX = screenCenterX + ((col - 1) * TILE_SIZE) - (TILE_SIZE / 2);
      int drawY = screenCenterY + ((row - 1) * TILE_SIZE) - (TILE_SIZE / 2);

      lv_obj_set_pos(tileImgs[row][col], drawX, drawY);
      lv_obj_move_foreground(tileImgs[row][col]);
    }
  }

  Serial.println("3x3 tile grid loaded");
}