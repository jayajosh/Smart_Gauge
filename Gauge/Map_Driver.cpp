#include "Map_Driver.h"
#include <SD_MMC.h>
#include <math.h>

#define TILE_SIZE 256
#define GRID_SIZE 5
#define GRID_CENTER 2
#define TILE_BYTES (TILE_SIZE * TILE_SIZE * 2)
#define MAX_ROUTE_POINTS 1000
#define ROUTE_DRAW_POINTS 80

static lv_obj_t* tileImgs[GRID_SIZE][GRID_SIZE];
static uint8_t* tileBuffers[GRID_SIZE][GRID_SIZE];
static lv_img_dsc_t tileDsc[GRID_SIZE][GRID_SIZE];

static lv_obj_t* mapParent = NULL;
static lv_obj_t * gpsMarker = NULL;

static int currentZoom = 16;
static int currentCenterX = 32375;
static int currentCenterY = 21350;

struct RoutePoint {
  float lat;
  float lon;
};

static RoutePoint routePoints[MAX_ROUTE_POINTS];

static lv_point_t routeScreenPoints[ROUTE_DRAW_POINTS];

static int routePointCount = 0;
static int currentRouteIndex = 0;
static lv_obj_t* routeLine = NULL;

static int currentOffsetX = 0;
static int currentOffsetY = 0;

void Map_CreateGpsMarker(void);
void Map_UpdateRouteLine(void);
void latLonToTilePixel(double lat, double lon, int* tileX, int* tileY, int* pixelX, int* pixelY);
int Map_FindClosestRoutePoint(double lat, double lon);

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

  routeLine = lv_line_create(mapParent);
  lv_obj_set_style_line_width(routeLine, 5, LV_PART_MAIN);
  lv_obj_set_style_line_color(routeLine, lv_color_hex(0x00AAFF), LV_PART_MAIN);
  lv_obj_set_style_line_rounded(routeLine, true, LV_PART_MAIN);
  lv_obj_move_foreground(routeLine);
  
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

  currentOffsetX = offsetX;
  currentOffsetY = offsetY;

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
   gpsMarker = lv_obj_create(mapParent);

  lv_obj_set_size(gpsMarker, 18, 18);

  lv_obj_set_style_radius(gpsMarker, LV_RADIUS_CIRCLE, LV_PART_MAIN);

  lv_obj_set_style_bg_color(
    gpsMarker,
    lv_color_hex(0xa8a4a1),
    LV_PART_MAIN
  );

  lv_obj_set_style_bg_opa(
    gpsMarker,
    LV_OPA_COVER,
    LV_PART_MAIN
  );

  lv_obj_set_style_border_width(gpsMarker, 3, LV_PART_MAIN);

  lv_obj_set_style_border_color(
    gpsMarker,
    lv_color_hex(0xFFFFFF),
    LV_PART_MAIN
  );

  lv_obj_clear_flag(gpsMarker, LV_OBJ_FLAG_SCROLLABLE);

  lv_obj_align(gpsMarker, LV_ALIGN_CENTER, 0, 0);

  lv_obj_move_foreground(gpsMarker);
}

//  --- GPS Updates ---
void Map_UpdateFromGPS(double lat, double lon)
{
  if (lat == 0.0 && lon == 0.0) return;

  double latRad = lat * DEG_TO_RAD;
  double n = pow(2.0, currentZoom);

  double xFloat = (lon + 180.0) / 360.0 * n;
  double yFloat = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / PI) / 2.0 * n;

  int tileX = (int)xFloat;
  int tileY = (int)yFloat;

  int pixelX = (int)((xFloat - tileX) * TILE_SIZE);
  int pixelY = (int)((yFloat - tileY) * TILE_SIZE);

  if (tileX != currentCenterX || tileY != currentCenterY) {
    currentCenterX = tileX;
    currentCenterY = tileY;
    Map_ShowTileGrid(currentZoom, currentCenterX, currentCenterY);
  }

  int offsetX = pixelX - (TILE_SIZE / 2);
  int offsetY = pixelY - (TILE_SIZE / 2);

  Map_SetOffset(offsetX, offsetY);

  if (routePointCount > 0) {
    currentRouteIndex = Map_FindClosestRoutePoint(lat, lon);
    Map_UpdateRouteLine();
  }
}

//  --- Routing ---

int Map_FindClosestRoutePoint(double lat, double lon)
{
  if (routePointCount == 0) return 0;

  int bestIndex = currentRouteIndex;
  double bestDist = 999999999.0;

  int searchStart = max(0, currentRouteIndex - 10);
  int searchEnd = min(routePointCount, currentRouteIndex + 80);

  for (int i = searchStart; i < searchEnd; i++) {
    double dLat = routePoints[i].lat - lat;
    double dLon = routePoints[i].lon - lon;

    double dist = (dLat * dLat) + (dLon * dLon);

    if (dist < bestDist) {
      bestDist = dist;
      bestIndex = i;
    }
  }

  return bestIndex;
}

void latLonToTilePixel(double lat, double lon, int* tileX, int* tileY, int* pixelX, int* pixelY)
{
  double latRad = lat * DEG_TO_RAD;
  double n = pow(2.0, currentZoom);

  double xFloat = (lon + 180.0) / 360.0 * n;
  double yFloat = (1.0 - log(tan(latRad) + 1.0 / cos(latRad)) / PI) / 2.0 * n;

  *tileX = (int)xFloat;
  *tileY = (int)yFloat;

  *pixelX = (int)((xFloat - *tileX) * TILE_SIZE);
  *pixelY = (int)((yFloat - *tileY) * TILE_SIZE);
}

void Map_UpdateRouteLine()
{
  if (routePointCount <= 1 || routeLine == NULL) return;

  int pointsToDraw = min(ROUTE_DRAW_POINTS, routePointCount - currentRouteIndex);

  if (pointsToDraw <= 1) return;

  for (int i = 0; i < pointsToDraw; i++) {
    int routeIndex = currentRouteIndex + i;

    int tileX, tileY, pixelX, pixelY;

    latLonToTilePixel(
      routePoints[routeIndex].lat,
      routePoints[routeIndex].lon,
      &tileX,
      &tileY,
      &pixelX,
      &pixelY
    );

    int screenX = 240 + ((tileX - currentCenterX) * TILE_SIZE) + pixelX - 128 - currentOffsetX;
    int screenY = 240 + ((tileY - currentCenterY) * TILE_SIZE) + pixelY - 128 - currentOffsetY;

    routeScreenPoints[i].x = screenX;
    routeScreenPoints[i].y = screenY;
  }

  lv_line_set_points(routeLine, routeScreenPoints, pointsToDraw);
  lv_obj_move_foreground(routeLine);

  if (gpsMarker != NULL) {
    lv_obj_move_foreground(gpsMarker);
  }
}

void Map_LoadRouteFromString(String routeString)
{
  routePointCount = 0;

  int start = 0;

  while (start < routeString.length() && routePointCount < MAX_ROUTE_POINTS) {
    int semi = routeString.indexOf(';', start);

    if (semi == -1) {
      semi = routeString.length();
    }

    String pair = routeString.substring(start, semi);
    int comma = pair.indexOf(',');

    if (comma > 0) {
      String latStr = pair.substring(0, comma);
      String lonStr = pair.substring(comma + 1);

      routePoints[routePointCount].lat = latStr.toFloat();
      routePoints[routePointCount].lon = lonStr.toFloat();

      routePointCount++;
    }

    start = semi + 1;
  }

  Serial.print("Parsed route points: ");
  Serial.println(routePointCount);


  currentRouteIndex = 0;
}
