/*
  Copyright 2013 Google Inc.

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
#include "ggadget/mac/ct_text_renderer.h"

#include <CoreText/CoreText.h>

#include "ggadget/canvas_interface.h"
#include "ggadget/mac/ct_font.h"
#include "ggadget/mac/mac_array.h"
#include "ggadget/mac/quartz_canvas.h"
#include "ggadget/mac/quartz_graphics.h"
#include "ggadget/mac/scoped_cftyperef.h"

namespace {

const double kCaretWidthFactor = 1.0 / 16.0;
const double kStrikeoutWidthFactor = 1.0 / 16.0;
const double kMaxBoundingSize = 2000.0;

CFComparisonResult CompareLineWithPos(CTLineRef line,
                                      int caret_pos) {
  CFRange range = CTLineGetStringRange(line);
  if (range.location > caret_pos) {
    return kCFCompareGreaterThan;
  }
  if (range.location + range.length <= caret_pos) {
    return kCFCompareLessThan;
  }
  return kCFCompareEqualTo;
}

CFComparisonResult CompareLineWithPos(const void *val1,
                                      const void *val2,
                                      void *context) {
  // CFArrayBSearchValues will randomly pass the value parameter as val1 or
  // val2. The only reliable way is to set value as NULL and pass the caret_pos
  // as context
  int caret_pos = *reinterpret_cast<int*>(context);
  if (val1 != NULL) {
    return CompareLineWithPos(reinterpret_cast<CTLineRef>(val1), caret_pos);
  } else {
    return static_cast<CFComparisonResult>(
        -CompareLineWithPos(reinterpret_cast<CTLineRef>(val2), caret_pos));
  }
}

} // namespace

namespace ggadget {
namespace mac {

class CtTextRenderer::Impl {
 public:
  // Defines the parameters of loagical runs.
  //
  // A logical run is a continuous part of a glyph run that share the same
  // text format.
  struct LogicalRunParam {
    CTRunRef run;
    double x;
    double y;
    double run_width;
    CGFloat run_ascent;
    CGFloat run_descent;
    CGFloat run_leading;
    double line_width;
    CGFloat line_ascent;
    CGFloat line_descent;
    CGFloat line_leading;
    TextFormat format;
    // The range of this logical run in the whole text
    // frame
    Range text_range;
    // The range of this logical run in the glyph run
    CFRange run_range;
  };

  typedef void (Impl::*LogicalRunFunc)(QuartzCanvas *canvas,
                                       const LogicalRunParam& param);

  explicit Impl(const QuartzGraphics* graphics)
      : align_(kCTLeftTextAlignment),
        valign_(CanvasInterface::VALIGN_TOP),
        trimming_(CanvasInterface::TRIMMING_NONE),
        x_(0.0),
        y_(0.0),
        width_(0.0),
        height_(0.0),
        word_wrap_(false),
        graphics_(graphics) {
    ASSERT(graphics);
  }

  /**
   * Return an attributed string. The return value is owned by Impl.
   */
  CFMutableAttributedStringRef GetAttributedString() {
    if (attr_string_.get()) {
      return attr_string_.get();
    }

    ScopedCFTypeRef<CFMutableAttributedStringRef> attr_string(
        CFAttributedStringCreateMutable(kCFAllocatorDefault, 0));
    if (!attr_string.get()) {
      return NULL;
    }
    ScopedCFTypeRef<CFStringRef> string(
        CFStringCreateWithCString(NULL, text_.c_str(), kCFStringEncodingUTF8));
    if (!string.get()) {
      return NULL;
    }
    CFAttributedStringBeginEditing(attr_string.get());
    CFAttributedStringReplaceString(attr_string.get(),
                                    CFRangeMake(0, 0),
                                    string);
    CFIndex len = CFAttributedStringGetLength(attr_string.get());
    // Sets default format
    TextFormatRange default_format_range = {
      .format = default_format_,
      .range = { .start = 0, .end = static_cast<int>(len), },
    };
    if (!SetAttributedStringFormat(attr_string.get(), default_format_range)) {
      return NULL;
    }

    // Sets formats
    for (size_t i = 0; i < formats_.size(); ++i) {
      TextFormatRange format_range = formats_[i];
      format_range.format.MergeIfNotHave(default_format_);
      if (!SetAttributedStringFormat(attr_string.get(), format_range)) {
        return NULL;
      }
    }

    ScopedCFTypeRef<CTParagraphStyleRef> paragraph_style(
        CreateParagraghStyle());
    CFAttributedStringSetAttribute(attr_string.get(),
                                   CFRangeMake(0, len),
                                   kCTParagraphStyleAttributeName,
                                   paragraph_style.get());
    CFAttributedStringEndEditing(attr_string.get());

    attr_string_.reset(attr_string.release());
    return attr_string_.get();
  }

  bool SetAttributedStringFormat(CFMutableAttributedStringRef attr_string,
                                 const TextFormatRange &format_range) const {
    CFRange range = CFRangeMake(format_range.range.start,
                                format_range.range.Length());
    TextFormat format = format_range.format;

    scoped_ptr<CtFont> font(down_cast<CtFont*>(graphics_->NewFont(format)));
    if (!font.get()) {
      return false;
    }
    CFAttributedStringSetAttribute(attr_string,
                                   range,
                                   kCTFontAttributeName,
                                   font->GetFont());

    // foreground_color
    if (format.has_foreground()) {
      ScopedCFTypeRef<CGColorRef> foreground_color(
          CreateColor(format.foreground()));
      if (!foreground_color.get()) {
        return false;
      }
      CFAttributedStringSetAttribute(attr_string,
                                     range,
                                     kCTForegroundColorAttributeName,
                                     foreground_color.get());
    }

    // underline
    if (format.has_underline()) {
      int underline_style_value;
      if (format.underline()) {
        underline_style_value = kCTUnderlineStyleSingle
            | kCTUnderlinePatternSolid;
      } else {
        underline_style_value = kCTUnderlineStyleNone;
      }
      ScopedCFTypeRef<CFNumberRef> underline_style(
          CFNumberCreate(NULL /* allocator */,
                         kCFNumberIntType,
                         &underline_style_value));
      if (!underline_style.get()) {
        return false;
      }
      CFAttributedStringSetAttribute(attr_string,
                                     range,
                                     kCTUnderlineStyleAttributeName,
                                     underline_style.get());
    }

    // underline_color
    if (format.has_underline_color()) {
      ScopedCFTypeRef<CGColorRef> underline_color(
          CreateColor(format.underline_color()));
      if (!underline_color.get()) {
        return false;
      }
      CFAttributedStringSetAttribute(attr_string,
                                     range,
                                     kCTUnderlineColorAttributeName,
                                     underline_color.get());
    }

    // script_type
    // TODO: this didn't work
    if (format.has_script_type()) {
      int script_type_value;
      switch (format.script_type()) {
      case SUPERSCRIPT:
        script_type_value = 1;
        break;
      case NORMAL:
        script_type_value = 0;
        break;
      case SUBSCRIPT:
        script_type_value = -1;
        break;
      }
      ScopedCFTypeRef<CFNumberRef> script_type(
          CFNumberCreate(NULL /* allocator */,
                         kCFNumberIntType,
                         &script_type_value));
      if (!script_type.get()) {
        return false;
      }
      CFAttributedStringSetAttribute(attr_string,
                                     range,
                                     kCTSuperscriptAttributeName,
                                     script_type.get());
    }
    // TODO: support the following format attributes:
    //  - rise
    //  - scale
    //  - text_rtl
    return true;
  }

  CGMutablePathRef CreateBoundingRect() const {
    CGMutablePathRef path = CGPathCreateMutable();

    // If |height_| or |width_| is negative, it is not limited.
    // We use a big enough value for it.
    double actual_width = width_ < 0 ? kMaxBoundingSize : ::ceil(width_);
    double actual_height = height_ < 0 ? kMaxBoundingSize : ::ceil(height_);
    CGRect bounding_rect =
        CGRectMake(x_, -y_ - actual_height, actual_width, actual_height);
    CGPathAddRect(path, NULL, bounding_rect);
    return path;
  }

  CGColorRef CreateColor(const Color &c) const {
    ScopedCFTypeRef<CGColorSpaceRef> rgbColorSpace(
        CGColorSpaceCreateDeviceRGB());
    if (!rgbColorSpace.get()) {
      return false;
    }
    CGFloat components[] = { c.red, c.green, c.blue, 1.0 };
    CGColorRef color = CGColorCreate(rgbColorSpace.get(), components);
    return color;
  }

  void SetAlignment(CanvasInterface::Alignment align) {
    switch (align) {
    case CanvasInterface::ALIGN_LEFT:
      align_ = kCTLeftTextAlignment;
      break;
    case CanvasInterface::ALIGN_RIGHT:
      align_ = kCTRightTextAlignment;
      break;
    case CanvasInterface::ALIGN_CENTER:
      align_ = kCTCenterTextAlignment;
      break;
    case CanvasInterface::ALIGN_JUSTIFY:
      align_ = kCTJustifiedTextAlignment;
      break;
    }
  }

  CTParagraphStyleSetting GetAlignSetting() const {
    CTParagraphStyleSetting align_setting = {
      .spec = kCTParagraphStyleSpecifierAlignment,
      .valueSize = sizeof(align_),
      .value = &align_,
    };
    return align_setting;
  }

  CTParagraphStyleRef CreateParagraghStyle() const {
    CTParagraphStyleSetting paragraph_settings[] = {
      GetAlignSetting(),
    };

    CTParagraphStyleRef paragraph_style = CTParagraphStyleCreate(
        paragraph_settings,
        arraysize(paragraph_settings));
    return paragraph_style;
  }

  /**
   * Gets a CTFramesetterRef. The returned value is owned by Impl
   */
  CTFramesetterRef GetFramesetter() {
    if (!framesetter_.get()) {
      CFMutableAttributedStringRef attr_string = GetAttributedString();
      framesetter_.reset(CTFramesetterCreateWithAttributedString(attr_string));
    }
    return framesetter_.get();
  }

  /**
   * Gets a CTFrameRef. The returned value is owned by Impl
   */
  CTFrameRef GetFrame() {
    if (!frame_.get()) {
      CTFramesetterRef framesetter = GetFramesetter();
      ScopedCFTypeRef<CGMutablePathRef> path(CreateBoundingRect());
      frame_.reset(CTFramesetterCreateFrame(framesetter,
                                            CFRangeMake(0, 0),
                                            path.get(),
                                            NULL));
    }
    return frame_.get();
  }

  void UpdateLayout() {
    attr_string_.reset(NULL);
    framesetter_.reset(NULL);
    frame_.reset(NULL);
  }

  void UpdateFrame() {
    frame_.reset(NULL);
  }

  QuartzCanvas* CreateTextMask() {
    scoped_ptr<QuartzCanvas> mask(
        down_cast<QuartzCanvas*>(graphics_->NewCanvas(width_, height_)));

    // As a mask, we need its background to be opacque and the text be
    // transparent
    Color black(0.0, 0.0, 0.0);
    mask->DrawFilledRect(0.0, 0.0, width_, height_, black);
    CGContextSetBlendMode(mask->GetContext(), kCGBlendModeDestinationOut);

    // Draw at the top-left corner;
    mask->TranslateCoordinates(-x_, -y_);
    DrawText(mask.get(), false /* draw_background */);
    return mask.release();
  }

  bool DrawText(CanvasInterface* canvas, bool draw_background) {
    ASSERT(canvas);
    QuartzCanvas* quartz_canvas = down_cast<QuartzCanvas*>(canvas);
    CGContextRef context = quartz_canvas->GetContext();

    CTFrameRef frame = GetFrame();

    quartz_canvas->PushState();
    // Quartz canvas was initialized to be upside down, we need to restore it
    // so the text will be rendered correctly
    quartz_canvas->ScaleCoordinates(1.0, -1.0);
    CGContextSetTextMatrix(context, CGAffineTransformIdentity);
    if (draw_background) {
      DrawBackground(quartz_canvas, frame);
    }

    CTFrameDraw(frame, context);
    DrawStrikeout(quartz_canvas, frame);
    quartz_canvas->PopState();
    return true;
  }

  void ForEachLogicalRun(QuartzCanvas* canvas, CTFrameRef frame,
                         LogicalRunFunc func) {
    size_t last_format_index = 0;

    LogicalRunParam param;

    // The origin of the frame is at the bottom left corner.
    // Keep in mind that in this context, the origin of the canvas
    // is at the top left corner and the y axis increases upwards
    double origin_x = x_;
    double origin_y = -y_ - height_;

    // If the formats don't cover all the glyph runs, we append an empty
    // format to cover the rest. It is guaranteed by NormalizeTextFormats
    // the the formats will start from range 0.
    bool has_appended_format = false;
    CFRange visible_range = CTFrameGetVisibleStringRange(frame);
    int format_end = formats_.empty()
        ? 0
        : formats_[formats_.size() - 1].range.end;
    if (visible_range.location + visible_range.length > format_end) {
      has_appended_format = true;
      TextFormatRange empty_format_range;
      empty_format_range.range.start = format_end;
      empty_format_range.range.end =
          static_cast<int>(visible_range.location + visible_range.length);
      formats_.push_back(empty_format_range);
    }

    // iterate over all glyph runs
    MacArray<CTLineRef> lines(CTFrameGetLines(frame));
    for (CFIndex i = 0; i < lines.size(); ++i) {
      // initialize the origin of line
      CGPoint line_origin;
      CTFrameGetLineOrigins(frame, CFRangeMake(i, 1), &line_origin);
      param.x = origin_x + line_origin.x;
      param.y = origin_y + line_origin.y;

      // We use ascent and descent of lines instead of that of each runs
      // to determine the height of the background of each run. We do that
      // because the ascent and descent might be different due to font family,
      // size or character type. It's ugly to have different top and bottom
      // background edges for runs in the same line
      param.line_width = CTLineGetTypographicBounds(lines[i],
                                                    &param.line_ascent,
                                                    &param.line_descent,
                                                    &param.line_leading);

      MacArray<CTRunRef> glyph_runs(CTLineGetGlyphRuns(lines[i]));
      for (CFIndex j = 0; j < glyph_runs.size(); ++j) {
        CTRunRef glyph_run = glyph_runs[j];
        // Find the related format
        TextFormat format = default_format_;
        CFRange run_range = CTRunGetStringRange(glyph_run);
        param.text_range.start = static_cast<int>(run_range.location);
        param.run_range.location = 0;
        param.run = glyph_run;

        while (last_format_index < formats_.size()
               && formats_[last_format_index].range.start
                 < run_range.location + run_range.length) {
          TextFormatRange& tfr = formats_[last_format_index];
          if (tfr.range.end > run_range.location) {
            param.format = default_format_;
            param.format.MergeFormat(tfr.format);
            param.text_range.end = std::min(
                tfr.range.end,
                static_cast<int>(run_range.location + run_range.length));
            param.run_range.length = param.text_range.Length();
            param.run_width = CTRunGetTypographicBounds(glyph_run,
                                                        param.run_range,
                                                        &param.run_ascent,
                                                        &param.run_descent,
                                                        &param.run_leading);
            (this->*func)(canvas, param);
            param.x += param.run_width;
            param.text_range.start = param.text_range.end;
            param.run_range.location += param.run_range.length;

            // Reach the end of glyph run, keep current format
            if (param.text_range.start
                >= run_range.location + run_range.length) {
              break;
            }
          }
          last_format_index++;
        }
      }
    }
  }

  void DrawBackgroundForLogicalRun(QuartzCanvas* canvas,
                                   const LogicalRunParam& param) {
    if (param.format.has_background()) {
      canvas->DrawFilledRect(param.x,
                             param.y - param.line_descent,
                             param.run_width,
                             param.line_ascent + param.line_descent,
                             param.format.background());
    }
  }

  void DrawBackground(QuartzCanvas* canvas, CTFrameRef frame) {
    ForEachLogicalRun(canvas, frame, &Impl::DrawBackgroundForLogicalRun);
  }

  void DrawStrikeoutForLogicalRun(QuartzCanvas* canvas,
                                  const LogicalRunParam& param) {
    if (param.format.strikeout()) {
      double y = param.y + (param.line_ascent - param.line_descent) / 2.0;
      Color color;
      if (param.format.has_strikeout_color()) {
        color = param.format.strikeout_color();
      } else {
        color = param.format.foreground();
      }
      double strikeout_width = ceil(
          kStrikeoutWidthFactor * (param.line_ascent + param.line_descent));
      canvas->DrawLine(param.x, y, param.x + param.run_width, y,
                       strikeout_width, color);
    }
  }

  void DrawStrikeout(QuartzCanvas* canvas, CTFrameRef frame) {
    ForEachLogicalRun(canvas, frame, &Impl::DrawStrikeoutForLogicalRun);
  }

  CTTextAlignment align_;
  CanvasInterface::VAlignment valign_;
  CanvasInterface::Trimming trimming_;
  double x_;
  double y_;
  double width_;
  double height_;
  bool word_wrap_;
  std::string text_;
  TextFormats formats_;
  TextFormat default_format_;
  const QuartzGraphics* graphics_;
  ScopedCFTypeRef<CFMutableAttributedStringRef> attr_string_;
  ScopedCFTypeRef<CTFramesetterRef> framesetter_;
  ScopedCFTypeRef<CTFrameRef> frame_;
}; // class Impl

