#!/bin/bash
#
#  Copyright 2007 Google Inc.
#
#  Licensed under the Apache License, Version 2.0 (the "License");
#  you may not use this file except in compliance with the License.
#  You may obtain a copy of the License at
#
#       http://www.apache.org/licenses/LICENSE-2.0
#
#  Unless required by applicable law or agreed to in writing, software
#  distributed under the License is distributed on an "AS IS" BASIS,
#  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
#  See the License for the specific language governing permissions and
#  limitations under the License.
#

# Generates DOM1 test suite from W3C DOM1 Core test suite.
# Test cases that relies on DTD functionalities, such as entities,
# attribute default values, etc., are disabled.
#
# Steps:
# 1. Checkout W3C DOM1 Core test suite from W3C CVS server:
#      cvs -d :pserver:anonymous@dev.w3.org:/sources/public login
#      (Password: anonymous)
#      cvs -d :pserver:anonymous@dev.w3.org:/sources/public checkout 2001/DOM-Test-Suite
#    See this page for details: http://www.w3.org/DOM/Test/
# 2. In 2001/DOM-Test-Suite, build the "dom1-core-gen-jsunit" target:
#    and dom1-core-gen-jsunit
# 3. Copy this file under 2001/DOM-Test-Suite, and execute it under the dir.
# 4. Copy the generated dom1_test.js into ggadget/scripts/smjs/tests under
#    our project home dir.

set -e
test_dir=tests/level1/core

sort <<END0 >/tmp/DOM_disabled_tests
attrdefaultvalue
attrnotspecifiedvalue
attrremovechild1
attrreplacechild1
attrsetvaluenomodificationallowederr
attrsetvaluenomodificationallowederrEE
attrspecifiedvalueremove
characterdataappenddatanomodificationallowederr
characterdataappenddatanomodificationallowederrEE
characterdatadeletedatanomodificationallowederr
characterdatadeletedatanomodificationallowederrEE
characterdataindexsizeerrdeletedatacountnegative
characterdataindexsizeerrreplacedatacountnegative
characterdataindexsizeerrsubstringcountnegative
characterdatainsertdatanomodificationallowederr
characterdatainsertdatanomodificationallowederrEE
characterdatareplacedatanomodificationallowederr
characterdatareplacedatanomodificationallowederrEE
characterdatasetdatanomodificationallowederr
characterdatasetdatanomodificationallowederrEE
documentcreateelementdefaultattr
documentcreateentityreference
documentcreateentityreferenceknown
documentgetdoctype
documentgetelementsbytagnametotallength
documentinvalidcharacterexceptioncreateentref
documentinvalidcharacterexceptioncreateentref1
documenttypegetdoctype
documenttypegetentities
documenttypegetentitieslength
documenttypegetentitiestype
documenttypegetnotations
documenttypegetnotationstype
elementremoveattribute
elementremoveattributenodenomodificationallowederr
elementremoveattributenodenomodificationallowederrEE
elementremoveattributenomodificationallowederr
elementremoveattributenomodificationallowederrEE
elementremoveattributerestoredefaultvalue
elementretrieveallattributes
elementsetattributenodenomodificationallowederr
elementsetattributenodenomodificationallowederrEE
elementsetattributenomodificationallowederr
elementsetattributenomodificationallowederrEE
entitygetentityname
entitygetpublicid
entitygetpublicidnull
nodeappendchildnomodificationallowederr
nodeappendchildnomodificationallowederrEE
nodedocumenttypenodename
nodedocumenttypenodetype
nodedocumenttypenodevalue
nodeentitynodeattributes
nodeentitynodename
nodeentitynodetype
nodeentitynodevalue
nodeentitysetnodevalue
nodeentityreferencenodeattributes
nodeentityreferencenodename
nodeentityreferencenodetype
nodeentityreferencenodevalue
nodenotationnodeattributes
nodenotationnodename
nodenotationnodetype
nodenotationnodevalue
noderemovechildnomodificationallowederr
noderemovechildnomodificationallowederrEE
nodereplacechildnomodificationallowederr
nodereplacechildnomodificationallowederrEE
nodesetnodevaluenomodificationallowederr
nodesetnodevaluenomodificationallowederrEE
notationgetnotationname
notationgetpublicid
notationgetpublicidnull
notationgetsystemid
notationgetsystemidnull
processinginstructionsetdatanomodificationallowederr
processinginstructionsetdatanomodificationallowederrEE
textsplittextnomodificationallowederr
textsplittextnomodificationallowederrEE
hc_attrgetvalue2
hc_characterdataindexsizeerrdeletedatacountnegative
hc_characterdataindexsizeerrreplacedatacountnegative
hc_characterdataindexsizeerrsubstringcountnegative
hc_documentgetdoctype
hc_elementretrieveallattributes
hc_entitiesremovenameditem1
hc_entitiessetnameditem1
hc_namednodemapchildnoderange
hc_namednodemapnumberofnodes
hc_namednodemapreturnfirstitem
hc_namednodemapreturnlastitem
hc_nodecloneattributescopied
hc_nodeelementnodeattributes
hc_nodevalue03
hc_nodevalue04
hc_nodevalue07
hc_nodevalue08
hc_notationsremovenameditem1
hc_notationssetnameditem1
namednodemapremovenameditem
namednodemapremovenameditemgetvalue
nodeinsertbeforenomodificationallowederr
nodeinsertbeforenomodificationallowederrEE
nodevalue03
nodevalue04
nodevalue07
nodevalue08
END0

