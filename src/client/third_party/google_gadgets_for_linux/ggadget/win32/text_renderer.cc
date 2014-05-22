/*
  Copyright 2011 Google Inc.

  Licensed under the Apache License, Version 2.0 (the "License");
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#include "text_renderer.h"

#include <mlang.h>
#include <windows.h>
#ifdef GDIPVER
#undef GDIPVER
#endif
#define GDIPVER 0x0110
using std::min;
using std::max;
#include <gdiplus.h>
#include <usp10.h>
#include <algorithm>
#include "ggadget/math_utils.h"
#include "ggadget/unicode_utils.h"
#include "ggadget/win32/font_fallback.h"
#include "ggadget/win32/gdiplus_canvas.h"
#include "ggadget/win32/gdiplus_graphics.h"
#include "ggadget/win32/private_font_database.h"

namespace ggadget {
namespace win32 {


namespace {
// The maximum supported number of Uniscribe runs; a SCRIPT_ITEM is 8 bytes.
const int kGuessItems = 100;
const int kMaxItems = 10000;

// The maximum supported number of Uniscribe glyphs; a glyph is 1 word.
const int kMaxGlyphs = 100000;

static const int kPointsPerInch = 72;

// Assign a to b, and if b != a, return true.
template <class T> bool AssignIfDiffer(const T& a, T* b) {
  if (*b == a)
    return false;
  *b = a;
  return true;
}

inline double PointToPixel(double point, HDC hdc) {
  return point * ::GetDeviceCaps(hdc, LOGPIXELSY) / kPointsPerInch;
}

// STLDeleteContainerPointers()
//  For a range within a container of pointers, calls delete
//  (non-array version) on these pointers.
// NOTE: for these three functions, we could just implement a DeleteObject
// functor and then call for_each() on the range and functor, but this
// requires us to pull in all of algorithm.h, which seems expensive.
// For hash_[multi]set, it is important that this deletes behind the iterator
// because the hash_set may call the hash function on the iterator when it is
// advanced, which could result in the hash function trying to deference a
// stale pointer.
template <class ForwardIterator>
void STLDeleteContainerPointers(ForwardIterator begin, ForwardIterator end) {
  while (begin != end) {
    ForwardIterator temp = begin;
    ++begin;
    delete *temp;
  }
}

inline bool ShouldBreak(const TextFormat& style_a, const TextFormat& style_b) {
  return style_a.font() != style_b.font() ||
         style_a.size() != style_b.size() ||
         style_a.scale() != style_b.scale() ||
         style_a.rise() != style_b.rise() ||
         style_a.script_type() != style_b.script_type() ||
         style_a.bold() != style_b.bold() ||
         style_a.italic() != style_b.italic();
}

// Gets the metrics of a font. The metrics include the position and size of the
// strikeout and underline stoke. If the font doesn't have strikeout or
// underline style, the corresponding size and postion will set to 0. The
// position is the distance between the stroke and font baseline.
void GetFontMetric(
    const TextFormat& format,
    HFONT hfont,
    Gdiplus::REAL* strikeout_pos, Gdiplus::REAL* strikeout_size,
    Gdiplus::REAL* underline_pos, Gdiplus::REAL* underline_size) {
  OUTLINETEXTMETRIC text_metric;
  HDC hdc = ::CreateCompatibleDC(NULL);
  double size = PointToPixel(format.size(), hdc) * format.scale();
  ::SelectObject(hdc, hfont);
  if (::GetOutlineTextMetrics(hdc, sizeof(OUTLINETEXTMETRIC), &text_metric)) {
    int font_heigth = text_metric.otmAscent - text_metric.otmDescent;
    *strikeout_pos = (size * text_metric.otmsStrikeoutPosition) /
                     font_heigth;
    *strikeout_size = (size * text_metric.otmsStrikeoutSize) /
                      font_heigth;
    *underline_pos = (size * text_metric.otmsUnderscorePosition) /
                     font_heigth;
    *underline_size = (size * text_metric.otmsUnderscoreSize) /
                      font_heigth;
  } else {
    *strikeout_pos = size * 6 / 21;
    *underline_pos = -size / 9;
    *strikeout_size = size / 18;
    *underline_size = size / 18;
  }
  ::DeleteDC(hdc);
  ::DeleteObject(hfont);
}

// Gets the font size and verticle offset if the format is subscript or
// superscript.
void GetScriptSizeAndOffset(const TextFormat& format,
                            const GdiplusGraphics* graphics,
                            double* pt_size, double* px_offset) {
  if (format.script_type() == NORMAL)
    return;
  HFONT hfont = graphics->CreateHFont(&format);
  OUTLINETEXTMETRIC text_metric;
  HDC hdc = ::CreateCompatibleDC(NULL);
  double size = format.size() * format.scale();  // In points.
  ::SelectObject(hdc, hfont);
  if (::GetOutlineTextMetrics(hdc, sizeof(OUTLINETEXTMETRIC), &text_metric)) {
    double font_height = text_metric.otmAscent - text_metric.otmDescent;
    if (format.script_type() == SUBSCRIPT) {
      *pt_size = text_metric.otmptSubscriptSize.y / font_height * size;
      *px_offset = PointToPixel(
          text_metric.otmptSubscriptOffset.y / font_height * size, hdc);
    } else {
      *pt_size = text_metric.otmptSuperscriptSize.y / font_height * size;
      *px_offset = -PointToPixel(
          text_metric.otmptSuperscriptOffset.y / font_height * size, hdc);
    }
  } else {
    *pt_size = format.size() / 2;
    if (format.script_type() == SUBSCRIPT)
      *px_offset = 0;
    else
      *px_offset = PointToPixel(size / 2, hdc);
  }
  ::DeleteDC(hdc);
  ::DeleteObject(hfont);
}

// Gets the line space(height) of given font.
void GetLineSpace(HFONT hfont, HDC hdc, double size, double* line_space) {
  OUTLINETEXTMETRIC text_metric;
  ::SelectObject(hdc, hfont);
  if (::GetOutlineTextMetrics(hdc, sizeof(OUTLINETEXTMETRIC), &text_metric)) {
    int font_heigth = text_metric.otmAscent - text_metric.otmDescent;
    *line_space = (size * text_metric.otmLineGap) / font_heigth;
  } else {
    *line_space = size *1.2;
  }
}

// Gets the font height (in pixel) including ascend and descend.
//
// _______________________________
//    |             |        |
//  __|  __         |     ascend
// |  | |  \      height     |
// \__| |__/        |     ___|___ baseline
//      |           |     descend
// _____|___________|________|___
void GetFontHeight(const TextFormat& format,
                   HFONT hfont,
                   const GdiplusGraphics* graphics,
                   double* height,
                   double* ascend, double* descend,
                   double* line_space) {
  TEXTMETRIC text_metric = {0};
  HDC hdc = ::CreateCompatibleDC(NULL);
  ::SelectObject(hdc, hfont);
  ::GetTextMetrics(hdc, &text_metric);
  *height = PointToPixel(format.size() * graphics->GetFontScale(), hdc) *
            format.scale();
  *ascend = (*height * text_metric.tmAscent) / text_metric.tmHeight;
  *descend = (*height * text_metric.tmDescent) / text_metric.tmHeight;
  GetLineSpace(hfont, hdc, *height, line_space);
  ::DeleteDC(hdc);
}

}  // namespace

class TextRenderer::Impl {
 public:
  explicit Impl(const GdiplusGraphics* graphics);
  ~Impl();
  // A TextRun is a sequence of text that has the same attributes (including
  // formats, language, scripts, reading direction, etc).
  class TextRun {
   public:
    TextRun()
      : x_(0),
        y_(0),
        y_offset_(0),
        width_(0),
        glyph_count_(0),
        script_cache_(NULL),
        font_(NULL),
        shaped_and_placed_(false) {
    }

    ~TextRun() {
      if (script_cache_) ::ScriptFreeCache(&script_cache_);
      if (font_) ::DeleteObject(font_);
    }

    int NextCharInRun(int current_char) {
      int next = current_char + (script_analysis_.fRTL ? -1 : 1);
      return next;
    }

    // Return the index of the first glyph of the given character at
    // |code_point|.
    int GetFirstGlyph(int code_point) {
      return logical_clusters_[code_point - range_.start];
    }

    // Calculates the total glyphs width of given character at |code_point|.
    double GetCharWidth(int code_point) {
      int glyph_start = GetFirstGlyph(code_point);
      int glyph_end = 0;
      int delta = script_analysis_.fRTL ? -1 : 1;
      int next_char = code_point + delta;
      if (InRun(next_char))
        glyph_end = GetFirstGlyph(next_char);
      else
        glyph_end = glyph_count_;
      double width = 0;
      for (int glyph = glyph_start; glyph < glyph_end; ++glyph)
        width += advance_widths_[glyph];
      return width;
    }

    // Returns true if |code_point| is in this text run
    bool InRun(int code_point) {
      return code_point >= range_.start &&
             code_point < range_.end;
    }

   private:
    ggadget::TextFormat format_;
    ggadget::Range range_;
    int x_;
    int y_;  // The y coordinate of baseline.
    int top_;  // The top of text bounding box.
    int y_offset_;
    int width_;
    int height_;
    SCRIPT_ANALYSIS script_analysis_;  // Filled by ScriptItemize.

    // The following variables will be filled by ScriptShape.
    int glyph_count_;
    ggadget::scoped_array<WORD> glyphs_;
    ggadget::scoped_array<WORD> logical_clusters_;
    ggadget::scoped_array<SCRIPT_VISATTR> visible_attributes_;

    // The following variables will be filled by ScriptPlace.
    ggadget::scoped_array<int> advance_widths_;
    ggadget::scoped_array<GOFFSET> offset_;
    ABC abc_widths_;

    // An internal cache for Uniscribe.
    SCRIPT_CACHE script_cache_;

    HFONT font_;
    // True if this text run is already processed by ScriptShape and
    // ScriptPlace.
    bool shaped_and_placed_;
    friend class TextRenderer::Impl;

    DISALLOW_EVIL_CONSTRUCTORS(TextRun);
  };

  struct FontFallbackData {
    TextRun* run;
    Impl* impl;
  };
  // Draw the attributed text.
  // If brush is given, the foreground color of StyleRange will
  // be omitted.
  void Draw(const Gdiplus::Brush* brush,
            double opacity,
            Gdiplus::Graphics* canvas);
  void SetMarkUpText(const std::string& mark_up_text,
                     const TextFormat& base_format);
  void SetTextAndFormats(const std::string& text, const TextFormats& formats);
  void SetText(const std::wstring& text);
  void SetFormats(const TextFormats& style_ranges);
  void GetTextExtents(double* width, double* height);
  int GetTextRangeBoundingBoxes(
      const Range& range,
      std::vector<Rectangle>* bounding_boxes);
  void DrawCaret(CanvasInterface* canvas,
                 int caret_pos,
                 const Color& color);
  void ScriptShape(TextRun* run);

 private:
  // Itemize the rich format text. Breaks the text into text runs.
  // A text run is a sequence of text of the same attribute (including format,
  // language, reading direction, etc).
  void ItemizeLogicalText();
  // Layouts all text. Returns the height of text.
  double LayoutVisualText();
  // Layouts a text run.
  void LayoutTextRun(TextRun* run);
  // Layout a line of text, returns the height of this line.
  double LayoutLine(int first_run, int last_run,
                    double current_y,
                    double line_width);
  // Finds a point to break the current line.
  void FindBreakPoint(int first_run,
                      int* last_run,
                      int* last_char,
                      double* width);
  TextRun* BreakTextRun(int break_point, TextRun* current_run);
  // Draw underline and strikeouts for the text run.
  void DrawTextRunDecorations(const TextRun& run,
                              double x, double y,
                              const Gdiplus::Brush* brush,
                              double opacity,
                              Gdiplus::Graphics* graphics);
  // Convert sub/superscript font size and offset.
  void ProcessRunScriptType(TextRun* run);
  // Breaks the text into several runs according to text direction.
  // Each item in |output_runs| will have its own text direction.
  void BreakTextByDirection(
      std::vector<std::pair<Range, bool /* rtl */> >* output_runs);
  CanvasInterface::Alignment align_;
  CanvasInterface::VAlignment valign_;
  // TODO(synch): support trimming.
  CanvasInterface::Trimming trimming_;
  bool word_wrap_;

  std::vector<TextRun*> runs_;

  std::wstring text_;
  TextFormats formats_;
  TextFormat default_format_;
  scoped_array<SCRIPT_LOGATTR> logical_attributes_;
  HDC dc_;
  double x_;
  double y_;              // The coordinates of the top-left of layout box.
  double width_;
  double height_;         // The actural width and height of the text.
  double layout_width_;
  double layout_height_;  // The Width and height of the layout box.
  // TODO(synch): use following flags to avoid layout the same text multiple
  // times.
  bool text_changed_;
  bool layout_changed_;

  const GdiplusGraphics* graphics_;

  friend class TextRenderer;
  DISALLOW_EVIL_CONSTRUCTORS(Impl);
};

