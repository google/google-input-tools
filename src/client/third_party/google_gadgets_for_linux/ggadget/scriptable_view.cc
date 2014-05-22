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

#include "scriptable_view.h"

#include <string>
#include "basic_element.h"
#include "common.h"
#include "elements.h"
#include "file_manager_factory.h"
#include "file_manager_interface.h"
#include "gadget_consts.h"
#include "gadget_interface.h"
#include "image_interface.h"
#include "logger.h"
#include "permissions.h"
#include "scriptable_event.h"
#include "scriptable_image.h"
#include "script_context_interface.h"
#include "small_object.h"
#include "string_utils.h"
#include "system_utils.h"
#include "unicode_utils.h"
#include "xml_dom.h"
#include "xml_parser_interface.h"
#include "xml_utils.h"

namespace ggadget {

class ScriptableView::Impl : public SmallObject<> {
 public:
  class GlobalObject : public ScriptableHelperNativeOwnedDefault {
   public:
    DEFINE_CLASS_ID(0x23840d38ed164ab2, ScriptableInterface);
    virtual bool IsStrict() const { return false; }
  };

  Impl(ScriptableView *owner, View *view, ScriptableInterface *prototype,
       ScriptContextInterface *script_context)
    : owner_(owner),
      view_(view),
      script_context_(script_context) {
    ASSERT(view_);

    if (prototype)
      global_object_.SetInheritsFrom(prototype);

    if (script_context_) {
      script_context_->SetGlobalObject(&global_object_);

      // Old "utils" global object, for backward compatibility.
      utils_.RegisterMethod("loadImage",
                            NewSlot(this, &Impl::LoadScriptableImage));
      utils_.RegisterMethod("setTimeout",
                            NewSlot(this, &Impl::SetTimeout));
      utils_.RegisterMethod("clearTimeout",
                            NewSlot(view_, &View::ClearTimeout));
      utils_.RegisterMethod("setInterval",
                            NewSlot(this, &Impl::SetInterval));
      utils_.RegisterMethod("clearInterval",
                            NewSlot(view_, &View::ClearInterval));
      utils_.RegisterMethod("alert",
                            NewSlot(view_, &View::Alert));
      utils_.RegisterMethod("confirm",
                            NewSlot(view_, &View::Confirm));
      utils_.RegisterMethod("prompt",
                            NewSlot(view_, &View::Prompt));

      script_context_->AssignFromNative(NULL, "", "utils", Variant(&utils_));
    }
  }

  ~Impl() {
    SimpleEvent e(Event::EVENT_CLOSE);
    view_->OnOtherEvent(e);
  }

  void DoRegister() {
    DLOG("Register ScriptableView properties.");

    view_->SetScriptable(owner_);
    view_->RegisterProperties(global_object_.GetRegisterable());

    // Register view.event property here, because we need set owner_ into
    // ScriptableEvent if its SrcElement is NULL.
    owner_->RegisterProperty("event", NewSlot(this, &Impl::GetEvent), NULL);
    global_object_.RegisterProperty("event", NewSlot(this, &Impl::GetEvent),
                                    NULL);

    global_object_.RegisterConstant("view", owner_);
    global_object_.SetDynamicPropertyHandler(
        NewSlot(this, &Impl::GetElementByNameVariant), NULL);
  }

  ScriptableEvent *GetEvent() {
    ScriptableEvent *event = view_->GetEvent();
    if (event && event->GetSrcElement() == NULL)
      event->SetSrcElement(owner_);
    return event;
  }

  int SetTimeout(Slot *slot, int timeout) {
    Slot0<void> *callback = slot ? new SlotProxy0<void>(slot) : NULL;
    return view_->SetTimeout(callback, timeout);
  }

  int SetInterval(Slot *slot, int interval) {
    Slot0<void> *callback = slot ? new SlotProxy0<void>(slot) : NULL;
    return view_->SetInterval(callback, interval);
  }

  ScriptableImage *LoadScriptableImage(const Variant &image_src) {
    ImageInterface *image = view_->LoadImage(image_src, false);
    return image ? new ScriptableImage(image) : NULL;
  }

  Variant GetElementByNameVariant(const char *name) {
    BasicElement *result = view_->GetElementByName(name);
    return result ? Variant(result) : Variant();
  }

