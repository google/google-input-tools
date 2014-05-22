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

#include <iostream>
#include <map>
#include <locale.h>

#include "ggadget/logger.h"
#include "ggadget/xml_dom.h"
#include "ggadget/xml_parser_interface.h"
#include "ggadget/xml_utils.h"
#include "unittest/gtest.h"

#if defined(OS_WIN)
#include "ggadget/win32/xml_parser.h"
#elif defined(OS_POSIX)
#include "init_extensions.h"
#endif

using namespace ggadget;

XMLParserInterface *g_xml_parser = NULL;

DOMDocumentInterface* CreateDocument() {
  // Use CreateDOMDocument(parser) instead of parser->CreateDOMDocument()
  // to ensure the created document instance is the one we want to test,
  // not the one that the parser may override.
  return CreateDOMDocument(g_xml_parser, false, false);
}

void TestBlankNode(DOMNodeInterface *node) {
  EXPECT_TRUE(node->GetFirstChild() == NULL);
  EXPECT_TRUE(node->GetLastChild() == NULL);
  EXPECT_TRUE(node->GetPreviousSibling() == NULL);
  EXPECT_TRUE(node->GetNextSibling() == NULL);
  EXPECT_TRUE(node->GetParentNode() == NULL);
  EXPECT_FALSE(node->HasChildNodes());

  DOMNodeListInterface *children = node->GetChildNodes();
  EXPECT_EQ(0U, children->GetLength());
  EXPECT_EQ(0, children->GetRefCount());
  delete children;
}

void TestChildren(DOMNodeInterface *parent, DOMNodeListInterface *children,
                  size_t num_child, ...) {
  ASSERT_EQ(num_child, children->GetLength());

  if (num_child == 0) {
    EXPECT_TRUE(parent->GetFirstChild() == NULL);
    EXPECT_TRUE(parent->GetLastChild() == NULL);
    return;
  }

  va_list ap;
  va_start(ap, num_child);
  for (size_t i = 0; i < num_child; i++) {
    DOMNodeInterface *expected_child = va_arg(ap, DOMNodeInterface *);

    if (i == 0) {
      EXPECT_TRUE(parent->GetFirstChild() == expected_child);
      EXPECT_TRUE(expected_child->GetPreviousSibling() == NULL);
    } else {
      EXPECT_TRUE(expected_child->GetPreviousSibling() ==
                  children->GetItem(i - 1));
    }

    if (i == num_child - 1) {
      EXPECT_TRUE(parent->GetLastChild() == expected_child);
      EXPECT_TRUE(expected_child->GetNextSibling() == NULL);
    } else {
      EXPECT_TRUE(expected_child->GetNextSibling() == children->GetItem(i + 1));
    }

    EXPECT_TRUE(expected_child->GetParentNode() == parent);
    EXPECT_TRUE(children->GetItem(i) == expected_child);
  }
  ASSERT_TRUE(children->GetItem(num_child) == NULL);
  ASSERT_TRUE(children->GetItem(num_child * 2) ==  NULL);
  ASSERT_TRUE(children->GetItem(static_cast<size_t>(-1)) == NULL);
  va_end(ap);
}

void TestNullNodeValue(DOMNodeInterface *node) {
  EXPECT_STREQ("", node->GetNodeValue().c_str());
  EXPECT_FALSE(node->AllowsNodeValue());
  EXPECT_EQ(DOM_NO_MODIFICATION_ALLOWED_ERR, node->SetNodeValue("abcde"));
  EXPECT_STREQ("", node->GetNodeValue().c_str());
}

