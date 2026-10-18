#include <atomic>
#include <cstddef>
#include <cstdint>
#include <memory>
#include <string>
#include <thread>
#include <termios.h>
#include <vector>


#define LOOPS 4


/*
 * ruleset on number value reference number function
 * on letter value reference letter function
 *
 * numbers: if next hex value has number in first slot and is greater than number than first slot swap last characters
 * if second value is number and greater than previous (if available) second slot swap first slots with 2 hexes foward (if last wrap to beginning)
 * if any value in hex is fibonacci add its number to the next hex spot  ( if overflow wrap to beginning of hex characters)
 *
 * letters: first slot if next hex value has number in first slot subtract 1
 * if 2 letters subtract one character index from first index and add one to last index
 * if previous hex has a letter operation swap hex values with a index n spaces away where n is equal to the hex value provided by the following hex value (allow wrap for all rules)
 * */

/*
 *  lifecycle
 *  ---------
 *  a(path): file -> gp() picks a guard g by magic bytes
 *           fb = g->dc(file)       editable bytes (png: inflated scanlines, else the file as is)
 *           g->mk(fb, m)           m[q] true = safe to swap
 *           hv = fb gathered at m  ix: hv -> fb index, rv: fb -> hv index (NF = locked)
 *
 *  a::r() -> st() -----> fe thread: lp()
 *    |                     for LOOPS, for every byte i of hv:
 *    |                       pl ? d(b::sf, i)
 *    |                       d(b::ls[..], i)       letters, first hit wins -> pl
 *    |                       !pl ? d(b::ns[..], i) every number rule
 *    |                     d(): rule fired -> g->ok() on new values, else revert
 *    |                          -> dr() frame -> sleep DL
 *    | (waits on R)
 *    v
 *  e(): join fe -> rs() terminal back -> w(): hv scattered into fb -> g->ec(fb) -> <name>_a.<ext>
 *
 *  guards           free to swap                          locked
 *  g  (abstract)
 *  |- rw  raw       everything                            -
 *  |- jp  jpeg      entropy coded scan data               markers, tables, ff00 stuffing, rst, never writes 0xff
 *  |- m3  mp3       frame main data                       id3, frame headers, crc, side info, first (xing) frame
 *  |- m4  mp4       h264/hevc slice data                  boxes, audio, nal lengths, nal + slice headers (NH)
 *  '- pn  png       filtered scanline bytes               filter byte per row, chunks rebuilt + crc'd on ec
 *
 *  byte 0x3a        rule fn ptr: bool f(hv, i, j&)
 *         ||          only ever touches hv[i] and hv[j],
 *         |+- lo      j stays i when it's a one byte rule
 *         +-- hi
 *
 *  hv  live free bytes  ov  hv as of the last frame (diffed to find highlights)
 *  rl  { f fn ptr, c ansi bg color, n legend label }
 */

class a {
public:
  struct b{
    using hx = std::vector<unsigned char>;
    using fn = bool(*)(hx&, size_t, size_t&);
    struct rl{ fn f; int c; const char* n; };

    static int hi(unsigned char x);
    static int lo(unsigned char x);
    static unsigned char pk(int h, int l);
    static bool nm(int n);
    static bool fb(int n);
    static void sh(hx& h, size_t i, size_t j);
    static void sl(hx& h, size_t i, size_t j);

    static bool n1(hx& h, size_t i, size_t& j);
    static bool n2(hx& h, size_t i, size_t& j);
    static bool n3(hx& h, size_t i, size_t& j);
    static bool l1(hx& h, size_t i, size_t& j);
    static bool l2(hx& h, size_t i, size_t& j);
    static bool l3(hx& h, size_t i, size_t& j);

    static const rl ns[3];
    static const rl ls[2];
    static const rl sf;
  };

  struct g{
    virtual ~g() = default;
    virtual const char* id() const = 0;
    virtual b::hx dc(const b::hx& f);
    virtual b::hx ec(const b::hx& d);
    virtual void mk(const b::hx& d, std::vector<bool>& m) = 0;
    virtual bool ok(unsigned char v) const;
    static uint32_t u2(const b::hx& f, size_t p);
    static uint32_t u4(const b::hx& f, size_t p);
    static uint64_t u8(const b::hx& f, size_t p);
    static uint32_t fc(const char* s);
  };

  struct rw : g{
    const char* id() const override;
    void mk(const b::hx& d, std::vector<bool>& m) override;
  };

  struct jp : g{
    const char* id() const override;
    void mk(const b::hx& d, std::vector<bool>& m) override;
    bool ok(unsigned char v) const override;
  };

  struct m3 : g{
    const char* id() const override;
    void mk(const b::hx& d, std::vector<bool>& m) override;
    static size_t fh(const b::hx& f, size_t p, size_t& hl);
  };

  struct m4 : g{
    const char* id() const override;
    void mk(const b::hx& d, std::vector<bool>& m) override;
  private:
    static constexpr size_t NH = 32; // slice header bytes kept at the front of each video nal, they're exp-golomb so no fixed size
    static bool bx(const b::hx& f, size_t p, size_t e, size_t& ps, size_t& be, uint32_t& t);
    static bool fd(const b::hx& f, size_t s, size_t e, uint32_t t, size_t& ps, size_t& pe);
    static void tk(const b::hx& f, size_t s, size_t e, std::vector<bool>& m);
    static void nl(const b::hx& f, size_t p, size_t e, size_t ls, bool hv, std::vector<bool>& m);
  };

  struct pn : g{
    const char* id() const override;
    b::hx dc(const b::hx& f) override;
    b::hx ec(const b::hx& d) override;
    void mk(const b::hx& d, std::vector<bool>& m) override;
  private:
    struct ck{ uint32_t t; b::hx d; };
    struct hf{ short c[16]; short s[320]; };
    struct zi{
      const b::hx& s;
      size_t p = 2;
      uint32_t bb = 0;
      int bc = 0;
      b::hx o;
      b::hx go();
      int bt(int n);
      int sy(const hf& h);
      void cd(const hf& l, const hf& d);
      static void bd(hf& h, const short* l, int n);
    };
    static uint32_t cr(const unsigned char* d, size_t n);
    static void pc(b::hx& o, uint32_t t, const unsigned char* d, size_t n);
    std::vector<ck> cs;
    uint64_t w = 0;
    uint64_t h = 0;
    int dp = 0;
    int ct = 0;
    int il = 0;
  };

  a(std::string pth);
  void r();
  void e();
private:
  bool d(const b::rl& r, size_t i);
  void st();
  void lp();
  void dr(const b::rl& r, size_t i, size_t j);
  void w();
  static std::string cl(const b::rl& r);
  static std::unique_ptr<g> gp(const b::hx& f);
  static void rs();
  static void sg(int);

  static constexpr int DL = 0; // us each rule frame stays on screen
  static constexpr size_t NF = SIZE_MAX;
  static termios ot;
  static bool tt;
  std::atomic<bool> R{false};
  std::thread fe;
  std::string p = "";
  std::unique_ptr<g> gd;
  b::hx fb;
  b::hx hv;
  b::hx ov;
  std::vector<size_t> ix;
  std::vector<size_t> rv;
  int lk = 0;
};
