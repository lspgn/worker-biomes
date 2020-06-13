#include <emscripten.h>
#include <inttypes.h>
#include <stdio.h>

#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ASSERT(x)
#define STBIR_ASSERT(x)
#define STBIW_ASSERT(x)

#include "cubiomes/finders.h"
#include "cubiomes/generator.h"
#include "cubiomes/util.h"

#define STB_IMAGE_IMPLEMENTATION
#include "stb/stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb/stb_image_write.h"

// Basic types and decls.
typedef signed char int8_t;
typedef unsigned char uint8_t;
typedef short int16_t;
typedef unsigned short uint16_t;
typedef int int32_t;
typedef unsigned int uint32_t;
typedef long long int64_t;
typedef unsigned long long uint64_t;

typedef unsigned long size_t;
typedef unsigned char byte;
typedef unsigned int uint;

#define NULL ((void *)0)

byte *image_buffer;

byte *EMSCRIPTEN_KEEPALIVE init(size_t image_size) {
  image_buffer = malloc(image_size);
  return image_buffer;
}

size_t write_off;
void write_to_buffer(void *context, void *data, int n) {
  memcpy(image_buffer + write_off, data, n);
  write_off += n;
}

size_t EMSCRIPTEN_KEEPALIVE generate(int64_t seed, int areaX, int areaZ,
                                     unsigned int areaWidth,
                                     unsigned int areaHeight,
                                     unsigned int scale, int version) {
  unsigned char biomeColours[256][3];

  initBiomes();
  initBiomeColours(biomeColours);

  LayerStack g = setupGenerator(version);
  Layer *layer = &g.layers[L_SHORE_16];

  unsigned int imgWidth = areaWidth * scale, imgHeight = areaHeight * scale;

  int *biomes = allocCache(layer, areaWidth, areaHeight);
  unsigned char *rgb = (unsigned char *)malloc(30 + 3 * imgWidth * imgHeight);
  int size_header = sprintf(rgb, "P6\n%d %d\n255\n", imgWidth, imgHeight);

  setWorldSeed(layer, seed);
  genArea(layer, biomes, areaX, areaZ, areaWidth, areaHeight);

  biomesToImage(size_header + rgb, biomeColours, biomes, areaWidth, areaHeight,
                scale, 2);

  int width, height, channels;
  write_off = 0;

  unsigned char *pixels =
      stbi_load_from_memory(rgb, size_header + 3 * imgWidth * imgHeight, &width,
                            &height, &channels, 0);
  write_off = 0;
  stbi_write_png_to_func(&write_to_buffer, NULL, width, height, channels,
                         pixels, 0);
  stbi_image_free(pixels);

  freeGenerator(g);
  free(biomes);
  free(rgb);
  return write_off;
}

int64_t S64(const char *s) {
  int64_t i;
  char c;
  int scanned = sscanf(s, "%" SCNd64 "%c", &i, &c);
  if (scanned == 1)
    return i;
  if (scanned > 1) {
    return i;
  }
  return 0;
}

int parse_version(char *version) {
  int versionc = MC_1_16;
  if (strcmp(version, "1.7") == 0) {
    versionc = MC_1_7;
  } else if (strcmp(version, "1.8") == 0) {
    versionc = MC_1_8;
  } else if (strcmp(version, "1.9") == 0) {
    versionc = MC_1_9;
  } else if (strcmp(version, "1.10") == 0) {
    versionc = MC_1_10;
  } else if (strcmp(version, "1.11") == 0) {
    versionc = MC_1_11;
  } else if (strcmp(version, "1.12") == 0) {
    versionc = MC_1_12;
  } else if (strcmp(version, "1.13") == 0) {
    versionc = MC_1_13;
  } else if (strcmp(version, "1.14") == 0) {
    versionc = MC_1_14;
  } else if (strcmp(version, "1.15") == 0) {
    versionc = MC_1_15;
  }
  return versionc;
}