TEST(XMLDOM, TestBlankDocument) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  ASSERT_TRUE(doc);
  EXPECT_STREQ(kDOMDocumentName, doc->GetNodeName().c_str());
  EXPECT_EQ(DOMNodeInterface::DOCUMENT_NODE, doc->GetNodeType());
  EXPECT_TRUE(doc->GetOwnerDocument() == NULL);
  EXPECT_TRUE(doc->GetAttributes() == NULL);
  TestBlankNode(doc);
  TestNullNodeValue(doc);
  EXPECT_TRUE(doc->GetDocumentElement() == NULL);
  EXPECT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestBlankElement) {
  DOMDocumentInterface *doc = CreateDocument();
  ASSERT_EQ(0, doc->GetRefCount());
  doc->Ref();
  ASSERT_EQ(1, doc->GetRefCount());

  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  ASSERT_EQ(0, root_ele->GetRefCount());
  ASSERT_EQ(2, doc->GetRefCount());
  root_ele->Ref();
  ASSERT_EQ(2, doc->GetRefCount());

  EXPECT_STREQ("root", root_ele->GetNodeName().c_str());
  EXPECT_STREQ("root", root_ele->GetTagName().c_str());
  EXPECT_EQ(DOMNodeInterface::ELEMENT_NODE, root_ele->GetNodeType());
  TestBlankNode(root_ele);
  TestNullNodeValue(root_ele);
  EXPECT_TRUE(root_ele->GetOwnerDocument() == doc);
  EXPECT_TRUE(doc->GetDocumentElement() == NULL);
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));
  EXPECT_TRUE(doc->GetDocumentElement() == root_ele);
  ASSERT_EQ(1, root_ele->GetRefCount());
  ASSERT_EQ(2, doc->GetRefCount());
  root_ele->Unref();
  // The element should not be deleted now because it still belongs to the doc.
  ASSERT_EQ(0, root_ele->GetRefCount());
  ASSERT_EQ(1, doc->GetRefCount());

  DOMElementInterface *ele1 = root_ele;
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, doc->CreateElement("&*(", &ele1));
  ASSERT_TRUE(ele1 == NULL);
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, doc->CreateElement("", &ele1));

  root_ele->Ref();
  ASSERT_EQ(1, root_ele->GetRefCount());
  ASSERT_EQ(2, doc->GetRefCount());
  root_ele->Unref();
  // The element should not be deleted now because it still belongs to the doc.
  ASSERT_EQ(0, root_ele->GetRefCount());
  ASSERT_EQ(1, doc->GetRefCount());
  ASSERT_EQ(DOM_NO_ERR, doc->RemoveChild(root_ele));
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestAttrSelf) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  DOMAttrInterface *attr;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr", &attr));
  ASSERT_EQ(0, attr->GetRefCount());
  attr->Ref();
  ASSERT_EQ(1, attr->GetRefCount());
  ASSERT_EQ(2, doc->GetRefCount());

  EXPECT_STREQ("attr", attr->GetNodeName().c_str());
  EXPECT_STREQ("attr", attr->GetName().c_str());
  EXPECT_EQ(DOMNodeInterface::ATTRIBUTE_NODE, attr->GetNodeType());
  TestBlankNode(attr);
  EXPECT_TRUE(attr->GetAttributes() == NULL);
  EXPECT_STREQ("", attr->GetNodeValue().c_str());
  EXPECT_STREQ("", attr->GetValue().c_str());
  EXPECT_STREQ("", attr->GetTextContent().c_str());
  attr->SetNodeValue("value1");
  EXPECT_STREQ("value1", attr->GetNodeValue().c_str());
  EXPECT_STREQ("value1", attr->GetValue().c_str());
  EXPECT_STREQ("value1", attr->GetTextContent().c_str());
  attr->SetValue("value2");
  EXPECT_STREQ("value2", attr->GetNodeValue().c_str());
  EXPECT_STREQ("value2", attr->GetValue().c_str());
  EXPECT_STREQ("value2", attr->GetTextContent().c_str());
  attr->SetTextContent("value3");
  EXPECT_STREQ("value3", attr->GetNodeValue().c_str());
  EXPECT_STREQ("value3", attr->GetValue().c_str());
  EXPECT_STREQ("value3", attr->GetTextContent().c_str());
  EXPECT_TRUE(attr->GetOwnerDocument() == doc);

  DOMAttrInterface *attr1 = attr;
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, doc->CreateAttribute("&*(", &attr1));
  ASSERT_TRUE(attr1 == NULL);
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR,
            doc->CreateAttribute("Invalid^Name", &attr1));
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, doc->CreateAttribute("", &attr1));

  ASSERT_EQ(1, attr->GetRefCount());
  attr->Unref();
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestParentChild) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  DOMNodeListInterface *children = root_ele->GetChildNodes();
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));
  LOG("No child");
  TestChildren(root_ele, children, 0);

  DOMElementInterface *ele1;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("ele1", &ele1));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(ele1));
  LOG("Children: ele1");
  TestChildren(root_ele, children, 1, ele1);

  DOMElementInterface *ele2;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("ele2", &ele2));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(ele2));
  LOG("Children: ele1, ele2");
  TestChildren(root_ele, children, 2, ele1, ele2);

  DOMElementInterface *ele3;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("ele3", &ele3));
  ASSERT_EQ(DOM_NO_ERR, root_ele->InsertBefore(ele3, ele1));
  LOG("Children: ele3, ele1, ele2");
  TestChildren(root_ele, children, 3, ele3, ele1, ele2);

  ASSERT_EQ(DOM_NO_ERR, root_ele->InsertBefore(ele3, ele3));
  LOG("Children: ele3, ele1, ele2");
  TestChildren(root_ele, children, 3, ele3, ele1, ele2);

  ASSERT_EQ(DOM_NO_ERR, root_ele->InsertBefore(ele3, NULL));
  LOG("Children: ele1, ele2, ele3");
  TestChildren(root_ele, children, 3, ele1, ele2, ele3);

  ASSERT_EQ(DOM_NO_ERR, root_ele->ReplaceChild(ele3, ele3));
  LOG("Children: ele1, ele2, ele3");
  TestChildren(root_ele, children, 3, ele1, ele2, ele3);

  ASSERT_EQ(DOM_NO_ERR, root_ele->ReplaceChild(ele3, ele2));
  LOG("Children: ele1, ele3");
  TestChildren(root_ele, children, 2, ele1, ele3);

  ASSERT_EQ(DOM_NO_ERR, root_ele->RemoveChild(ele3));
  LOG("Children: ele1");
  TestChildren(root_ele, children, 1, ele1);

  ASSERT_EQ(DOM_NO_ERR, root_ele->RemoveChild(ele1));
  LOG("No Child");
  TestChildren(root_ele, children, 0);

  ASSERT_EQ(2, doc->GetRefCount());
  children->Ref();
  ASSERT_EQ(1, children->GetRefCount());
  ASSERT_EQ(2, doc->GetRefCount());
  children->Unref();
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestParentChildErrors) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();

  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));

  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->AppendChild(NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->InsertBefore(NULL, NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->RemoveChild(NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->ReplaceChild(NULL, NULL));

  DOMElementInterface *ele1;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("ele1", &ele1));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(ele1));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->ReplaceChild(NULL, ele1));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, root_ele->ReplaceChild(ele1, NULL));

  DOMElementInterface *ele2;
  doc->CreateElement("ele2", &ele2);
  ASSERT_EQ(DOM_NOT_FOUND_ERR, root_ele->RemoveChild(ele2));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, doc->RemoveChild(ele1));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, ele2->RemoveChild(root_ele));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, root_ele->InsertBefore(ele2, ele2));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, ele2->InsertBefore(ele1, root_ele));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, root_ele->ReplaceChild(ele2, ele2));
  ASSERT_EQ(DOM_NOT_FOUND_ERR, ele2->ReplaceChild(ele1, root_ele));

  DOMNodeInterface *ele2a = ele2->CloneNode(true);
  ASSERT_EQ(DOM_NO_ERR, ele1->AppendChild(ele2));
  ASSERT_EQ(DOM_NO_ERR, ele2->AppendChild(ele2a));

  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->AppendChild(ele2));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->AppendChild(ele1));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->AppendChild(root_ele));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(ele2, NULL));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(ele1, NULL));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(root_ele, NULL));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(ele2, ele2a));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(ele1, ele2a));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->InsertBefore(root_ele, ele2a));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->ReplaceChild(ele2, ele2a));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->ReplaceChild(ele1, ele2a));
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele2->ReplaceChild(root_ele, ele2a));

  DOMDocumentInterface *doc1 = CreateDocument();
  doc1->Ref();
  DOMElementInterface *ele3;
  doc1->CreateElement("ele3", &ele3);
  ASSERT_EQ(2, doc1->GetRefCount());

  ASSERT_EQ(DOM_WRONG_DOCUMENT_ERR, root_ele->AppendChild(ele3));
  ASSERT_EQ(DOM_WRONG_DOCUMENT_ERR, root_ele->InsertBefore(ele3, ele1));
  ASSERT_EQ(DOM_WRONG_DOCUMENT_ERR, root_ele->ReplaceChild(ele3, ele1));

  ASSERT_EQ(2, doc1->GetRefCount());
  delete ele3;
  ASSERT_EQ(1, doc1->GetRefCount());
  ASSERT_EQ(1, doc->GetRefCount());
  doc1->Unref();
  doc->Unref();
}

