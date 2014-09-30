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

#include "unittest/gtest.h"
#include "ggadget/basic_element.h"
#include "ggadget/element_factory.h"
#include "ggadget/elements.h"
#include "ggadget/event.h"
#include "ggadget/scriptable_event.h"
#include "ggadget/scriptable_view.h"
#include "ggadget/slot.h"
#include "ggadget/view.h"
#include "ggadget/xml_utils.h"
#include "mocked_element.h"
#include "mocked_timer_main_loop.h"
#include "mocked_view_host.h"

#if defined(OS_WIN)
#include "ggadget/win32/xml_parser.h"
#elif defined(OS_POSIX)
#include "init_extensions.h"
#endif

ggadget::ElementFactory *g_factory = NULL;
MockedTimerMainLoop main_loop(0);
using ggadget::View;
using ggadget::ViewInterface;
using ggadget::ViewHostInterface;
using ggadget::ScriptableView;

class EventHandler {
 public:
  EventHandler(ggadget::View *view)
      : fired1_(false), fired2_(false), view_(view) {
    signal1_.Connect(ggadget::NewSlot(this, &EventHandler::Handle1));
    signal2_.Connect(ggadget::NewSlot(this, &EventHandler::Handle2));
  }
  void Handle1() {
    ASSERT_FALSE(fired2_);
    fired1_ = true;
    ggadget::ScriptableEvent *current_scriptable_event = view_->GetEvent();
    ASSERT_EQ(ggadget::Event::EVENT_KEY_DOWN,
              current_scriptable_event->GetEvent()->GetType());
    ggadget::MouseEvent event(ggadget::Event::EVENT_MOUSE_CLICK, 123, 456,
                              999, 888, ggadget::MouseEvent::BUTTON_LEFT,  666);
    ggadget::ScriptableEvent scriptable_event(&event, NULL, NULL);
    view_->FireEvent(&scriptable_event, signal2_);
    // The current event should be the same as before.
    ASSERT_EQ(current_scriptable_event, view_->GetEvent());
    ASSERT_EQ(ggadget::Event::EVENT_KEY_DOWN,
              current_scriptable_event->GetEvent()->GetType());
  }
  void Handle2() {
    ASSERT_TRUE(fired1_);
    fired2_ = true;
    ggadget::ScriptableEvent *scriptable_event = view_->GetEvent();
    const ggadget::Event *current_event = scriptable_event->GetEvent();
    ASSERT_EQ(ggadget::Event::EVENT_MOUSE_CLICK, current_event->GetType());
    const ggadget::MouseEvent *mouse_event =
        static_cast<const ggadget::MouseEvent *>(current_event);
    ASSERT_EQ(123, mouse_event->GetX());
    ASSERT_EQ(456, mouse_event->GetY());
    ASSERT_EQ(ggadget::MouseEvent::BUTTON_LEFT, mouse_event->GetButton());
    ASSERT_EQ(999, mouse_event->GetWheelDeltaX());
    ASSERT_EQ(888, mouse_event->GetWheelDeltaY());
  }

  ggadget::EventSignal signal1_, signal2_;
  bool fired1_, fired2_;
  ggadget::View *view_;
};

TEST(ViewTest, FireEvent) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  EventHandler handler(&view);
  ggadget::KeyboardEvent event(ggadget::Event::EVENT_KEY_DOWN,
                               2468, 1357, NULL);
  ggadget::ScriptableEvent scriptable_event(&event, NULL, NULL);
  view.FireEvent(&scriptable_event, handler.signal1_);
  ASSERT_TRUE(handler.fired1_);
  ASSERT_TRUE(handler.fired2_);
}