CtTextRenderer::CtTextRenderer(const QuartzGraphics* graphics)
    : impl_(new Impl(graphics)) {
}

void CtTextRenderer::Destroy() {
  delete this;
}

CtTextRenderer::~CtTextRenderer() {
}

void CtTextRenderer::SetTextAndFormat(const std::string& text,
                                      const TextFormats& formats) {
  impl_->text_ = text;
  impl_->formats_ = NormalizeTextFormats(formats);
  impl_->UpdateLayout();
}

void CtTextRenderer::SetAlignment(CanvasInterface::Alignment align) {
  impl_->SetAlignment(align);
  impl_->UpdateLayout();
}

void CtTextRenderer::SetVAlignment(CanvasInterface::VAlignment valign) {
  // TODO: implement valignment support
}

void CtTextRenderer::SetWordWrap(bool word_wrap) {
  // TODO: implement word_wrap support
}

void CtTextRenderer::SetLayoutRectangle(double x, double y,
                                        double width, double height) {
  impl_->x_ = x;
  impl_->y_ = y;
  impl_->width_ = width;
  impl_->height_ = height;
  impl_->UpdateFrame();
}

void CtTextRenderer::SetTrimming(CanvasInterface::Trimming trimming) {
  // TODO: support trimming
}

bool CtTextRenderer::DrawText(CanvasInterface* canvas) {
  return impl_->DrawText(canvas, true /* draw_background */);
}