void TestAttributes(DOMElementInterface *ele, DOMNamedNodeMapInterface *attrs,
                    size_t num_attr, ...) {
  std::map<std::string, std::string> expected_attrs;
  std::string log = "Attrs: ";
  va_list ap;
  va_start(ap, num_attr);
  for (size_t i = 0; i < num_attr; i++) {
    const char *name = va_arg(ap, const char *);
    const char *value = va_arg(ap, const char *);
    log += name;
    log += ':';
    log += value;
    log += ' ';
    expected_attrs[name] = value;
  }
  LOG("%s", log.c_str());
  va_end(ap);

  ASSERT_EQ(num_attr, attrs->GetLength());

  for (size_t i = 0; i < num_attr; i++) {
    DOMAttrInterface *attr = down_cast<DOMAttrInterface *>(attrs->GetItem(i));
    std::string name = attr->GetName();
    std::map<std::string, std::string>::iterator it =
        expected_attrs.find(name);
    EXPECT_FALSE(it == expected_attrs.end());
    EXPECT_EQ(it->first, name);
    EXPECT_EQ(it->second, attr->GetValue());
    EXPECT_TRUE(attr->GetOwnerElement() == ele);
    EXPECT_TRUE(ele->GetAttributeNode(name.c_str()) == attr);
    EXPECT_TRUE(attrs->GetNamedItem(name.c_str()) == attr);
    expected_attrs.erase(it);
  }

  ASSERT_TRUE(attrs->GetItem(num_attr) == NULL);
  ASSERT_TRUE(attrs->GetItem(num_attr * 2) ==  NULL);
  ASSERT_TRUE(attrs->GetItem(static_cast<size_t>(-1)) == NULL);
  ASSERT_TRUE(expected_attrs.empty());
}

