/*
Copyright (C) 2007 Google Inc.

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

var HTML = "<html><head>" +
  "<meta http-equiv='content-type' content='text/html; charset=utf8'" +
  "<script>" +
  "var obj1 = {" +
  "  'browser 汉字 Prop': 'browser 汉字 Value'," +
  "  callback: function(view) {" +
  "    return 'Text: ' + this.text + ' view.width: ' + view.width;" +
  "  }" +
  "};" +
  "function func1(view) {" +
  "  return 'view.height: ' + view.height;" +
  "}" +
  "function Test1() {" +
  "  window.external.function1(window.external['汉字 Prop'], obj1, func1);" +
  "}" +
  "function Test2() {" +
  "  window.external.display(window.external.prop1 + '\\n' +" +
  "                          window.external.function2());" +
  "}" +
  "function Test3() {" +
  "  var p = window.external.function3(obj1, func1);" +
  "  window.external.display(p);" +
  "}" +
  "function Test4() {" +
  "  window.external.display(func1(window.external.prop3.prop2) + '\\n' +" +
  "                          func1(window.external.prop2));" +
  "}" +
  "function Test5() {" +
  "  window.external.prop2.resizeBy(10, 10);" +
  "}" +
  "</script>" +
  "<body>Click an Item<p>" +
  "<a href=\"\" onclick=\"Test1(); return false;\">Test1</a><br>" +
  "<a href=\"\" onclick=\"Test2(); return false;\">Test2</a><br>" +
  "<a href=\"\" onclick=\"Test3(); return false;\">Test3</a><br>" +
  "<a href=\"\" onclick=\"Test4(); return false;\">Test4</a><br>" +
  "<a href=\"\" onclick=\"Test5(); return false;\">Test5</a><br>" +
  "</body>" +
  "</html>";

function onTextClick() {
  var htmlDetailsView = new DetailsView();
  htmlDetailsView.html_content = true;
  htmlDetailsView.setContent("", undefined, HTML, false, 0);

  var external = {
    display: function(x) { result.innerText = x; },
    function1 : Function1,
    function2 : function() { return this.prop1; },
    prop1: 1,
    prop2: view,
    "汉字 Prop": "汉字 Value"
  };
  external.prop3 = external;
  external.function3 = function(object, callback) {
    object.text = "text of function3";
    return callback(external.prop2) + "\n" + object.callback(external.prop2);
  };
  htmlDetailsView.external = external;

  // Show the details view
  pluginHelper.showDetailsView(htmlDetailsView, "HTML Details View", 
    gddDetailsViewFlagToolbarOpen, null);
}

function Function1(text, object, callback) {
  object.text = text;
  result.innerText = text + "\n" + object['browser 汉字 Prop'] + "\n" +
                     object.callback(view) + "\n" + callback(view);
}