TextRenderer::Impl::Impl(const GdiplusGraphics* graphics)
    : align_(CanvasInterface::ALIGN_LEFT),
      valign_(CanvasInterface::VALIGN_TOP),
      word_wrap_(false),
      dc_(::CreateCompatibleDC(NULL)),
      x_(0),
      y_(0),
      width_(0),
      height_(0),
      layout_width_(0),
      layout_height_(0),
      text_changed_(true),
      layout_changed_(true),
      graphics_(graphics) {
  ASSERT(graphics);
  ASSERT(dc_);
}

TextRenderer::Impl::~Impl() {
  ::DeleteDC(dc_);
  STLDeleteContainerPointers(runs_.begin(), runs_.end());
}

struct RunBoundary {
  enum BoundaryType {
    FORMAT_START,
    FORMAT_END,
    ITEM,
  };
  RunBoundary(int param_code_point,
              BoundaryType param_type,
              int param_format_index)
      : code_point(param_code_point),
        type(param_type),
        format_index(param_format_index) {
  }

  int code_point;
  BoundaryType type;
  int format_index;
};

bool LessThan(const RunBoundary&a, const RunBoundary&b) {
  if (a.code_point > b.code_point) {
    return false;
  } else if (a.code_point == b.code_point) {
    if (a.type == RunBoundary::FORMAT_END &&
        b.type == RunBoundary::FORMAT_START) {
      return true;
    }
    return false;
  }
  return true;
}

