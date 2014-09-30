/*
  Copyright 2012 Google Inc.

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

// This header is meant to be included in multiple passes, hence no traditional
// header guard.

// NOTICE: Do NOT include this file directly if you want to use text_formats,
//         include text_formats.h

// You can declare a attribute of text format use macro:
// DECLARE_FORMAT(id, type, name, capital_name, default_value)
// the parameters are:
//   id: the unique id of the attribute, used internally. id should be positive
//       and less than 32.
//   type: the datatype of the attribute
//   name: the name of the attribute, the attribute can be access using method:
//         [name](), set_[name](), has_[name]().
//   capital_name: the capital name of the attribute, used to define a const
//         variable k[capital_name] to indicate the attribute.
//   default_value: the default value of the attribute.
DECLARE_FORMAT(0, string, font, Font, kDefaultFontName)
DECLARE_FORMAT(1, double, size, Size, kDefaultFontSize)
DECLARE_FORMAT(2, double, scale, Scale, 1)
DECLARE_FORMAT(3, double, rise, Rise, 0)
DECLARE_FORMAT(4, bool, bold, Bold, false)
DECLARE_FORMAT(5, bool, italic, Italic, false)
DECLARE_FORMAT(6, bool, underline, Underline, false)
DECLARE_FORMAT(7, bool, strikeout, Strikeout, false)
DECLARE_FORMAT(8, ScriptType, script_type, ScriptType, NORMAL)
DECLARE_FORMAT(9, Color, foreground, Foreground, Color(0, 0, 0))
DECLARE_FORMAT(10, Color, background, Background, Color(1, 1, 1))
DECLARE_FORMAT(11, Color, underline_color, UnderlineColor, Color(0, 0, 0))
DECLARE_FORMAT(12, Color, strikeout_color, StrikeoutColor, Color(0, 0, 0))
DECLARE_FORMAT(13, bool, text_rtl, TextRTL, false)
