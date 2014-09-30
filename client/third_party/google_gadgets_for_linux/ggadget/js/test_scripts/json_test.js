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

TEST("Test JSONEncode|JSONDecode null", function() {
  ASSERT(EQ("null", jsonEncode(null)));
  ASSERT(EQ("null", jsonEncode(function() { })));
  ASSERT(EQ("null", jsonEncode(undefined)));
  ASSERT(STRICT_EQ(null, jsonDecode('null')));
  ASSERT(UNDEFINED(jsonDecode('')));
});

TEST("Test JSONEncode|JSONDecode boolean", function() {
  ASSERT(EQ("true", jsonEncode(true)));
  ASSERT(EQ("false", jsonEncode(false)));
  ASSERT(STRICT_EQ(true, jsonDecode('true')));
  ASSERT(STRICT_EQ(false, jsonDecode('false')));
});

TEST("Test JSONEncode|JSONDecode number", function() {
  ASSERT(EQ("0", jsonEncode(0)));
  ASSERT(EQ("0", jsonEncode(Number.POSITIVE_INFINITY)));
  ASSERT(EQ("0", jsonEncode(Number.NEGATIVE_INFINITY)));
  ASSERT(EQ("0", jsonEncode(Number.NaN)));
  ASSERT(EQ("12345", jsonEncode(12345)));
  ASSERT(EQ("123456.25", jsonEncode(123456.25)));
  ASSERT(EQ("-123456.25", jsonEncode(-123456.25)));
  ASSERT(STRICT_EQ(0, jsonDecode('0')));
  ASSERT(EQ(12345, jsonDecode('12345')));
  ASSERT(EQ(123456.25, jsonDecode('123456.25')));
  ASSERT(EQ(-123456.25, jsonDecode('-123456.25')));
  ASSERT(EQ(-1.23456E-20, jsonDecode('-1.23456E-20')));
});

TEST("Test JSONEncode|JSONDecode string", function() {
  ASSERT(EQ('""', jsonEncode('')));
  ASSERT(EQ('"\\\"\\r\\n\\u0008"', jsonEncode('\"\r\n\b')));
  ASSERT(EQ('"\\u4E2D\\u56FD\\u4E2D\\u56FDUnicode"',
            jsonEncode('\u4E2D\u56FD中国Unicode')));
  ASSERT(STRICT_EQ("", jsonDecode('""')));
  ASSERT(EQ('""\r\n\b', jsonDecode('"\\\"\\x22\\r\\n\\u0008"')));
  ASSERT(EQ('\u4E2D\u56FD中国Unicode',
            jsonDecode('"\\u4E2D\\u56FD\\u4E2D\\u56FDUnicode"')));
});

TEST("Test JSONEncode|JSONDecode date", function() {
  var d = new Date(1234567);
  ASSERT(EQ('"\\/Date(1234567)\\/"', jsonEncode(d)));
  var d1 = jsonDecode(jsonEncode(d));
  print(d1);
  ASSERT(EQ(d.getTime(), d1.getTime()));
  var obj = { date1: new Date(234567), date2: new Date(987654) };
  ASSERT(EQ('{"date1":"\\/Date(234567)\\/","date2":"\\/Date(987654)\\/"}',
         jsonEncode(obj)));
  var obj1 = jsonDecode(jsonEncode(obj));
  ASSERT(EQ(234567, obj1.date1.getTime()));
  ASSERT(EQ(987654, obj1.date2.getTime()));
});

TEST("Test JSONEncode complex", function() {
  ASSERT(EQ('[]', jsonEncode([])));
  ASSERT(EQ('{}', jsonEncode({})));
  ASSERT(EQ('[{},[]]', jsonEncode([{},[]])));
  ASSERT(EQ('[1,[2,[3,[4,5],6]]]', jsonEncode([1,[2,[3,[4,5],6]]])));
  ASSERT(EQ('{"a":10,"b":{},"c":"string","d":true,"e":null}',
            jsonEncode({a:10,b:{},c:"string",d:true,e:null})));
  ASSERT(OBJECT_STRICT_EQ({}, jsonDecode('{}')));
  ASSERT(ARRAY_STRICT_EQ([], jsonDecode('[]')));
  ASSERT(EQ('[1,[2,[3,[4,5],6]]]',
            jsonEncode(jsonDecode('[1,[2,[3,[4,5],6]]]'))));
  ASSERT(EQ('{"a":10,"b":{},"c":"string","d":true,"e":null}',
            jsonEncode(jsonDecode(
                '{"a":10,"b":{},"c":"string","d":true,"e":null}'))));
});

DEATH_TEST("Test JSONEncode invalid1", function() {
  jsonDecode('{a:10}');
  ASSERT(DEATH());
});

DEATH_TEST("Test JSONEncode invalid2", function() {
  jsonDecode('function() { }');
  ASSERT(DEATH());
});

DEATH_TEST("Test JSONEncode invalid3", function() {
  jsonDecode('{ print(1); }');
  ASSERT(DEATH());
});

DEATH_TEST("Test JSONEncode invalid4", function() {
  jsonDecode('{ print(1); }');
  ASSERT(DEATH());
});

DEATH_TEST("Test JSONEncode invalid5", function() {
  jsonDecode('print(1)');
  ASSERT(DEATH());
});

DEATH_TEST("Test JSONEncode invalid6", function() {
  jsonDecode('new Date()');
  ASSERT(DEATH());
});

DEATH_TEST("Test JSONEncode invalid7", function() {
  jsonDecode('"\\/Date(abcde)\\/"');
  ASSERT(DEATH());
});

RUN_ALL_TESTS();