void TextRenderer::Impl::BreakTextByDirection(
      std::vector<std::pair<Range, bool /* rtl */> >* output_runs) {
  std::vector<RunBoundary> boundaries;
  for (size_t i = 0; i < formats_.size(); ++i) {
    if (formats_[i].format.has_text_rtl()) {
      boundaries.push_back(
          RunBoundary(formats_[i].range.start, RunBoundary::FORMAT_START, i));
      boundaries.push_back(
          RunBoundary(formats_[i].range.end, RunBoundary::FORMAT_END, i));
    }
  }
  // Add a boundary at the end of the text.
  boundaries.push_back(RunBoundary(text_.length(), RunBoundary::ITEM, 0));
  std::stable_sort(boundaries.begin(), boundaries.end(), LessThan);
  bool current_rtl = default_format_.text_rtl();
  int current_code_point = 0;
  std::set<int /*format index */ > format_stack;
  for (size_t i = 0; i < boundaries.size(); ++i) {
    if (current_code_point < boundaries[i].code_point) {
      std::pair<Range, bool> range;
      range.first.start = current_code_point;
      range.first.end = boundaries[i].code_point;
      range.second = current_rtl;
      output_runs->push_back(range);
    }
    current_code_point = boundaries[i].code_point;
    switch (boundaries[i].type) {
      case RunBoundary::FORMAT_START:
        current_rtl = formats_[boundaries[i].format_index].format.text_rtl();
        format_stack.insert(boundaries[i].format_index);
        break;
      case RunBoundary::FORMAT_END:
        format_stack.erase(boundaries[i].format_index);
        if (format_stack.empty())
          current_rtl = default_format_.text_rtl();
        else
          current_rtl = formats_[*format_stack.rbegin()].format.text_rtl();
        break;
      case RunBoundary::ITEM:
        break;
    }
  }
}