bool CtTextRenderer::DrawTextWithTexture(const CanvasInterface* texture,
                                         CanvasInterface* canvas) {
  ASSERT(canvas);
  ASSERT(texture);
  scoped_ptr<QuartzCanvas> text_mask(impl_->CreateTextMask());
  canvas->DrawCanvasWithMask(impl_->x_, impl_->y_, texture,
                             impl_->x_, impl_->y_, text_mask.get());
  return true;
}

bool CtTextRenderer::GetTextExtents(double* width, double* height) {
  CTFrameRef frame = impl_->GetFrame();
  if (!frame) {
    return false;
  }

  MacArray<CTLineRef> lines(CTFrameGetLines(frame));

  if (width != NULL) {
    *width = 0.0;
    for (CFIndex i = 0; i < lines.size(); ++i) {
      double line_width = CTLineGetTypographicBounds(lines[i],
                                                     NULL, // ascents
                                                     NULL, // descents
                                                     NULL); // leading
      *width = std::max(*width, line_width);
    }
  }

  if (height != NULL) {
    // Height is the difference between the top of the first line and the bottom
    // of the last line
    // Keep in mind that y coordinate increates from bottom to top

    CFIndex line_count = lines.size();
    CGPoint origins[line_count];
    CTFrameGetLineOrigins(frame,
                          CFRangeMake(0, 0), // copy origins of all lines
                          origins);

    CGFloat first_ascent;
    CTLineGetTypographicBounds(
        lines[0],
        &first_ascent,
        NULL,  // descent
        NULL); // leading
    double top = origins[0].y + first_ascent;

    CGFloat last_descent;
    CTLineGetTypographicBounds(
        lines[line_count - 1],
        NULL,  // ascent
        &last_descent,
        NULL); // leading
    double bottom = origins[line_count - 1].y - last_descent;
    *height = top - bottom;
  }

  return true;
}

