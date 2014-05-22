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

#include <string>
#include <map>
#include "unittest/gtest.h"
#include "mocked_file_manager.h"
#include "ggadget/file_manager_wrapper.h"
#include "ggadget/file_manager_factory.h"
#include "ggadget/graphics_interface.h"
#include "ggadget/image_interface.h"
#include "ggadget/image_cache.h"
#include "ggadget/logger.h"
#include "ggadget/gadget_consts.h"

using namespace ggadget;

#if defined(OS_WIN)
#define SEP "\\"
#elif defined(OS_POSIX)
#define SEP "/"
#endif
class MockedGraphics : public ggadget::GraphicsInterface {
  class MockedImage : public ggadget::ImageInterface {
   public:
    MockedImage(MockedGraphics *gfx, const std::string &tag,
                bool share, bool is_mask)
      : gfx_(gfx), tag_(tag), is_mask_(is_mask) {
      if (share) {
        if (is_mask) {
          EXPECT_TRUE(gfx->mask_images_.find(tag_) == gfx->mask_images_.end());
          gfx->mask_images_[tag_] = this;
        } else {
          EXPECT_TRUE(gfx->images_.find(tag_) == gfx->images_.end());
          gfx->images_[tag_] = this;
        }
      }
    }
    ~MockedImage() {
      if (is_mask_)
        gfx_->mask_images_.erase(tag_);
      else
        gfx_->images_.erase(tag_);
    }
    virtual void Destroy() { delete this; }
    virtual const CanvasInterface *GetCanvas() const { return NULL; }
    virtual void Draw(CanvasInterface *canvas, double x, double y) const { }
    virtual void StretchDraw(CanvasInterface *canvas,
                             double x, double y,
                             double width, double height) const { }
    virtual double GetWidth() const { return 0; }
    virtual double GetHeight() const { return 0; }
    virtual ImageInterface *MultiplyColor(const Color &color) const {
      return new MockedImage(gfx_, tag_.c_str(), false, is_mask_);
    }
    virtual bool GetPointValue(double x, double y,
                               Color *color, double *opacity) const {
      return false;
    }
    virtual std::string GetTag() const { return tag_; }
    virtual bool IsFullyOpaque() const { return false; }

    MockedGraphics *gfx_;
    std::string tag_;
    bool is_mask_;
  };
 public:
  virtual ggadget::CanvasInterface *NewCanvas(double w, double h) const {
    return NULL;
  }
  virtual ggadget::ImageInterface *NewImage(const std::string &tag,
                                            const std::string &data,
                                            bool is_mask) const {
    return new MockedImage(const_cast<MockedGraphics*>(this), tag, true,
                           is_mask);
  }
  virtual ggadget::FontInterface *NewFont(
      const std::string &family, double pt_size,
      ggadget::FontInterface::Style style,
      ggadget::FontInterface::Weight weight) const {
    return NULL;
  }
  virtual ggadget::TextRendererInterface *NewTextRenderer() const {
    return NULL;
  }
  virtual double GetZoom() const { return 1.; }
  virtual void SetZoom(double z) { }
  virtual ggadget::Connection *ConnectOnZoom(
      ggadget::Slot1<void, double>* slot) const {
    return NULL;
  }

 public:
  std::map<std::string, MockedImage *> images_;
  std::map<std::string, MockedImage *> mask_images_;
};


FileManagerWrapper g_local_fm;
MockedFileManager *local_root;
MockedFileManager *global_root;
MockedFileManager *local;
MockedFileManager *resource;

TEST(ImageCache, LoadImage) {
  MockedGraphics gfx;
  ImageCache img_cache;
  ImageInterface *img1, *img2;

  img1 = img_cache.LoadImage(&gfx, &g_local_fm, "local-image", false);
  ASSERT_TRUE(img1);
  ASSERT_STREQ("local-image", local->requested_file_.c_str());
  ASSERT_STREQ("local-image", img1->GetTag().c_str());

  img2 = img1->MultiplyColor(Color(0.8, 0.3, 0.6));
  ASSERT_TRUE(img2);
  ASSERT_NE(img2, img1);
  img2->Destroy();
  img2 = img1->MultiplyColor(Color::kMiddleColor);
  ASSERT_EQ(img2, img1);
  img2->Destroy();

  local->requested_file_.clear();
  img2 = img_cache.LoadImage(&gfx, &g_local_fm, "local-image", false);
  ASSERT_EQ(img1, img2);
  ASSERT_TRUE(local->requested_file_.empty());
  ASSERT_STREQ("local-image", img2->GetTag().c_str());

  img2->Destroy();
  ASSERT_STREQ("local-image", img1->GetTag().c_str());

  img2 = img_cache.LoadImage(&gfx, &g_local_fm, "local-image", true);
  ASSERT_STREQ("local-image", img2->GetTag().c_str());
  ASSERT_STREQ("local-image", local->requested_file_.c_str());
  ASSERT_NE(img1, img2);

  img1->Destroy();
  img2->Destroy();

  img1 = img_cache.LoadImage(&gfx, &g_local_fm, SEP "global-image", false);
  ASSERT_STREQ("global-image", local_root->requested_file_.c_str());
  ASSERT_STREQ(SEP "global-image", img1->GetTag().c_str());
  global_root->requested_file_.clear();
  img1->Destroy();

  local_root->should_fail_ = true;
  img1 = img_cache.LoadImage(&gfx, &g_local_fm, SEP "global-image", false);
  ASSERT_STREQ("global-image", local_root->requested_file_.c_str());
  ASSERT_STREQ("global-image", global_root->requested_file_.c_str());
  img1->Destroy();

  global_root->should_fail_ = true;
  local_root->requested_file_.clear();
  global_root->requested_file_.clear();
  img2 = img_cache.LoadImage(&gfx, NULL, SEP "global-image2", false);
  ASSERT_STREQ("", local_root->requested_file_.c_str());
  ASSERT_STREQ("global-image2", global_root->requested_file_.c_str());
  ASSERT_TRUE(img2);
  ASSERT_STREQ(SEP "global-image2", img2->GetTag().c_str());
  ASSERT_FALSE(img2->GetCanvas());
  img2->Destroy();

  local->should_fail_ = true;
  local->requested_file_.clear();
  img1 = img_cache.LoadImage(&gfx, &g_local_fm, "non-exist-file", false);
  ASSERT_STREQ("non-exist-file", local->requested_file_.c_str());
  ASSERT_TRUE(img1);
  ASSERT_STREQ("non-exist-file", img1->GetTag().c_str());
  img2 = img1->MultiplyColor(Color::kMiddleColor);
  ASSERT_TRUE(img2 == NULL);
  img1->Destroy();

  ASSERT_FALSE(img_cache.LoadImage(&gfx, NULL, "", false));
}

int main(int argc, char *argv[]) {
  testing::ParseGTestFlags(&argc, argv);

  FileManagerWrapper *fm = new FileManagerWrapper();
  global_root = new MockedFileManager(SEP);
  fm->RegisterFileManager(kDirSeparatorStr, global_root);
  resource = new MockedFileManager(SEP "usr" SEP "share" SEP "google-gadgets"
                                   SEP "resources" SEP);
  fm->RegisterFileManager(kGlobalResourcePrefix, resource);
  SetGlobalFileManager(fm);

  local = new MockedFileManager(SEP "test" SEP "gadgets" SEP);
  g_local_fm.RegisterFileManager("", local);
  local_root = new MockedFileManager(SEP);
  g_local_fm.RegisterFileManager(SEP, local_root);

  return RUN_ALL_TESTS();
}