// Itemizes the given text. It will break the text into text runs. A text run
// is a sequence of text with the same language and formats.
void TextRenderer::Impl::ItemizeLogicalText() {
  STLDeleteContainerPointers(runs_.begin(), runs_.end());
  runs_.clear();
  if (text_.empty())
    return;

  // Breaks the text into runs with unique text direction.
  std::vector<std::pair<Range, bool /* rtl */> > output_runs;
  BreakTextByDirection(&output_runs);

  // Breaks the text into logical runs.
  const wchar_t* raw_text = text_.c_str();
  const int text_length = text_.length();

  HRESULT hr = E_OUTOFMEMORY;
  std::vector<SCRIPT_ITEM> script_items;
  for (size_t i = 0; i < output_runs.size(); ++i) {
    int current_script_items_count = 0;
    hr = E_OUTOFMEMORY;
    scoped_array<SCRIPT_ITEM> current_script_items;
    for (size_t n = kGuessItems; hr == E_OUTOFMEMORY && n < kMaxItems; n *= 2) {
      // Derive the array of Uniscribe script items from the logical text.
      // ScriptItemize always adds a terminal array item so that the length of
      // the last item can be derived from the terminal SCRIPT_ITEM::iCharPos.
      current_script_items.reset(new SCRIPT_ITEM[n]);
      SCRIPT_CONTROL control = {0};
      control.fNeutralOverride = output_runs[i].second;
      SCRIPT_STATE state = {0};
      if (default_format_.text_rtl()) {
        state.uBidiLevel = output_runs[i].second ? 1 : 2;
      } else {
        state.uBidiLevel = output_runs[i].second ? 1 : 0;
      }
      hr = ::ScriptItemize(raw_text + output_runs[i].first.start,
                           output_runs[i].first.Length(),
                           n - 1,
                           &control,
                           &state,
                           current_script_items.get(),
                           &current_script_items_count);
    }
    ASSERT(SUCCEEDED(hr));
    for (int j = 0; j < current_script_items_count; ++j)
      current_script_items[j].iCharPos += output_runs[i].first.start;
    script_items.insert(
        script_items.end(),
        current_script_items.get(),
        current_script_items.get() + current_script_items_count);
  }
  SCRIPT_ITEM last_item = {0};
  last_item.iCharPos = text_length;
  script_items.push_back(last_item);
  if (script_items.empty())
    return;
  logical_attributes_.reset(new SCRIPT_LOGATTR[text_length]);
  for (size_t i = 0; i < script_items.size() - 1; ++i) {
    ::ScriptBreak(raw_text + script_items[i].iCharPos,
                  script_items[i + 1].iCharPos - script_items[i].iCharPos,
                  &script_items[i].a,
                  logical_attributes_.get() + script_items[i].iCharPos);
    logical_attributes_[script_items[i].iCharPos].fWordStop = true;
  }
  // Merge logical runs with format runs.
  // Build the list of runs, merge font/underline styles.
  // Here is an example of text run merging:
  // ***********************************           Text
  // |___formatA____|  |__formatB______|
  // |__________formatC________________|           Formats
  // |_item1_|__item2____|___item3_____|           script_items
  //
  // |___1___|__2___|3_|4|_____5_______|           merged text runs
  // The attributes of text runs will be:
  // Text Run |  script items |  format:
  //    1            1            A+C
  //    2            2            A+C
  //    3            2            C
  //    4            2            B+C
  //    5            3            B+C
  std::vector<RunBoundary> boundaries;
  for (size_t i = 0; i < script_items.size(); ++i) {
    boundaries.push_back(
        RunBoundary(script_items[i].iCharPos, RunBoundary::ITEM, i));
  }
  for (size_t i = 0; i < formats_.size(); ++i) {
    if (formats_[i].range.Length() == 0)
      continue;
    boundaries.push_back(
        RunBoundary(formats_[i].range.start, RunBoundary::FORMAT_START, i));
    boundaries.push_back(
        RunBoundary(formats_[i].range.end, RunBoundary::FORMAT_END, i));
  }
  std::stable_sort(boundaries.begin(), boundaries.end(), LessThan);
  std::vector<TextFormat> format_stack;
  TextFormat current_format;
  int last_boundary = 0;
  int current_item = -1;
  for (size_t i = 0; i < boundaries.size(); ++i) {
    if (boundaries[i].code_point > last_boundary) {
      TextRun* run = new TextRun;
      run->format_ = current_format;
      run->format_.set_default_format(&default_format_);
      run->range_.start = last_boundary;
      run->range_.end = boundaries[i].code_point;
      run->script_analysis_ = script_items[current_item].a;
      if (runs_.size()) {
        if (!ShouldBreak(runs_.back()->format_, run->format_)) {
          runs_.back()->script_analysis_.fLinkAfter = true;
          run->script_analysis_.fLinkBefore = true;
        }
      }
      ProcessRunScriptType(run);
      runs_.push_back(run);
    }
    last_boundary = boundaries[i].code_point;
    switch (boundaries[i].type) {
      case RunBoundary::FORMAT_START:
        // If it is a FORMAT_START boundary, push the current format into a
        // stack, and merge current format with the format corresponding to the
        // boundary.
        format_stack.push_back(current_format);
        current_format.MergeFormat(formats_[boundaries[i].format_index].format);
        break;
      case RunBoundary::FORMAT_END:
        // If it is a FORMAT_END boundary, next run's format should be the
        // |current_format| that before we process the corresponding
        // FORMAT_START boundary, which is the top item of the format stack.
        current_format = format_stack.back();
        format_stack.pop_back();
        break;
      case RunBoundary::ITEM:
        // If it is a ITEM boundary, the next run will be in the next item of
        // current_item.
        ++current_item;
        break;
    }
  }
  text_changed_ = false;
}

// Tries to shape the text.
// "Shape" means turning unicode characters into glyphs.
void TextRenderer::Impl::ScriptShape(TextRun* run) {
  HRESULT hr = E_FAIL;
  size_t run_length = run->range_.end - run->range_.start;
  const wchar_t* run_text = text_.c_str() + run->range_.start;
  run->logical_clusters_.reset(new WORD[run_length]);
  run->glyph_count_ = 0;
  if (run->font_) ::DeleteObject(run->font_);
  run->font_ = graphics_->CreateHFont(&(run->format_));
  HGDIOBJ old_font = ::SelectObject(dc_, run->font_);
  if (FontFallback::ShouldFallback(dc_, run_text[0])) {
    LOGFONT fallback_font = FontFallback::GetFallbackFont(dc_, run_text[0]);
    HFONT font = ::CreateFontIndirect(&fallback_font);
    ::SelectObject(dc_, font);
    ::DeleteObject(run->font_);
    run->font_ = font;
  }
  if (run->script_cache_)
    ::ScriptFreeCache(&(run->script_cache_));
  // Max glyph guess: http://msdn.microsoft.com/en-us/library/dd368564.aspx
  size_t max_glyphs = static_cast<size_t>(1.5 * run_length + 16);
  while (max_glyphs < kMaxGlyphs) {
    run->glyphs_.reset(new WORD[max_glyphs]);
    run->visible_attributes_.reset(new SCRIPT_VISATTR[max_glyphs]);
    hr = ::ScriptShape(dc_,
                       &run->script_cache_,
                       run_text,
                       run_length,
                       max_glyphs,
                       &(run->script_analysis_),
                       run->glyphs_.get(),
                       run->logical_clusters_.get(),
                       run->visible_attributes_.get(),
                       &(run->glyph_count_));
    if (hr == E_OUTOFMEMORY) {
      max_glyphs *= 2;
    } else {
      break;
    }
  }
  ASSERT(SUCCEEDED(hr) || hr == USP_E_SCRIPT_NOT_IN_FONT);
  ::SelectObject(dc_, old_font);
}