TEST(XMLDOM, TestElementAttr) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  DOMElementInterface *ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &ele));
  DOMNamedNodeMapInterface *attrs = ele->GetAttributes();
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(ele));

  TestAttributes(ele, attrs, 0);
  ele->SetAttribute("attr1", "value1");
  TestAttributes(ele, attrs, 1, "attr1", "value1");
  ele->SetAttribute("attr1", "value1a");
  TestAttributes(ele, attrs, 1, "attr1", "value1a");
  ele->SetAttribute("attr2", "value2");
  TestAttributes(ele, attrs, 2, "attr1", "value1a", "attr2", "value2");
  ele->SetAttribute("attr1", "value1b");
  ele->SetAttribute("attr2", "value2a");
  TestAttributes(ele, attrs, 2, "attr1", "value1b", "attr2", "value2a");

  DOMAttrInterface *attr1;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr1", &attr1));
  attr1->SetValue("value1c");
  ASSERT_EQ(DOM_NO_ERR, ele->SetAttributeNode(attr1));
  TestAttributes(ele, attrs, 2, "attr2", "value2a", "attr1", "value1c");

  ASSERT_EQ(DOM_NO_ERR, ele->SetAttributeNode(attr1));
  TestAttributes(ele, attrs, 2, "attr2", "value2a", "attr1", "value1c");

  DOMAttrInterface *attr3;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr3", &attr3));
  attr3->SetValue("value3");
  ASSERT_EQ(DOM_NO_ERR, ele->SetAttributeNode(attr3));
  TestAttributes(ele, attrs, 3, "attr2", "value2a", "attr1", "value1c",
                 "attr3", "value3");

  ASSERT_EQ(DOM_NO_ERR, ele->RemoveAttributeNode(attr3));
  TestAttributes(ele, attrs, 2, "attr2", "value2a", "attr1", "value1c");

  ele->RemoveAttribute("not-exists");
  TestAttributes(ele, attrs, 2, "attr2", "value2a", "attr1", "value1c");
  ele->RemoveAttribute("attr2");
  TestAttributes(ele, attrs, 1, "attr1", "value1c");
  ele->RemoveAttribute("attr1");
  TestAttributes(ele, attrs, 0);
  ele->RemoveAttribute("not-exists");
  TestAttributes(ele, attrs, 0);

  ASSERT_EQ(2, doc->GetRefCount());
  attrs->Ref();
  ASSERT_EQ(2, doc->GetRefCount());
  ASSERT_EQ(1, attrs->GetRefCount());
  attrs->Unref(true);
  ASSERT_EQ(0, attrs->GetRefCount());
  ASSERT_EQ(2, doc->GetRefCount());
  delete attrs;
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestElementAttributes) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  DOMElementInterface *ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &ele));
  DOMNamedNodeMapInterface *attrs = ele->GetAttributes();
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(ele));

  TestAttributes(ele, attrs, 0);
  ele->SetAttribute("attr1", "value1");
  ele->SetAttribute("attr2", "value2");
  TestAttributes(ele, attrs, 2, "attr1", "value1", "attr2", "value2");

  DOMAttrInterface *attr1;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr1", &attr1));
  attr1->SetValue("value1c");
  ASSERT_EQ(DOM_NO_ERR, attrs->SetNamedItem(attr1));
  TestAttributes(ele, attrs, 2, "attr2", "value2", "attr1", "value1c");

  ASSERT_EQ(DOM_NO_ERR, attrs->SetNamedItem(attr1));
  TestAttributes(ele, attrs, 2, "attr2", "value2", "attr1", "value1c");

  DOMAttrInterface *attr3;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr3", &attr3));
  attr3->SetValue("value3");
  ASSERT_EQ(DOM_NO_ERR, attrs->SetNamedItem(attr3));
  TestAttributes(ele, attrs, 3, "attr2", "value2", "attr1", "value1c",
                 "attr3", "value3");

  ASSERT_EQ(DOM_NO_ERR, attrs->RemoveNamedItem("attr3"));
  TestAttributes(ele, attrs, 2, "attr2", "value2", "attr1", "value1c");

  ASSERT_TRUE(attrs->GetNamedItem("not-exist") == NULL);
  attrs->RemoveNamedItem("attr2");
  TestAttributes(ele, attrs, 1, "attr1", "value1c");
  attrs->RemoveNamedItem("attr1");
  TestAttributes(ele, attrs, 0);
  attrs->RemoveNamedItem("not-exists");
  TestAttributes(ele, attrs, 0);
  ASSERT_TRUE(attrs->GetNamedItem("not-exist") == NULL);

  ASSERT_EQ(2, doc->GetRefCount());
  attrs->Ref();
  ASSERT_EQ(2, doc->GetRefCount());
  ASSERT_EQ(1, attrs->GetRefCount());
  attrs->Unref(true);
  ASSERT_EQ(0, attrs->GetRefCount());
  ASSERT_EQ(2, doc->GetRefCount());
  delete attrs;
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestElementAttrErrors) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  DOMElementInterface *ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &ele));
  DOMNamedNodeMapInterface *attrs = ele->GetAttributes();
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(ele));

  TestAttributes(ele, attrs, 0);
  ele->SetAttribute("attr1", "value1");
  ele->SetAttribute("attr2", "value2");

  DOMAttrInterface *fake_attr2;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateAttribute("attr2", &fake_attr2));
  fake_attr2->SetValue("value2");
  ASSERT_EQ(DOM_NOT_FOUND_ERR, ele->RemoveAttributeNode(fake_attr2));
  TestAttributes(ele, attrs, 2, "attr1", "value1", "attr2", "value2");
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, ele->AppendChild(fake_attr2));
  delete fake_attr2;

  ele->SetAttribute("attr2", "");
  TestAttributes(ele, attrs, 2, "attr1", "value1", "attr2", "");

  ASSERT_EQ(DOM_NOT_FOUND_ERR, attrs->RemoveNamedItem("not-exist"));
  TestAttributes(ele, attrs, 2, "attr1", "value1", "attr2", "");

  ASSERT_EQ(DOM_NULL_POINTER_ERR, ele->SetAttributeNode(NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, attrs->SetNamedItem(NULL));
  ASSERT_EQ(DOM_NULL_POINTER_ERR, ele->RemoveAttributeNode(NULL));

  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, ele->SetAttribute("&*(", "abcde"));
  ASSERT_EQ(DOM_INVALID_CHARACTER_ERR, ele->SetAttribute("", "abcde"));

  DOMElementInterface *ele1;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &ele1));
  ele1->SetAttribute("attr1", "value1d");
  ASSERT_EQ(DOM_INUSE_ATTRIBUTE_ERR,
            attrs->SetNamedItem(ele1->GetAttributeNode("attr1")));
  ASSERT_EQ(DOM_INUSE_ATTRIBUTE_ERR,
            ele->SetAttributeNode(ele1->GetAttributeNode("attr1")));
  DOMAttrInterface *cloned = down_cast<DOMAttrInterface *>(
      ele1->GetAttributeNode("attr1")->CloneNode(false));
  ASSERT_EQ(DOM_NO_ERR, attrs->SetNamedItem(cloned));
  ASSERT_EQ(DOM_NO_ERR, ele->SetAttributeNode(cloned));
  TestAttributes(ele, attrs, 2, "attr2", "", "attr1", "value1d");
  delete ele1;

  DOMDocumentInterface *doc1 = CreateDocument();
  doc1->Ref();
  DOMAttrInterface *attr_doc1;
  ASSERT_EQ(DOM_NO_ERR, doc1->CreateAttribute("attr_doc1", &attr_doc1));
  ASSERT_EQ(DOM_WRONG_DOCUMENT_ERR, attrs->SetNamedItem(attr_doc1));
  ASSERT_EQ(DOM_WRONG_DOCUMENT_ERR, ele->SetAttributeNode(attr_doc1));
  TestAttributes(ele, attrs, 2, "attr2", "", "attr1", "value1d");
  delete attr_doc1;
  delete attrs;
  ASSERT_EQ(1, doc1->GetRefCount());
  ASSERT_EQ(1, doc->GetRefCount());
  doc1->Unref();
  doc->Unref();
}

