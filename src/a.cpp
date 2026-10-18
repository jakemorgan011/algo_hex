#include "./a.hpp"
#include <algorithm>
#include <chrono>
#include <csignal>
#include <cstdio>
#include <cstdlib>
#include <fstream>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <sys/ioctl.h>
#include <unistd.h>

const a::b::rl a::b::ns[3] = {{n1, 41, "n1 swp lo"}, {n2, 42, "n2 swp hi"}, {n3, 43, "n3 fib"}};
const a::b::rl a::b::ls[2] = {{l2, 45, "l2 two"}, {l1, 44, "l1 dec"}};
const a::b::rl a::b::sf = {l3, 46, "l3 far"};
termios a::ot{};
bool a::tt = false;
constexpr size_t a::NF;

int a::b::hi(unsigned char x){ return x >> 4; }
int a::b::lo(unsigned char x){ return x & 0x0f; }
unsigned char a::b::pk(int h, int l){ return ((h & 0x0f) << 4) | (l & 0x0f); }
bool a::b::nm(int n){ return n < 10; }
bool a::b::fb(int n){ return n == 0 || n == 1 || n == 2 || n == 3 || n == 5 || n == 8 || n == 13; }

void a::b::sh(hx& h, size_t i, size_t j){
  unsigned char x = h[i];
  h[i] = pk(hi(h[j]), lo(h[i]));
  h[j] = pk(hi(x), lo(h[j]));
}

void a::b::sl(hx& h, size_t i, size_t j){
  unsigned char x = h[i];
  h[i] = pk(hi(h[i]), lo(h[j]));
  h[j] = pk(hi(h[j]), lo(x));
}

bool a::b::n1(hx& h, size_t i, size_t& j){
  j = (i + 1) % h.size();
  if(!nm(hi(h[i])) || !nm(hi(h[j])) || hi(h[j]) <= hi(h[i]))
    return false;
  sl(h, i, j);
  return true;
}

bool a::b::n2(hx& h, size_t i, size_t& j){
  if(i == 0 || !nm(lo(h[i])) || lo(h[i]) <= lo(h[i - 1]))
    return false;
  j = (i + 2) % h.size();
  sh(h, i, j);
  return true;
}

bool a::b::n3(hx& h, size_t i, size_t& j){
  int fh = hi(h[i]);
  int fl = lo(h[i]);
  if(!fb(fh) && !fb(fl))
    return false;
  if(fb(fh))
    h[i] = pk(fh, fl + fh);
  if(fb(fl)){
    j = (i + 1) % h.size();
    h[j] = pk(hi(h[j]) + fl, lo(h[j]));
  }
  return true;
}

bool a::b::l1(hx& h, size_t i, size_t&){
  if(nm(hi(h[i])) || !nm(hi(h[(i + 1) % h.size()])))
    return false;
  h[i] = pk(hi(h[i]) - 1, lo(h[i]));
  return true;
}

bool a::b::l2(hx& h, size_t i, size_t&){
  if(nm(hi(h[i])) || nm(lo(h[i])))
    return false;
  h[i] = pk(hi(h[i]) - 1, lo(h[i]) + 1);
  return true;
}

bool a::b::l3(hx& h, size_t i, size_t& j){
  size_t n = h.size();
  j = (i + h[(i + 1) % n] % n) % n;
  std::swap(h[i], h[j]);
  return true;
}

a::b::hx a::g::dc(const b::hx& f){ return f; }
a::b::hx a::g::ec(const b::hx& d){ return d; }
bool a::g::ok(unsigned char) const { return true; }
uint32_t a::g::u2(const b::hx& f, size_t p){ return (f[p] << 8) | f[p + 1]; }
uint32_t a::g::u4(const b::hx& f, size_t p){ return (uint32_t(f[p]) << 24) | (f[p + 1] << 16) | (f[p + 2] << 8) | f[p + 3]; }
uint64_t a::g::u8(const b::hx& f, size_t p){ return (uint64_t(u4(f, p)) << 32) | u4(f, p + 4); }
uint32_t a::g::fc(const char* s){ return (uint32_t(uint8_t(s[0])) << 24) | (uint8_t(s[1]) << 16) | (uint8_t(s[2]) << 8) | uint8_t(s[3]); }

const char* a::rw::id() const { return "raw"; }
void a::rw::mk(const b::hx&, std::vector<bool>& m){ m.assign(m.size(), true); }

