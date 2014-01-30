#include <stdio.h>
#include <stdlib.h>

#include "stb_image.h"
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

typedef unsigned char u8;
typedef unsigned int  u32;

class Image {
public:
  Image(u8* pixels, int w, int h)
    : data(pixels), width(w), height(h)
  {
  }

  void set(int x, int y, u32 rgba) {
    setA(x, y, rgba & 0xff);
    rgba >>= 8;
    setB(x, y, rgba & 0xff);
    rgba >>= 8;
    setG(x, y, rgba & 0xff);
    rgba >>= 8;
    setR(x, y, rgba & 0xff);
  }
  void setA(int x, int y, u8 val) {
    setComponent(x, y, 3, val);
  }
  void setR(int x, int y, u8 val) {
    setComponent(x, y, 0, val);
  }
  void setG(int x, int y, u8 val) {
    setComponent(x, y, 1, val);
  }
  void setB(int x, int y, u8 val) {
    setComponent(x, y, 2, val);
  }

  void fix(int left, int bottom, bool padding=true) {
    u32 emptyColor = 0x00000000u;
    u32 borderColor = 0x000000ffu;
    u32 baseColor = 0xf9f2e2ffu;

    for (int y=0; y<192; y++) {
      for (int x=0; x<128; x++) {
        if (y<16 || x<4 || x>=128-4 || y>=192-16) {
          set(left+x, bottom-y, emptyColor);
        } else if (y<17 || x<5 || x>=128-5 || y>=192-17) {
          set(left+x, bottom-y, borderColor);
        } else if (padding && (y<20 || x<8 || x>=128-8 || y>=192-20)) {
          set(left+x, bottom-y, baseColor);
        }
      }
    }
  }

private:
  u8* data;
  int width;
  int height;

  static const int COMPONENTS = 4;

  void setComponent(int x, int y, int offset, u8 val) {
    int pos = (y*width+x)*COMPONENTS + offset;
    data[pos] = val;
  }
};

int main(int argc, char* argv[]) {
  if (argc != 3) {
    printf("args: input.png output.png\n");
    return 0;
  }

  int w = -1;
  int h = -1;
  int components = -1;
  u8* pixels = stbi_load(argv[1], &w, &h, &components, 0);

  if (pixels != NULL) {
    Image img(pixels, w, h);
    // base cards
    for (int y=0; y<4; y++) {
      for (int x=0; x<13; x++) {
        img.fix(x*128, 1024-y*192-1);  
      }
    }
    // placeholder cards
    for (int y=0; y<4; y++) {
      img.fix(128*13, 1024-y*192-1, false);
    }
    // card back
    img.fix(14*128, 1023);
  } else {
    printf("Some problems with loading...\n");
    return 0;
  }

  int result = stbi_write_png(argv[2], w, h, components, pixels, 0);

  if (result == 1) {
    printf("Okey-dokey, saved!\n");
    printf("width: %d,   height: %d,   components: %d\n", w, h, components);
  } else {
    printf("Could not save the resulting image, too bad...\n");
  }

  stbi_image_free(pixels);

  return 0;
}
