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

class BasicElementTest : public testing::Test {
 protected:
  BasicElementTest()
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
  }

  virtual void TearDown() {
    delete view_;
    // view_host_ will be deleted when destroying view_;
    delete gadget_;
    delete element_factory_;
  }

  ElementFactory *element_factory_;
  MockedGadget* gadget_;
  MockedViewHost* view_host_;
  View* view_;
};

TEST_F(BasicElementTest, TestCreate) {
  MuffinElement m(view_, NULL);
  PieElement p(view_, NULL);
}

TEST_F(BasicElementTest, TestChildren) {
  MuffinElement m(view_, NULL);
  BasicElement *c1 = m.GetChildren()->AppendElement("muffin", NULL);
  BasicElement *c2 = m.GetChildren()->InsertElement("pie", c1, "First");
  BasicElement *c3 = m.GetChildren()->AppendElement("pie", "Last");
  ASSERT_EQ(3U, m.GetChildren()->GetCount());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(0) == c2);
  ASSERT_EQ(0U, c2->GetIndex());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(1) == c1);
  ASSERT_EQ(1U, c1->GetIndex());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(2) == c3);
  ASSERT_EQ(2U, c3->GetIndex());
  ASSERT_TRUE(m.GetChildren()->GetItemByName("First") == c2);
  ASSERT_TRUE(m.GetChildren()->GetItemByName("Last") == c3);
  m.GetChildren()->RemoveElement(c2);
  ASSERT_EQ(2U, m.GetChildren()->GetCount());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(0) == c1);
  ASSERT_EQ(0U, c1->GetIndex());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(1) == c3);
  ASSERT_EQ(1U, c3->GetIndex());
  m.GetChildren()->RemoveElement(c3);
  ASSERT_EQ(1U, m.GetChildren()->GetCount());
  ASSERT_TRUE(m.GetChildren()->GetItemByIndex(0) == c1);
  ASSERT_EQ(0U, c1->GetIndex());
  m.GetChildren()->RemoveAllElements();
  ASSERT_EQ(0U, m.GetChildren()->GetCount());
}

TEST_F(BasicElementTest, TestCursor) {
  MuffinElement m(view_, NULL);
  ASSERT_EQ(ViewInterface::CURSOR_DEFAULT, m.GetCursor());
  m.SetCursor(ViewInterface::CURSOR_BUSY);
  ASSERT_EQ(ViewInterface::CURSOR_BUSY, m.GetCursor());
}

TEST_F(BasicElementTest, TestDropTarget) {
  Permissions* permissions = gadget_->GetMutablePermissions();
  MuffinElement m(view_, NULL);
  ASSERT_FALSE(m.IsDropTarget());
  m.SetDropTarget(true);
  // No permission yet.
  EXPECT_FALSE(m.IsDropTarget());
  permissions->SetRequired(Permissions::FILE_READ, true);
  permissions->GrantAllRequired();
  EXPECT_FALSE(m.IsDropTarget());
  m.SetDropTarget(true);
  EXPECT_TRUE(m.IsDropTarget());
  m.SetDropTarget(false);
  EXPECT_FALSE(m.IsDropTarget());
}

TEST_F(BasicElementTest, TestEnabled) {
  MuffinElement m(view_, NULL);
  ASSERT_FALSE(m.IsEnabled());
  m.SetEnabled(true);
  ASSERT_TRUE(m.IsEnabled());
}

TEST_F(BasicElementTest, TestPixelHeight) {
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  view_->SetSize(100, 100);

  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  ASSERT_DOUBLE_EQ(0.0, m->GetPixelHeight());
  m->SetPixelHeight(100.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelHeight());
  // Setting the height as negative value will make no effect.
  m->SetPixelHeight(-100.0);
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelHeight());
  BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetPixelHeight(50.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  // Modifying the height of the parent will not effect the child.
  m->SetPixelHeight(150.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelHeight());
}