void TestBlankNodeList(DOMNodeListInterface *list) {
  ASSERT_EQ(0U, list->GetLength());
  ASSERT_TRUE(list->GetItem(0) == NULL);
  ASSERT_TRUE(list->GetItem(static_cast<size_t>(-1)) == NULL);
  ASSERT_TRUE(list->GetItem(1) == NULL);
}

TEST(XMLDOM, TestBlankGetElementsByTagName) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  DOMNodeListInterface *elements = doc->GetElementsByTagName("");
  TestBlankNodeList(elements);
  delete elements;

  elements = doc->GetElementsByTagName("");
  LOG("Blank document blank name");
  TestBlankNodeList(elements);
  delete elements;

  elements = doc->GetElementsByTagName("*");
  LOG("Blank document wildcard name");
  TestBlankNodeList(elements);
  delete elements;

  elements = doc->GetElementsByTagName("not-exist");
  LOG("Blank document non-existent name");
  TestBlankNodeList(elements);
  delete elements;
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestAnyGetElementsByTagName) {
  const char *xml =
    "<root>"
    " <s/>"
    " <s1><s/></s1>\n"
    " <s><s><s/></s></s>\n"
    " <s><s1><s1/></s1></s>\n"
    "</root>";

  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  ASSERT_TRUE(doc->LoadXML(xml));
  DOMNodeListInterface *elements = doc->GetElementsByTagName("");
  TestBlankNodeList(elements);
  delete elements;

  elements = doc->GetElementsByTagName("");
  LOG("Non-blank document blank name");
  TestBlankNodeList(elements);
  delete elements;

  elements = doc->GetElementsByTagName("not-exist");
  LOG("Non-blank document non-existant name");
  TestBlankNodeList(elements);
  delete elements;

  elements = doc->GetElementsByTagName("*");
  LOG("Non-blank document wildcard name");
  ASSERT_EQ(10U, elements->GetLength());
  ASSERT_TRUE(elements->GetItem(10U) == NULL);
  ASSERT_TRUE(elements->GetItem(0U) == doc->GetDocumentElement());
  DOMNodeInterface *node = elements->GetItem(4U);
  ASSERT_TRUE(node->GetParentNode() == doc->GetDocumentElement());
  ASSERT_STREQ("s", node->GetNodeName().c_str());
  ASSERT_EQ(DOMNodeInterface::ELEMENT_NODE, node->GetNodeType());
  ASSERT_EQ(DOM_NO_ERR, doc->GetDocumentElement()->RemoveChild(node));
  ASSERT_EQ(7U, elements->GetLength());
  ASSERT_TRUE(elements->GetItem(7U) == NULL);
  ASSERT_EQ(DOM_NO_ERR, doc->RemoveChild(doc->GetDocumentElement()));
  TestBlankNodeList(elements);
  delete elements;
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestGetElementsByTagName) {
  const char *xml =
    "<root>"
    " <s/>"
    " <s1><s/></s1>\n"
    " <s><s><s/></s></s>\n"
    " <s><s1><s1/></s1></s>\n"
    "</root>";

  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  ASSERT_TRUE(doc->LoadXML(xml));
  DOMNodeListInterface *elements = doc->GetElementsByTagName("s");
  LOG("Non-blank document name 's'");
  ASSERT_EQ(6U, elements->GetLength());
  ASSERT_TRUE(elements->GetItem(6U) == NULL);
  for (size_t i = 0; i < 6U; i++) {
    DOMNodeInterface *node = elements->GetItem(i);
    ASSERT_STREQ("s", node->GetNodeName().c_str());
    ASSERT_EQ(DOMNodeInterface::ELEMENT_NODE, node->GetNodeType());
  }

  ASSERT_EQ(DOM_NO_ERR,
            elements->GetItem(2U)->RemoveChild(elements->GetItem(3U)));
  ASSERT_EQ(4U, elements->GetLength());
  for (size_t i = 0; i < 4U; i++) {
    DOMNodeInterface *node = elements->GetItem(i);
    ASSERT_STREQ("s", node->GetNodeName().c_str());
    ASSERT_EQ(DOMNodeInterface::ELEMENT_NODE, node->GetNodeType());
  }

  ASSERT_TRUE(elements->GetItem(4U) == NULL);
  ASSERT_EQ(DOM_NO_ERR, doc->RemoveChild(doc->GetDocumentElement()));
  TestBlankNodeList(elements);
  delete elements;
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestText) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();

  static UTF16Char data[] = { 'd', 'a', 't', 'a', 0 };
  DOMTextInterface *text = doc->CreateTextNode(data);
  ASSERT_EQ(0, text->GetRefCount());
  text->Ref();
  ASSERT_EQ(1, text->GetRefCount());
  ASSERT_EQ(2, doc->GetRefCount());

  UTF16String blank_utf16;
  EXPECT_TRUE(UTF16String(text->GetData()) == data);

  EXPECT_STREQ(kDOMTextName, text->GetNodeName().c_str());
  TestBlankNode(text);
  EXPECT_STREQ("data", text->GetNodeValue().c_str());
  EXPECT_STREQ("data", text->GetTextContent().c_str());
  text->SetNodeValue("");
  EXPECT_STREQ("", text->GetNodeValue().c_str());
  EXPECT_STREQ("", text->GetTextContent().c_str());
  EXPECT_TRUE(blank_utf16 == text->GetData());
  text->SetTextContent("data1");
  EXPECT_STREQ("data1", text->GetNodeValue().c_str());
  EXPECT_STREQ("data1", text->GetTextContent().c_str());

  text->SetData(data);
  EXPECT_STREQ("data", text->GetNodeValue().c_str());
  EXPECT_TRUE(UTF16String(text->GetData()) == data);

  UTF16String data_out = data;
  EXPECT_EQ(DOM_NO_ERR, text->SubstringData(0, 5, &data_out));
  EXPECT_TRUE(data_out == data);
  EXPECT_EQ(DOM_INDEX_SIZE_ERR, text->SubstringData(5, 0, &data_out));
  EXPECT_TRUE(data_out == blank_utf16);
  EXPECT_EQ(DOM_NO_ERR, text->SubstringData(0, 4, &data_out));
  EXPECT_TRUE(data_out == data);
  EXPECT_EQ(DOM_NO_ERR, text->SubstringData(1, 2, &data_out));
  UTF16Char expected_data[] = { 'a', 't', 0 };
  EXPECT_TRUE(data_out == expected_data);
  EXPECT_EQ(DOM_NO_ERR, text->SubstringData(1, 0, &data_out));
  EXPECT_TRUE(blank_utf16 == data_out);

  text->AppendData(blank_utf16);
  EXPECT_TRUE(UTF16String(text->GetData()) == data);
  static UTF16Char extra_data[] = { 'D', 'A', 'T', 'A', 0 };
  text->AppendData(extra_data);
  EXPECT_STREQ("dataDATA", text->GetNodeValue().c_str());
  text->SetNodeValue("");
  text->AppendData(data);
  ASSERT_TRUE(UTF16String(text->GetData()) == data);

  EXPECT_EQ(DOM_NO_ERR, text->InsertData(0, extra_data));
  EXPECT_STREQ("DATAdata", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_NO_ERR, text->InsertData(8, extra_data));
  EXPECT_STREQ("DATAdataDATA", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_NO_ERR, text->InsertData(6, extra_data));
  EXPECT_STREQ("DATAdaDATAtaDATA", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_INDEX_SIZE_ERR, text->InsertData(17, extra_data));
  text->SetNodeValue("");
  EXPECT_EQ(DOM_NO_ERR, text->InsertData(0, data));
  ASSERT_TRUE(UTF16String(text->GetData()) == data);

  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(0, 0));
  ASSERT_TRUE(UTF16String(text->GetData()) == data);
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(4, 0));
  ASSERT_TRUE(UTF16String(text->GetData()) == data);
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(0, 1));
  EXPECT_STREQ("ata", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(1, 1));
  EXPECT_STREQ("aa", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(0, 2));
  EXPECT_STREQ("", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(0, 0));
  EXPECT_STREQ("", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_NO_ERR, text->InsertData(0, data));
  EXPECT_EQ(DOM_INDEX_SIZE_ERR, text->DeleteData(5, 0));
  EXPECT_EQ(DOM_NO_ERR, text->DeleteData(0, 5));
  EXPECT_STREQ("", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_NO_ERR, text->InsertData(0, data));
  ASSERT_TRUE(UTF16String(text->GetData()) == data);

  EXPECT_EQ(DOM_NO_ERR, text->ReplaceData(0, 0, extra_data));
  EXPECT_STREQ("DATAdata", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_NO_ERR, text->ReplaceData(6, 2, extra_data));
  EXPECT_STREQ("DATAdaDATA", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_NO_ERR, text->ReplaceData(6, 1, extra_data));
  EXPECT_STREQ("DATAdaDATAATA", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_INDEX_SIZE_ERR, text->ReplaceData(14, 0, extra_data));
  EXPECT_EQ(DOM_NO_ERR, text->ReplaceData(0, 14, extra_data));
  EXPECT_STREQ("DATA", text->GetNodeValue().c_str());
  text->SetNodeValue("");
  EXPECT_EQ(DOM_NO_ERR, text->ReplaceData(0, 0, data));
  ASSERT_TRUE(UTF16String(text->GetData()) == data);

  DOMTextInterface *text1 = doc->CreateTextNode(data);
  EXPECT_EQ(DOM_HIERARCHY_REQUEST_ERR, text->AppendChild(text1));
  ASSERT_EQ(3, doc->GetRefCount());
  delete text1;
  ASSERT_EQ(2, doc->GetRefCount());

  EXPECT_EQ(DOM_NO_ERR, text->SplitText(0, &text1));
  EXPECT_STREQ("", text->GetNodeValue().c_str());
  EXPECT_STREQ("data", text1->GetNodeValue().c_str());
  ASSERT_EQ(1, text->GetRefCount());
  ASSERT_EQ(3, doc->GetRefCount());
  text->Unref();
  ASSERT_EQ(2, doc->GetRefCount());

  EXPECT_EQ(DOM_NO_ERR, text1->SplitText(4, &text));
  EXPECT_STREQ("", text->GetNodeValue().c_str());
  EXPECT_STREQ("data", text1->GetNodeValue().c_str());
  delete text;

  EXPECT_EQ(DOM_NO_ERR, text1->SplitText(2, &text));
  EXPECT_STREQ("ta", text->GetNodeValue().c_str());
  EXPECT_STREQ("da", text1->GetNodeValue().c_str());

  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(text));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(text1));
  root_ele->Normalize();
  text = down_cast<DOMTextInterface *>(root_ele->GetFirstChild());
  ASSERT_TRUE(text->GetNextSibling() == NULL);
  EXPECT_STREQ("tada", text->GetNodeValue().c_str());
  EXPECT_EQ(DOM_INDEX_SIZE_ERR, text->SplitText(5, &text1));
  EXPECT_TRUE(text1 == NULL);
  EXPECT_EQ(DOM_NO_ERR, text->SplitText(2, &text1));
  EXPECT_TRUE(text1->GetParentNode() == root_ele);
  EXPECT_TRUE(text1->GetPreviousSibling() == text);
  EXPECT_STREQ("ta", text->GetNodeValue().c_str());
  EXPECT_STREQ("da", text1->GetNodeValue().c_str());

  DOMTextInterface *text2;
  EXPECT_EQ(DOM_NO_ERR, text->SplitText(1, &text2));
  EXPECT_TRUE(text2->GetParentNode() == root_ele);
  EXPECT_TRUE(text2->GetPreviousSibling() == text);
  EXPECT_TRUE(text2->GetNextSibling() == text1);
  EXPECT_STREQ("t", text->GetNodeValue().c_str());
  EXPECT_STREQ("a", text2->GetNodeValue().c_str());
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestDocumentFragmentAndTextContent) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));

  DOMDocumentFragmentInterface *fragment = doc->CreateDocumentFragment();
  fragment->Ref();
  TestBlankNode(fragment);
  TestNullNodeValue(fragment);
  ASSERT_EQ(DOMNodeInterface::DOCUMENT_FRAGMENT_NODE, fragment->GetNodeType());
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(fragment));
  EXPECT_TRUE(root_ele->GetFirstChild() == NULL);

  static UTF16Char data[] = { 'd', 'a', 't', 'a', 0 };
  fragment->SetTextContent("DATA");
  ASSERT_EQ(DOM_NO_ERR, fragment->AppendChild(doc->CreateTextNode(data)));
  ASSERT_STREQ("DATAdata", fragment->GetTextContent().c_str());
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(fragment));
  TestBlankNode(fragment);
  ASSERT_STREQ("", fragment->GetTextContent().c_str());

  EXPECT_TRUE(root_ele->GetFirstChild());
  EXPECT_TRUE(root_ele->GetFirstChild()->GetNextSibling());
  EXPECT_TRUE(root_ele->GetFirstChild()->GetNextSibling()->GetNextSibling() ==
              NULL);
  EXPECT_STREQ("DATAdata", root_ele->GetTextContent().c_str());
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(root_ele->CloneNode(true)));
  data[0] = 'E';
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(doc->CreateCDATASection(data)));
  data[0] = 'F';
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(doc->CreateComment(data)));
  EXPECT_STREQ("DATAdataDATAdataEata", root_ele->GetTextContent().c_str());

  root_ele->SetTextContent("NEW");
  EXPECT_STREQ("NEW", root_ele->GetTextContent().c_str());
  ASSERT_EQ(2, doc->GetRefCount());
  ASSERT_EQ(1, fragment->GetRefCount());
  fragment->Unref();
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, Others) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  DOMElementInterface *root_ele;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateElement("root", &root_ele));
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(root_ele));

  ASSERT_TRUE(doc->GetDoctype() == NULL);
  DOMImplementationInterface *impl = doc->GetImplementation();
  ASSERT_TRUE(impl->HasFeature("XML", "1.0"));
  ASSERT_TRUE(impl->HasFeature("XML", NULL));
  ASSERT_FALSE(impl->HasFeature("XPATH", NULL));

  DOMCommentInterface *comment = doc->CreateCommentUTF8("");
  comment->Ref();
  TestBlankNode(comment);
  ASSERT_EQ(DOMNodeInterface::COMMENT_NODE, comment->GetNodeType());
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(comment));

  DOMCDATASectionInterface *cdata = doc->CreateCDATASectionUTF8("");
  cdata->Ref();
  TestBlankNode(cdata);
  ASSERT_EQ(DOMNodeInterface::CDATA_SECTION_NODE, cdata->GetNodeType());
  ASSERT_EQ(DOM_HIERARCHY_REQUEST_ERR, doc->AppendChild(cdata));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(cdata));

  DOMProcessingInstructionInterface *pi;
  ASSERT_EQ(DOM_NO_ERR, doc->CreateProcessingInstruction("pi", "value", &pi));
  pi->Ref();
  TestBlankNode(pi);
  ASSERT_EQ(DOMNodeInterface::PROCESSING_INSTRUCTION_NODE, pi->GetNodeType());
  ASSERT_EQ(DOM_NO_ERR, doc->AppendChild(pi));
  ASSERT_EQ(DOM_NO_ERR, root_ele->AppendChild(pi));

  ASSERT_EQ(4, doc->GetRefCount());
  comment->Unref();
  cdata->Unref();
  pi->Unref();
  ASSERT_EQ(0, comment->GetRefCount());
  ASSERT_EQ(0, cdata->GetRefCount());
  ASSERT_EQ(0, pi->GetRefCount());
  ASSERT_TRUE(doc->CloneNode(true) == NULL);
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

