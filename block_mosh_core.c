// DLL core for @BlockMosh.obj.
// Build as block_mosh_core.dll and place it beside the script.
#include <math.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

#include "lua.h"
#include "lauxlib.h"

static double clampd(double v, double lo, double hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static int clampi(int v, int lo, int hi) {
  if (v < lo) return lo;
  if (v > hi) return hi;
  return v;
}

static int roundi(double v) {
  return (int)floor(v + 0.5);
}

static double fract(double v) {
  return v - floor(v);
}

static double rnd(double n, double seed, int frame) {
  return fract(sin(n * 12.9898 + seed * 78.233 + frame * 27.17) * 43758.5453123);
}

static uint32_t pack_rgb(int r, int g, int b) {
  r = clampi(r, 0, 255);
  g = clampi(g, 0, 255);
  b = clampi(b, 0, 255);
  return 0xff000000u | ((uint32_t)r << 16) | ((uint32_t)g << 8) | (uint32_t)b;
}

static int red(uint32_t c) { return (int)((c >> 16) & 0xff); }
static int green(uint32_t c) { return (int)((c >> 8) & 0xff); }
static int blue(uint32_t c) { return (int)(c & 0xff); }

static double luma_col(uint32_t c) {
  return (red(c) * 0.299 + green(c) * 0.587 + blue(c) * 0.114) / 255.0;
}

static uint32_t sample_px(const uint32_t *src, int w, int h, double x, double y) {
  int ix = clampi(roundi(x), 0, w - 1);
  int iy = clampi(roundi(y), 0, h - 1);
  return src[iy * w + ix];
}

static void blend_px(uint32_t *dst, int idx, uint32_t src, double a) {
  if (a <= 0.0) return;
  if (a >= 1.0) {
    dst[idx] = 0xff000000u | (src & 0x00ffffffu);
    return;
  }
  uint32_t d = dst[idx];
  int r = roundi(red(d) * (1.0 - a) + red(src) * a);
  int g = roundi(green(d) * (1.0 - a) + green(src) * a);
  int b = roundi(blue(d) * (1.0 - a) + blue(src) * a);
  dst[idx] = pack_rgb(r, g, b);
}

static void fill_rect(uint32_t *dst, int w, int h, int x0, int y0, int x1, int y1, uint32_t col, double a) {
  x0 = clampi(x0, 0, w);
  y0 = clampi(y0, 0, h);
  x1 = clampi(x1, 0, w);
  y1 = clampi(y1, 0, h);
  for (int y = y0; y < y1; y++) {
    int row = y * w;
    for (int x = x0; x < x1; x++) {
      blend_px(dst, row + x, col, a);
    }
  }
}

static void copy_tile(uint32_t *dst, const uint32_t *src, int w, int h,
                      double dx0, double dy0, double dx1, double dy1,
                      double ux0, double uy0, double ux1, double uy1, double a) {
  int x0 = clampi((int)floor(dx0), 0, w);
  int y0 = clampi((int)floor(dy0), 0, h);
  int x1 = clampi((int)ceil(dx1), 0, w);
  int y1 = clampi((int)ceil(dy1), 0, h);
  double dw = dx1 - dx0;
  double dh = dy1 - dy0;
  if (dw == 0.0 || dh == 0.0) return;
  for (int y = y0; y < y1; y++) {
    double sy = uy0 + ((y + 0.5 - dy0) / dh) * (uy1 - uy0);
    int row = y * w;
    for (int x = x0; x < x1; x++) {
      double sx = ux0 + ((x + 0.5 - dx0) / dw) * (ux1 - ux0);
      blend_px(dst, row + x, sample_px(src, w, h, sx, sy), a);
    }
  }
}

static double map_value(const uint32_t *src, int w, int h, double x, double y,
                        int block, int map, int invert, double seed, int frame, double time) {
  double v = 1.0;
  if (map == 0) {
    v = luma_col(sample_px(src, w, h, x, y));
  } else if (map == 1) {
    uint32_t c = sample_px(src, w, h, x, y);
    int r = red(c), g = green(c), b = blue(c);
    int mx = r > g ? r : g;
    mx = mx > b ? mx : b;
    int mn = r < g ? r : g;
    mn = mn < b ? mn : b;
    v = (mx - mn) / 255.0;
  } else if (map == 2) {
    double l0 = luma_col(sample_px(src, w, h, x, y));
    double lx = luma_col(sample_px(src, w, h, fmin(w - 1.0, x + block * 0.6), y));
    double ly = luma_col(sample_px(src, w, h, x, fmin(h - 1.0, y + block * 0.6)));
    v = clampd((fabs(lx - l0) + fabs(ly - l0)) * 2.4, 0.0, 1.0);
  } else if (map == 3) {
    double px = (x / w - 0.5) * 2.0;
    double py = (y / h - 0.5) * 2.0;
    v = 1.0 - clampd(sqrt(px * px + py * py), 0.0, 1.0);
  } else if (map == 4) {
    v = rnd(floor(x / block) * 13.0 + floor(y / block) * 31.0, seed, frame);
  } else if (map == 5) {
    v = (sin(y * 0.035 + time * 3.0) + 1.0) * 0.5;
  } else if (map == 6) {
    int p = ((int)floor(x / block) + (int)floor(y / block) + (int)floor(time * 4.0)) & 1;
    v = p == 0 ? 1.0 : 0.15;
  } else {
    v = 1.0;
  }
  if (invert) v = 1.0 - v;
  return clampd(v, 0.0, 1.0);
}

static void draw_chroma(uint32_t *dst, const uint32_t *src, int w, int h, double chroma, double alpha) {
  double c = clampd(chroma, 0.0, 80.0);
  if (c <= 0.0) return;
  double a1 = alpha * c / 360.0;
  double a2 = alpha * c / 380.0;
  for (int y = 0; y < h; y++) {
    int row = y * w;
    for (int x = 0; x < w; x++) {
      uint32_t rsrc = sample_px(src, w, h, x - c, y + c * 0.1);
      uint32_t csrc = sample_px(src, w, h, x + c * 0.8, y - c * 0.12);
      int lr = roundi(luma_col(rsrc) * 255.0);
      int lc = roundi(luma_col(csrc) * 255.0);
      blend_px(dst, row + x, pack_rgb(lr, roundi(lr * 0.20), roundi(lr * 0.33)), a1);
      blend_px(dst, row + x, pack_rgb(roundi(lc * 0.20), roundi(lc * 0.87), lc), a2);
    }
  }
}

static int apply(lua_State *L) {
  uint32_t *data = (uint32_t *)lua_touserdata(L, 1);
  int w = (int)luaL_checknumber(L, 2);
  int h = (int)luaL_checknumber(L, 3);
  int map = clampi(roundi(luaL_optnumber(L, 4, 0.0)), 0, 7);
  int mode = clampi(roundi(luaL_optnumber(L, 5, 0.0)), 0, 5);
  int block = clampi(roundi(luaL_optnumber(L, 6, 32.0)), 12, 128);
  double intensity = clampd(luaL_optnumber(L, 7, 64.0), 0.0, 100.0) / 100.0;
  double carry = clampd(luaL_optnumber(L, 8, 58.0), -100.0, 100.0) / 100.0;
  double smear = clampd(luaL_optnumber(L, 9, 42.0), 0.0, 100.0) / 100.0;
  double threshold = clampd(luaL_optnumber(L, 10, 38.0), 0.0, 100.0) / 100.0;
  double drop = clampd(luaL_optnumber(L, 11, 12.0), 0.0, 100.0) / 100.0;
  double chroma = clampd(luaL_optnumber(L, 12, 7.0), 0.0, 80.0);
  double alpha = clampd(luaL_optnumber(L, 13, 100.0), 0.0, 100.0) / 100.0;
  double seed = luaL_optnumber(L, 14, 1.0);
  uint32_t tint_arg = (uint32_t)luaL_optnumber(L, 15, 0x82e8ff);
  uint32_t tint = pack_rgb((int)((tint_arg >> 16) & 0xff),
                           (int)((tint_arg >> 8) & 0xff),
                           (int)(tint_arg & 0xff));
  int base = roundi(luaL_optnumber(L, 16, 1.0)) != 0;
  int debug = roundi(luaL_optnumber(L, 17, 0.0)) != 0;
  int invert = roundi(luaL_optnumber(L, 18, 0.0)) != 0;
  int mask_only = roundi(luaL_optnumber(L, 19, 0.0)) != 0;
  double time = luaL_optnumber(L, 20, 0.0);
  int frame = (int)floor(time * 10.0);

  if (!data || w <= 0 || h <= 0) {
    lua_settop(L, 1);
    return 1;
  }

  size_t bytes = (size_t)w * (size_t)h * sizeof(uint32_t);
  uint32_t *src = (uint32_t *)malloc(bytes);
  if (!src) {
    lua_settop(L, 1);
    return 1;
  }
  memcpy(src, data, bytes);

  if (!base || mask_only) {
    for (int i = 0; i < w * h; i++) data[i] = 0xff050607u;
  }

  int total = ((w + block - 1) / block) * ((h + block - 1) / block);
  if (total > 2600) {
    block = (int)ceil(sqrt((double)w * (double)h / 2600.0));
    if (block < 1) block = 1;
  }

  for (int y = 0; y < h; y += block) {
    for (int x = 0; x < w; x += block) {
      int x1 = x + block < w ? x + block : w;
      int y1 = y + block < h ? y + block : h;
      double cx = (x + x1) * 0.5;
      double cy = (y + y1) * 0.5;
      double m = map_value(src, w, h, cx, cy, block, map, invert, seed, frame, time);
      if (debug || mask_only) {
        int g = roundi(m * 255.0);
        uint32_t col = pack_rgb(roundi(g * 0.45), g, roundi(255.0 - g * 0.35));
        fill_rect(data, w, h, x, y, x1, y1, col, 0.72);
      } else if (m >= threshold) {
        double n = rnd(x * 0.21 + y * 0.37, seed, frame);
        double amp = intensity * m;
        double dx = 0.0, dy = 0.0;
        if (mode == 0) {
          dx = (n - 0.5) * block * 4.2 * carry;
          dy = (rnd(x + y + 8.0, seed, frame) - 0.5) * block * 1.2 * carry;
        } else if (mode == 1) {
          dx = sin(cy * 0.025 + time * 2.4) * block * 3.2 * carry;
        } else if (mode == 2) {
          dx = (cx - w * 0.5) / w * block * 7.0 * carry;
          dy = (cy - h * 0.5) / h * block * 3.5 * carry;
        } else if (mode == 3) {
          dx = -(cy - h * 0.5) / h * block * 6.0 * carry;
          dy = (cx - w * 0.5) / w * block * 3.0 * carry;
        } else if (mode == 4) {
          dx = floor((n - 0.5) * 8.0) * block * 0.5 * carry;
          dy = floor((rnd(x + y + 4.0, seed, frame) - 0.5) * 4.0) * block * 0.5 * carry;
        } else {
          dx = sin((x + y) * 0.019 + time * 4.0) * block * 2.8 * carry;
          dy = cos((x - y) * 0.017 - time * 2.0) * block * 1.4 * carry;
        }
        dx *= amp;
        dy *= amp;
        if (rnd(x * 0.07 + y * 0.09, seed, frame) < drop * m) {
          fill_rect(data, w, h, x, y, x1, y1, tint, alpha * 0.10 * m);
        } else {
          copy_tile(data, src, w, h, x + dx, y + dy, x1 + dx, y1 + dy, x, y, x1, y1,
                    alpha * (0.22 + m * 0.78));
          if (smear > 0.0) {
            int steps = clampi(roundi(1.0 + smear * 4.0), 1, 5);
            for (int s = 1; s <= steps; s++) {
              double k = (double)s / (double)steps;
              copy_tile(data, src, w, h,
                        x + dx * k * 1.8, y + dy * k * 1.4,
                        x1 + dx * k * 1.8, y1 + dy * k * 1.4,
                        x, y, x1, y1,
                        alpha * m * smear * 0.12 / s);
            }
          }
        }
      }
    }
  }

  if (!debug && !mask_only) {
    draw_chroma(data, src, w, h, chroma, alpha);
  }

  free(src);
  lua_settop(L, 1);
  return 1;
}

int luaopen_block_mosh_core(lua_State *L) {
  lua_newtable(L);
  lua_pushcfunction(L, apply);
  lua_setfield(L, -2, "apply");
  return 1;
}