TEST_F(BasicElementTest, TestRelativeHeight) {
  view_->SetSize(400, 300);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(100);
  m->SetRelativeHeight(0.50);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, m->GetRelativeHeight());
  ASSERT_DOUBLE_EQ(150.0, m->GetPixelHeight());
  BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeHeight(0.50);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeHeight());
  ASSERT_DOUBLE_EQ(75.0, c->GetPixelHeight());
  // Setting the height as negative value will make no effect.
  c->SetRelativeHeight(-0.50);
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeHeight());
  ASSERT_DOUBLE_EQ(75.0, c->GetPixelHeight());
  // Modifying the height of the parent will effect the child.
  m->SetRelativeHeight(1.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeHeight());
  ASSERT_DOUBLE_EQ(150.0, c->GetPixelHeight());
  // Modifying the height of the parent will effect the child.
  m->SetPixelHeight(100.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelHeight());
}

TEST_F(BasicElementTest, TestHitTest) {
  MuffinElement m(view_, NULL);
  m.SetPixelWidth(1);
  m.SetPixelHeight(1);
  ASSERT_TRUE(m.GetHitTest(0, 0) == ViewInterface::HT_CLIENT);
  m.SetHitTest(ViewInterface::HT_CAPTION);
  ASSERT_TRUE(m.GetHitTest(0, 0) == ViewInterface::HT_CAPTION);
  ASSERT_TRUE(m.GetHitTest(1, 1) == ViewInterface::HT_TRANSPARENT);
}

/*TEST_F(BasicElementTest, TestMask) {
  MuffinElement m(view_, NULL);
  ASSERT_STREQ("", m.GetMask().c_str());
  m.SetMask("mymask.png");
  ASSERT_STREQ("mymask.png", m.GetMask().c_str());
  m.SetMask(NULL);
  ASSERT_STREQ("", m.GetMask().c_str());
}*/

TEST_F(BasicElementTest, TestName) {
  MuffinElement m(view_, "mymuffin");
  ASSERT_STREQ("mymuffin", m.GetName().c_str());
}

TEST_F(BasicElementTest, TestConst) {
  MuffinElement m(view_, NULL);
  BasicElement *c = m.GetChildren()->AppendElement("pie", NULL);
  const BasicElement *cc = c;
  ASSERT_TRUE(cc->GetView() == view_);
  ASSERT_TRUE(cc->GetParentElement() == &m);
}

TEST_F(BasicElementTest, TestOpacity) {
  view_->SetSize(100, 100);

  MuffinElement m(view_, NULL);
  ASSERT_DOUBLE_EQ(1.0, m.GetOpacity());
  m.SetOpacity(0.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.5, m.GetOpacity());
  // Setting the value greater than 1 will make no effect.
  m.SetOpacity(1.5);
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.5, m.GetOpacity());
  // Setting the value less than 0 will make no effect.
  m.SetOpacity(-0.5);
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.5, m.GetOpacity());
}

TEST_F(BasicElementTest, TestPixelPinX) {
  view_->SetSize(400, 300);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  ASSERT_DOUBLE_EQ(0.0, m->GetPixelPinX());
  m->SetPixelWidth(100.0);
  m->SetPixelHeight(100.0);
  m->SetPixelPinX(100.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.5, m->GetPixelPinX());
  // Modifying the width of the parent will not effect the pin x.
  m->SetPixelWidth(150.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.5, m->GetPixelPinX());
  ASSERT_FALSE(m->PinXIsRelative());
  m->SetPixelPinX(-50.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-50.5, m->GetPixelPinX());
}

TEST_F(BasicElementTest, TestRelativePinX) {
  view_->SetSize(400, 300);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(200.0);
  m->SetPixelHeight(100.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  m->SetRelativePinX(0.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelPinX());
  // Modifying the width will effect the pin x.
  m->SetPixelWidth(400.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelPinX());
  ASSERT_TRUE(m->PinXIsRelative());
  m->SetRelativePinX(-0.25);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-100.0, m->GetPixelPinX());
}