const char* a::jp::id() const { return "jpeg"; }
bool a::jp::ok(unsigned char v) const { return v != 0xff; }

void a::jp::mk(const b::hx& f, std::vector<bool>& m){
  size_t n = f.size();
  if(n < 4 || f[0] != 0xff || f[1] != 0xd8)
    return;
  size_t p = 2;
  while(p + 1 < n){
    if(f[p] != 0xff)
      return;
    while(p < n && f[p] == 0xff)
      p++;
    if(p >= n)
      return;
    int mr = f[p++];
    if(mr == 0xd9)
      return;
    if((mr >= 0xd0 && mr <= 0xd7) || mr == 0x01)
      continue;
    if(p + 2 > n)
      return;
    p += u2(f, p);
    if(mr != 0xda)
      continue;
    while(p < n){
      if(f[p] != 0xff){
        m[p++] = true;
        continue;
      }
      if(p + 1 < n && (f[p + 1] == 0x00 || (f[p + 1] >= 0xd0 && f[p + 1] <= 0xd7))){
        p += 2;
        continue;
      }
      break;
    }
  }
}

const char* a::m3::id() const { return "mp3"; }

size_t a::m3::fh(const b::hx& f, size_t p, size_t& hl){
  static const int b1[] = {0, 32, 40, 48, 56, 64, 80, 96, 112, 128, 160, 192, 224, 256, 320};
  static const int b2[] = {0, 8, 16, 24, 32, 40, 48, 56, 64, 80, 96, 112, 128, 144, 160};
  static const int sr[] = {44100, 48000, 32000};
  if(p + 4 > f.size() || f[p] != 0xff || (f[p + 1] & 0xe0) != 0xe0)
    return 0;
  int vr = (f[p + 1] >> 3) & 3;
  int ly = (f[p + 1] >> 1) & 3;
  int pr = f[p + 1] & 1;
  int bi = f[p + 2] >> 4;
  int si = (f[p + 2] >> 2) & 3;
  int pd = (f[p + 2] >> 1) & 1;
  int ch = f[p + 3] >> 6;
  if(vr == 1 || ly != 1 || bi == 0 || bi == 15 || si == 3)
    return 0;
  bool v1 = vr == 3;
  int br = (v1 ? b1 : b2)[bi] * 1000;
  int s = sr[si] >> (v1 ? 0 : vr == 2 ? 1 : 2);
  hl = 4 + (pr ? 0 : 2) + (v1 ? (ch == 3 ? 17 : 32) : (ch == 3 ? 9 : 17));
  return (v1 ? 144 : 72) * br / s + pd;
}

void a::m3::mk(const b::hx& f, std::vector<bool>& m){
  size_t p = 0;
  size_t e = f.size();
  if(e >= 10 && f[0] == 'I' && f[1] == 'D' && f[2] == '3')
    p = 10 + ((f[6] & 0x7f) << 21 | (f[7] & 0x7f) << 14 | (f[8] & 0x7f) << 7 | (f[9] & 0x7f)) + (f[5] & 0x10 ? 10 : 0);
  if(e >= 128 && f[e - 128] == 'T' && f[e - 127] == 'A' && f[e - 126] == 'G')
    e -= 128;
  bool fs = true;
  while(p + 4 <= e){
    size_t hl = 0;
    size_t h2 = 0;
    size_t ln = fh(f, p, hl);
    size_t nx = p + ln;
    if(ln <= hl || nx > e || (nx + 4 <= e && fh(f, nx, h2) == 0)){
      p++;
      continue;
    }
    if(!fs)
      for(size_t q = p + hl; q < nx; q++)
        m[q] = true;
    fs = false;
    p = nx;
  }
}

const char* a::m4::id() const { return "mp4"; }

bool a::m4::bx(const b::hx& f, size_t p, size_t e, size_t& ps, size_t& be, uint32_t& t){
  if(p + 8 > e)
    return false;
  uint64_t sz = u4(f, p);
  t = u4(f, p + 4);
  ps = p + 8;
  if(sz == 1){
    if(p + 16 > e)
      return false;
    sz = u8(f, p + 8);
    ps = p + 16;
  }else if(sz == 0){
    sz = e - p;
  }
  if(sz < ps - p || sz > e - p)
    return false;
  be = p + sz;
  return true;
}

