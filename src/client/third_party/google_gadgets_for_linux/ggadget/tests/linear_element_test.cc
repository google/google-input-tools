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
#include "ggadget/element_factory.h"
#include "ggadget/elements.h"
#include "ggadget/linear_element.h"
#include "ggadget/xml_utils.h"
#include "ggadget/view.h"
#include "mocked_element.h"
#include "mocked_gadget.h"
#include "mocked_timer_main_loop.h"
#include "mocked_view_host.h"

#if defined(OS_WIN)
#include "ggadget/win32/xml_parser.h"
#elif defined(OS_POSIX)
#include "init_extensions.h"
#endif

MockedTimerMainLoop main_loop(0);

using namespace ggadget;

class LinearElementTest : public testing::Test {
 protected:
  LinearElementTest()
      : element_factory_(NULL),
        gadget_(NULL),
        view_host_(NULL),
        view_(NULL) {
  }

  virtual void SetUp() {
    element_factory_ = new ElementFactory();
    element_factory_->RegisterElementClass(
        "muffin", MuffinElement::CreateInstance);
    element_factory_->RegisterElementClass(
        "pie", PieElement::CreateInstance);
    gadget_ = new MockedGadget(NULL);
    view_host_ = new MockedViewHost(ViewHostInterface::VIEW_HOST_MAIN);
    view_ = new View(view_host_, gadget_, element_factory_, NULL);
    view_->SetWidth(100);
    view_->SetHeight(100);
    linear_ = down_cast<LinearElement*>(
        view_->GetChildren()->AppendElement("linear", NULL));
  }

  virtual void TearDown() {
    // linear_ will be deleted when destroying view_;
    delete view_;
    // view_host_ will be deleted when destroying view_;
    delete gadget_;
    delete element_factory_;
  }

  ElementFactory *element_factory_;
  MockedGadget* gadget_;
  MockedViewHost* view_host_;
  View* view_;
  LinearElement* linear_;
};

TEST_F(LinearElementTest, TestProperties) {
  linear_->SetOrientation(LinearElement::ORIENTATION_HORIZONTAL);
  ASSERT_EQ(LinearElement::ORIENTATION_HORIZONTAL, linear_->GetOrientation());

  linear_->SetOrientation(LinearElement::ORIENTATION_VERTICAL);
  ASSERT_EQ(LinearElement::ORIENTATION_VERTICAL, linear_->GetOrientation());

  ASSERT_TRUE(linear_->SetProperty("orientation", Variant("horizontal")));
  ASSERT_EQ(Variant("horizontal"), linear_->GetProperty("orientation").v());
  ASSERT_EQ(LinearElement::ORIENTATION_HORIZONTAL, linear_->GetOrientation());

  ASSERT_TRUE(linear_->SetProperty("orientation", Variant("vertical")));
  ASSERT_EQ(Variant("vertical"), linear_->GetProperty("orientation").v());
  ASSERT_EQ(LinearElement::ORIENTATION_VERTICAL, linear_->GetOrientation());

  linear_->SetHorizontalAutoSizing(true);
  ASSERT_TRUE(linear_->IsHorizontalAutoSizing());
  linear_->SetHorizontalAutoSizing(false);
  ASSERT_FALSE(linear_->IsHorizontalAutoSizing());

  linear_->SetVerticalAutoSizing(true);
  ASSERT_TRUE(linear_->IsVerticalAutoSizing());
  linear_->SetVerticalAutoSizing(false);
  ASSERT_FALSE(linear_->IsVerticalAutoSizing());

  ASSERT_TRUE(linear_->SetProperty("width", Variant("auto")));
  ASSERT_EQ(Variant("auto"), linear_->GetProperty("width").v());
  ASSERT_TRUE(linear_->IsHorizontalAutoSizing());

  ASSERT_TRUE(linear_->SetProperty("height", Variant("auto")));
  ASSERT_EQ(Variant("auto"), linear_->GetProperty("height").v());
  ASSERT_TRUE(linear_->IsVerticalAutoSizing());

  linear_->SetPadding(100.0);
  ASSERT_DOUBLE_EQ(100.0, linear_->GetPadding());
  ASSERT_EQ(Variant(100.0), linear_->GetProperty("padding").v());

  BasicElement *c1 = linear_->GetChildren()->AppendElement("muffin", NULL);

  linear_->SetChildLayoutDirection(c1, LinearElement::LAYOUT_BACKWARD);
  ASSERT_EQ(LinearElement::LAYOUT_BACKWARD,
            linear_->GetChildLayoutDirection(c1));
  ASSERT_EQ(Variant("backward"), c1->GetProperty("linearLayoutDir").v());

  linear_->SetChildLayoutDirection(c1, LinearElement::LAYOUT_FORWARD);
  ASSERT_EQ(LinearElement::LAYOUT_FORWARD,
            linear_->GetChildLayoutDirection(c1));
  ASSERT_EQ(Variant("forward"), c1->GetProperty("linearLayoutDir").v());

  view_->GetChildren()->AppendElement(c1);
  ASSERT_EQ(ScriptableInterface::PROPERTY_NOT_EXIST,
            c1->GetPropertyInfo("linearLayoutDir", NULL));
}

