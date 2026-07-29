# imglib

A minimal image-processing library written from scratch in C++17 — no OpenCV, no external
image libraries. The goal is not feature coverage but understanding: every design decision
here is deliberate and defensible.

The library provides an `Image` type, PPM/PGM file I/O (P6 and P5), three filters
(`to_grayscale`, `box_blur`, `sobel`), a doctest-based unit test suite, and a Qt5 GUI viewer
that loads an image, applies filters via buttons, and saves the result.

---

## Build

```bash
mkdir build && cd build
cmake ..
cmake --build .
./imglib_test      # demo driver
./unit_tests       # doctest suite
./viewer           # Qt GUI (requires qtbase5-dev)
```

Requires a C++17 compiler. `build/` is generated and not tracked in git.

```
imglib/
├── CMakeLists.txt
├── include/imglib/
│   ├── image.hpp      # Image class (interface + inline bodies)
│   ├── ppm.hpp        # load_ppm / save_ppm declarations
│   └── filters.hpp    # to_grayscale / box_blur / sobel declarations
├── src/
│   ├── ppm.cpp        # load_ppm / save_ppm definitions
│   ├── filters.cpp    # to_grayscale / box_blur / sobel definitions
│   └── main.cpp       # test / demo driver
├── tests/
│   ├── doctest.h      # single-header test framework
│   └── tests.cpp      # unit tests
└── apps/viewer/       # Qt5 GUI (Stage 3)
    ├── main.cpp       # QApplication entry point
    ├── viewer_window.hpp
    └── viewer_window.cpp
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
- A useful side effect: the constructor zero-fills the buffer, so a freshly constructed
  `Image` is all-black. Tests can paint only the pixels they care about and rely on the
  rest being 0.

### Rule of Zero — no hand-written copy/move/destructor

`Image` declares no copy constructor, move constructor, assignment operators, or destructor.
Its only resource-owning member is `std::vector<uint8_t>`, which already manages its own
memory correctly, so the compiler-generated special members are correct by construction:
copy does a deep copy (via the vector's copy constructor), move transfers ownership (via the
vector's move constructor). Writing them by hand would at best duplicate what the compiler
does and at worst introduce a bug.

This was verified empirically: temporary instrumented copy/move constructors (printing
"COPY"/"MOVE") showed that `Image b = a;` triggers a deep copy, `Image c = std::move(a);`
triggers a move (single buffer, ownership transferred, source emptied), and a filter's
`return result;` triggers a move rather than a copy — cheap, no pixel data duplicated. The
instrumentation was then removed; the class is optimal without it.

(Note: filters have two return paths — `return src;` and `return result;` — which prevents
full return-value optimization, so the return moves rather than eliding entirely. A move is
cheap enough that unifying the return path for RVO isn't worth it.)

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

### P5 and P6 share one code path

`save_ppm` / `load_ppm` handle both P6 (3-channel RGB, `.ppm`) and P5 (1-channel grayscale,
`.pgm`). The channel count is **derived from the magic number**, never passed as an argument:
P6 → 3, P5 → 1.

- The data is self-describing — the file already states its format. Asking the caller to
  supply the channel count would be redundant (they'd have to open the file to know) and
  unsafe (a caller/file mismatch produces garbage).
- The pixel loops iterate over `channels`, so the same loop serves both formats.

### Failure signalling

- `load_ppm` returns `std::optional<Image>` — every `Image` is a valid image, so it cannot
  itself represent "failed to load". `optional` cleanly expresses "either an image or
  nothing". Chosen over exceptions: a corrupt file is an *expected* outcome, not an
  exceptional one, and C++ exceptions were deliberately left out of this stage's scope.
- `save_ppm` returns `bool` — it produces no value, only success/failure.
- `load_ppm` validates at five points (file open, magic number, dimensions, maxval, and —
  after the pixel loop — a truncated-file check via `if (!file)`), returning `nullopt` at
  each. The last check was added after a unit test caught that a header-valid but
  pixel-truncated file was silently accepted (`file.get()` returns EOF, which cast to
  `uint8_t` became 255) instead of rejected.

### Filters signal invalid input by returning the source unchanged

`to_grayscale`, `box_blur`, and `sobel` all return `Image` (not `optional<Image>`), and on
invalid input they return `src` unchanged:

- `to_grayscale` on a non-3-channel image → returns `src`.
- `box_blur` with an even or non-positive kernel size → returns `src`.
- `sobel` on a non-1-channel image → returns `src`.

Reasoning:
- Filters are meant to be **chained** (`sobel(to_grayscale(img))`). Returning
  `optional<Image>` would break the chain — each call would need unwrapping. Invalid filter
  input is a *programming* error, not the *external, expected* failure that `optional` is for.
- Returning `src` keeps all filters consistent with each other and never destroys data
  (unlike returning a blank image). This is a deliberate, uniform contract, revisited under
  the refactor TODO (assert vs. `expected` vs. current behaviour).

### `const` placement

`const` is decided per position by asking "will *I* modify this / am I promising the caller
not to touch it", not applied as a blanket rule.

- Parameters (`const std::string& path`, `const Image& img`, `const Image& src`) are const:
  the function reads them but promises not to change them.
- Return values are **not** const: the caller will process the returned image (e.g. apply a
  filter), so it must be free to modify it.

### Binary I/O uses `put` / `get`, not `<<` / `>>`

`<<` and `>>` are formatted text streams — they would interpret a `uint8_t` as a character.
Raw pixel bytes are written with `file.put(static_cast<char>(...))` and read with
`file.get()`. The header (magic number, dimensions, maxval) *is* text and is written/read
with `<<` / `>>`; the body is raw bytes. The boundary between the two is the subtle part of
the parser.

---

## Filters

All three are free functions taking `const Image&` and returning a new `Image` (never
mutating the source — the `const&` parameter makes that a compiler-enforced guarantee, and a
separate output buffer avoids corrupting not-yet-read neighbours mid-pass).

### `to_grayscale` — RGB → single channel

A **point operation**: each output pixel depends only on its own input pixel. Uses the
BT.601 luma formula `0.299·R + 0.587·G + 0.114·B`, chosen over a naive `(R+G+B)/3` average
because it reflects human perceptual sensitivity (most sensitive to green, least to blue).
Intermediate value is computed in `float`, rounded with `std::round`, then narrowed to
`uint8_t`. Output is 1-channel.

### `box_blur` — neighbourhood average

A **neighbourhood operation**: each output pixel is the mean of its NxN window — the first
filter where a pixel's output depends on its neighbours.

- **Kernel size must be odd** (3, 5, 7…): the window needs a centre pixel. Even/non-positive
  sizes are rejected (return `src`), matching OpenCV's behaviour of rejecting even kernels.
  `radius = kernel_size / 2`.
- **Border handling: clamp (replicate).** Out-of-range neighbour coordinates are pulled to
  the nearest valid pixel. Chosen over zero-pad (darkens edges — an artefact), crop (changes
  output size), and reflect (more complex). Clamp gives least information loss with no visible
  artefact.
- **Signed coordinate arithmetic.** Neighbour offsets can be negative (`-radius`), but
  coordinates are `size_t` (unsigned). `x + dx` in unsigned arithmetic underflows to a huge
  value. So coordinates are computed as `int`, clamped, then cast back to `size_t`.
- **Accumulator is `float`, not `uint8_t`.** A window sum exceeds 255 (e.g. 9·255 for 3×3),
  which would overflow a byte.
- Output preserves the input channel count (unlike `to_grayscale`), and each channel is
  blurred independently.

### `sobel` — edge detection

A **gradient operation**: measures how fast brightness changes. Flat regions give ~0 (dark
output); sharp edges give high values (bright output).

- **Input must be 1-channel** (non-1-channel → return `src`). Gradient is meaningful on
  brightness, not colour — so callers grayscale first: `sobel(to_grayscale(img))`. Keeping
  the conversion out of `sobel` preserves single responsibility and chainability.
- **Two fixed 3×3 kernels**, `sobel_x` and `sobel_y`, applied to the same window: each
  neighbour is multiplied by its kernel weight and summed into `gx` / `gy` (unlike `box_blur`,
  which just sums). Kernel index maps the `[-1, +1]` offset to `[0, 2]` via `[dy+1][dx+1]`.
- **Gradients are `int`, not `uint8_t`.** Sobel weights include negatives, so `gx` / `gy` can
  go negative — an unsigned type would wrap.
- **Magnitude and clamp.** The two gradients combine as `sqrt(gx² + gy²)`, always positive but
  able to exceed 255 on strong edges — so it is clamped to `[0, 255]` before narrowing. Reuses
  the same clamped-coordinate border handling as `box_blur`.
- Output is 1-channel.

Validated on a synthetic vertical edge (left half 0, right half 255): the edge map reads
`0 255 255 0` across the boundary — zero in the flat regions, saturated at the transition.

---

## Tests

A doctest-based suite (`tests/tests.cpp`, run via `./unit_tests`) — 37 assertions across four
cases. Tests treat each function as a black box: set up an input, call the function, check the
output. They never inspect internals.

- **Image basics** — dimensions, zero-initialization, pixel write/read.
- **`load_ppm` failure paths** — one test per validation branch: nonexistent file, invalid
  magic, zero dimensions, invalid maxval, truncated pixel data. Each writes a deliberately
  malformed file, then asserts `nullopt`. (Writing a valid header but omitting pixel bytes is
  how the truncated-file bug above was caught.)
- **Round-trip** — build an `Image`, `save_ppm`, `load_ppm`, compare values, for both P6 and P5.
- **Filter happy paths** — grayscale (red→76, green→150, white→255), box_blur (single
  point→28), sobel (vertical edge→`0 255 255 0`).
- **Filter edge cases** — each filter's `return src` contract (wrong channel count, even kernel)
  and box_blur on a 1×1 image.

---

## Viewer (Stage 3)

A Qt5 `QWidget` GUI (`apps/viewer/`) with an image area and six buttons: **Load**, **Save**,
**Original**, **Grayscale**, **Blur**, **Sobel**.

- **State model — two images.** `original_` (the loaded image, never mutated) and `current_`
  (what is displayed), both `std::optional<imglib::Image>` so "nothing loaded yet" is
  representable without a default-constructed `Image`. Every filter reads `original_` and
  writes `current_`, so filters never stack (Grayscale then Blur re-starts from the original,
  not the greyed image), and **Original** simply restores `current_ = original_` — there is no
  separate "undo", just switching which image is shown.
- **Image → screen bridge.** `Image::data()` exposes the raw buffer (`const uint8_t*`); a
  `QImage` is constructed over it (format chosen by channel count: `Format_RGB888` for 3,
  `Format_Grayscale8` for 1), then `QPixmap::fromImage` copies it into the label. `QImage`
  only references the buffer, but `QPixmap` copies, so the transient `QImage` is safe.
- **Signals and slots.** Each button's `clicked` signal is `connect`-ed to a slot; every slot
  guards on `if (!original_) return` first. Save reports failure via `QMessageBox`.
- **imglib stays Qt-free.** The library has no Qt dependency; only the viewer links Qt. The one
  library change the GUI required was adding `Image::data()` (a raw-buffer accessor that was
  already on the refactor TODO).

---

## Known Limitations (deliberate scope cuts for Stage 1)

These are conscious boundaries, not oversights:

- **Only `maxval == 255`** is accepted.
- **Header comments (`#...`)** — legal in real PPM files — are not handled.
- **Whitespace assumption:** after `maxval`, exactly one byte is skipped (`ignore(1)`),
  which assumes a single `\n`. Multiple or mixed whitespace in the header would break it.