bool a::m4::fd(const b::hx& f, size_t s, size_t e, uint32_t t, size_t& ps, size_t& pe){
  size_t bs = 0;
  size_t be = 0;
  uint32_t bt = 0;
  for(size_t p = s; bx(f, p, e, bs, be, bt); p = be){
    if(bt == t){
      ps = bs;
      pe = be;
      return true;
    }
  }
  return false;
}

void a::m4::mk(const b::hx& f, std::vector<bool>& m){
  size_t ms = 0, me = 0, bs = 0, be = 0;
  uint32_t bt = 0;
  if(!fd(f, 0, f.size(), fc("moov"), ms, me))
    return;
  for(size_t p = ms; bx(f, p, me, bs, be, bt); p = be)
    if(bt == fc("trak"))
      tk(f, bs, be, m);
}

void a::m4::tk(const b::hx& f, size_t s, size_t e, std::vector<bool>& m){
  size_t ds, de, hs, he, is, ie, ts, te, as, ae, es, ee, cs, ce, zs, ze, ks, ke, os, oe;
  uint32_t et = 0;
  if(!fd(f, s, e, fc("mdia"), ds, de) || !fd(f, ds, de, fc("hdlr"), hs, he) || he - hs < 12 || u4(f, hs + 8) != fc("vide"))
    return;
  if(!fd(f, ds, de, fc("minf"), is, ie) || !fd(f, is, ie, fc("stbl"), ts, te))
    return;
  if(!fd(f, ts, te, fc("stsd"), as, ae) || !bx(f, as + 8, ae, es, ee, et))
    return;
  bool hv = et == fc("hvc1") || et == fc("hev1");
  if(!hv && et != fc("avc1") && et != fc("avc3"))
    return;
  size_t ls = 4;
  if(es + 78 < ee){
    if(fd(f, es + 78, ee, fc("avcC"), cs, ce) && ce - cs > 4)
      ls = (f[cs + 4] & 3) + 1;
    else if(fd(f, es + 78, ee, fc("hvcC"), cs, ce) && ce - cs > 21)
      ls = (f[cs + 21] & 3) + 1;
  }
  if(!fd(f, ts, te, fc("stsz"), zs, ze) || !fd(f, ts, te, fc("stsc"), ks, ke) || ze - zs < 12 || ke - ks < 8)
    return;
  bool c6 = !fd(f, ts, te, fc("stco"), os, oe);
  if(c6 && !fd(f, ts, te, fc("co64"), os, oe))
    return;
  uint32_t fx = u4(f, zs + 4);
  size_t sn = u4(f, zs + 8);
  size_t kn = u4(f, ks + 4);
  size_t on = u4(f, os + 4);
  if((!fx && zs + 12 + sn * 4 > ze) || ks + 8 + kn * 12 > ke || os + 8 + on * (c6 ? 8 : 4) > oe)
    return;

  size_t sm = 0;
  size_t ki = 0;
  for(size_t c = 0; c < on && sm < sn; c++){
    while(ki + 1 < kn && u4(f, ks + 8 + (ki + 1) * 12) <= c + 1)
      ki++;
    size_t sp = kn ? u4(f, ks + 8 + ki * 12 + 4) : 0;
    uint64_t of = c6 ? u8(f, os + 8 + c * 8) : u4(f, os + 8 + c * 4);
    for(size_t k = 0; k < sp && sm < sn; k++, sm++){
      uint64_t sz = fx ? fx : u4(f, zs + 12 + sm * 4);
      if(of + sz > f.size())
        return;
      nl(f, of, of + sz, ls, hv, m);
      of += sz;
    }
  }
}

void a::m4::nl(const b::hx& f, size_t p, size_t e, size_t ls, bool hv, std::vector<bool>& m){
  while(p + ls < e){
    size_t ln = 0;
    for(size_t k = 0; k < ls; k++)
      ln = (ln << 8) | f[p + k];
    p += ls;
    if(ln == 0 || ln > e - p)
      return;
    int ty = hv ? (f[p] >> 1) & 0x3f : f[p] & 0x1f;
    bool vc = hv ? ty < 32 : ty >= 1 && ty <= 5;
    if(vc)
      for(size_t q = p + NH; q + 1 < p + ln; q++)
        m[q] = true;
    p += ln;
  }
}

const char* a::pn::id() const { return "png"; }

