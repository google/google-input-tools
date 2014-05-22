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

function TestScriptableBasics(scriptable) {
  scriptable.Buffer = "TestBuffer";
  ASSERT(EQ("Buffer:TestBuffer", scriptable.Buffer));
  scriptable.Buffer = "TestAgainBuffer";
  ASSERT(EQ("Buffer:TestAgainBuffer", scriptable.Buffer));
  scriptable.Buffer = true;
  ASSERT(EQ("Buffer:true", scriptable.Buffer));
  scriptable.Buffer = 123.45;
  ASSERT(EQ("Buffer:123.45", scriptable.Buffer));
  scriptable.Buffer = NaN;
  ASSERT(EQ("Buffer:NaN", scriptable.Buffer));
  scriptable.Buffer = Number.POSITIVE_INFINITY;
  ASSERT(EQ("Buffer:Infinity", scriptable.Buffer));
  scriptable.Buffer = Number.NEGATIVE_INFINITY;
  ASSERT(EQ("Buffer:-Infinity", scriptable.Buffer));
  scriptable.ClearBuffer();  // It will clear the buffer.
  ASSERT(STRICT_EQ("", scriptable.Buffer));

  ASSERT(STRICT_EQ(0, scriptable.DoubleProperty));
  ASSERT(EQ("GetDoubleProperty()=0.000\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  scriptable.DoubleProperty = 3.25;
  ASSERT(EQ("SetDoubleProperty(3.250)\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  ASSERT(STRICT_EQ(3.25, scriptable.DoubleProperty));
  ASSERT(EQ("GetDoubleProperty()=3.250\n", scriptable.Buffer));
  scriptable.ClearBuffer();

  ASSERT(STRICT_EQ(0, scriptable.IntProperty));
  ASSERT(EQ("GetIntProperty()=0\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  scriptable.IntProperty = null;
  ASSERT(EQ("SetIntProperty(0)\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  ASSERT(STRICT_EQ(0, scriptable.IntProperty));
  ASSERT(EQ("GetIntProperty()=0\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  scriptable.IntProperty = undefined;
  ASSERT(EQ("SetIntProperty(0)\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  ASSERT(STRICT_EQ(0, scriptable.IntProperty));
  ASSERT(EQ("GetIntProperty()=0\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  scriptable.IntProperty = 1.6;
  ASSERT(EQ("SetIntProperty(2)\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  ASSERT(STRICT_EQ(2, scriptable.IntProperty));
  ASSERT(EQ("GetIntProperty()=2\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  scriptable.IntProperty = 1.2;
  ASSERT(EQ("SetIntProperty(1)\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  ASSERT(STRICT_EQ(1, scriptable.IntProperty));
  ASSERT(EQ("GetIntProperty()=1\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  scriptable.IntProperty = 100000000000;
  ASSERT(EQ("SetIntProperty(100000000000)\n", scriptable.Buffer));
  scriptable.ClearBuffer();
  ASSERT(STRICT_EQ(100000000000, scriptable.IntProperty));
  ASSERT(EQ("GetIntProperty()=100000000000\n", scriptable.Buffer));
  scriptable.ClearBuffer();

  ASSERT(STRICT_EQ(scriptable.VALUE_0, scriptable.EnumSimple));
  ASSERT(EQ(1, scriptable.VALUE_1));
  scriptable.EnumSimple = scriptable.VALUE_1;
  ASSERT(EQ(scriptable.VALUE_1, scriptable.EnumSimple));
  scriptable.EnumSimple = "100.2";
  ASSERT(EQ(100, scriptable.EnumSimple));
  scriptable.EnumSimple = "100.5";
  ASSERT(EQ(101, scriptable.EnumSimple));

  ASSERT(EQ(123456789, scriptable.Fixed));

  ASSERT(STRICT_EQ(0, scriptable.VariantProperty));
  scriptable.VariantProperty = 1234;
  ASSERT(EQ(1234, scriptable.VariantProperty));
  scriptable.VariantProperty = "100.2";
  ASSERT(EQ("100.2", scriptable.VariantProperty));
  scriptable.VariantProperty = "80%";
  ASSERT(EQ("80%", scriptable.VariantProperty));

  ASSERT(UNDEFINED(scriptable.NotDefinedProperty));
}

TEST("Test global method", function() {
  var a = globalMethod();
  ASSERT(EQ("hello world", a));
});

TEST("Test property & method basics", function() {
  // "scriptable" is registered as a global property in wrapper_test_shell.cc.
  TestScriptableBasics(scriptable);
});

DEATH_TEST("Test readonly property", function() {
  // Buffer and BufferReadOnly are backed with the same single C++ object.
  scriptable.Buffer = "Buffer";
  // Assignment to a readonly property has no effect.
  scriptable.BufferReadOnly = "TestBuffer";
  ASSERT(DEATH());
});

DEATH_TEST("Test integer property with non-number", function() {
  // The following assignment should cause an error.
  scriptable.EnumSimple = "80%";
  ASSERT(DEATH());
});

TEST("Test string property with an array", function() {
  scriptable.Buffer = ["string"];
  ASSERT(EQ("Buffer:string", scriptable.Buffer));
  scriptable.Buffer = [[[1],2]];
  ASSERT(EQ("Buffer:1,2", scriptable.Buffer));
});

TEST("Test constants", function() {
  ASSERT(EQ(123456789, scriptable.Fixed));
  for (var i = 0; i < 10; i++) {
    ASSERT(STRICT_EQ(i, scriptable["ICONSTANT" + i]));
    ASSERT(EQ("SCONSTANT" + i, scriptable["SCONSTANT" + i]));
  }
});

TEST("Test constant assignment", function() {
  // Assignment to constant has no effect.
  scriptable.Fixed = 100;
  ASSERT(EQ(123456789, scriptable.Fixed));
});

TEST("Test inheritance & prototype", function() {
  ASSERT(NE(scriptable, scriptable2));
  TestScriptableBasics(scriptable2);
  // Const is defined in scriptable2's prototype.
  ASSERT(EQ(987654321, scriptable2.Const));
});

DEATH_TEST("Test strict", function() {
  scriptable.NotDefinedProperty = "TestValue";
  ASSERT(DEATH());
});

TEST("Test not strict", function() {
  scriptable2.NotDefinedProperty = "TestValue";
  ASSERT(EQ("TestValue", scriptable2.NotDefinedProperty));
});

TEST("Test array", function() {
  ASSERT(EQ(20, scriptable2.length));
  for (var i = 0; i < scriptable2.length; i++)
    scriptable2[i] = i * 2;
  for (i = 0; i < scriptable2.length; i++) {
    ASSERT(STRICT_EQ(10000 + i * 2, scriptable2[i]));
    ASSERT(STRICT_EQ(10000 + i * 2, scriptable2["" + i]));
  }
  ASSERT(UNDEFINED(scriptable2[scriptable2.length]));
  ASSERT(UNDEFINED(scriptable2[scriptable2.length + 1]));
});

TEST("Test signals", function() {
  // Defined in scriptable1.
  ASSERT(NULL(scriptable2.ondelete));
  // Defined in prototype.
  ASSERT(NULL(scriptable2.ontest));
  ASSERT(NULL(scriptable2.onlunch));
  ASSERT(NULL(scriptable2.onsupper));

  // Test empty slot
  scriptable2.onlunch = '';
  ASSERT(NE(null, scriptable2.onlunch));
  ASSERT(NE(undefined, scriptable2.onlunch));
  scriptable2.onlunch = null;
  ASSERT(NULL(scriptable2.onlunch));

  var scriptable2_in_closure = scriptable2;

  var onlunch_triggered = false;
  var onsupper_triggered = false;
  onlunch_function = function(s) {
    ASSERT(EQ(scriptable2, scriptable2_in_closure));
    ASSERT(EQ("Have lunch", s));
    // A long lunch taking a whole afternoon :).
    scriptable2.time = "supper";   // Will cause recursive onsupper.
    ASSERT(TRUE(onsupper_triggered));
    ASSERT(EQ("Supper finished", scriptable2.SignalResult));
    // Lunch is finished after finishing supper :).
    onlunch_triggered = true;
    return "Lunch finished";
  };

  var supper_is_lunch = false;
  onsupper_function = function(s, s_param) {
    ASSERT(EQ(scriptable2, scriptable2_in_closure));
    if (supper_is_lunch) {
      ASSERT(EQ("Have lunch", s));
    } else {
      ASSERT(EQ(scriptable2, s_param));
      ASSERT(EQ("Have supper", s));
    }
    onsupper_triggered = true;
    return "Supper finished";
  };

  scriptable2.onlunch = onlunch_function;
  ASSERT(EQ(onlunch_function, scriptable2.onlunch));
  scriptable2.onsupper = onsupper_function;
  ASSERT(EQ(onsupper_function, scriptable2.onsupper));

  // Trigger onlunch.
  scriptable2.time = "lunch";
  ASSERT(EQ("Lunch finished", scriptable2.SignalResult));
  ASSERT(TRUE(onsupper_triggered));
  ASSERT(TRUE(onlunch_triggered));

  onlunch_triggered = false;
  onsupper_triggered = false;
  // Now we have supper when the others are having lunch.
  supper_is_lunch = true;
  scriptable2.onlunch = scriptable2.onsupper;
  ASSERT(TRUE(scriptable2.onlunch));
  ASSERT(EQ(scriptable2.onlunch, scriptable2.onsupper));
  scriptable2.time = "lunch";
  ASSERT(EQ("Supper finished", scriptable2.SignalResult));
  ASSERT(TRUE(onsupper_triggered));
  ASSERT(FALSE(onlunch_triggered));

  // Test disconnect.
  scriptable2.onlunch = null;
  onlunch_triggered = false;
  onsupper_triggered = false;
  scriptable2.time = "lunch";
  ASSERT(FALSE(onsupper_triggered));
  ASSERT(FALSE(onlunch_triggered));
  ASSERT(NULL(scriptable2.onlunch));
});

TEST("Test dynamic properties", function() {
  for (var i = 0; i < 10; i++)
    scriptable2["d" + i] = "v" + i;
  for (i = 0; i < 10; i++)
    ASSERT(EQ("Value:v" + i, scriptable2["d" + i]));

  var slot_called = false;
  var slot = function() { slot_called = true; };
  scriptable2["s"] = slot;
  scriptable2.FireDynamicSignal();
  ASSERT(TRUE(slot_called));

  /* following test is only supported by webkit_script_runtime.
  ASSERT(NULL(scriptable2["s"]));
  scriptable2["s"] = slot;
  ASSERT(EQ(slot, scriptable2["s"]));
  scriptable2["s"] = null;
  ASSERT(NULL(scriptable2["s"]));
  */
});

TEST("Test scriptables", function() {
  // Self is defined in prototype, a different object from scriptable2.
  ASSERT(EQ(scriptable2.PrototypeSelf, scriptable2.PrototypeSelf));
  // Test prototype object itself.
  var proto = scriptable2.PrototypeSelf;
  ASSERT(EQ(proto, proto.PrototypeSelf));

  ASSERT(NULL(proto.PrototypeMethod(null)));
  ASSERT(EQ(scriptable, proto.PrototypeMethod(scriptable)));
  ASSERT(EQ(proto, proto.PrototypeMethod(proto)));
  ASSERT(EQ(scriptable2, proto.PrototypeMethod(scriptable2)));

  ASSERT(EQ(scriptable2, scriptable2.OverrideSelf));
  ASSERT(NULL(scriptable2.ObjectMethod(null)));
  // Incompatible type.
  ASSERT(NULL(scriptable2.ObjectMethod(scriptable)));
});

TEST("Test default method", function() {
  // The default method is registered as ObjectMethod().
  ASSERT(NULL(scriptable2(null)));
  ASSERT(EQ(scriptable2, scriptable2(scriptable2)));
});

TEST("Test dynamic allocated objects", function() {
  var a = scriptable2.NewObject(true);
  var a_deleted = false;
  a.my_ondelete = function() {
    a_deleted = true;
  };
  var b = scriptable2.NewObject(true);
  var b_deleted = false;
  b.my_ondelete = function() {
    b_deleted = true;
  }
  ASSERT(NOT_NULL(a));
  ASSERT(NOT_NULL(b));
  ASSERT(NE(a, b));
  ASSERT(NE(scriptable2, a));
  ASSERT(NE(scriptable2, b));
  TestScriptableBasics(a);
  TestScriptableBasics(b);
  scriptable2.ReleaseObject(a);
  ASSERT(TRUE(a_deleted));
  ASSERT(FALSE(b_deleted));
  scriptable2.ReleaseObject(b);
  ASSERT(TRUE(b_deleted));
});

DEATH_TEST("Test access after deleted", function() {
  var a = scriptable2.NewObject(true);
  scriptable2.ReleaseObject(a);
  a.Buffer = "Buffer";
  print("*************" + a.Buffer);
  ASSERT(DEATH());
});

TEST("Test string callback", function() {
  scriptable2.onlunch = "msg = 'lunch_callback'; return msg;";
  scriptable2.time = "lunch";
  ASSERT(EQ("lunch_callback", msg));
  ASSERT(EQ(msg, scriptable2.SignalResult));
});

TEST("Test ownership policies", function() {
  gc();
  a = scriptable2.NewObject(false);  // without initial ref.
  scriptable.ClearBuffer();
  a = null;
  gc();
  ASSERT(EQ("Destruct\n", scriptable.Buffer));

  var b = scriptable2.NewObject(false);
  b = scriptable2.ObjectMethod(b);
  b = null;
  scriptable.ClearBuffer();
  gc();
  ASSERT(EQ("Destruct\n", scriptable.Buffer));

  a = scriptable2.NewObject(true);
  scriptable.ClearBuffer();
  // Pass the ownership to the native side, and at the same time, the native
  // side deletes it.
  scriptable2.ReleaseObject(a);
  a = null;
  gc();  // Should not cause errors.
  ASSERT(EQ("Destruct\n", scriptable.Buffer));
});

TEST("Test UTF8 strings", function() {
  scriptable.Buffer = "\u4E2D\u56FDUnicode";
  // 16 is the length of "Buffer:中国Unicode".
  ASSERT(EQ(16, scriptable.Buffer.length));
  // Although we support UTF8 chars in JavaScript source code, we disencourage
  // it.  We should Use unicode escape sequences or external string/ resources.
  ASSERT(EQ("中", scriptable.Buffer[7]));
  ASSERT(EQ("国", scriptable.Buffer[8]));
  ASSERT(EQ("e", scriptable.Buffer[15]));
  ASSERT(EQ("Buffer:中国Unicode", scriptable.Buffer));
  ASSERT(EQ(0x4E2D, scriptable.Buffer.charCodeAt(7)));
  ASSERT(EQ(0x56FD, scriptable.Buffer.charCodeAt(8)));
});

TEST("Test string callback in UTF8", function() {
  scriptable2.onlunch = "msg = '午餐'; return msg;";
  scriptable2.time = "lunch";
  ASSERT(EQ("\u5348\u9910", msg));
  ASSERT(EQ(msg, scriptable2.SignalResult));
});

TEST("Test JSON property", function() {
  scriptable.JSON = {};
  ASSERT(EQ("{}", scriptable.Buffer));
  ASSERT(OBJECT_STRICT_EQ({}, scriptable.JSON));
});

function NewAndUse() {
  var s = new TestScriptable();
  s.onlunch = function() {
    s.EnumSimple = 100;
  };
  gc(); // The function should not be deleted now.
  s.time = "lunch";
  ASSERT(EQ(100, s.EnumSimple));
  scriptable.ClearBuffer();
  s = null;
}

TEST("Test global class", function() {
  var s = new TestScriptable();
  if (typeof isWebkit == "undefined") {
    // It will break this test when running with webkit-script-runtime.
    // Don't know the reason yet.
    TestScriptableBasics(s);
  }
  scriptable.ClearBuffer();
  s = new TestScriptable();
  gc();
  // The previous 's' should be deleted now.
  ASSERT(EQ("Destruct\n", scriptable.Buffer));

  NewAndUse();
  gc();
  // The object allocated in NewAndUse() should be deleted now.
  ASSERT(EQ("Destruct\n", scriptable.Buffer));

  TestScriptableBasics(s);
  var s1 = new TestScriptable();
  TestScriptableBasics(s1);
  // Don't call GC now to test GC on destroying the script context.
});

DEATH_TEST("Test null constructor", function() {
  var n = new TestNullCtor();
  ASSERT(DEATH());
});

TEST("Test default args", function() {
  var s = scriptable2.NewObject();
  ASSERT(TRUE(s.NativeOwned));
  scriptable2.ReleaseObject();
  scriptable2.ReleaseObject(s);
});

TEST("Test scriptable array", function() {
  ASSERT(NULL(scriptable2.ConcatArray(null, null)));
  var x;
  var arr = scriptable2.ConcatArray(
      [1,2,3,4,5],
      [5,4,3,{a:[1,2,3],b:[2,3,4]}, function(x1) { x = x1; }]);
  ASSERT(EQ(10, arr.length));
  ASSERT(EQ(1, arr[0]));
  var nn = arr[8].b;
  ASSERT(EQ(3, nn.length));
  ASSERT(EQ(2, nn[0]));
  ASSERT(EQ(4, arr[8].b[2]));
  var arr1 = scriptable2.ConcatArray(arr, arr);
  ASSERT(EQ(20, arr1.length));
  ASSERT(EQ(1, arr1[10]));
  ASSERT(EQ(4, arr1[18].b[2]));
  print(arr1[19]);
  ASSERT(EQ("function", typeof(arr1[19])));
  arr1[19](100);
  ASSERT(EQ(100, x));
});

// The global scriptable object has properties named 's1' and 's2'.
// The following declarations should override the properties.
var s1 = { a: 1, b: 2};
function s2() {
}

TEST("Test name overriding", function() {
  ASSERT(EQ(1, s1.a));
  ASSERT(EQ("function", typeof(s2)));
});

TEST("Test name overriding1", function() {
  var s1 = 100;
  ASSERT(EQ(100, s1));
});

TEST("Test JS callback as function parameter", function() {
  var x0, y0;
  scriptable2.SetCallback(function(x, y) {
    x0 = x;
    y0 = y;
    return x * 2;
  });
  var s = scriptable2.CallCallback(10);
  ASSERT(EQ(10, x0));
  ASSERT(UNDEFINED(y0));
  // CallCallback returns the Print() result of the input Variant.
  ASSERT(EQ("INT64:20", s));
});

TEST("Test complex signal returning JS object", function() {
  scriptable2.oncomplex = function(s, i) {
    return s ? {
      s: s, i: i,
      get_s: function() { return s },
      get_i: function() { return i },
    } : null;
  };

  scriptable2.FireComplexSignal("string", 100);
  ASSERT(EQ("string", scriptable2.ComplexSignalData.s));
  ASSERT(EQ(100, scriptable2.ComplexSignalData.i));
  ASSERT(EQ("string", scriptable2.ComplexSignalData.get_s())); 
  ASSERT(EQ(100, scriptable2.ComplexSignalData.get_i()));

  scriptable2.FireComplexSignal(null, 0);
  ASSERT(NULL(scriptable2.ComplexSignalData));

  // Let ComplexSignalData hold a JS object to test if it leaks.
  scriptable2.FireComplexSignal("string", 100);
});

TEST("Test Enumeration", function() {
  if (0) { // Feature disabled.
    var expected_array = [
      "Buffer", "BufferReadOnly", "CallCallback", "ConcatArray", "Const",
      "ReleaseObject", "DoubleProperty", "EnumSimple", "EnumString",
      "Fixed", "ICONSTANT0", "ICONSTANT1", "ICONSTANT2", "ICONSTANT3",
      "ICONSTANT4", "ICONSTANT5", "ICONSTANT6", "ICONSTANT7", "ICONSTANT8",
      "ICONSTANT9", "JSON", "NewObject", "OverrideSelf", "PrototypeMethod",
      "PrototypeSelf", "SCONSTANT0", "SCONSTANT1", "SCONSTANT2", "SCONSTANT3",
      "SCONSTANT4", "SCONSTANT5", "SCONSTANT6", "SCONSTANT7", "SCONSTANT8",
      "SCONSTANT9", "ScriptOwned", "SetCallback", "SignalResult",
      "ObjectMethod", "MethodDouble2", "ClearBuffer", "VALUE_0", "VALUE_1",
      "VALUE_2", "VariantProperty", "length", "my_ondelete", "onlunch",
      "onsupper", "ontest", "time",
    ];
    var expected_index = new Object();
    for (var i = 0; i < expected_array.length; i++)
      expected_index[expected_array[i]] = true;
    var count = 0;
    for (var i in scriptable2) {
      ASSERT(TRUE(expected_index[i]), "Key: " + i);
      count++;
    }
    ASSERT(EQ(expected_array.length, count));
  } else {
    for (var i in scriptable2) {
      ASSERT(TRUE(FALSE));
    }
  }
});

RUN_ALL_TESTS();