TEST_F(LinearElementTest, TestHorizontalLayout) {
  linear_->SetOrientation(LinearElement::ORIENTATION_HORIZONTAL);
  linear_->SetHorizontalAutoSizing(true);
  linear_->SetVerticalAutoSizing(true);
  linear_->SetPadding(10.0);

  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.0, linear_->GetPixelWidth());

  BasicElement *c1 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c2 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c3 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c4 = linear_->GetChildren()->AppendElement("muffin", NULL);

  c1->SetPixelWidth(10.0);
  c1->SetPixelHeight(50.0);

  c2->SetRelativeWidth(0.1);
  c2->SetRelativeHeight(1);

  c3->SetPixelWidth(20.0);
  c3->SetPixelHeight(30.0);

  c4->SetRelativeWidth(0.4);
  c4->SetRelativeHeight(0.5);

  linear_->SetChildLayoutDirection(c3, LinearElement::LAYOUT_BACKWARD);
  linear_->SetChildLayoutDirection(c4, LinearElement::LAYOUT_BACKWARD);

  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(120.0, linear_->GetPixelWidth());
  ASSERT_DOUBLE_EQ(120.0, linear_->GetMinWidth());

  ASSERT_DOUBLE_EQ(50.0, linear_->GetPixelHeight());
  ASSERT_DOUBLE_EQ(50.0, linear_->GetMinHeight());

  ASSERT_DOUBLE_EQ(0.0, c1->GetPixelX());
  ASSERT_DOUBLE_EQ(20.0, c2->GetPixelX());
  ASSERT_DOUBLE_EQ(12.0, c2->GetPixelWidth());
  ASSERT_DOUBLE_EQ(42.0, c3->GetPixelX());
  ASSERT_DOUBLE_EQ(72.0, c4->GetPixelX());
  ASSERT_DOUBLE_EQ(48.0, c4->GetPixelWidth());

  c2->SetMinWidth(20.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, linear_->GetPixelWidth());
  ASSERT_DOUBLE_EQ(200.0, linear_->GetMinWidth());

  ASSERT_DOUBLE_EQ(0.0, c1->GetPixelX());
  ASSERT_DOUBLE_EQ(20.0, c2->GetPixelX());
  ASSERT_DOUBLE_EQ(20.0, c2->GetPixelWidth());
  ASSERT_DOUBLE_EQ(90.0, c3->GetPixelX());
  ASSERT_DOUBLE_EQ(120.0, c4->GetPixelX());
  ASSERT_DOUBLE_EQ(80.0, c4->GetPixelWidth());

  c1->SetRelativeY(0.2);
  c1->SetPixelPinY(20.0);
  c1->SetPixelHeight(80.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());

  ASSERT_DOUBLE_EQ(100.0, linear_->GetPixelHeight());
  ASSERT_DOUBLE_EQ(100.0, linear_->GetMinHeight());

  view_->SetWidth(300);
  linear_->SetHorizontalAutoSizing(false);
  linear_->SetRelativeWidth(1.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(300.0, linear_->GetPixelWidth());
  ASSERT_DOUBLE_EQ(200.0, linear_->GetMinWidth());

  ASSERT_DOUBLE_EQ(0.0, c1->GetPixelX());
  ASSERT_DOUBLE_EQ(20.0, c2->GetPixelX());
  ASSERT_DOUBLE_EQ(30.0, c2->GetPixelWidth());
  ASSERT_DOUBLE_EQ(150.0, c3->GetPixelX());
  ASSERT_DOUBLE_EQ(180.0, c4->GetPixelX());
  ASSERT_DOUBLE_EQ(120.0, c4->GetPixelWidth());

  linear_->SetPixelWidth(100);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, linear_->GetPixelWidth());
  ASSERT_DOUBLE_EQ(0.0, linear_->GetMinWidth());
}