void TextRenderer::Impl::LayoutTextRun(TextRun* run) {
  // Generate glyphs for text run.
  ScriptShape(run);

  HRESULT hr = E_FAIL;
  size_t run_length = run->range_.end - run->range_.start;
  const wchar_t* run_text = text_.c_str() + run->range_.start;
  HGDIOBJ old_font = ::SelectObject(dc_, run->font_);
  // Layout the glyphs.
  if (run->glyph_count_ > 0) {
    HRESULT hr = E_FAIL;
    run->advance_widths_.reset(new int[run->glyph_count_]);
    run->offset_.reset(new GOFFSET[run->glyph_count_]);
    hr = ::ScriptPlace(dc_,
                       &run->script_cache_,
                       run->glyphs_.get(),
                       run->glyph_count_,
                       run->visible_attributes_.get(),
                       &(run->script_analysis_),
                       run->advance_widths_.get(),
                       run->offset_.get(),
                       &(run->abc_widths_));
    ASSERT(SUCCEEDED(hr));
    // TODO(synch): When using font Times New Roman Italic, the width of double
    // quote is 0, it is a bug cause by the font, but we have to find a way to
    // walk around it.
    const ABC& abc = run->abc_widths_;
    run->width_ = abc.abcA + abc.abcB + abc.abcC;
    run->shaped_and_placed_ = true;
  } else {
    run->width_ = 0;
  }
  ::SelectObject(dc_, old_font);
}

double TextRenderer::Impl::LayoutLine(int first_run, int last_run,
                                      double current_y,
                                      double line_width) {
  double line_height = 0;
  int runs_count = last_run - first_run + 1;
  ASSERT(runs_count > 0);
  scoped_array<int> visual_to_logical(new int[runs_count]);
  if (runs_count > 1) {
    HRESULT hr = E_FAIL;

    // Build the array of bidirectional embedding levels.
    scoped_array<BYTE> levels(new BYTE[runs_count]);
    for (int i = first_run; i <= last_run; ++i)
      levels[i - first_run] = runs_[i]->script_analysis_.s.uBidiLevel;

    scoped_array<int> logical_to_visual(new int[runs_count]);
    hr = ::ScriptLayout(runs_count,
                        levels.get(),
                        visual_to_logical.get(),
                        logical_to_visual.get());
    ASSERT(SUCCEEDED(hr));
  } else {
    visual_to_logical[0] = 0;
  }
  double top = INT_MAX;
  double bottom = -INT_MAX;
  // Set the y coordinate of the baseline to zeros, calculate the top and bottom
  // of each run.
  for (int i = first_run; i <= last_run; ++i) {
    double height = 0;
    double descend = 0;
    double ascend = 0;
    double line_space = 0;
    GetFontHeight(runs_[i]->format_, runs_[i]->font_, graphics_, &height,
                  &ascend, &descend, &line_space);
    double run_top = -ascend  - PointToPixel(runs_[i]->format_.rise(), dc_);
    double run_bottom = descend  - PointToPixel(runs_[i]->format_.rise(), dc_) +
                        line_space;
    runs_[i]->top_ = -ascend;
    runs_[i]->height_ = height + line_space;
    top = std::min(run_top, top);
    bottom = std::max(run_bottom, bottom);
  }
  line_height = bottom - top;
  double current_x = 0;
  switch (align_) {
    case CanvasInterface::ALIGN_LEFT: {
      break;
    }
    case CanvasInterface::ALIGN_CENTER: {
      current_x = (layout_width_ - line_width) / 2;
      break;
    }
    case CanvasInterface::ALIGN_RIGHT: {
      current_x = layout_width_ - line_width;
      break;
    }
    case CanvasInterface::ALIGN_JUSTIFY: {
      if (layout_width_ <= line_width)
        break;
      std::vector<int> word_stops;
      word_stops.resize(runs_count, 0);
      double word_breaks_mul_size = 0;
      for (int i = first_run; i <= last_run; ++i) {
        for (int j = runs_[i]->range_.start; j < runs_[i]->range_.end; ++j) {
          int next_char = runs_[i]->NextCharInRun(j);
          if (next_char < runs_[last_run]->range_.end &&
              next_char >= runs_[first_run]->range_.start &&
              logical_attributes_[next_char].fWordStop) {
            ++word_stops[i - first_run];
          }
        }
        word_breaks_mul_size += word_stops[i - first_run] *
                                runs_[i]->format_.size() *
                                runs_[i]->format_.scale();
      }
      if (word_breaks_mul_size == 0)
        break;
      double additional_width_ratio =
          (layout_width_ - line_width) / word_breaks_mul_size;
      for (int i = first_run; i <= last_run; ++i) {
        int justify_width = runs_[i]->format_.size() *
                            runs_[i]->format_.scale() *
                            additional_width_ratio;
        for (int j = runs_[i]->range_.start; j < runs_[i]->range_.end; ++j) {
          int next_char = runs_[i]->NextCharInRun(j);
          if (next_char < runs_[last_run]->range_.end &&
              next_char >= runs_[first_run]->range_.start &&
              logical_attributes_[next_char].fWordStop) {
            runs_[i]->advance_widths_[j - runs_[i]->range_.start] +=
                justify_width;
            runs_[i]->width_ += justify_width;
          }
        }
      }
      break;
    }
    default:
      break;
  }
  for (int i = first_run; i <= last_run; ++i) {
    int run_id = visual_to_logical[i - first_run] + first_run;
    runs_[run_id]->x_ = current_x;
    runs_[run_id]->y_ = current_y - top + runs_[run_id]->y_offset_ -
                        PointToPixel(runs_[run_id]->format_.rise(), dc_);
    runs_[run_id]->top_ += runs_[run_id]->y_;
    current_x += runs_[run_id]->width_;
  }
  width_ = std::max(width_, current_x);
  return line_height;
}

// Break the current text run from break_point to two text runs.
// The first text run (in visual order) will be stored in current_run
// and the latter text run will be stored in a new TextRun and returned
// to the caller.
TextRenderer::Impl::TextRun* TextRenderer::Impl::BreakTextRun(
    int break_point,
    TextRenderer::Impl::TextRun* current_run) {
  TextRun* new_run = new TextRun;
  new_run->format_ = current_run->format_;
  new_run->range_ = current_run->range_;
  new_run->script_analysis_ = current_run->script_analysis_;
  new_run->range_.start = break_point;
  current_run->range_.end = break_point;
  current_run->script_analysis_.fLinkAfter = false;
  current_run->script_analysis_.fLinkBefore = false;
  LayoutTextRun(current_run);
  LayoutTextRun(new_run);
  return new_run;
}