# ant dom1-core-gen-jsunit
echo
echo Generating dom1_test_org.js ...
grep suite.member $test_dir/alltests.xml |
    sed 's/^.*="//;s/.xml".*$//' | sort >/tmp/DOM_all_tests
all_tests=`comm -2 -3 /tmp/DOM_all_tests /tmp/DOM_disabled_tests |
    sed 's#^#'$test_dir/'#;s/$/.xml/'`

xsltproc --stringparam interfaces-docname ../build/dom1-interfaces.xml \
    transforms/test-to-ecmascript.xsl $all_tests >dom1_test_org.js
echo
echo Converting dom1_test_org.js to dom1_test.js ...

sed_command="s/'/\\\\'/g;s/^/'/;s/$/\\\\n'+/"

cat <<END >dom1_test.js
// DOM1 test suite. Generated by gen_tests.sh from W3C DOM1 core test suite.
// See below for original file copyrights.

var builder = { contentType: "text/xml" };

var _xml_file_contents = {
staff:
`sed $sed_command $test_dir/files/staff.xml`
'',
hc_staff:
`sed $sed_command $test_dir/files/hc_staff.xml`
'',
hc_nodtdstaff:
`sed $sed_command $test_dir/files/hc_nodtdstaff.xml`
''};

function equalsAutoCase(dummy, x, y) {
  return x === y;
}

// This is only for some assertTrue calls that span multiple lines and
// can't be converted by gen_tests.sh script.
function assertTrue(name, cond) {
  ASSERT(STRICT_EQ(true, cond), name);
}

// We only tests XML, so case-sensitive.
function toLowerArray(arr) {
  return arr;
}

function load(dummy1, dummy2, name) {
  var doc = new DOMDocument();
  doc.loadXML(_xml_file_contents[name]);
  return doc;
}

function DOM_COLLECTION_EQ(expected, list) {
  var eq_length = EQ(expected.length, list.length);
  if (eq_length != null)
    return eq_length;
  for (var i = 0; i < expected.length; i++) {
    var matches = 0;
    for (var j = 0; j < list.length; j++) {
      if (expected[i] == list[j])
        matches++;
    }
    if (matches == 0)
      return _Message("DOM_COLLECTION_EQ[" + i + "]", expected[i], "Not Found");
    if (matches > 1)
      return _Message("DOM_COLLECTION_EQ[" + i + "]", expected[i],
                      "Found multiple");
  }
  return null;
}