TEST_F(BasicElementTest, TestPixelPinY) {
  view_->SetSize(400, 300);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelHeight(150.5);
  m->SetPixelWidth(150.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  m->SetPixelPinY(100.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.5, m->GetPixelPinY());
  // Modifying the width will not effect the pin y.
  m->SetPixelHeight(300.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.5, m->GetPixelPinY());
  ASSERT_FALSE(m->PinYIsRelative());
  m->SetPixelPinY(-50.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-50.5, m->GetPixelPinY());
}

TEST_F(BasicElementTest, TestRelativePinY) {
  view_->SetSize(400, 300);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(150.0);
  m->SetPixelHeight(150.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  m->SetRelativePinY(0.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(75.0, m->GetPixelPinY());
  // Modifying the width of the parent will not effect the pin y.
  m->SetPixelHeight(300.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(150.0, m->GetPixelPinY());
  ASSERT_TRUE(m->PinYIsRelative());
  m->SetRelativePinY(-0.25);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-75.0, m->GetPixelPinY());
}

TEST_F(BasicElementTest, TestRotation) {
  MuffinElement m(view_, NULL);
  m.SetPixelWidth(100.0);
  m.SetPixelHeight(100.0);
  ASSERT_DOUBLE_EQ(0.0, m.GetRotation());
  m.SetRotation(0.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.5, m.GetRotation());
}

TEST_F(BasicElementTest, TestTooltip) {
  MuffinElement m(view_, NULL);
  ASSERT_STREQ("", m.GetTooltip().c_str());
  m.SetTooltip("mytooltip");
  ASSERT_STREQ("mytooltip", m.GetTooltip().c_str());
  m.SetTooltip("");
  ASSERT_STREQ("", m.GetTooltip().c_str());
}

TEST_F(BasicElementTest, TestPixelWidth) {
  view_->SetSize(400, 300);
  MuffinElement m(view_, NULL);
  ASSERT_DOUBLE_EQ(0.0, m.GetPixelWidth());
  m.SetPixelWidth(100.0);
  m.SetPixelHeight(100.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m.GetPixelWidth());
  // Setting the width as negative value will make no effect.
  m.SetPixelWidth(-100.0);
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m.GetPixelWidth());
  BasicElement *c = m.GetChildren()->AppendElement("pie", NULL);
  c->SetPixelWidth(50.0);
  // Modifying the width of the parent will not effect the child.
  m.SetPixelWidth(200.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelWidth());
}

TEST_F(BasicElementTest, TestRelativeWidth) {
  view_->SetSize(400, 300);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelHeight(100);
  m->SetRelativeWidth(0.50);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, m->GetRelativeWidth());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelWidth());
  BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeWidth(0.50);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeWidth());
  ASSERT_DOUBLE_EQ(100.0, c->GetPixelWidth());
  // Setting the width as negative value will make no effect.
  c->SetRelativeWidth(-0.50);
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeWidth());
  ASSERT_DOUBLE_EQ(100.0, c->GetPixelWidth());
  // Modifying the width of the parent will effect the child.
  m->SetRelativeWidth(1.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.50, c->GetRelativeWidth());
  ASSERT_DOUBLE_EQ(200.0, c->GetPixelWidth());
  // Modifying the width of the parent will effect the child.
  m->SetPixelWidth(150.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(75.0, c->GetPixelWidth());
}

TEST_F(BasicElementTest, TestVisible) {
  MuffinElement m(view_, NULL);
  ASSERT_TRUE(m.IsVisible());
  m.SetVisible(false);
  ASSERT_FALSE(m.IsVisible());
}

TEST_F(BasicElementTest, TestPixelX) {
  view_->SetSize(400, 300);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  ASSERT_DOUBLE_EQ(0.0, m->GetPixelX());
  m->SetPixelWidth(100.0);
  m->SetPixelHeight(100.0);
  m->SetPixelX(100.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelX());
  BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetPixelX(50.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  // Modifying the width of the parent will not effect the child.
  m->SetPixelWidth(150.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelX());
  m->SetPixelX(-50.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-50.5, m->GetPixelX());
}

TEST_F(BasicElementTest, TestRelativeX) {
  view_->SetSize(400, 300);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(100.0);
  m->SetPixelHeight(100.0);
  m->SetRelativeWidth(0.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  m->SetRelativeX(0.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelX());
  BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeX(0.50);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, c->GetPixelX());
  // Modifying the width of the parent will effect the child.
  m->SetPixelWidth(100.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelX());
  m->SetRelativeX(-0.25);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-100.0, m->GetPixelX());
}