// Finds the last run and last character in the current line within width
// starting from first run.
// We will first find breakpoints at word boundaries and if there's no word
// boundary, we will find breakpoints at character boundaries.
void TextRenderer::Impl::FindBreakPoint(int first_run,
                                        int* last_run,
                                        int* last_char,
                                        double* width) {
  int current_char_in_run = *last_char - 1;
  int glyph_end = runs_[*last_run]->glyph_count_;
  int last_run_in_line = *last_run;
  int first_char_in_line =
      runs_[first_run]->range_.start;
  int word_break_point = 0;
  int char_break_point = 0;
  int char_break_run = 0;
  int first_char_stop = 0;
  int first_char_stop_run = 0;
  double current_line_width = *width;
  while (current_char_in_run > first_char_in_line) {
    current_line_width -=
        runs_[last_run_in_line]->GetCharWidth(current_char_in_run);
    if (logical_attributes_[current_char_in_run].fCharStop) {
      first_char_stop = current_char_in_run;
      first_char_stop_run = last_run_in_line;
    }
    if (current_line_width < layout_width_) {
      if (logical_attributes_[current_char_in_run].fWordStop) {
        word_break_point = current_char_in_run;
        break;
      } else if (logical_attributes_[current_char_in_run].fCharStop &&
                 char_break_point == 0) {
        char_break_point = current_char_in_run;
        char_break_run = last_run_in_line;
      }
    }
    --current_char_in_run;
    if (!runs_[last_run_in_line]->InRun(current_char_in_run)) {
      --last_run_in_line;
    }
  }
  int break_point = 0;
  if (word_break_point) {
    break_point = word_break_point;
  } else if (char_break_point) {
    break_point = char_break_point;
    last_run_in_line = char_break_run;
  } else if (first_char_stop) {
    // If char_break_point is not set, then the first glyph can not fit in
    // the line, so break this line from the first character stop.
    break_point = first_char_stop;
    last_run_in_line = first_char_stop_run;
  } else {
    // This run contains only 1 character and it can not fit in the line.
    break_point = 0;
  }
  double line_width_before_last_run = *width;
  for (int i = *last_run; i >= last_run_in_line; --i)
    line_width_before_last_run -= runs_[i]->width_;
  if (break_point > runs_[last_run_in_line]->range_.start) {
    // Breaking texts may change the glyphs and may expand the line with.
    // So we have to check the line width again and if texts still can not be
    // fit into current line, find a better break point.
    scoped_ptr<TextRun> run(new TextRun);
    run->format_ = runs_[last_run_in_line]->format_;
    run->range_ = runs_[last_run_in_line]->range_;
    run->range_.end = break_point;
    run->script_cache_ = NULL;
    run->script_analysis_ = runs_[last_run_in_line]->script_analysis_;
    HGDIOBJ old_font = ::SelectObject(dc_, runs_[last_run_in_line]->font_);
    LayoutTextRun(run.get());
    ::SelectObject(dc_, old_font);
    *width = run->width_ + line_width_before_last_run;
    if (*width > layout_width_)
      FindBreakPoint(first_run, &last_run_in_line, &break_point, width);
  } else if (break_point) {
    *width = line_width_before_last_run;
  } else {
    break_point = first_char_stop;
    last_run_in_line = first_char_stop_run;
  }
  *last_run = last_run_in_line;
  *last_char = break_point;
}

double TextRenderer::Impl::LayoutVisualText() {
  size_t first_run_in_line = 0;
  double current_line_width = 0;
  double current_y = 0;
  size_t current_run_id = 0;
  while (current_run_id < runs_.size()) {
    TextRun* run = runs_[current_run_id];
    // Select the font desired for glyph generation.
    if (!run->shaped_and_placed_) {
      LayoutTextRun(run);
    }
    current_line_width += run->width_;
    if (wcschr(L"\r\n\u2028\u2029", text_[run->range_.start])) {
      if (first_run_in_line != current_run_id) {
        int last_run_in_line = current_run_id - 1;
        current_y += LayoutLine(first_run_in_line, last_run_in_line,
                                current_y, current_line_width);
        current_line_width = 0;
      }
      ++current_run_id;
      first_run_in_line = current_run_id;
    } else if (word_wrap_ && current_line_width > layout_width_) {
      int last_run_in_line = current_run_id;
      int break_point = run->range_.end;
      FindBreakPoint(first_run_in_line, &last_run_in_line, &break_point,
                     &current_line_width);
      if (break_point > runs_[last_run_in_line]->range_.start) {
        TextRenderer::Impl::TextRun* new_run =
            BreakTextRun(break_point, runs_[last_run_in_line]);
        runs_.insert(runs_.begin() + last_run_in_line + 1, new_run);
      } else if (break_point) {
        --last_run_in_line;
      }
      // layout current line.
      current_y += LayoutLine(first_run_in_line, last_run_in_line,
                              current_y, current_line_width);
      current_line_width = 0;
      current_run_id = last_run_in_line + 1;
      first_run_in_line = current_run_id;
    } else {
      ++current_run_id;
    }
  }
  if (first_run_in_line < runs_.size()) {
    current_y += LayoutLine(first_run_in_line, runs_.size() - 1,
                            current_y, current_line_width);
  }
  height_ = current_y;
  layout_changed_ = false;
  width_ += 1;
  return current_y;
}


