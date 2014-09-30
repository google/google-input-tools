/*
  Copyright 2008 Google Inc.

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

#include <ggadget/common.h>
#include <ggadget/element_factory.h>
#include <ggadget/view.h>
#include <ggadget/basic_element.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/string_utils.h>

namespace ggadget {
namespace internal {

static const char kHtmlFlashCode[] =
  "<html>\n"
  "<meta http-equiv=\"Content-Type\" content=\"text/html; charset=utf-8\">\n"
  "<style>*{ margin:0px; padding:0px }</style>\n"
  "<body oncontextmenu=\"return false;\">\n"
  "<embed src=\"%s\" "
  "quality=\"high\" bgcolor=\"#ffffff\" width=\"100%\" play=\"true\" "
  "height=\"100%\" type=\"application/x-shockwave-flash\" "
  "swLiveConnect=\"true\" wmode=\"transparent\" name=\"movieObject\" "
  "pluginspage=\"http://www.adobe.com/go/getflashplayer\"/>\n"
  "</body>\n"
  "<script language=\"JavaScript\">\n"
  "window.external.movieObject = window.document.movieObject;\n"
  "</script>\n"
  "</html>";

static const char *kFlashMethods[] = {
  "GetVariable",
  "GotoFrame",
  "IsPlaying",
  "LoadMovie",
  "Pan",
  "PercentLoaded",
  "Play",
  "Rewind",
  "SetVariable",
  "SetZoomRect",
  "StopPlay",
  "TotalFrames",
  "Zoom",
  "TCallFrame",
  "TCallLabel",
  "TCurrentFrame",
  "TCurrentLabel",
  "TGetProperty",
  "TGetPropertyAsNumber",
  "TGotoFrame",
  "TGotoLabel",
  "TPlay",
  "TSetProperty",
  "TStopPlay"
};

class HtmlFlashElement : public BasicElement {
  class ExternalObject : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0x64eaa63bd2cc4efb, ScriptableInterface);

    ExternalObject(HtmlFlashElement *owner) : owner_(owner) { }

   protected:
    virtual void DoRegister() {
      RegisterProperty("movieObject", NULL,
                       NewSlot(owner_, &HtmlFlashElement::SetMovieObject));
    }

   private:
     HtmlFlashElement *owner_;
  };

  class MethodCaller : public Slot {
   public:
    MethodCaller(HtmlFlashElement *owner, const char *name)
      : owner_(owner), name_(name) {
    }

    virtual ResultVariant Call(ScriptableInterface *object,
                               int argc, const Variant argv[]) const {
      GGL_UNUSED(object);
      if (owner_ && name_ && owner_->movie_object_.Get()) {
        Slot *slot = NULL;
        ResultVariant prop = owner_->movie_object_.Get()->GetProperty(name_);
        if (prop.v().type() == Variant::TYPE_SCRIPTABLE) {
          ScriptableInterface *obj =
              VariantValue<ScriptableInterface *>()(prop.v());
          if (obj) {
            ResultVariant slot_prop = obj->GetProperty("");
            if (slot_prop.v().type() == Variant::TYPE_SLOT) {
              slot = VariantValue<Slot *>()(slot_prop.v());
            }
          }
        } else if (prop.v().type() == Variant::TYPE_SLOT) {
          slot = VariantValue<Slot *>()(prop.v());
        }
        if (slot) {
          return slot->Call(owner_->movie_object_.Get(), argc, argv);
        }
      }
      return ResultVariant();
    }

    virtual bool HasMetadata() const {
      return false;
    }

    virtual Variant::Type GetReturnType() const {
      return Variant::TYPE_VARIANT;
    }

    virtual bool operator==(const Slot &another) const {
      return owner_ == down_cast<const MethodCaller *>(&another)->owner_ &&
          name_ == down_cast<const MethodCaller *>(&another)->name_;
    }

   private:
    HtmlFlashElement *owner_;
    const char *name_;
  };

 public:
  DEFINE_CLASS_ID(0x2613c535747940a6, BasicElement);

  HtmlFlashElement(View *view, const char *name)
    : BasicElement(view, "flash", name, false),
      browser_(view->GetElementFactory()->CreateElement("_browser", view, "")),
      external_(this) {
    SetPixelX(0);
    SetPixelY(0);
    SetRelativeWidth(1.0);
    SetRelativeHeight(1.0);
    if (browser_) {
      browser_->SetParentElement(this);
      browser_->SetPixelX(0);
      browser_->SetPixelY(0);
      browser_->SetRelativeWidth(1.0);
      browser_->SetRelativeHeight(1.0);
      browser_->SetEnabled(true);
      // Force the browser window to be loaded.
      browser_->RecursiveLayout();
      if (!browser_->SetProperty("external", Variant(&external_))) {
        DLOG("Invalid browser element.");
        delete browser_;
        browser_ = NULL;
      }
    } else {
      DLOG("Failed to create _browser element.");
    }
  }

  virtual ~HtmlFlashElement() {
    movie_object_.Reset(NULL);
    delete browser_;
  }

  static BasicElement *CreateInstance(View *view, const char *name) {
    return new HtmlFlashElement(view, name);
  }

 public:
  virtual void Layout() {
    BasicElement::Layout();
    if (browser_)
      browser_->RecursiveLayout();
  }

 protected:
  virtual void DoClassRegister() {
    // It's not necessary to call BasicElement::DoClassRegister(),
    // if it's loaded in object element.
    BasicElement::DoClassRegister();
    RegisterProperty("movie",
                     NewSlot(&HtmlFlashElement::GetSrc),
                     NewSlot(&HtmlFlashElement::SetSrc));
    RegisterProperty("src",
                     NewSlot(&HtmlFlashElement::GetSrc),
                     NewSlot(&HtmlFlashElement::SetSrc));
  }

  virtual void DoRegister() {
    if (browser_) {
      for (size_t i = 0; i < arraysize(kFlashMethods); ++i) {
        RegisterMethod(kFlashMethods[i],
                       new MethodCaller(this, kFlashMethods[i]));
      }

      SetDynamicPropertyHandler(NewSlot(this, &HtmlFlashElement::GetProperty),
                                NewSlot(this, &HtmlFlashElement::SetProperty));
    }
  }

  virtual void DoDraw(CanvasInterface *canvas) {
    if (browser_)
      browser_->Draw(canvas);
  }

  virtual EventResult HandleMouseEvent(const MouseEvent &event) {
    BasicElement *fired, *in;
    ViewInterface::HitTest hittest;
    return browser_ ?
        browser_->OnMouseEvent(event, true, &fired, &in, &hittest) :
        EVENT_RESULT_UNHANDLED;
  }

  virtual EventResult HandleDragEvent(const DragEvent &event) {
    BasicElement *fired;
    return browser_ ? browser_->OnDragEvent(event, true, &fired) :
        EVENT_RESULT_UNHANDLED;
  }

  virtual EventResult HandleKeyEvent(const KeyboardEvent &event) {
    return browser_ ? browser_->OnKeyEvent(event) : EVENT_RESULT_UNHANDLED;
  }

  virtual EventResult HandleOtherEvent(const Event &event) {
    return browser_ ? browser_->OnOtherEvent(event) : EVENT_RESULT_UNHANDLED;
  }

  virtual void AggregateMoreClipRegion(const Rectangle &boundary,
                                       ClipRegion *region) {
    if (browser_) {
      browser_->AggregateClipRegion(boundary, region);
    }
  }

 private:
  Variant GetProperty(const std::string &name) {
    if (movie_object_.Get()) {
      Variant value;
      ScriptableInterface *scriptable = NULL;
      { // Life scope of ResultVariant result.
        ResultVariant result = movie_object_.Get()->GetProperty(name.c_str());
        value = result.v();
        if (value.type() == Variant::TYPE_SCRIPTABLE) {
          scriptable = VariantValue<ScriptableInterface *>()(value);
          // Add a reference to prevent ResultVariant from deleting the object.
          if (scriptable)
            scriptable->Ref();
        }
      }
      // Release the temporary reference but don't delete the object.
      if (scriptable)
        scriptable->Unref(true);
      return value;
    }
    return Variant();
  }

  bool SetProperty(const std::string &name, const Variant &value) {
    if (movie_object_.Get())
      return movie_object_.Get()->SetProperty(name.c_str(), value);
    return false;
  }

  void SetSrc(const char *src) {
    DLOG("SetSrc: %s", src);
    if (browser_) {
      movie_object_.Reset(NULL);
      src_ = src ? src : "";
      std::string content = StringPrintf(kHtmlFlashCode, src_.c_str());
      browser_->SetProperty("innerText", Variant(content));
    }
  }

  std::string GetSrc() {
    return src_;
  }

  void SetMovieObject(ScriptableInterface *movie_object) {
    DLOG("SetMovieObject: %p, Id=%jx",
         movie_object, movie_object ? movie_object->GetClassId() : 0);
    movie_object_.Reset(movie_object);
  }

 private:
  DISALLOW_EVIL_CONSTRUCTORS(HtmlFlashElement);

  BasicElement *browser_;
  ScriptableHolder<ScriptableInterface> movie_object_;
  ExternalObject external_;
  std::string src_;
};

} // namespace internal
} // namespace ggadget

#define Initialize html_flash_element_LTX_Initialize
#define Finalize html_flash_element_LTX_Finalize
#define RegisterElementExtension html_flash_element_LTX_RegisterElementExtension

extern "C" {
  bool Initialize() {
    LOGI("Initialize html_flash_element extension.");
    return true;
  }

  void Finalize() {
    LOGI("Finalize html_flash_element extension.");
  }

  bool RegisterElementExtension(ggadget::ElementFactory *factory) {
    if (factory) {
      LOGI("Register html_flash_element extension, using name \"flash\".");
      factory->RegisterElementClass(
          "clsid:D27CDB6E-AE6D-11CF-96B8-444553540000",
          &ggadget::internal::HtmlFlashElement::CreateInstance);
      factory->RegisterElementClass(
          "progid:ShockwaveFlash.ShockwaveFlash.9",
          &ggadget::internal::HtmlFlashElement::CreateInstance);
      factory->RegisterElementClass(
          "progid:ShockwaveFlash.ShockwaveFlash",
          &ggadget::internal::HtmlFlashElement::CreateInstance);
      factory->RegisterElementClass(
          "flash",
          &ggadget::internal::HtmlFlashElement::CreateInstance);
    }
    return true;
  }
}