int CtTextRenderer::GetTextRangeBoundingBoxes(
    const Range& range,
    std::vector<Rectangle>* bounding_boxes) {
  // TODO: implement this
  return -1;
}

void CtTextRenderer::SetDefaultFormat(const TextFormat& default_format) {
  impl_->default_format_ = default_format;
}

void CtTextRenderer::DrawCaret(CanvasInterface* canvas,
                               int caret_pos,
                               const Color& color) {
  CTFrameRef frame = impl_->GetFrame();
  if (!frame) {
    return;
  }

  if (caret_pos < 0 || caret_pos > CTFrameGetVisibleStringRange(frame).length) {
    return;
  }

  // Find the line of the caret
  MacArray<CTLineRef> lines(CTFrameGetLines(frame));
  CFIndex line_index = CFArrayBSearchValues(
      lines.get(),
      CFRangeMake(0, lines.size()),
      NULL,  // value
      CompareLineWithPos,
      &caret_pos);
  CTLineRef line = lines[line_index];

  // Get the height of the line
  CGFloat ascent, descent;
  CTLineGetTypographicBounds(line, &ascent, &descent, NULL /* leading */);

  // Get the origin of the line
  CGPoint origin;
  CTFrameGetLineOrigins(frame, CFRangeMake(line_index, 1), &origin);

  // Get the offset of the caret
  CGFloat offset = CTLineGetOffsetForStringIndex(line,
                                                 caret_pos,
                                                 NULL); // secondaryOffset

  double x = impl_->x_ + offset + origin.x;
  // The origin of the frame is the bottom-left corner
  double y = impl_->y_ + impl_->height_ - origin.y - ascent;
  double height = ascent + descent;
  canvas->DrawLine(x, y,
                   x, y + height,
                   ceil(kCaretWidthFactor * height),
                   color);
}

} // namespace mac
} // namespace ggadget