void TextRenderer::Impl::Draw(const Gdiplus::Brush* brush,
                              double opacity,
                              Gdiplus::Graphics* canvas) {
  if (text_.empty())
    return;
  if (text_changed_)
    ItemizeLogicalText();
  if (layout_changed_)
    LayoutVisualText();
  double y_offset = 0;
  switch (valign_) {
    case CanvasInterface::VALIGN_TOP:
      break;
    case CanvasInterface::VALIGN_MIDDLE:
      y_offset = (layout_height_ - height_) / 2;
      break;
    case CanvasInterface::VALIGN_BOTTOM:
      y_offset = layout_height_ - height_;
      break;
  }
  canvas->SetTextRenderingHint(Gdiplus::TextRenderingHintAntiAlias);
  for (size_t i = 0; i < runs_.size(); ++i) {
    // Get the run specified by the visual-to-logical map.
    TextRun* run = runs_[i];

    // Draw background.
    if (run->format_.has_background()) {
      Gdiplus::SolidBrush background_brush(
          Gdiplus::Color(opacity * 255,
                         run->format_.background().RedInt(),
                         run->format_.background().GreenInt(),
                         run->format_.background().BlueInt()));
      canvas->FillRectangle(&background_brush,
                            run->x_ + x_, run->top_ + y_ + y_offset,
                            run->width_, run->height_);
    }
    scoped_array<Gdiplus::PointF> glyph_pos(
        new Gdiplus::PointF[run->glyph_count_]);
    double current_x_in_run = run->x_;
    for (int j = 0; j < run->glyph_count_; ++j) {
      glyph_pos[j].X = current_x_in_run + x_;
      glyph_pos[j].Y = run->y_ + y_ + y_offset;
      current_x_in_run += run->advance_widths_[j];
    }
    scoped_ptr<Gdiplus::Font> font(new Gdiplus::Font(dc_, run->font_));
    if (brush) {
      canvas->DrawDriverString(run->glyphs_.get(), run->glyph_count_,
                               font.get(), brush, glyph_pos.get(), 0, NULL);
      if (run->format_.strikeout() || run->format_.underline())
        DrawTextRunDecorations(*run, x_, y_ + y_offset, brush, 1, canvas);
    } else {
      Gdiplus::SolidBrush run_brush(
          Gdiplus::Color(opacity * 255,
                         run->format_.foreground().RedInt(),
                         run->format_.foreground().GreenInt(),
                         run->format_.foreground().BlueInt()));
      canvas->DrawDriverString(
          run->glyphs_.get(), run->glyph_count_, font.get(),
          &run_brush,
          glyph_pos.get(), 0, NULL);
      if (run->format_.strikeout() || run->format_.underline())
        DrawTextRunDecorations(*run, x_, y_ + y_offset, NULL, opacity, canvas);
    }
  }
}

void TextRenderer::Impl::DrawTextRunDecorations(
    const TextRun& run,
    double x, double y,
    const Gdiplus::Brush* brush,
    double opacity,
    Gdiplus::Graphics* graphics) {
  Gdiplus::REAL underline_pos = 0;
  Gdiplus::REAL underline_size = 0;
  Gdiplus::REAL strikeout_pos = 0;
  Gdiplus::REAL strikeout_size = 0;
  GetFontMetric(run.format_, run.font_, &strikeout_pos, &strikeout_size,
                &underline_pos, &underline_size);
  if (run.format_.strikeout()) {
    Gdiplus::SolidBrush strikeout_brush(
        Gdiplus::Color(opacity * 255,
                       run.format_.strikeout_color().RedInt(),
                       run.format_.strikeout_color().GreenInt(),
                       run.format_.strikeout_color().BlueInt()));
    const Gdiplus::Brush* actual_brush = brush;
    if (!brush)
      actual_brush = &strikeout_brush;
    graphics->FillRectangle(
        actual_brush,
        Gdiplus::RectF(run.x_ + x, y + run.y_ - strikeout_pos,
                       run.width_, strikeout_size));
  }
  if (run.format_.underline()) {
    Gdiplus::SolidBrush underline_brush(
        Gdiplus::Color(opacity * 255,
                       run.format_.underline_color().RedInt(),
                       run.format_.underline_color().GreenInt(),
                       run.format_.underline_color().BlueInt()));
    const Gdiplus::Brush* actual_brush = brush;
    if (!brush)
      actual_brush = &underline_brush;
    graphics->FillRectangle(
        actual_brush,
        Gdiplus::RectF(run.x_ + x, y + run.y_ - underline_pos,
                       run.width_, underline_size));
  }
}

void TextRenderer::Impl::SetMarkUpText(const std::string& mark_up_text,
                                       const TextFormat& base_format) {
  std::string text_utf8;
  formats_.clear();
  ParseMarkUpText(mark_up_text, &base_format, &text_utf8, &formats_);
  ConvertStringUTF8ToUTF16(text_utf8, &text_);
  text_changed_ = true;
  layout_changed_ = true;
}

void TextRenderer::Impl::GetTextExtents(double* width, double* height) {
  width_ = height_ = 0;
  if (text_changed_)
    ItemizeLogicalText();
  if (layout_changed_)
    LayoutVisualText();
  *width = width_;
  *height = height_;
}

void TextRenderer::Impl::SetTextAndFormats(const std::string& text,
                                           const TextFormats& formats) {
  text_ = L"";
  formats_ = formats;
  ConvertStringUTF8ToUTF16(text.c_str(), &text_);
  text_changed_ = true;
  layout_changed_ = true;
}

void TextRenderer::Impl::ProcessRunScriptType(TextRun* run) {
  if (run->format_.script_type() != NORMAL) {
    double px_offset = 0;
    double pt_size = 0;
    GetScriptSizeAndOffset(run->format_, graphics_, &pt_size, &px_offset);
    run->format_.set_size(pt_size);
    run->y_offset_ = px_offset;
  }
}

int TextRenderer::Impl::GetTextRangeBoundingBoxes(
    const Range& range,
    std::vector<Rectangle>* bounding_boxes) {
  if (text_changed_)
    ItemizeLogicalText();
  if (layout_changed_)
    LayoutVisualText();
  std::vector<TextRun*>::iterator it = runs_.begin();
  while (it != runs_.end() && (*it)->range_.end < range.start)
    ++it;
  double y_offset = 0;
  switch (valign_) {
    case CanvasInterface::VALIGN_TOP:
      break;
    case CanvasInterface::VALIGN_MIDDLE:
      y_offset = (layout_height_ - height_) / 2;
      break;
    case CanvasInterface::VALIGN_BOTTOM:
      y_offset = layout_height_ - height_;
      break;
  }
  Rectangle rect;
  int count = 0;
  while (it != runs_.end() && (*it)->range_.end <= range.end) {
    double width = 0;
    if ((*it)->range_.start >= range.start &&
        (*it)->range_.end <= (*it)->range_.end) {
      width = (*it)->width_;
    } else {
      int start = max(range.start, (*it)->range_.start);
      int end = min(range.end, (*it)->range_.end);
      for (int i = start; i < end; ++i) {
        width_ += (*it)->GetCharWidth(i);
      }
    }
    rect.Set((*it)->x_, y_offset + (*it)->y_ - (*it)->height_,
             width, (*it)->height_);
    bounding_boxes->push_back(rect);
    ++count;
  }
  return count;
}