size_t EMSCRIPTEN_KEEPALIVE generate_string(char *seed, char *areaX,
                                            char *areaZ, char *areaWidth,
                                            char *areaHeight, char *scale,
                                            char *version) {
  int64_t seedc = S64(seed);
  int64_t areaXc = S64(areaX);
  int64_t areaZc = S64(areaZ);
  int64_t areaWidthc = S64(areaWidth);
  int64_t areaHeightc = S64(areaHeight);
  int64_t scalec = S64(scale);
  int versionc = parse_version(version);

  return generate(seedc, areaXc, areaZc, areaWidthc, areaHeightc, scalec,
                  versionc);
}

char *EMSCRIPTEN_KEEPALIVE structure_info(int64_t seed, int regionX,
                                          int regionZ, char *type,
                                          int version) {

  initBiomes();
  LayerStack g = setupGenerator(version);
  Layer *layer = &g.layers[L_SHORE_16];
  setWorldSeed(layer, seed);

  Pos p;
  int isViable;
  StructureConfig featureConfig;
  if (strcmp(type, "village") == 0) {
    featureConfig = VILLAGE_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableVillagePos(g, NULL, p.x, p.z);
  } else if (strcmp(type, "desert_pyramid") == 0) {
    featureConfig = DESERT_PYRAMID_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableFeaturePos(Desert_Pyramid, g, NULL, p.x, p.z);
  } else if (strcmp(type, "jungle_pyramid") == 0) {
    featureConfig = JUNGLE_PYRAMID_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableFeaturePos(Jungle_Pyramid, g, NULL, p.x, p.z);
  } else if (strcmp(type, "shipwreck") == 0) {
    featureConfig = SHIPWRECK_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableFeaturePos(Shipwreck, g, NULL, p.x, p.z);
    /*} else if(strcmp(type, "monument") == 0) {
      featureConfig = MONUMENT_CONFIG;
      p = getStructurePos(featureConfig, seed, regionX, regionZ);
      isViable = isViableFeaturePos(Monument, g, NULL, p.x, p.z);*/
  } else if (strcmp(type, "mansion") == 0) {
    featureConfig = MANSION_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableMansionPos(g, NULL, p.x, p.z);
  } else if (strcmp(type, "ruined_portal") == 0) {
    featureConfig = RUINED_PORTAL_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableFeaturePos(Ruined_Portal, g, NULL, p.x, p.z);
  } else if (strcmp(type, "outpost") == 0) {
    featureConfig = OUTPOST_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableFeaturePos(Outpost, g, NULL, p.x, p.z);
  } else if (strcmp(type, "swamp_hut") == 0) {
    featureConfig = SWAMP_HUT_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableFeaturePos(Swamp_Hut, g, NULL, p.x, p.z);
  } else if (strcmp(type, "igloo") == 0) {
    featureConfig = IGLOO_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableFeaturePos(Igloo, g, NULL, p.x, p.z);
  } else if (strcmp(type, "ocean_ruin") == 0) {
    featureConfig = OCEAN_RUIN_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableOceanMonumentPos(g, NULL, p.x, p.z);
  } else if (strcmp(type, "treasure") == 0) {
    featureConfig = TREASURE_CONFIG;
    p = getStructurePos(featureConfig, seed, regionX, regionZ);
    isViable = isViableFeaturePos(Desert_Pyramid, g, NULL, p.x, p.z);
  } else if (strcmp(type, "spawn") == 0) {
    p = getSpawn(version, &g, NULL, seed);
    isViable = 1;
  }

  char output[80];
  sprintf(output, "{\"type\":\"%s\", \"x\":%d, \"y\":%d,\"viable\":%d}", type,
          p.x, p.z, isViable);

  freeGenerator(g);
  return output;
}

char *EMSCRIPTEN_KEEPALIVE structure_info_string(char *seed, char *regionX,
                                                 char *regionZ, char *type,
                                                 char *version) {
  int64_t seedc = S64(seed);
  int64_t regionXc = S64(regionX);
  int64_t regionZc = S64(regionZ);
  int versionc = parse_version(version);

  return structure_info(seedc, regionXc, regionZc, type, versionc);
}