- **`operator()` is unchecked** — out-of-range access is undefined behaviour.
- **Filters return `src` on invalid input** — a silent fallback, not an error signal (see
  refactor TODO).

---

## Future TODO

**Stage 1 — core (done):**
- [x] `Image`, PPM/PGM I/O (P6 + P5)
- [x] `to_grayscale`, `box_blur`, `sobel`
- [x] Rule of Zero verified (copy/move behaviour measured empirically)

**Stage 2 — tooling:**
- [x] Unit tests (doctest): Image, PPM round-trip, filters, failure paths
- [ ] CLI argument parser (load → filter → save from the command line)

**Robustness (lift the scope cuts above):**
- [ ] Handle header comments (`#`)
- [ ] Robust header whitespace parsing (replace `ignore(1)`)
- [ ] Add a checked accessor `at(x, y, c)` alongside `operator()`
- [ ] Per-byte EOF checking in the pixel-read loop (currently one post-loop `!file` check)

**Test hygiene:**
- [ ] Tests leave scratch files (`bad_magic.ppm`, `rt6.ppm`, …) in the build dir — clean up
      with `std::remove`, or write to a temp dir.
- [ ] `.gitignore` the generated `*.ppm` / `*.pgm` scratch files.

**Refactoring:**
- [ ] The three nested pixel loops are duplicated in `save_ppm` and `load_ppm`. A bulk
      raw-buffer accessor on `Image` would collapse both (and speed up I/O to a single
      `write`/`read`). Weigh against widening the `Image` interface.
- [ ] The index computation is duplicated across the two `operator()` overloads; the const
      version can delegate, or a private helper can hold it.
- [ ] Unify the filter error strategy: `return src` vs. `assert` vs. `expected<Image, Error>`
      (C++23). Currently `return src` for consistency; revisit once the library has a
      coherent error policy.
- [ ] `box_blur` is O(w·h·k²). A separable blur (horizontal then vertical pass) is O(w·h·k)
      and much faster for large kernels.
- [ ] The failure-path tests share a "write malformed file, expect nullopt" shape — a small
      helper could collapse them.

**Stage 3 — Qt viewer (done):**
- [x] Display an image, apply filters (grayscale/blur/sobel) via buttons
- [x] Load / Save via QFileDialog, save-failure reported via QMessageBox
- [ ] Auto-append `.ppm`/`.pgm` extension on save when the user omits it (currently the user
      types the extension; QFileDialog's filter shows the right one)
- [ ] Nicer layout — Load/Save on the side rather than stacked with the filter buttons

**Stage 4 — video:** replace the file source with a GStreamer / RTSP stream.