TEST_F(BasicElementTest, TestPixelY) {
  view_->SetSize(400, 300);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  ASSERT_DOUBLE_EQ(0.0, m->GetPixelY());
  m->SetPixelWidth(100.0);
  m->SetPixelHeight(100.0);
  m->SetPixelY(100.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelY());
  BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetPixelY(50.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  // Modifying the height of the parent will not effect the child.
  m->SetPixelHeight(150.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(50.0, c->GetPixelY());
  m->SetPixelY(-150.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-150.5, m->GetPixelY());
}

TEST_F(BasicElementTest, TestRelativeY) {
  view_->SetSize(400, 300);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(100.0);
  m->SetRelativeHeight(0.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  m->SetRelativeY(0.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(150.0, m->GetPixelY());
  BasicElement *c = m->GetChildren()->AppendElement("pie", NULL);
  c->SetRelativeY(0.50);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(75.0, c->GetPixelY());
  // Modifying the height of the parent will effect the child.
  m->SetPixelHeight(150.0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(75.0, c->GetPixelY());
  m->SetRelativeY(-0.125);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(-37.5, m->GetPixelY());
}

// This test is not merely for BasicElement, but mixed test for xml_utils
// and Elements.
TEST_F(BasicElementTest, TestFromXML) {
  MuffinElement m(view_, NULL);
  Elements *children = down_cast<Elements *>(m.GetChildren());
  BasicElement *e1 = children->InsertElementFromXML("<muffin/>", NULL);
  BasicElement *e2 = children->InsertElementFromXML("<pie/>", e1);
  BasicElement *e3 = children->InsertElementFromXML(
      "<pie name=\"a-pie\"/>", e2);
  BasicElement *e4 = children->AppendElementFromXML("<bread/>");
  BasicElement *e5 = children->InsertElementFromXML("<bread/>", e2);
  BasicElement *e6 = children->AppendElementFromXML("<pie name=\"big-pie\"/>");
  ASSERT_EQ(4U, children->GetCount());
  ASSERT_TRUE(e1 == children->GetItemByIndex(2));
  ASSERT_EQ(2U, e1->GetIndex());
  ASSERT_STREQ("muffin", e1->GetTagName());
  ASSERT_STREQ("", e1->GetName().c_str());
  ASSERT_TRUE(e2 == children->GetItemByIndex(1));
  ASSERT_EQ(1U, e2->GetIndex());
  ASSERT_STREQ("pie", e2->GetTagName());
  ASSERT_STREQ("", e2->GetName().c_str());
  ASSERT_TRUE(e3 == children->GetItemByIndex(0));
  ASSERT_EQ(0U, e3->GetIndex());
  ASSERT_TRUE(e3 == children->GetItemByName("a-pie"));
  ASSERT_STREQ("pie", e3->GetTagName());
  ASSERT_STREQ("a-pie", e3->GetName().c_str());
  ASSERT_TRUE(NULL == e4);
  ASSERT_TRUE(NULL == e5);
  ASSERT_TRUE(e6 == children->GetItemByIndex(3));
  ASSERT_EQ(3U, e6->GetIndex());
  ASSERT_TRUE(e6 == children->GetItemByName("big-pie"));
  ASSERT_STREQ("pie", e6->GetTagName());
  ASSERT_STREQ("big-pie", e6->GetName().c_str());
}

// This test is not merely for BasicElement, but mixed test for xml_utils
// and Elements.
TEST_F(BasicElementTest, XMLConstruction) {
  MuffinElement m(view_, NULL);

  const char *xml =
    "<muffin n1=\"yy\" name=\"top\">\n"
    "  <pie tooltip=\"pie-tooltip\" x=\"50%\" y=\"100\">\n"
    "    <muffin tagName=\"haha\" name=\"muffin\"/>\n"
    "  </pie>\n"
    "  <pie name=\"pie1\"/>\n"
    "</muffin>\n";
  m.GetChildren()->InsertElementFromXML(xml, NULL);
  ASSERT_EQ(1U, m.GetChildren()->GetCount());
  BasicElement *e1 = m.GetChildren()->GetItemByIndex(0);
  ASSERT_TRUE(e1);
  ASSERT_EQ(0U, e1->GetIndex());
  ASSERT_TRUE(e1->IsInstanceOf(MuffinElement::CLASS_ID));
  ASSERT_FALSE(e1->IsInstanceOf(PieElement::CLASS_ID));
  ASSERT_TRUE(e1->IsInstanceOf(BasicElement::CLASS_ID));
  MuffinElement *m1 = down_cast<MuffinElement *>(e1);
  ASSERT_STREQ("top", m1->GetName().c_str());
  ASSERT_STREQ("muffin", m1->GetTagName());
  ASSERT_EQ(2U, m1->GetChildren()->GetCount());
  BasicElement *e2 = m1->GetChildren()->GetItemByIndex(0);
  ASSERT_EQ(0U, e2->GetIndex());
  ASSERT_TRUE(e2);
  ASSERT_TRUE(e2->IsInstanceOf(PieElement::CLASS_ID));
  ASSERT_FALSE(e2->IsInstanceOf(MuffinElement::CLASS_ID));
  ASSERT_TRUE(e2->IsInstanceOf(BasicElement::CLASS_ID));
  PieElement *p1 = down_cast<PieElement *>(e2);
  ASSERT_STREQ("", p1->GetName().c_str());
  ASSERT_STREQ("pie", p1->GetTagName());
  ASSERT_STREQ("pie-tooltip", p1->GetTooltip().c_str());
  ASSERT_TRUE(p1->XIsRelative());
  ASSERT_DOUBLE_EQ(0.5, p1->GetRelativeX());
  ASSERT_FALSE(p1->YIsRelative());
  ASSERT_DOUBLE_EQ(100, p1->GetPixelY());
  ASSERT_EQ(1U, p1->GetChildren()->GetCount());
}

TEST_F(BasicElementTest, TestMinWidth) {
  view_->SetSize(400, 400);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelHeight(100);

  m->SetPixelWidth(100);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelWidth());

  m->SetMinWidth(150);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(150.0, m->GetPixelWidth());

  m->SetPixelWidth(200);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelWidth());

  m->SetMinWidth(0);

  m->SetPixelWidth(0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.0, m->GetPixelWidth());

  m->SetRelativeWidth(0.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelWidth());

  m->SetMinWidth(100);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelWidth());

  view_->SetSize(100, 400);
  ASSERT_TRUE(view_host_->GetQueueResize());
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelWidth());

  view_->SetSize(300, 400);
  ASSERT_TRUE(view_host_->GetQueueResize());
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(150.0, m->GetPixelWidth());
}

TEST_F(BasicElementTest, TestMinHeight) {
  view_->SetSize(400, 400);
  BasicElement *m = view_->GetChildren()->AppendElement("muffin", NULL);
  m->SetPixelWidth(100);

  m->SetPixelHeight(100);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelHeight());

  m->SetMinHeight(150);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(150.0, m->GetPixelHeight());

  m->SetPixelHeight(200);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelHeight());

  m->SetMinHeight(0);

  m->SetPixelHeight(0);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(0.0, m->GetPixelHeight());

  m->SetRelativeHeight(0.5);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelHeight());

  m->SetMinHeight(100);
  ASSERT_TRUE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(200.0, m->GetPixelHeight());

  view_->SetSize(400, 100);
  ASSERT_TRUE(view_host_->GetQueueResize());
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(100.0, m->GetPixelHeight());

  view_->SetSize(400, 300);
  ASSERT_TRUE(view_host_->GetQueueResize());
  ASSERT_FALSE(view_host_->GetQueuedDraw());
  ASSERT_DOUBLE_EQ(150.0, m->GetPixelHeight());
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
