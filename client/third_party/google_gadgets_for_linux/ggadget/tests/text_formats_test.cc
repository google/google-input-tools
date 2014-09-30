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
#include "ggadget/text_formats.h"
#ifdef OS_WINDOWS
#include "ggadget/win32/xml_parser.h"
#else
#include "init_extensions.h"
#endif
#include "unittest/gtest.h"

using namespace ggadget;

TEST(TextFormats, SetAndGet) {
  TextFormat format;
  format.set_size(1.0);
  EXPECT_TRUE(format.has_size());
  EXPECT_EQ(1.0, format.size());
  format.set_font("font");
  EXPECT_TRUE(format.has_font());
  EXPECT_EQ("font", format.font());
  format.set_foreground(Color(1.0, 0.5, 0.1));
  EXPECT_TRUE(format.has_foreground());
  EXPECT_EQ(1.0, format.foreground().red);
  EXPECT_EQ(0.5, format.foreground().green);
  EXPECT_EQ(0.1, format.foreground().blue);
}

TEST(TextFormats, SetFormat) {
  static const FormatEntry entrys[] = {
    { TextFormat::kFontName, Variant("font") },
    { TextFormat::kSizeName, Variant(1.0) },
    { TextFormat::kForegroundName, Variant("#FF00FF") },
  };

  TextFormat format;
  format.SetFormat(entrys, arraysize(entrys));
  EXPECT_TRUE(format.has_font());
  EXPECT_TRUE(format.has_size());
  EXPECT_TRUE(format.has_foreground());
  EXPECT_EQ("font", format.font());
  EXPECT_EQ(1.0, format.size());
  EXPECT_EQ("#FF00FF", format.foreground().ToString());
  EXPECT_FALSE(format.has_background());
}

TEST(TextFormats, MergeFormat) {
  static const FormatEntry entrys[] = {
    { TextFormat::kFontName, Variant("font") },
    { TextFormat::kSizeName, Variant(1.0) },
    { TextFormat::kForegroundName, Variant("#FF00FF") },
  };

  static const FormatEntry new_entrys[] = {
    { TextFormat::kFontName, Variant("font") },
    { TextFormat::kScaleName, Variant(3.0) },
  };
  TextFormat old_format, format, new_format;
  format.SetFormat(entrys, arraysize(entrys));
  old_format = format;
  new_format.SetFormat(new_entrys, arraysize(new_entrys));
  format.MergeFormat(new_format);
  EXPECT_TRUE(format.has_font());
  EXPECT_TRUE(format.has_size());
  EXPECT_TRUE(format.has_scale());
  EXPECT_TRUE(format.has_foreground());
  EXPECT_EQ(new_format.font(), format.font());
  EXPECT_EQ(new_format.scale(), format.scale());
  EXPECT_EQ(old_format.size(), format.size());
  EXPECT_EQ(old_format.foreground().ToString(), format.foreground().ToString());
}

TEST(TextFormats, MergeFormatIfNothave) {
  static const FormatEntry entrys[] = {
    { TextFormat::kFontName, Variant("font") },
    { TextFormat::kSizeName, Variant(1.0) },
    { TextFormat::kForegroundName, Variant("#FF00FF") },
  };

  static const FormatEntry new_entrys[] = {
    { TextFormat::kFontName, Variant("font") },
    { TextFormat::kScaleName, Variant(3.0) },
  };
  TextFormat old_format, format, new_format;
  format.SetFormat(entrys, arraysize(entrys));
  old_format = format;
  new_format.SetFormat(new_entrys, arraysize(new_entrys));
  format.MergeIfNotHave(new_format);
  EXPECT_TRUE(format.has_font());
  EXPECT_TRUE(format.has_size());
  EXPECT_TRUE(format.has_scale());
  EXPECT_TRUE(format.has_foreground());
  EXPECT_EQ(old_format.font(), format.font());
  EXPECT_EQ(new_format.scale(), format.scale());
  EXPECT_EQ(old_format.size(), format.size());
  EXPECT_EQ(old_format.foreground().ToString(), format.foreground().ToString());
}

TEST(TextFormats, ParseMarkUpText) {
  std::string mark_up_text =
      "a"
      "<font size='1.0'>"
        " b"
        "<font face='font'>"
          "c "
          "<b>"
            "<sub>"
              "d  "
            "</sub>"
            "e"
              "<font color='#FF00FF'>"
                "   f"
              "</font>"
              "<del></del>"
          "</b>"
        "</font>"
        "<i>"
          "g"
        "</i>"
      "</font>";
  std::string text;
  TextFormats formats;
  ParseMarkUpText(mark_up_text, NULL, &text, &formats);
  EXPECT_EQ("a bc d  e   fg", text);
  EXPECT_EQ(6L, formats.size());
  EXPECT_EQ(1, formats[0].range.start);
  EXPECT_EQ(14, formats[0].range.end);
  EXPECT_TRUE(formats[0].format.has_size());
  EXPECT_EQ(1.0, formats[0].format.size());

  EXPECT_EQ(3, formats[1].range.start);
  EXPECT_EQ(13, formats[1].range.end);
  EXPECT_TRUE(formats[1].format.has_font());
  EXPECT_EQ("font", formats[1].format.font());

  EXPECT_EQ(5, formats[2].range.start);
  EXPECT_EQ(13, formats[2].range.end);
  EXPECT_TRUE(formats[2].format.has_bold());
  EXPECT_TRUE(formats[2].format.bold());

  EXPECT_EQ(5, formats[3].range.start);
  EXPECT_EQ(8, formats[3].range.end);
  EXPECT_TRUE(formats[3].format.has_script_type());
  EXPECT_EQ(SUBSCRIPT, formats[3].format.script_type());

  EXPECT_EQ(9, formats[4].range.start);
  EXPECT_EQ(13, formats[4].range.end);
  EXPECT_TRUE(formats[4].format.has_foreground());
  EXPECT_EQ("#FF00FF", formats[4].format.foreground().ToString());

  EXPECT_EQ(13, formats[5].range.start);
  EXPECT_EQ(14, formats[5].range.end);
  EXPECT_TRUE(formats[5].format.has_italic());
  EXPECT_TRUE(formats[5].format.italic());
}

int main(int argc, char **argv) {
#ifdef OS_WINDOWS
  ggadget::win32::XMLParser* xml_parser = new ggadget::win32::XMLParser;
  if (!xml_parser) return -1;
  ggadget::SetXMLParser(xml_parser);
#else
  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);
#endif
  testing::ParseGTestFlags(&argc, argv);
  return RUN_ALL_TESTS();
}
