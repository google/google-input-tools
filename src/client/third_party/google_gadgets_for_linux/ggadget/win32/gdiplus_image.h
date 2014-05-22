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

// This header defines the class GdiplusImage which is the implementation of
// the ImageInterface on Gdi+.


#ifndef GGADGET_WIN32_GDIPLUS_IMAGE_H_
#define GGADGET_WIN32_GDIPLUS_IMAGE_H_

#include <string>
#include <ggadget/common.h>
#include <ggadget/image_interface.h>
#include <ggadget/scoped_ptr.h>

namespace ggadget {
namespace win32 {
class GdiplusGraphics;

// This class realizes the ImageInterface using Gdi+ Image.
class GdiplusImage : public ImageInterface {
 public:
  GdiplusImage();
  // Initializes the object using specified image data.
  // @param tag: the unique id of the image. Images with the same tag will be
  //             treated as the same image.
  // @param data: the raw data in bytes of image file. Data size is data.size().
  //              Currently supports BMP, GIF, JPEG, PNG, TIFF formats.
  // @param is_mask: true if the image will be used as a mask.
  // returns true if the initialization successes.
  bool Init(const std::string& tag, const std::string& data, bool is_mask);
  // Initializes the object with a blank bitmap with the given width and height.
  bool Init(int width, int height);
  // Check if the GdiplusImage object is valid.
  bool IsValid() const;
  virtual ~GdiplusImage();
  virtual void Destroy();
  virtual const CanvasInterface* GetCanvas() const;
  virtual void Draw(CanvasInterface* canvas, double x, double y) const;
  virtual void StretchDraw(CanvasInterface* canvas,
                           double x, double y,
                           double width, double height) const;
  virtual double GetWidth() const;
  virtual double GetHeight() const;
  virtual ImageInterface* MultiplyColor(const Color &color) const;
  virtual bool GetPointValue(double x, double y,
                             Color* color, double* opacity) const;
  virtual std::string GetTag() const;
  virtual bool IsFullyOpaque() const;

 private:
  class Impl;
  scoped_ptr<Impl> impl_;
  DISALLOW_EVIL_CONSTRUCTORS(GdiplusImage);
};

}  // namespace win32
}  // namespace ggadget

#endif  // GGADGET_WIN32_GDIPLUS_IMAGE_H_