// This test is not merely for View, but mixed test for xml_utils and Elements.
TEST(ViewTest, XMLConstruction) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);
  ScriptableView scriptable_view(&view, NULL, NULL);

  ASSERT_FALSE(view.GetShowCaptionAlways());
  ASSERT_EQ(ggadget::ViewInterface::RESIZABLE_ZOOM, view.GetResizable());
  ASSERT_STREQ("", view.GetCaption().c_str());
  ASSERT_EQ(0U, view.GetChildren()->GetCount());

  const char *xml =
    "<view width=\"123\" height=\"456\" caption=\"View-Caption\"\n"
    "    resizable=\"zoom\" showCaptionAlways=\"true\">\n"
    "  <pie tooltip=\"pie-tooltip\" x=\"50%\" y=\"100\">\n"
    "    <muffin tagName=\"haha\" name=\"muffin\"/>\n"
    "  </pie>\n"
    "  <pie name=\"pie1\"/>\n"
    "</view>\n";
  ASSERT_TRUE(scriptable_view.InitFromXML(xml, "filename"));
  ASSERT_STREQ("View-Caption", view.GetCaption().c_str());
  ASSERT_EQ(ggadget::ViewInterface::RESIZABLE_ZOOM, view.GetResizable());
  ASSERT_TRUE(view.GetShowCaptionAlways());
  ASSERT_EQ(123, view.GetWidth());
  ASSERT_EQ(456, view.GetHeight());
  ASSERT_EQ(2U, view.GetChildren()->GetCount());

  ggadget::BasicElement *m = view.GetElementByName("muffin");
  ASSERT_TRUE(m != NULL);
  ASSERT_EQ(m, view.GetChildren()->GetItemByIndex(0)->
               GetChildren()->GetItemByIndex(0));

  ggadget::BasicElement *m1 = view.GetElementByName("pie1");
  ASSERT_TRUE(m1 != NULL);
  ASSERT_EQ(m1, view.GetChildren()->GetItemByIndex(1));

  view.GetChildren()->GetItemByIndex(0)->GetChildren()->RemoveElement(m);
  ASSERT_TRUE(view.GetElementByName("muffin") == NULL);

  ggadget::BasicElement *m2 =
      ggadget::down_cast<ggadget::Elements *>(
          view.GetChildren()->GetItemByIndex(0)->GetChildren())->
      AppendElementFromXML("<muffin name=\"new-muffin\"/>");
  ASSERT_TRUE(m2 != NULL);
  ASSERT_EQ(m2, view.GetChildren()->GetItemByIndex(0)->
                GetChildren()->GetItemByIndex(0));
}

TEST(ViewTest, MinSize) {
  MockedViewHost *host = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
  View view(host, NULL, g_factory, NULL);

  view.SetSize(100, 100);
  ASSERT_DOUBLE_EQ(100.0, view.GetWidth());
  ASSERT_DOUBLE_EQ(100.0, view.GetHeight());

  view.SetMinWidth(120);
  ASSERT_DOUBLE_EQ(120.0, view.GetWidth());
  view.SetWidth(80);
  ASSERT_DOUBLE_EQ(120.0, view.GetWidth());
  view.SetWidth(200);
  ASSERT_DOUBLE_EQ(200.0, view.GetWidth());

  view.SetMinHeight(120);
  ASSERT_DOUBLE_EQ(120.0, view.GetHeight());
  view.SetHeight(80);
  ASSERT_DOUBLE_EQ(120.0, view.GetHeight());
  view.SetHeight(200);
  ASSERT_DOUBLE_EQ(200.0, view.GetHeight());
}

int main(int argc, char *argv[]) {
  ggadget::SetGlobalMainLoop(&main_loop);
  testing::ParseGTestFlags(&argc, argv);

#if defined(OS_WIN)
  ggadget::win32::XMLParser xml_parser;
  ggadget::SetXMLParser(&xml_parser);
#elif defined(OS_POSIX)
  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);
#endif

  g_factory = new ggadget::ElementFactory();
  g_factory->RegisterElementClass("muffin", MuffinElement::CreateInstance);
  g_factory->RegisterElementClass("pie", PieElement::CreateInstance);
  int result = RUN_ALL_TESTS();
  delete g_factory;
  return result;
}