void TextRenderer::Impl::DrawCaret(CanvasInterface* canvas,
                                   int caret_pos,
                                   const Color& color) {
  if (text_.empty())
    return;
  if (text_changed_)
    ItemizeLogicalText();
  if (layout_changed_)
    LayoutVisualText();
  for (size_t i = 0; i < runs_.size(); ++i) {
    const TextRun& run = *runs_[i];
    if (caret_pos >= run.range_.start && caret_pos < run.range_.end) {
      int caret_x = 0;
      HRESULT res = ::ScriptCPtoX(caret_pos - run.range_.start,
                                  FALSE,
                                  run.range_.Length(),
                                  run.glyph_count_,
                                  run.logical_clusters_.get(),
                                  run.visible_attributes_.get(),
                                  run.advance_widths_.get(),
                                  &run.script_analysis_,
                                  &caret_x);
      ASSERT(SUCCEEDED(res));
      canvas->DrawLine(caret_x + x_ + run.x_, run.y_ + y_,
                       caret_x + x_ + run.x_, run.y_ - y_ - run.height_,
                       1,
                       color);
      return;
    }
  }
  // If not found, then the caret is at last position.
  const TextRun& run = *runs_.back();
  canvas->DrawLine(x_ + run.x_ + run.width_, run.y_ + y_,
                   x_ + run.x_ + run.width_, run.y_ - y_ - run.height_,
                   1,
                   color);
}

TextRenderer::TextRenderer(const GdiplusGraphics* graphics)
  : impl_(new Impl(graphics)) {
  ASSERT(impl_.get());
}

TextRenderer::~TextRenderer() {
}

void TextRenderer::SetAlignment(CanvasInterface::Alignment align) {
  impl_->layout_changed_ = AssignIfDiffer(align, &impl_->align_) ||
                           impl_->layout_changed_;
}

void TextRenderer::SetVAlignment(CanvasInterface::VAlignment valign) {
  impl_->layout_changed_ = AssignIfDiffer(valign, &impl_->valign_) ||
                           impl_->layout_changed_;
}

void TextRenderer::SetTrimming(CanvasInterface::Trimming trimming) {
  impl_->layout_changed_ = AssignIfDiffer(trimming, &impl_->trimming_) ||
                           impl_->layout_changed_;
}

void TextRenderer::SetTextAndFormat(const std::string& text,
                                    const TextFormats& formats) {
  impl_->SetTextAndFormats(text, formats);
}

void TextRenderer::SetLayoutRectangle(double x, double y,
                                      double width, double height) {
  impl_->x_ = x;
  impl_->y_ = y;
  impl_->layout_width_ = (width <= 0 ? INT_MAX : width);
  impl_->layout_height_ = (height <= 0 ? INT_MAX : height);
  impl_->layout_changed_ = true;
}

void TextRenderer::SetWordWrap(bool word_wrap) {
  impl_->layout_changed_ = AssignIfDiffer(word_wrap, &impl_->word_wrap_) ||
                           impl_->layout_changed_;
}

bool TextRenderer::DrawText(CanvasInterface* canvas) {
  GdiplusCanvas* gdiplus_canvas = down_cast<GdiplusCanvas*>(canvas);
  Gdiplus::Graphics* graphics = gdiplus_canvas->GetGdiplusGraphics();
  impl_->Draw(NULL, gdiplus_canvas->GetOpacity(), graphics);
  return true;
}

bool TextRenderer::DrawTextWithTexture(const CanvasInterface* texture,
                                       CanvasInterface* canvas) {
  GdiplusCanvas* gdiplus_canvas = down_cast<GdiplusCanvas*>(canvas);
  if (gdiplus_canvas == NULL) return false;
  Gdiplus::Graphics* graphics = gdiplus_canvas->GetGdiplusGraphics();
  if (graphics == NULL) return false;
  Gdiplus::Image* source = down_cast<const GdiplusCanvas*>(texture)->GetImage();
  if (source == NULL) return false;
  Gdiplus::ImageAttributes image_attributes;
  if (gdiplus_canvas->GetOpacity() < 1) {
    Gdiplus::ColorMatrix m = {1, 0, 0, 0, 0,
                              0, 1, 0, 0, 0,
                              0, 0, 1, 0, 0,
                              0, 0, 0, gdiplus_canvas->GetOpacity(), 0,
                              0, 0, 0, 0, 1
                             };
    image_attributes.SetColorMatrix(&m);
  }
  Gdiplus::RectF rect(0, 0, source->GetWidth(), source->GetHeight());
  Gdiplus::TextureBrush brush(source, rect, &image_attributes);
  impl_->Draw(&brush, gdiplus_canvas->GetOpacity(), graphics);
  return true;
}

bool TextRenderer::GetTextExtents(double* width, double* height) {
  impl_->GetTextExtents(width, height);
  return true;
}

void TextRenderer::Destroy() {
  delete this;
}

void TextRenderer::SetDefaultFormat(const TextFormat& default_format) {
  impl_->layout_changed_ |=
      impl_->default_format_.font() != default_format.font() ||
      impl_->default_format_.scale() != default_format.scale() ||
      impl_->default_format_.size() != default_format.size() ||
      impl_->default_format_.italic() != default_format.italic() ||
      impl_->default_format_.bold() != default_format.bold() ||
      impl_->default_format_.script_type() != default_format.script_type();
  impl_->text_changed_ |=
      impl_->default_format_.text_rtl() != default_format.text_rtl();
  impl_->default_format_ = default_format;
}

int TextRenderer::GetTextRangeBoundingBoxes(
    const Range& range,
    std::vector<Rectangle>* bounding_boxes) {
  return impl_->GetTextRangeBoundingBoxes(range, bounding_boxes);
}

void TextRenderer::DrawCaret(CanvasInterface* canvas,
                             int caret_pos,
                             const Color& color) {
  impl_->DrawCaret(canvas, caret_pos, color);
}

}  // namespace win32
}  // namespace ggadget
