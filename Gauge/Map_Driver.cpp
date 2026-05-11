#include "Map_Driver.h"
#include <SD_MMC.h>

#define TILE_SIZE 256
#define GRID_SIZE 5
#define GRID_CENTER 2
#define TILE_BYTES (TILE_SIZE * TILE_SIZE * 2)

static lv_obj_t* tileImgs[GRID_SIZE][GRID_SIZE];
static uint8_t* tileBuffers[GRID_SIZE][GRID_SIZE];
static lv_img_dsc_t tileDsc[GRID_SIZE][GRID_SIZE];

static lv_obj_t* mapParent = NULL;
static lv_obj_t * gpsMarker = NULL;

static int currentZoom = 16;
static int currentCenterX = 32363; //TODO Make gps?
static int currentCenterY = 21355;

void Map_CreateGpsMarker(void);

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
  Map_CreateGpsMarker();
  return true;
}

//  --- Map directional shifting ---

void RefreshDescriptors()
{
  for (int row = 0; row < GRID_SIZE; row++) {
    for (int col = 0; col < GRID_SIZE; col++) {
      tileDsc[row][col].data = tileBuffers[row][col];
      lv_img_set_src(tileImgs[row][col], &tileDsc[row][col]);
    }
  }
  if (gpsMarker != NULL) {
    lv_obj_move_foreground(gpsMarker);
  }
}

void Map_ReloadColumn(int col)
{
  for (int row = 0; row < GRID_SIZE; row++) {
    int tileX = currentCenterX + (col - GRID_CENTER);
    int tileY = currentCenterY + (row - GRID_CENTER);

    char path[80];
    snprintf(path, sizeof(path), "/tiles/%d/%d/%d.bin", currentZoom, tileX, tileY);

    loadTileToBuffer(path, tileBuffers[row][col]);
    lv_img_set_src(tileImgs[row][col], &tileDsc[row][col]);
  }
}

void Map_ReloadRow(int row)
{
  for (int col = 0; col < GRID_SIZE; col++) {

    int tileX = currentCenterX + (col - GRID_CENTER);
    int tileY = currentCenterY + (row - GRID_CENTER);

    char path[80];
    snprintf(path, sizeof(path), "/tiles/%d/%d/%d.bin",
             currentZoom, tileX, tileY);

    loadTileToBuffer(path, tileBuffers[row][col]);

    tileDsc[row][col].data = tileBuffers[row][col];
    lv_img_set_src(tileImgs[row][col], &tileDsc[row][col]);
  }
}

void Map_ShiftRight()
{
  lv_obj_t* oldImgs[GRID_SIZE];
  uint8_t* oldBuffers[GRID_SIZE];

  for (int row = 0; row < GRID_SIZE; row++) {
    oldImgs[row] = tileImgs[row][0];
    oldBuffers[row] = tileBuffers[row][0];
  }

  for (int col = 0; col < GRID_SIZE - 1; col++) {
    for (int row = 0; row < GRID_SIZE; row++) {
      tileImgs[row][col] = tileImgs[row][col + 1];
      tileBuffers[row][col] = tileBuffers[row][col + 1];
    }
  }

  for (int row = 0; row < GRID_SIZE; row++) {
    tileImgs[row][GRID_SIZE - 1] = oldImgs[row];
    tileBuffers[row][GRID_SIZE - 1] = oldBuffers[row];
  }

  Map_ReloadColumn(GRID_SIZE - 1);
  RefreshDescriptors();
}

void Map_ShiftLeft()
 {
  lv_obj_t* oldImgs[GRID_SIZE];
  uint8_t* oldBuffers[GRID_SIZE];

  for (int row = 0; row < GRID_SIZE; row++) {
    oldImgs[row] = tileImgs[row][GRID_SIZE - 1];
    oldBuffers[row] = tileBuffers[row][GRID_SIZE - 1];
  }

  for (int col = GRID_SIZE - 1; col > 0; col--) {
    for (int row = 0; row < GRID_SIZE; row++) {
      tileImgs[row][col] = tileImgs[row][col - 1];
      tileBuffers[row][col] = tileBuffers[row][col - 1];
    }
  }

  for (int row = 0; row < GRID_SIZE; row++) {
    tileImgs[row][0] = oldImgs[row];
    tileBuffers[row][0] = oldBuffers[row];
  }


  Map_ReloadColumn(0);
  RefreshDescriptors();
}

