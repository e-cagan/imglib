# imglib

A minimal image-processing library written from scratch in C++17 — no OpenCV, no external
image libraries. The goal is not feature coverage but understanding: every design decision
here is deliberate and defensible.

The library currently provides an `Image` type and PPM (P6) file I/O. Filters
(grayscale, box blur, Sobel) are the next stage.

---

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
./imglib_test
```

Requires a C++17 compiler. `build/` is generated and not tracked in git.

```
imglib/
├── CMakeLists.txt
├── include/imglib/
│   ├── image.hpp      # Image class (interface + inline bodies)
│   └── ppm.hpp        # load_ppm / save_ppm declarations
└── src/
    ├── ppm.cpp        # load_ppm / save_ppm definitions
    └── main.cpp       # test / demo driver
```

---

## Design Decisions

Each choice below is recorded with the alternative it was chosen over and the reason.
This is the core value of the project: the decisions, not just the code.

### Memory layout: one flat buffer, not nested arrays

Pixels are stored in a single `std::vector<uint8_t>`, indexed manually, rather than a
nested `vector<vector<vector<>>>`.

- A nested structure scatters every row (or pixel) across separate heap allocations:
  thousands of allocations, poor cache locality, expensive to copy.
- A flat buffer is a single contiguous block: cache-friendly (neighbouring pixels are
  neighbours in memory), cheap to copy, one allocation.
- The convenient `[y][x][c]` syntax is not lost — it is recovered through `operator()`.

### Container: `std::vector<uint8_t>`

- Pixel values are 0–255, so a single byte (`uint8_t`) is the right element type.
- `std::vector` (not `std::array`) because the size is **not known at compile time** —
  it depends on which file the user opens, so dynamic allocation is required. The reason
  is "size known only at runtime", not "the image gets resized" (an image's dimensions are
  fixed once constructed).

### Dimension types

- `width_`, `height_` → `std::size_t`. They routinely reach the thousands, so `uint8_t`
  (max 255) would silently overflow — e.g. 640 would wrap to 128. `size_t` also matches
  the type returned by `vector::size()`, avoiding signed/unsigned mismatches, and holds the
  index product (up to ~6M for 1920×1080×3) without overflow.
- `channels_` → `uint8_t`. It is only ever 1 or 3.
- Pixel values → `uint8_t`. Range 0–255 fits a byte.

### Dimension metadata lives inside the object

`width_`, `height_`, `channels_` are member variables, not parameters passed to every
function.

- A flat buffer alone is meaningless: 921600 bytes could be 640×480×3 or something else.
  Interpreting it requires `width` and `channels`.
- The index formula needs them, so they must live somewhere: either in the object or in
  every call's parameter list. Storing them in the object means `blur(img)` reads
  `img.channels()` itself — the caller never passes them by hand.
- This is exactly what a class is for: bundling raw data with the metadata needed to
  interpret it (like a NumPy array carrying its `.shape`).

### Two `operator()` overloads (const-correctness)

```cpp
uint8_t& operator()(size_t x, size_t y, uint8_t c);        // non-const: returns reference (write)
uint8_t  operator()(size_t x, size_t y, uint8_t c) const;  // const: returns value (read only)
```

- You cannot take a mutable reference from a const object — the compiler forbids it. Without
  the const overload, any function taking `const Image&` could not even *read* pixels.
- The const version returns by value (a copy), so it permits reading but not writing.
- The compiler selects the overload based on whether the object is const — not on intent.
  A read from a non-const object still calls the non-const version.

This is the mechanism that turns "don't modify the original" (a `const Image&` parameter)
into a compiler-enforced guarantee.

### Index formula

```
index = y * (width_ * channels_) + x * channels_ + c
```

Row-major, coarse-to-fine: first skip to the right row (`y × row_size`, where
`row_size = width × channels`), then to the right pixel in that row (`x × channels`),
then to the right channel (`c`). Same idea as a NumPy stride of `(width*channels, channels, 1)`.
The formula yields an **index** into the buffer; the returned value is `pixels_[index]`,
not the index itself.

### `operator()` defined inline in the header

It is the hottest path in image processing — called millions of times. Inlining removes the
function-call overhead. (By contrast, PPM I/O lives in a `.cpp`: it runs once per image, so
inlining buys nothing and keeping `<fstream>` out of the header avoids leaking that
dependency to every user of `Image`.)

### `operator()` performs no bounds checking (deliberate)

Bounds checks on every pixel access would slow the hot loop. The caller is responsible.
This follows the `std::vector` convention: `operator[]` (unchecked, fast) vs `at()`
(checked, safe). A checked accessor may be added later.

### Constructor uses a member initializer list

```cpp
Image(size_t width, size_t height, uint8_t channels)
    : width_(width), height_(height), channels_(channels),
      pixels_(width * height * channels, 0)
{}
```

- Assignment in the body would default-construct each member and *then* assign (two steps);
  the initializer list constructs directly (one step), which matters for members like
  `std::vector`.
- `pixels_` cannot be sized in the class body (the dimensions aren't assigned yet, and the
  syntax is ambiguous); the initializer list has the parameters in hand.
- Members are initialized in **declaration order**, not the order written in the list.
  `pixels_` is declared last, so the dimensions it depends on are already set.

### PPM I/O as free functions, not member functions

`load_ppm` and `save_ppm` are free functions (`imglib::load_ppm(...)`), not methods on `Image`.

- An image is meaningful even if the PPM format never existed — a format is an external
  encoding applied to pixels, not part of their essence. So file I/O is not `Image`'s job.
- Keeping it out means `Image` stays minimal (single responsibility). Adding `load_png`
  later touches no existing code — just a new free function.
- Code that uses `Image` but never touches disk doesn't have to compile `<fstream>`.
- (PIL bundles everything into its `Image` because it hides *many* formats behind one
  interface and Python has no compile-time cost — but even PIL separates each format
  internally.)

### Failure signalling

- `load_ppm` returns `std::optional<Image>` — every `Image` is a valid image, so it cannot
  itself represent "failed to load". `optional` cleanly expresses "either an image or
  nothing". Chosen over exceptions: a corrupt file is an *expected* outcome, not an
  exceptional one, and C++ exceptions were deliberately left out of this stage's scope.
- `save_ppm` returns `bool` — it produces no value, only success/failure.

### `const` placement

`const` is decided per position by asking "will *I* modify this / am I promising the caller
not to touch it", not applied as a blanket rule.

- Parameters (`const std::string& path`, `const Image& img`) are const: the function reads
  them but promises not to change them.
- Return values are **not** const: the caller will process the returned image (e.g. apply a
  filter), so it must be free to modify it.

### Binary I/O uses `put` / `get`, not `<<` / `>>`

`<<` and `>>` are formatted text streams — they would interpret a `uint8_t` as a character.
Raw pixel bytes are written with `file.put(static_cast<char>(...))` and read with
`file.get()`. The header (magic number, dimensions, maxval) *is* text and is written/read
with `<<` / `>>`; the body is raw bytes. The boundary between the two is the subtle part of
the parser.

---

## Known Limitations (deliberate scope cuts for Stage 1)

These are conscious boundaries, not oversights:

- **Only P6 (3-channel RGB)** is supported. P5 (grayscale) is rejected.
- **Only `maxval == 255`** is accepted.
- **Header comments (`#...`)** — legal in real PPM files — are not handled.
- **Whitespace assumption:** after `maxval`, exactly one byte is skipped (`ignore(1)`),
  which assumes a single `\n`. Multiple or mixed whitespace in the header would break it.
