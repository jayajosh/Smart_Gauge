#include "Map_Driver.h"
#include "SD_Card.h"
#include <SD_MMC.h>

static lv_obj_t* mapImg = NULL;
static uint8_t* tileBuffer = NULL;
static lv_img_dsc_t tileDsc;

bool Map_Init(lv_obj_t * parent)
{
  tileBuffer = (uint8_t*)ps_malloc(256 * 256 * 2);

  if (!tileBuffer) {
    Serial.println("Map tile buffer allocation failed");
    return false;
  }

  mapImg = lv_img_create(parent);
  lv_obj_center(mapImg);

  return true;
}

bool Map_LoadTile(const char* path)
{
  File f = SD_MMC.open(path, FILE_READ);

  size_t expectedSize = 256 * 256 * 2;
  size_t fileSize = f.size();

  if (fileSize == expectedSize + 12) {
    Serial.println("Tile has 12-byte header, skipping it");
    f.seek(12);
  }
  else if (fileSize != expectedSize) {
    Serial.print("Wrong tile size: ");
    Serial.println(fileSize);
    f.close();
    return false;
  }

  f.read(tileBuffer, expectedSize);
  f.close();

  tileDsc.header.always_zero = 0;
  tileDsc.header.w = 256;
  tileDsc.header.h = 256;
  tileDsc.header.cf = LV_IMG_CF_TRUE_COLOR;
  tileDsc.data_size = 256 * 256 * 2;
  tileDsc.data = tileBuffer;

  lv_img_set_src(mapImg, &tileDsc);
  lv_obj_center(mapImg);

  Serial.println("Tile loaded");
  return true;
}

void Map_ShowTestTile(void)
{
  Map_LoadTile("/tiles/16/32363/21355.bin");
}