void Map_ShiftDown()
{
  lv_obj_t* oldImgs[GRID_SIZE];
  uint8_t* oldBuffers[GRID_SIZE];

  for (int col = 0; col < GRID_SIZE; col++) {
    oldImgs[col] = tileImgs[0][col];
    oldBuffers[col] = tileBuffers[0][col];
  }

  for (int row = 0; row < GRID_SIZE - 1; row++) {
    for (int col = 0; col < GRID_SIZE; col++) {
      tileImgs[row][col] = tileImgs[row + 1][col];
      tileBuffers[row][col] = tileBuffers[row + 1][col];
    }
  }

  for (int col = 0; col < GRID_SIZE; col++) {
    tileImgs[GRID_SIZE - 1][col] = oldImgs[col];
    tileBuffers[GRID_SIZE - 1][col] = oldBuffers[col];
  }

  Map_ReloadRow(GRID_SIZE - 1);
  RefreshDescriptors();
}

void Map_ShiftUp()
{
  lv_obj_t* oldImgs[GRID_SIZE];
  uint8_t* oldBuffers[GRID_SIZE];

  for (int col = 0; col < GRID_SIZE; col++) {
    oldImgs[col] = tileImgs[GRID_SIZE - 1][col];
    oldBuffers[col] = tileBuffers[GRID_SIZE - 1][col];
  }

  for (int row = GRID_SIZE - 1; row > 0; row--) {
    for (int col = 0; col < GRID_SIZE; col++) {
      tileImgs[row][col] = tileImgs[row - 1][col];
      tileBuffers[row][col] = tileBuffers[row - 1][col];
    }
  }

  for (int col = 0; col < GRID_SIZE; col++) {
    tileImgs[0][col] = oldImgs[col];
    tileBuffers[0][col] = oldBuffers[col];
  }

  Map_ReloadRow(0);
  RefreshDescriptors();
}

//  --- Set map shift --

void Map_SetOffset(int offsetX, int offsetY)
{
  int screenCenterX = 240;
  int screenCenterY = 240;

  for (int row = 0; row < GRID_SIZE; row++) {
    for (int col = 0; col < GRID_SIZE; col++) {

      int drawX = screenCenterX + ((col - GRID_CENTER) * TILE_SIZE) - (TILE_SIZE / 2) - offsetX;
      int drawY = screenCenterY + ((row - GRID_CENTER) * TILE_SIZE) - (TILE_SIZE / 2) - offsetY;

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
    Map_ShiftRight();
  }

  while (offsetX < 0) {
    offsetX += TILE_SIZE;
    currentCenterX -= 1;
    Map_ShiftLeft();;
  }

  while (offsetY >= TILE_SIZE) {
    offsetY -= TILE_SIZE;
    currentCenterY += 1;
    Map_ShiftDown();
  }

  while (offsetY < 0) {
    offsetY += TILE_SIZE;
    currentCenterY -= 1;
    Map_ShiftUp();
  }

  Map_SetOffset(offsetX, offsetY);
}

void Map_ShowTileGrid(int zoom, int centerX, int centerY)
{
  int screenCenterX = 240;
  int screenCenterY = 240;

  for (int row = 0; row < GRID_SIZE; row++) {
    for (int col = 0; col < GRID_SIZE; col++) {

      int tileX = centerX + (col - GRID_CENTER);
      int tileY = centerY + (row - GRID_CENTER);

      char path[80];
      snprintf(path, sizeof(path), "/tiles/%d/%d/%d.bin", zoom, tileX, tileY);

      if (loadTileToBuffer(path, tileBuffers[row][col])) {
        lv_img_set_src(tileImgs[row][col], &tileDsc[row][col]);
      }

      int drawX = screenCenterX + ((col - GRID_CENTER) * TILE_SIZE) - (TILE_SIZE / 2);
      int drawY = screenCenterY + ((row - GRID_CENTER) * TILE_SIZE) - (TILE_SIZE / 2);

      lv_obj_set_pos(tileImgs[row][col], drawX, drawY);
      lv_obj_move_foreground(tileImgs[row][col]);
    }
  }
  if (gpsMarker != NULL) {
    lv_obj_move_foreground(gpsMarker);
  }
}


//  --- Centre Marker ---
void Map_CreateGpsMarker()
{
  gpsMarker = lv_label_create(mapParent);

  lv_label_set_text(gpsMarker, "▲");

  lv_obj_set_style_text_color(gpsMarker,
                              lv_color_hex(0xFF3333),
                              LV_PART_MAIN);

  lv_obj_set_style_text_font(gpsMarker,
                             &lv_font_montserrat_14,
                             LV_PART_MAIN);

  lv_obj_align(gpsMarker, LV_ALIGN_CENTER, 0, -4);

  lv_obj_move_foreground(gpsMarker);
}