- **`operator()` is unchecked** — out-of-range access is undefined behaviour.
- **Copy/move semantics are compiler-generated** — currently correct (deep copy, inherited
  from `std::vector`) but not yet deliberately designed.

---

## Future TODO

**Stage 1 completion — filters (next):**
- [ ] `to_grayscale` (RGB → single channel)
- [ ] `box_blur`
- [ ] `sobel` (edge detection)

**Rule of five / move semantics:**
- [ ] Explicitly design copy and move constructors / assignment (learncpp 15, 22). Filters
      return an `Image` by value, which makes move semantics observable — the natural place
      to introduce it.

**Robustness (lift the scope cuts above):**
- [ ] Support P5 (grayscale) alongside P6
- [ ] Handle header comments (`#`)
- [ ] Robust header whitespace parsing (replace `ignore(1)`)
- [ ] Add a checked accessor `at(x, y, c)` alongside `operator()`
- [ ] Per-byte EOF checking in the pixel-read loop

**Refactoring:**
- [ ] The three nested pixel loops are duplicated in `save_ppm` and `load_ppm`. A bulk
      raw-buffer accessor on `Image` would collapse both (and speed up I/O to a single
      `write`/`read`). Weigh against widening the `Image` interface.
- [ ] The index computation is duplicated across the two `operator()` overloads; the const
      version can delegate, or a private helper can hold it.

**Later stages:**
- [ ] Stage 2: proper unit tests (Catch2 / doctest), a CLI argument parser
- [ ] Stage 3: Qt viewer — display an image, apply filters via buttons
- [ ] Stage 4: video — replace the file source with a GStreamer / RTSP stream