TEST(XMLDOM, TestCommentSerialize) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  UTF16Char src[] = {
    '-', '-', 'a', '-', '-', '-', 'b', '-', '-', '-', '-',
    'c', '-', '-', 0
  };
  DOMCommentInterface *comment = doc->CreateComment(src);
  EXPECT_STREQ("--a---b----c--", comment->GetNodeValue().c_str());
  EXPECT_STREQ("--a---b----c--", comment->GetTextContent().c_str());
  EXPECT_STREQ("<!--- -a- - -b- - - -c- - -->\n",
               comment->GetXML().c_str());
}

TEST(XMLDOM, TestCDataSerialize) {
  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  UTF16Char src[] = { ']', ']', '>', '>', ']', ']', ']', '>', 0 };
  DOMCDATASectionInterface *cdata = doc->CreateCDATASection(src);
  EXPECT_STREQ("]]>>]]]>", cdata->GetNodeValue().c_str());
  EXPECT_STREQ("]]>>]]]>", cdata->GetTextContent().c_str());
  EXPECT_STREQ("<![CDATA[]]]]><![CDATA[>>]]]]]><![CDATA[>]]>\n",
               cdata->GetXML().c_str());
}

TEST(XMLDOM, TestXMLLoadAndSerialize) {
  const char *xml =
    "<?pi pi=\"pi\"?>\n"
    "<root attr=\"lt;&quot;&gt;\" attr2=\"&amp;1234\">"
    " <s/>text&lt;&gt;"
    " <s1><s/><![CDATA[\n cdata <>\n]]></s1>\n"
    " <s><s><!-- some comments --><s/></s></s>\n"
    " <s><s1><s1>text</s1></s1></s>\n"
    "</root>";

  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  doc->LoadXML(xml);
  DOMElementInterface *ele = doc->GetDocumentElement();
  for (int i = 0; i < 20; i++) {
    ele->SetAttribute(StringPrintf("new-attr%d", i).c_str(),
                      StringPrintf("new-value%d", i).c_str());
  }
  std::string xml_out = doc->GetXML();
  LOG("xml_out: '%s'", xml_out.c_str());
  doc->LoadXML(xml_out.c_str());
  std::string xml_out2 = doc->GetXML();
  ASSERT_STREQ(xml_out2.c_str(), xml_out.c_str());
  ASSERT_EQ(1, doc->GetRefCount());
  doc->Unref();
}

