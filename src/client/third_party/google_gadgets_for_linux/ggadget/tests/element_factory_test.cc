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

#include "unittest/gtest.h"
#include "ggadget/element_factory.h"
#include "mocked_element.h"
#include "mocked_timer_main_loop.h"
#include "mocked_view_host.h"

MockedTimerMainLoop main_loop(0);

class ElementFactoryTest : public testing::Test {
 protected:
  virtual void SetUp() {
  }

  virtual void TearDown() {
  }
};

TEST_F(ElementFactoryTest, TestRegister) {
  ggadget::ElementFactory factory;
  ASSERT_TRUE(factory.RegisterElementClass(
      "muffin",
      MuffinElement::CreateInstance));
  ASSERT_FALSE(factory.RegisterElementClass(
      "muffin",
      MuffinElement::CreateInstance));
  ASSERT_TRUE(factory.RegisterElementClass(
      "pie",
      PieElement::CreateInstance));
  ASSERT_FALSE(factory.RegisterElementClass(
      "pie",
      PieElement::CreateInstance));
}

TEST_F(ElementFactoryTest, TestCreate) {
  ggadget::ElementFactory factory;
  ggadget::View view(
      new MockedViewHost(ggadget::ViewHostInterface::VIEW_HOST_MAIN),
      NULL, &factory, NULL);
  factory.RegisterElementClass("muffin", MuffinElement::CreateInstance);
  factory.RegisterElementClass("pie", PieElement::CreateInstance);

  ggadget::BasicElement *e1 = factory.CreateElement("muffin", &view, NULL);
  ASSERT_TRUE(e1 != NULL);
  ASSERT_STREQ("muffin", e1->GetTagName());

  ggadget::BasicElement *e2 = factory.CreateElement("pie", &view, NULL);
  ASSERT_TRUE(e2 != NULL);
  ASSERT_STREQ("pie", e2->GetTagName());

  ggadget::BasicElement *e3 = factory.CreateElement("bread", &view, NULL);
  ASSERT_TRUE(e3 == NULL);
  delete ggadget::down_cast<MuffinElement *>(e1);
  delete ggadget::down_cast<PieElement *>(e2);
}

int main(int argc, char *argv[]) {
  ggadget::SetGlobalMainLoop(&main_loop);

  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