TEST_F(LinearElementTest, TestVerticalLayout) {
  linear_->SetOrientation(LinearElement::ORIENTATION_VERTICAL);
  linear_->SetHorizontalAutoSizing(true);
  linear_->SetVerticalAutoSizing(true);
  linear_->SetPadding(10.0);

  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.0, linear_->GetPixelHeight());

  BasicElement *c1 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c2 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c3 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c4 = linear_->GetChildren()->AppendElement("muffin", NULL);

  c1->SetPixelHeight(10.0);
  c1->SetPixelWidth(50.0);

  c2->SetRelativeHeight(0.1);
  c2->SetRelativeWidth(1);

  c3->SetPixelHeight(20.0);
  c3->SetPixelWidth(30.0);

  c4->SetRelativeHeight(0.4);
  c4->SetRelativeWidth(0.5);

  linear_->SetChildLayoutDirection(c3, LinearElement::LAYOUT_BACKWARD);
  linear_->SetChildLayoutDirection(c4, LinearElement::LAYOUT_BACKWARD);

  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(120.0, linear_->GetPixelHeight());
  ASSERT_DOUBLE_EQ(120.0, linear_->GetMinHeight());

  ASSERT_DOUBLE_EQ(50.0, linear_->GetPixelWidth());
  ASSERT_DOUBLE_EQ(50.0, linear_->GetMinWidth());

  ASSERT_DOUBLE_EQ(0.0, c1->GetPixelY());
  ASSERT_DOUBLE_EQ(20.0, c2->GetPixelY());
  ASSERT_DOUBLE_EQ(12.0, c2->GetPixelHeight());
  ASSERT_DOUBLE_EQ(42.0, c3->GetPixelY());
  ASSERT_DOUBLE_EQ(72.0, c4->GetPixelY());
  ASSERT_DOUBLE_EQ(48.0, c4->GetPixelHeight());

  c2->SetMinHeight(20.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, linear_->GetPixelHeight());
  ASSERT_DOUBLE_EQ(200.0, linear_->GetMinHeight());

  ASSERT_DOUBLE_EQ(0.0, c1->GetPixelY());
  ASSERT_DOUBLE_EQ(20.0, c2->GetPixelY());
  ASSERT_DOUBLE_EQ(20.0, c2->GetPixelHeight());
  ASSERT_DOUBLE_EQ(90.0, c3->GetPixelY());
  ASSERT_DOUBLE_EQ(120.0, c4->GetPixelY());
  ASSERT_DOUBLE_EQ(80.0, c4->GetPixelHeight());

  c1->SetRelativeX(0.2);
  c1->SetPixelPinX(20.0);
  c1->SetPixelWidth(80.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());

  ASSERT_DOUBLE_EQ(100.0, linear_->GetPixelWidth());
  ASSERT_DOUBLE_EQ(100.0, linear_->GetMinWidth());

  view_->SetHeight(300);
  linear_->SetVerticalAutoSizing(false);
  linear_->SetRelativeHeight(1.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(300.0, linear_->GetPixelHeight());
  ASSERT_DOUBLE_EQ(200.0, linear_->GetMinHeight());

  ASSERT_DOUBLE_EQ(0.0, c1->GetPixelY());
  ASSERT_DOUBLE_EQ(20.0, c2->GetPixelY());
  ASSERT_DOUBLE_EQ(30.0, c2->GetPixelHeight());
  ASSERT_DOUBLE_EQ(150.0, c3->GetPixelY());
  ASSERT_DOUBLE_EQ(180.0, c4->GetPixelY());
  ASSERT_DOUBLE_EQ(120.0, c4->GetPixelHeight());

  linear_->SetPixelHeight(100);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, linear_->GetPixelHeight());
  ASSERT_DOUBLE_EQ(0.0, linear_->GetMinHeight());
}

TEST_F(LinearElementTest, TestZeroSizeChildren) {
  linear_->SetOrientation(LinearElement::ORIENTATION_HORIZONTAL);
  linear_->SetHorizontalAutoSizing(true);
  linear_->SetVerticalAutoSizing(true);
  linear_->SetPadding(0.0);

  for (int i = 0; i < 10; ++i) {
    BasicElement *c = linear_->GetChildren()->AppendElement("muffin", NULL);
    c->SetPixelWidth(0.0);
    c->SetPixelHeight(0.0);
  }

  BasicElement *c = linear_->GetChildren()->AppendElement("muffin", NULL);
  c->SetPixelWidth(1.0);
  c->SetPixelHeight(1.0);

  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(1.0, linear_->GetPixelWidth());
  ASSERT_DOUBLE_EQ(1.0, linear_->GetPixelHeight());

  linear_->SetOrientation(LinearElement::ORIENTATION_VERTICAL);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(1.0, linear_->GetPixelWidth());
  ASSERT_DOUBLE_EQ(1.0, linear_->GetPixelHeight());
}