void TestSelectNode(DOMNodeInterface *node, const char *xpath,
                    size_t count, ...) {
  LOG("XPATH: %s", xpath);
  va_list ap;
  va_start(ap, count);
  DOMNodeInterface *single = node->SelectSingleNode(xpath);
  DOMNodeListInterface *multi = node->SelectNodes(xpath);
  ASSERT_EQ(count, multi->GetLength());
  if (count == 0) {
    ASSERT_TRUE(single == NULL);
  } else {
    ASSERT_TRUE(single != NULL);
    ASSERT_TRUE(single == multi->GetItem(0));
  }

  for (size_t i = 0; i < count; i++) {
    const char *id = va_arg(ap, const char *);
    DOMNodeInterface *node = multi->GetItem(i);
    if (strcmp(id, "#") == 0) {
      ASSERT_EQ(node->GetNodeType(), DOMNodeInterface::DOCUMENT_NODE);
    } else {
      ASSERT_EQ(node->GetNodeType(), DOMNodeInterface::ELEMENT_NODE);
      ASSERT_STREQ(id,
          down_cast<DOMElementInterface *>(node)->GetAttribute("id").c_str());
    }
  }
  delete multi;
}

TEST(XMLDOM, SelectNodes) {
  const char *xml =
    "<root id=\"0\">\n"
    " <a id=\"1\">\n"
    "  <a id=\"11\"><a id=\"111\"/><b id=\"112\"/></a>\n"
    "  <b id=\"12\"><a id=\"121\"/><b id=\"122\"/></b>\n"
    " </a>\n"
    " <b id=\"2\">\n"
    "  <a id=\"21\"><a id=\"211\"/><b id=\"212\"/></a>\n"
    "  <b id=\"22\"><a id=\"221\"/><b id=\"222\"/></b>\n"
    " </b>\n"
    "</root>";

  DOMDocumentInterface *doc = CreateDocument();
  doc->Ref();
  doc->LoadXML(xml);
  TestSelectNode(doc, "", 0);
  TestSelectNode(doc, NULL, 0);
  TestSelectNode(doc, "/", 1, "#");
  TestSelectNode(doc, "/root", 1, "0");
  TestSelectNode(doc, "root", 1, "0");
  TestSelectNode(doc, "root/b", 1, "2");
  TestSelectNode(doc, "//b", 7, "112", "12", "122", "2", "212", "22", "222");
  TestSelectNode(doc, ".//b", 7, "112", "12", "122", "2", "212", "22", "222");
  TestSelectNode(doc, "//*", 15, "0",
                 "1", "11", "111", "112", "12", "121", "122",
                 "2", "21", "211", "212", "22", "221", "222");
  TestSelectNode(doc, "//.", 16, "#", "0",
                 "1", "11", "111", "112", "12", "121", "122",
                 "2", "21", "211", "212", "22", "221", "222");
  TestSelectNode(doc, "/*", 1, "0");
  TestSelectNode(doc, "/*//a/b", 3, "112", "12", "212");
  TestSelectNode(doc, "/*//././a/././b", 3, "12", "112", "212");
  // FIXME: TestSelectNode(doc, "*//a//b", 4, "112", "12", "122", "212");
  DOMNodeInterface *node = doc->SelectSingleNode("/root/a");
  ASSERT_TRUE(node);
  node->Ref();
  TestSelectNode(node, "", 0);
  TestSelectNode(node, NULL, 0);
  TestSelectNode(node, "/", 1, "#");
  TestSelectNode(node, "/root", 1, "0");
  TestSelectNode(node, "root", 0);
  TestSelectNode(node, "/root/b", 1, "2");
  TestSelectNode(node, "//b", 7, "112", "12", "122", "2", "212", "22", "222");
  TestSelectNode(node, ".//b", 3, "112", "12", "122");
  TestSelectNode(node, ".//*", 6, "11", "111", "112", "12", "121", "122");
  TestSelectNode(node, ".//.", 7, "1", "11", "111", "112", "12", "121", "122");
  TestSelectNode(node, "*", 2, "11", "12");
  node->Unref();
  doc->Unref();
}

int main(int argc, char **argv) {
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
  g_xml_parser = GetXMLParser();
  int result = RUN_ALL_TESTS();
  return result;
}