  bool InitFromXML(const std::string &xml, const char *filename) {
    DOMDocumentInterface *xmldoc = GetXMLParser()->CreateDOMDocument();
    xmldoc->Ref();
    GadgetInterface *gadget = view_->GetGadget();
    bool success = false;
    if (gadget) {
      success = gadget->ParseLocalizedXML(xml, filename, xmldoc);
    } else {
      // For unittest. Parse without encoding fallback and localization.
      success = GetXMLParser()->ParseContentIntoDOM(xml, NULL, filename,
                                                    NULL, NULL, NULL,
                                                    xmldoc, NULL, NULL);
    }
    if (!success) {
      xmldoc->Unref();
      return false;
    }

    DOMElementInterface *view_element = xmldoc->GetDocumentElement();
    if (!view_element ||
        GadgetStrCmp(view_element->GetTagName().c_str(), kViewTag) != 0) {
      LOG("No valid root element in view file: %s", filename);
      xmldoc->Unref();
      return false;
    }

    view_->EnableEvents(false);
    SetupScriptableProperties(owner_, script_context_, view_element, filename);

    Elements *children = view_->GetChildren();
    for (const DOMNodeInterface *child = view_element->GetFirstChild();
         child; child = child->GetNextSibling()) {
      if (child->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
        InsertElementFromDOM(children, script_context_,
                             down_cast<const DOMElementInterface *>(child),
                             NULL, filename);
      }
    }

    // Call layout here to make sure all elements' initial layout is correct
    // prior to running any script code.
    view_->Layout();
    view_->EnableEvents(true);

    if (script_context_ && !HandleAllScriptElements(view_element, filename)) {
      // Don't load the gadget if any script file can't be loaded.
      xmldoc->Unref();
      return false;
    }

    ASSERT(xmldoc->GetRefCount() == 1);
    xmldoc->Unref();

    // Fire "onopen" event here, to make sure that it's only fired once.
    view_->OnOtherEvent(SimpleEvent(Event::EVENT_OPEN));
    // Fire "onsize" event here. Some gadgets rely on it to initialize
    // view layout.
    view_->OnOtherEvent(SimpleEvent(Event::EVENT_SIZE));
    return true;
  }

  bool HandleScriptElement(const DOMElementInterface *xml_element,
                           const char *filename) {
    int lineno = xml_element->GetRow();
    std::string script;
    std::string src = GetAttributeGadgetCase(xml_element, kSrcAttr);

    if (!src.empty()) {
      if (strncmp(kGlobalResourcePrefix, src.c_str(),
                  sizeof(kGlobalResourcePrefix) - 1) == 0) {
        if (!GetGlobalFileManager()->ReadFile(src.c_str(), &script))
          return false;
      } else if (!view_->GetFileManager()->ReadFile(src.c_str(), &script)) {
        return false;
      }
        
      filename = src.c_str();
      lineno = 1;
      std::string temp;
      if (DetectAndConvertStreamToUTF8(script, &temp, NULL))
        script = temp;
    } else {
      // Uses the Windows version convention, that inline scripts should be
      // quoted in comments.
      for (const DOMNodeInterface *child = xml_element->GetFirstChild();
           child; child = child->GetNextSibling()) {
        if (child->GetNodeType() == DOMNodeInterface::COMMENT_NODE) {
          script = child->GetTextContent();
          break;
        } else if (child->GetNodeType() != DOMNodeInterface::TEXT_NODE ||
                   !TrimString(child->GetTextContent()).empty()) {
          // Other contents are not allowed under <script></script>.
          LOG("%s:%d:%d: This content is not allowed in script element",
              filename, child->GetRow(), child->GetColumn());
        }
      }
    }

    if (!script.empty())
      script_context_->Execute(script.c_str(), filename, lineno);
    return true;
  }

  bool HandleAllScriptElements(const DOMElementInterface *xml_element,
                               const char *filename) {
    for (const DOMNodeInterface *child = xml_element->GetFirstChild();
         child; child = child->GetNextSibling()) {
      if (child->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
        const DOMElementInterface *child_ele =
            down_cast<const DOMElementInterface *>(child);
        bool result;
        if (GadgetStrCmp(child_ele->GetTagName().c_str(), kScriptTag) == 0) {
          result = HandleScriptElement(child_ele, filename);
        } else {
          result = HandleAllScriptElements(child_ele, filename);
        }
        if (!result)
          return false;
      }
    }
    return true;
  }

  ScriptableView *owner_;
  View *view_;
  ScriptContextInterface *script_context_;

  NativeOwnedScriptable<UINT64_C(0x364d74f3646848ce)> utils_;
  GlobalObject global_object_;
};

ScriptableView::ScriptableView(View *view, ScriptableInterface *prototype,
                               ScriptContextInterface *script_context)
  : impl_(new Impl(this, view, prototype, script_context)) {
}

ScriptableView::~ScriptableView() {
  delete impl_;
  impl_ = NULL;
}

bool ScriptableView::InitFromXML(const std::string &xml, const char *filename) {
  return impl_->InitFromXML(xml, filename);
}

View *ScriptableView::view() {
  return impl_->view_;
}

void ScriptableView::DoRegister() {
  impl_->DoRegister();
}

} // namespace ggadget