TEST_F(LinearElementTest, TestRightToLeftLayout) {
  linear_->SetOrientation(LinearElement::ORIENTATION_HORIZONTAL);
  linear_->SetTextDirection(BasicElement::TEXT_DIRECTION_RIGHT_TO_LEFT);
  linear_->SetHorizontalAutoSizing(true);
  linear_->SetVerticalAutoSizing(true);
  linear_->SetPadding(10.0);

  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.0, linear_->GetPixelWidth());

  BasicElement *c1 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c2 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c3 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c4 = linear_->GetChildren()->AppendElement("muffin", NULL);

  c1->SetPixelWidth(10.0);
  c1->SetPixelHeight(50.0);

  c2->SetRelativeWidth(0.1);
  c2->SetRelativeHeight(1);

  c3->SetPixelWidth(20.0);
  c3->SetPixelHeight(30.0);

  c4->SetRelativeWidth(0.4);
  c4->SetRelativeHeight(0.5);

  linear_->SetChildLayoutDirection(c3, LinearElement::LAYOUT_BACKWARD);
  linear_->SetChildLayoutDirection(c4, LinearElement::LAYOUT_BACKWARD);

  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(120.0, linear_->GetPixelWidth());
  ASSERT_DOUBLE_EQ(120.0, linear_->GetMinWidth());

  ASSERT_DOUBLE_EQ(50.0, linear_->GetPixelHeight());
  ASSERT_DOUBLE_EQ(50.0, linear_->GetMinHeight());

  ASSERT_DOUBLE_EQ(110.0, c1->GetPixelX());
  ASSERT_DOUBLE_EQ(88.0, c2->GetPixelX());
  ASSERT_DOUBLE_EQ(12.0, c2->GetPixelWidth());
  ASSERT_DOUBLE_EQ(58.0, c3->GetPixelX());
  ASSERT_DOUBLE_EQ(0.0, c4->GetPixelX());
  ASSERT_DOUBLE_EQ(48.0, c4->GetPixelWidth());
}


TEST_F(LinearElementTest, TestAutoStretch) {
  linear_->SetOrientation(LinearElement::ORIENTATION_HORIZONTAL);
  linear_->SetVerticalAutoSizing(true);
  linear_->SetPadding(10.0);

  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.0, linear_->GetPixelWidth());

  BasicElement *c1 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c2 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c3 = linear_->GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c4 = linear_->GetChildren()->AppendElement("muffin", NULL);

  c1->SetPixelWidth(10.0);
  c1->SetPixelHeight(50.0);

  c2->SetRelativeWidth(0.7);
  c2->SetRelativeHeight(1);

  c3->SetRelativeWidth(0.1);
  c3->SetPixelHeight(30.0);

  c4->SetRelativeWidth(0.3);
  c4->SetRelativeHeight(0.5);

  linear_->SetChildLayoutDirection(c3, LinearElement::LAYOUT_BACKWARD);
  linear_->SetChildLayoutDirection(c4, LinearElement::LAYOUT_BACKWARD);
  linear_->SetChildAutoStretch(c2, true);
  linear_->SetChildAutoStretch(c4, true);
  linear_->SetPixelWidth(100);
  ASSERT_TRUE(view_host_->GetQueuedDraw());

  ASSERT_DOUBLE_EQ(50.0, linear_->GetPixelHeight());
  ASSERT_DOUBLE_EQ(50.0, linear_->GetMinHeight());

  ASSERT_DOUBLE_EQ(0.0, c1->GetPixelX());
  ASSERT_DOUBLE_EQ(20.0, c2->GetPixelX());
  ASSERT_DOUBLE_EQ(35.0, c2->GetPixelWidth());
  ASSERT_DOUBLE_EQ(65.0, c3->GetPixelX());
  ASSERT_DOUBLE_EQ(10.0, c3->GetPixelWidth());
  ASSERT_DOUBLE_EQ(85.0, c4->GetPixelX());
  ASSERT_DOUBLE_EQ(15.0, c4->GetPixelWidth());

  c2->SetMinWidth(10);
  c3->SetMinWidth(10);
  c4->SetMinWidth(10);
  linear_->SetHorizontalAutoSizing(true);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, linear_->GetPixelWidth());
  ASSERT_DOUBLE_EQ(0.0, c1->GetPixelX());
  ASSERT_DOUBLE_EQ(20.0, c2->GetPixelX());
  ASSERT_DOUBLE_EQ(35.0, c2->GetPixelWidth());
  ASSERT_DOUBLE_EQ(65.0, c3->GetPixelX());
  ASSERT_DOUBLE_EQ(10.0, c3->GetPixelWidth());
  ASSERT_DOUBLE_EQ(85.0, c4->GetPixelX());
  ASSERT_DOUBLE_EQ(15.0, c4->GetPixelWidth());
}

int main(int argc, char *argv[]) {
  SetGlobalMainLoop(&main_loop);
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

  return RUN_ALL_TESTS();
}