uint32_t a::pn::cr(const unsigned char* d, size_t n){
  static uint32_t t[256] = {0};
  if(!t[1]){
    for(uint32_t k = 0; k < 256; k++){
      uint32_t c = k;
      for(int s = 0; s < 8; s++)
        c = c & 1 ? 0xedb88320u ^ (c >> 1) : c >> 1;
      t[k] = c;
    }
  }
  uint32_t c = 0xffffffffu;
  for(size_t k = 0; k < n; k++)
    c = t[(c ^ d[k]) & 0xff] ^ (c >> 8);
  return c ^ 0xffffffffu;
}

void a::pn::pc(b::hx& o, uint32_t t, const unsigned char* d, size_t n){
  b::hx c = {uint8_t(t >> 24), uint8_t(t >> 16), uint8_t(t >> 8), uint8_t(t)};
  c.insert(c.end(), d, d + n);
  uint32_t k = cr(c.data(), c.size());
  o.push_back(n >> 24); o.push_back(n >> 16); o.push_back(n >> 8); o.push_back(n);
  o.insert(o.end(), c.begin(), c.end());
  o.push_back(k >> 24); o.push_back(k >> 16); o.push_back(k >> 8); o.push_back(k);
}

a::b::hx a::pn::dc(const b::hx& f){
  static const unsigned char sg[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  if(f.size() < 8 || !std::equal(sg, sg + 8, f.begin()))
    throw std::runtime_error("not a png");
  b::hx z;
  size_t p = 8;
  while(p + 12 <= f.size()){
    uint32_t ln = u4(f, p);
    uint32_t t = u4(f, p + 4);
    if(ln > f.size() - p - 12)
      throw std::runtime_error("chunk runs past eof");
    b::hx d(f.begin() + p + 8, f.begin() + p + 8 + ln);
    if(t == fc("IHDR") && ln >= 13){
      w = u4(d, 0);
      h = u4(d, 4);
      dp = d[8];
      ct = d[9];
      il = d[12];
    }
    if(t == fc("IDAT")){
      z.insert(z.end(), d.begin(), d.end());
      d.clear();
    }
    cs.push_back({t, d});
    p += 12 + ln;
    if(t == fc("IEND"))
      break;
  }
  zi q{z, 2, 0, 0, {}};
  return q.go();
}

a::b::hx a::pn::ec(const b::hx& d){
  // stored (uncompressed) deflate blocks, valid zlib, just bigger than the original
  b::hx z = {0x78, 0x01};
  size_t p = 0;
  do{
    size_t k = std::min<size_t>(65535, d.size() - p);
    z.push_back(p + k == d.size());
    z.push_back(k); z.push_back(k >> 8); z.push_back(~k); z.push_back(~k >> 8);
    z.insert(z.end(), d.begin() + p, d.begin() + p + k);
    p += k;
  }while(p < d.size());
  uint32_t a1 = 1, a2 = 0;
  for(unsigned char x : d){
    a1 = (a1 + x) % 65521;
    a2 = (a2 + a1) % 65521;
  }
  uint32_t ad = (a2 << 16) | a1;
  z.push_back(ad >> 24); z.push_back(ad >> 16); z.push_back(ad >> 8); z.push_back(ad);

  b::hx o = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
  bool dn = false;
  for(const ck& c : cs){
    if(c.t != fc("IDAT")){
      pc(o, c.t, c.d.data(), c.d.size());
      continue;
    }
    if(dn)
      continue;
    for(size_t q = 0; q < z.size(); q += 1 << 20)
      pc(o, c.t, z.data() + q, std::min<size_t>(1 << 20, z.size() - q));
    dn = true;
  }
  return o;
}

void a::pn::mk(const b::hx& d, std::vector<bool>& m){
  static const int ch[] = {1, 0, 3, 1, 2, 0, 4};
  static const int ps[8][4] = {{0, 0, 1, 1}, {0, 0, 8, 8}, {4, 0, 8, 8}, {0, 4, 4, 8}, {2, 0, 4, 4}, {0, 2, 2, 4}, {1, 0, 2, 2}, {0, 1, 1, 2}};
  if(ct < 0 || ct > 6 || !ch[ct])
    return;
  uint64_t bp = ch[ct] * dp;
  size_t q = 0;
  for(int k = il ? 1 : 0; k < (il ? 8 : 1); k++){
    uint64_t pw = w > uint64_t(ps[k][0]) ? (w - ps[k][0] + ps[k][2] - 1) / ps[k][2] : 0;
    uint64_t ph = h > uint64_t(ps[k][1]) ? (h - ps[k][1] + ps[k][3] - 1) / ps[k][3] : 0;
    if(!pw || !ph)
      continue;
    uint64_t rb = (pw * bp + 7) / 8;
    for(uint64_t y = 0; y < ph; y++){
      q++;
      for(uint64_t x = 0; x < rb && q < d.size(); x++)
        m[q++] = true;
      if(q >= d.size())
        return;
    }
  }
}

int a::pn::zi::bt(int n){
  while(bc < n){
    if(p >= s.size())
      throw std::runtime_error("deflate ran out of data");
    bb |= uint32_t(s[p++]) << bc;
    bc += 8;
  }
  int v = bb & ((1u << n) - 1);
  bb >>= n;
  bc -= n;
  return v;
}

void a::pn::zi::bd(hf& h, const short* l, int n){
  short of[16] = {0};
  std::fill(h.c, h.c + 16, 0);
  for(int k = 0; k < n; k++)
    h.c[l[k]]++;
  h.c[0] = 0;
  for(int k = 1; k < 15; k++)
    of[k + 1] = of[k] + h.c[k];
  for(int k = 0; k < n; k++)
    if(l[k])
      h.s[of[l[k]]++] = k;
}

int a::pn::zi::sy(const hf& h){
  int cd = 0, fs = 0, ix = 0;
  for(int ln = 1; ln < 16; ln++){
    cd |= bt(1);
    int ct = h.c[ln];
    if(cd - ct < fs)
      return h.s[ix + (cd - fs)];
    ix += ct;
    fs += ct;
    fs <<= 1;
    cd <<= 1;
  }
  throw std::runtime_error("bad huffman code");
}

void a::pn::zi::cd(const hf& l, const hf& d){
  static const short lb[] = {3, 4, 5, 6, 7, 8, 9, 10, 11, 13, 15, 17, 19, 23, 27, 31, 35, 43, 51, 59, 67, 83, 99, 115, 131, 163, 195, 227, 258};
  static const short le[] = {0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 2, 2, 2, 2, 3, 3, 3, 3, 4, 4, 4, 4, 5, 5, 5, 5, 0};
  static const short db[] = {1, 2, 3, 4, 5, 7, 9, 13, 17, 25, 33, 49, 65, 97, 129, 193, 257, 385, 513, 769, 1025, 1537, 2049, 3073, 4097, 6145, 8193, 12289, 16385, 24577};
  static const short de[] = {0, 0, 0, 0, 1, 1, 2, 2, 3, 3, 4, 4, 5, 5, 6, 6, 7, 7, 8, 8, 9, 9, 10, 10, 11, 11, 12, 12, 13, 13};
  for(;;){
    int v = sy(l);
    if(v < 256){
      o.push_back(v);
      continue;
    }
    if(v == 256)
      return;
    v -= 257;
    if(v >= 29)
      throw std::runtime_error("bad length code");
    size_t ln = lb[v] + bt(le[v]);
    int ds = sy(d);
    if(ds >= 30)
      throw std::runtime_error("bad distance code");
    size_t dt = db[ds] + bt(de[ds]);
    if(dt > o.size())
      throw std::runtime_error("distance past start");
    for(size_t k = 0; k < ln; k++)
      o.push_back(o[o.size() - dt]);
  }
}

a::b::hx a::pn::zi::go(){
  static const short od[] = {16, 17, 18, 0, 8, 7, 9, 6, 10, 5, 11, 4, 12, 3, 13, 2, 14, 1, 15};
  if(s.size() < 2 || (s[0] & 0x0f) != 8 || (s[1] & 0x20))
    throw std::runtime_error("bad zlib header");
  int la = 0;
  while(!la){
    la = bt(1);
    int ty = bt(2);
    if(ty == 0){
      bb = 0;
      bc = 0;
      if(p + 4 > s.size())
        throw std::runtime_error("stored block past eof");
      size_t ln = s[p] | (s[p + 1] << 8);
      p += 4;
      if(p + ln > s.size())
        throw std::runtime_error("stored block past eof");
      o.insert(o.end(), s.begin() + p, s.begin() + p + ln);
      p += ln;
      continue;
    }
    hf l, d;
    short ll[320] = {0};
    if(ty == 1){
      for(int k = 0; k < 288; k++)
        ll[k] = k < 144 ? 8 : k < 256 ? 9 : k < 280 ? 7 : 8;
      bd(l, ll, 288);
      std::fill(ll, ll + 30, 5);
      bd(d, ll, 30);
      cd(l, d);
      continue;
    }
    if(ty != 2)
      throw std::runtime_error("bad block type");
    int nl = bt(5) + 257;
    int nd = bt(5) + 1;
    int nc = bt(4) + 4;
    for(int k = 0; k < nc; k++)
      ll[od[k]] = bt(3);
    bd(l, ll, 19);
    int k = 0;
    while(k < nl + nd){
      int v = sy(l);
      if(v < 16){
        ll[k++] = v;
        continue;
      }
      short rp = 0;
      int n = 0;
      if(v == 16){
        if(!k)
          throw std::runtime_error("repeat with nothing before");
        rp = ll[k - 1];
        n = 3 + bt(2);
      }else{
        n = v == 17 ? 3 + bt(3) : 11 + bt(7);
      }
      if(k + n > nl + nd)
        throw std::runtime_error("too many code lengths");
      while(n--)
        ll[k++] = rp;
    }
    bd(l, ll, nl);
    bd(d, ll + nl, nd);
    cd(l, d);
  }
  return o;
}

std::unique_ptr<a::g> a::gp(const b::hx& f){
  size_t hl = 0;
  if(f.size() >= 3 && f[0] == 0xff && f[1] == 0xd8 && f[2] == 0xff)
    return std::unique_ptr<g>(new jp());
  if(f.size() >= 8 && f[0] == 0x89 && f[1] == 'P' && f[2] == 'N' && f[3] == 'G')
    return std::unique_ptr<g>(new pn());
  if(f.size() >= 12 && g::u4(f, 4) == g::fc("ftyp"))
    return std::unique_ptr<g>(new m4());
  if((f.size() >= 3 && f[0] == 'I' && f[1] == 'D' && f[2] == '3') || m3::fh(f, 0, hl))
    return std::unique_ptr<g>(new m3());
  return std::unique_ptr<g>(new rw());
}

a::a(std::string pth){
  p = pth;
  std::ifstream f(pth, std::ios::binary);
  if(!f.is_open()){
    std::cerr << "cant open " << pth << std::endl;
    return;
  }
  std::stringstream ss;
  ss << f.rdbuf();
  std::string s = ss.str(); // raw bytes
  b::hx fl(s.begin(), s.end());
  gd = gp(fl);
  try{
    fb = gd->dc(fl);
  }catch(const std::exception& x){
    std::cerr << "cant decode " << gd->id() << ": " << x.what() << std::endl;
    return;
  }
  std::vector<bool> m(fb.size(), false);
  gd->mk(fb, m);
  rv.assign(fb.size(), NF);
  for(size_t q = 0; q < fb.size(); q++){
    if(!m[q])
      continue;
    rv[q] = ix.size();
    ix.push_back(q);
    hv.push_back(fb[q]);
  }
}


void a::r(){
  if(hv.empty()){
    if(gd)
      std::cerr << "nothing safe to swap in " << p << " (" << gd->id() << ")" << std::endl;
    return;
  }
  st();
  while(R)
    std::this_thread::sleep_for(std::chrono::milliseconds(50));
  e();
}

void a::st(){
  ov = hv;
  if(isatty(STDIN_FILENO)){
    tcgetattr(STDIN_FILENO, &ot);
    termios t = ot;
    t.c_lflag &= ~(ECHO | ICANON);
    tcsetattr(STDIN_FILENO, TCSANOW, &t);
    tt = true;
  }
  std::signal(SIGINT, sg);
  std::cout << "\x1b[?25l\x1b[2J" << std::flush; // hide cursor, clear screen
  R = true;
  fe = std::thread(&a::lp, this);
}

void a::lp(){
  for(lk = 0; lk < LOOPS; lk++){
    bool pl = false;
    for(size_t i = 0; i < hv.size(); i++){
      if(pl)
        d(b::sf, i);
      pl = false;
      for(const b::rl& r : b::ls){
        if(d(r, i)){
          pl = true;
          break;
        }
      }
      if(!pl)
        for(const b::rl& r : b::ns)
          d(r, i);
    }
  }
  R = false;
}

bool a::d(const b::rl& r, size_t i){
  size_t j = i;
  if(!r.f(hv, i, j))
    return false;
  if(!gd->ok(hv[i]) || !gd->ok(hv[j])){
    hv[i] = ov[i];
    hv[j] = ov[j];
    return false;
  }
  dr(r, i, j);
  ov[i] = hv[i];
  ov[j] = hv[j];
  std::this_thread::sleep_for(std::chrono::microseconds(DL));
  return true;
}

std::string a::cl(const b::rl& r){
  return "\x1b[30;" + std::to_string(r.c) + "m " + r.n + " \x1b[0m";
}

void a::dr(const b::rl& r, size_t i, size_t j){
  static constexpr char hd[] = "0123456789abcdef";
  int cw = 80;
  int rh = 24;
  winsize ws{};
  if(ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) == 0 && ws.ws_col > 0){
    cw = ws.ws_col;
    rh = ws.ws_row;
  }

  size_t n = fb.size();
  size_t bp = std::max(1, (cw - 10) / 3);
  size_t vr = std::max(1, rh - 3);
  size_t tr = (n + bp - 1) / bp;
  size_t cr = ix[i] / bp;
  size_t tp = cr > vr / 2 ? cr - vr / 2 : 0;
  if(tp + vr > tr)
    tp = tr > vr ? tr - vr : 0;

  std::string o = "\x1b[H";
  for(const b::rl& q : b::ns)
    o += cl(q) + " ";
  for(const b::rl& q : b::ls)
    o += cl(q) + " ";
  o += cl(b::sf) + "\x1b[K\n";
  o += p + " " + gd->id() + "  loop " + std::to_string(lk + 1) + "/" + std::to_string(LOOPS)
     + "  free " + std::to_string(i + 1) + "/" + std::to_string(hv.size()) + "  " + cl(r) + "\x1b[K\n";

  char ofs[16];
  for(size_t y = tp; y < std::min(tp + vr, tr); y++){
    std::snprintf(ofs, sizeof(ofs), "%08zx  ", y * bp);
    o += ofs;
    for(size_t q = y * bp; q < std::min((y + 1) * bp, n); q++){
      size_t k = rv[q];
      if(k == NF){
        o += "\x1b[2m";
        o += hd[b::hi(fb[q])];
        o += hd[b::lo(fb[q])];
        o += "\x1b[0m ";
        continue;
      }
      for(int s = 0; s < 2; s++){
        int v = s ? b::lo(hv[k]) : b::hi(hv[k]);
        int w = s ? b::lo(ov[k]) : b::hi(ov[k]);
        bool ch = (k == i || k == j) && v != w;
        if(ch)
          o += "\x1b[30;" + std::to_string(r.c) + "m";
        else if(k == i)
          o += "\x1b[4m";
        o += hd[v];
        if(ch || k == i)
          o += "\x1b[0m";
      }
      o += ' ';
    }
    o += "\x1b[K\n";
  }
  o += "\x1b[J";
  std::cout << o << std::flush;
}

void a::e(){
  if(fe.joinable())
    fe.join();
  rs();
  w();
}

void a::rs(){
  static constexpr char sc[] = "\x1b[0m\x1b[?25h\n"; // reset color, show cursor
  write(STDOUT_FILENO, sc, sizeof(sc) - 1);
  if(tt)
    tcsetattr(STDIN_FILENO, TCSANOW, &ot);
}

void a::sg(int){
  rs();
  std::_Exit(130);
}

void a::w(){
  size_t sl = p.find_last_of('/');
  size_t dt = p.find_last_of('.');
  bool ex = dt != std::string::npos && dt > (sl == std::string::npos ? 0 : sl + 1);
  std::string o = ex ? p.substr(0, dt) + "_a" + p.substr(dt) : p + "_a";
  for(size_t k = 0; k < ix.size(); k++)
    fb[ix[k]] = hv[k];
  b::hx ot = gd->ec(fb);
  std::ofstream f(o, std::ios::binary);
  if(!f.is_open()){
    std::cerr << "cant write " << o << std::endl;
    return;
  }
  f.write(reinterpret_cast<const char*>(ot.data()), ot.size());
  std::cout << "wrote " << o << std::endl;
}