function DOM_LIST_EQ(expected, list) {
  var eq_length = EQ(expected.length, list.length);
  if (eq_length != null)
    return eq_length;
  for (var i = 0; i < expected.length; i++) {
    var eq_value = EQ(expected[i], list[i]);
    if (eq_value != null)
      return eq_value;
  }
  return null;
}

function DOM_SAME(expected, node) {
  if (expected == node)
    return null;
  var eq = EQ(expected.nodeType, node.nodeType);
  if (eq != null)
    return eq;
  return EQ(expected.nodeValue, node.nodeValue);
}

END

awk '
$0 == "/*" {
  # Remove the repeated copyright comments except the first one.
  last = $0;
  getline;
  if (sub("test-to-java", "test-to-ecmascript")) {
    if (!after_first_copyright) {
      print last;
      print;
      print "and gen_tests.sh,";
      getline;
    }
    do {
      if (!after_first_copyright) print;
      getline;
    } while ($0 != "*/");
    if (!after_first_copyright) print;
    after_first_copyright = 1;
    next;
  } else {
    # This is not a copyright comment.
    print last;
  }
}

/checkInitialization/ { next; }

{
  $0 = gensub(/^(.*)assertEquals\((".*"),(.*),(.*)\)(.*)$/,
              "\\1ASSERT(STRICT_EQ(\\3,\\4),\\2)\\5", 1);
  $0 = gensub(/^(.*)assertNull\((".*"),(.*)\)(.*)$/,
              "\\1ASSERT(NULL(\\3),\\2)\\4", 1);
  $0 = gensub(/^(.*)assertNotNull\((".*"),(.*)\)(.*)$/,
              "\\1ASSERT(NOT_NULL(\\3),\\2)\\4", 1);
  $0 = gensub(/^(.*)assertTrue\((".*"),(.*)\)(.*)$/,
              "\\1ASSERT(STRICT_EQ(true,\\3),\\2)\\4", 1);
  $0 = gensub(/^(.*)assertFalse\((".*"),(.*)\)(.*)$/,
              "\\1ASSERT(STRICT_EQ(false,\\3),\\2)\\4", 1);
  $0 = gensub(/^(.*)assertSize\((".*"),(.*),(.*)\)(.*)$/,
              "\\1ASSERT(STRICT_EQ(\\3,\\4.length),\\2)\\5", 1);
  $0 = gensub(/^(.*)assertEqualsAutoCase\((".*"),(.*),(.*),(.*)\)(.*)$/,
              "\\1ASSERT(STRICT_EQ(\\4,\\5),\\3)\\6", 1);
  $0 = gensub(/^(.*)assertEqualsCollection\((".*"),(.*),(.*)\)(.*)$/,
              "\\1ASSERT(DOM_COLLECTION_EQ(\\3,\\4),\\2)\\5", 1);
  $0 = gensub(/^(.*)assertEqualsList\((".*"),(.*),(.*)\)(.*)$/,
              "\\1ASSERT(DOM_LIST_EQ(\\3,\\4),\\2)\\5", 1);
  $0 = gensub(/^(.*)assertEqualsListAutoCase\((".*"),(.*),(.*),(.*)\)(.*)$/,
              "\\1ASSERT(DOM_LIST_EQ(\\4,\\5),\\3)\\6", 1);
  $0 = gensub(/^(.*)assertSame\((".*"),(.*),(.*)\)(.*)$/,
              "\\1ASSERT(DOM_SAME(\\3,\\4),\\2)\\5", 1);
  # assertInstanceOf is only used to judge if a node is an attribute.
  $0 = gensub(/^(.*)assertInstanceOf\((".*"),(".*"),(.*)\)(.*)$/,
              "\\1ASSERT(EQ(2,\\4.nodeType),\\2)\\5", 1);
  $0 = gensub(/^function (.*)\(\) \{$/, "TEST(\"\\1\", function() {", 1);
  $0 = gensub(/^\}$/, "});", 1);
  print $0;
}
' <dom1_test_org.js >>dom1_test.js

echo "RUN_ALL_TESTS();" >>dom1_test.js
