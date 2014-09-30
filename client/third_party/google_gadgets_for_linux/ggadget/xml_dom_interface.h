/*
  Copyright 2008 Google Inc.

  virtual Licensed under the Apache License, Version 2.0 (the "License") = 0;
  you may not use this file except in compliance with the License.
  You may obtain a copy of the License at

       http://www.apache.org/licenses/LICENSE-2.0

  Unless required by applicable law or agreed to in writing, software
  distributed under the License is distributed on an "AS IS" BASIS,
  WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
  See the License for the specific language governing permissions and
  limitations under the License.
*/

#ifndef GGADGET_XML_DOM_INTERFACE_H__
#define GGADGET_XML_DOM_INTERFACE_H__

#include <ggadget/common.h>
#include <ggadget/scriptable_interface.h>
#include <ggadget/unicode_utils.h>

namespace ggadget {

class DOMNodeListInterface;
class DOMNamedNodeMapInterface;
class DOMDocumentInterface;
class DOMElementInterface;
class XMLParserInterface;

/**
 * @defgroup XMLDOMInterfaces XML DOM Interfaces
 * @ingroup Interfaces
 * @{
 */

/* TODO: DOM2
const char kXMLPrefix[] = "xml";
const char kXMLNamespaceURI[] = "http://www.w3.org/XML/1998/namespace";
const char kXMLNSPrefix[] = "xmlns";
const char kXMLNSNamespaceURI[] = "http://www.w3.org/2000/xmlns/";
*/

const char kDOMCDATASectionName[] = "#cdata-section";
const char kDOMCommentName[] = "#comment";
const char kDOMDocumentName[] = "#document";
const char kDOMDocumentFragmentName[] = "#document-fragment";
const char kDOMTextName[] = "#text";

/**
 * DOM interfaces.
 * Reference:
 *   - http://www.w3.org/TR/2000/REC-DOM-Level-2-Core-20001113/
 *
 * Notes about memory management:
 *   - All passed in <code>const char *</code> arguments are made copies.
 *   - All returned <code>const char *</code> must be used transiently, or
 *     the caller must make copies by itself.
 *   - @see @c DOMNodeInterface::Attach()
 *   - @see @c DOMNodeInterface::Detach()
 */
enum DOMExceptionCode {
  /**
   * @c NO_ERR is only used in C++ API as return values of methods to
   * indicate no error occurs. It should not be reflected to scripts.
   */
  DOM_NO_ERR,
  DOM_INDEX_SIZE_ERR              = 1,
  DOM_DOMSTRING_SIZE_ERR          = 2,
  DOM_HIERARCHY_REQUEST_ERR       = 3,
  DOM_WRONG_DOCUMENT_ERR          = 4,
  DOM_INVALID_CHARACTER_ERR       = 5,
  DOM_NO_DATA_ALLOWED_ERR         = 6,
  DOM_NO_MODIFICATION_ALLOWED_ERR = 7,
  DOM_NOT_FOUND_ERR               = 8,
  DOM_NOT_SUPPORTED_ERR           = 9,
  DOM_INUSE_ATTRIBUTE_ERR         = 10,
  /* TODO: DOM2
  DOM_INVALID_STATE_ERR           = 11,
  DOM_SYNTAX_ERR                  = 12,
  DOM_INVALID_MODIFICATION_ERR    = 13,
  DOM_NAMESPACE_ERR               = 14,
  DOM_INVALID_ACCESS_ERR          = 15,
  */
  /** Extended error code to indicate an unexpected null pointer argument. */
  DOM_NULL_POINTER_ERR            = 200
};

namespace internal {
class DOMNodeImpl;
}

class DOMNodeInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0x7787eb3be55b4266);

  enum NodeType {
    ELEMENT_NODE                = 1,
    ATTRIBUTE_NODE              = 2,
    TEXT_NODE                   = 3,
    CDATA_SECTION_NODE          = 4,
    ENTITY_REFERENCE_NODE       = 5,
    ENTITY_NODE                 = 6,
    PROCESSING_INSTRUCTION_NODE = 7,
    COMMENT_NODE                = 8,
    DOCUMENT_NODE               = 9,
    DOCUMENT_TYPE_NODE          = 10,
    DOCUMENT_FRAGMENT_NODE      = 11,
    NOTATION_NODE               = 12
  };

  /** For implementation only.  Placed here to ensure consistent vtbl offset. */
  virtual internal::DOMNodeImpl *GetImpl() const = 0;

  /**
   * Redeclare @c ScriptableInterface::Ref() and
   * @c ScriptableInterface::Unref() here to give additional notes.
   *
   * All @c DOMNodeInterface derived objects are reference counted. Normally
   * you should use @c Ref() and @c Unref() to add and remove references.
   *
   * Reference counting is based on DOM trees. References to any node in
   * a tree are counted as the reference count of the whole tree. When the
   * reference count decreases to zero, the whole tree will be deleted. When a
   * subtree is removed from a tree, the reference count will be splitted into
   * two, each containing the sum count of the individual new tree. When a
   * tree is added into another tree, the reference count will be summed up.
   *
   * The reference count of a document includes not only the references to the
   * tree itself, but also the number of orphan trees, because one can find
   * the document through a node in an orphan tree.
   *
   * All newly created nodes returned from DOMDocumentInterface::CreateXXX(),
   * any CloneNode() or SplitText(), etc. are initially have a zero count.
   * You should either use the @c delete operator to delete the node,
   * or call @c Ref() if you need to further operate on it, but never ignore
   * the results from such methods.
   */
  virtual void Ref() const = 0;
  virtual void Unref(bool transient = false) const = 0;

  virtual std::string GetNodeName() const = 0;

  /**
   * The standard requires to sometimes return NULL, but this method returns
   * blank string in the case.
   * JavaScript behavior still conforms the standard.
   */
  virtual std::string GetNodeValue() const = 0;
  virtual DOMExceptionCode SetNodeValue(const std::string &node_value) = 0;
  virtual bool AllowsNodeValue() const = 0;
  virtual NodeType GetNodeType() const = 0;

  virtual DOMNodeInterface *GetParentNode() = 0;
  virtual const DOMNodeInterface *GetParentNode() const = 0;

  /** Caller must delete or Ref/Unref the returned object. */
  virtual DOMNodeListInterface *GetChildNodes() = 0;
  virtual const DOMNodeListInterface *GetChildNodes() const = 0;

  virtual DOMNodeInterface *GetFirstChild() = 0;
  virtual const DOMNodeInterface *GetFirstChild() const = 0;
  virtual DOMNodeInterface *GetLastChild() = 0;
  virtual const DOMNodeInterface *GetLastChild() const = 0;
  virtual DOMNodeInterface *GetPreviousSibling() = 0;
  virtual const DOMNodeInterface *GetPreviousSibling() const = 0;
  virtual DOMNodeInterface *GetNextSibling() = 0;
  virtual const DOMNodeInterface *GetNextSibling() const = 0;

  /** Caller must delete or Ref/Unref the returned object. */
  virtual DOMNamedNodeMapInterface *GetAttributes() = 0;
  virtual const DOMNamedNodeMapInterface *GetAttributes() const = 0;

  virtual DOMDocumentInterface *GetOwnerDocument() = 0;
  virtual const DOMDocumentInterface *GetOwnerDocument() const = 0;

  virtual DOMExceptionCode InsertBefore(DOMNodeInterface *new_child,
                                        DOMNodeInterface *ref_child) = 0;
  virtual DOMExceptionCode ReplaceChild(DOMNodeInterface *new_child,
                                        DOMNodeInterface *old_child) = 0;
  virtual DOMExceptionCode RemoveChild(DOMNodeInterface *old_child) = 0;
  virtual DOMExceptionCode AppendChild(DOMNodeInterface *new_child) = 0;

  virtual bool HasChildNodes() const = 0;
  virtual DOMNodeInterface *CloneNode(bool deep) const = 0;

  /**
   * Though Node.normalize() is in DOM2, DOM1 has only Element.normalize().
   * Declare it here for convenience.
   * We can prevent script program from accesssing it by only registering the
   * script method into Element class only.
   */
  virtual void Normalize() = 0;

  /**
   * Declare these methods here for convenience.
   * We can prevent script program from accesssing it by only registering the
   * script method into Element and Document classes only.
   *
   * Caller must delete or Ref/Unref the returned object.
   */
  virtual DOMNodeListInterface *GetElementsByTagName(
      const std::string &name) = 0;
  virtual const DOMNodeListInterface *GetElementsByTagName(
      const std::string &name) const = 0;

  /**
   * @c textContent is a DOM3 property.
   * Defined here for convenience and compatibility with the Windows version.
   */
  virtual std::string GetTextContent() const = 0;
  virtual void SetTextContent(const std::string &text_content) = 0;

  /** @c xml property is a Microsoft extension. */
  virtual std::string GetXML() const = 0;

  /**
   * Gets and sets the column and row numbers of this node where it is defined
   * in the source XML file.
   */
  virtual int GetRow() const = 0;
  virtual void SetRow(int row) = 0;
  virtual int GetColumn() const = 0;
  virtual void SetColumn(int column) = 0;

  /**
   * Part of DOM2 methods that are widely used in gadgets.
   * The standard requires to sometimes return NULL, but this method returns
   * blank string in the case.
   * JavaScript behavior still conforms the standard.
   */
  virtual std::string GetPrefix() const = 0;
  virtual DOMExceptionCode SetPrefix(const std::string &prefix) = 0;
  virtual std::string GetLocalName() const = 0;

  /**
   * Microsoft extension. The implementation may only support a very limited
   * subset of xpath grammar, just to make some gadget run without error.
   */
  virtual DOMNodeInterface *SelectSingleNode(const char *xpath) = 0;
  virtual const DOMNodeInterface *SelectSingleNode(const char *xpath) const = 0;

  /** Caller must delete or Ref/Unref the returned object. */
  virtual DOMNodeListInterface *SelectNodes(const char *xpath) = 0;
  virtual const DOMNodeListInterface *SelectNodes(const char *xpath) const = 0;

  /* TODO: DOM2
  virtual bool IsSupported(const char *feature, const char *version) const = 0;
  virtual std::string GetNamespaceURI() const = 0;
  virtual bool HasAttributes() const = 0;
  // Declare these methods here for convenience.
  virtual DOMNodeListInterface *GetElementsByTagNameNS(
      const std::string &namespace_uri, const std::string &local_name) = 0;
  virtual const DOMNodeListInterface *GetElementsByTagNameNS(
      const std::string &namespace_uri,
      const std::string &local_name) const = 0;
  */

  friend class internal::DOMNodeImpl;
};
CLASS_ID_IMPL(DOMNodeInterface, ScriptableInterface)

class DOMNodeListInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0x9935a8188f734afe);

  virtual DOMNodeInterface *GetItem(size_t index) = 0;
  virtual const DOMNodeInterface *GetItem(size_t index) const = 0;
  virtual size_t GetLength() const = 0;
};
CLASS_ID_IMPL(DOMNodeListInterface, ScriptableInterface)

class DOMNamedNodeMapInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0xd2c849db6fb6416f);

  virtual DOMNodeInterface *GetNamedItem(const std::string &name) = 0;
  virtual const DOMNodeInterface *GetNamedItem(
      const std::string &name) const = 0;
  virtual DOMExceptionCode SetNamedItem(DOMNodeInterface *arg) = 0;
  virtual DOMExceptionCode RemoveNamedItem(const std::string &name) = 0;
  virtual DOMNodeInterface *GetItem(size_t index) = 0;
  virtual const DOMNodeInterface *GetItem(size_t index) const = 0;
  virtual size_t GetLength() const = 0;

  /* TODO: DOM2
  virtual DOMNodeInterface *GetNamedItemNS(const std::string &namespace_uri,
                                           const std::string &local_name) = 0;
  virtual const DOMNodeInterface *GetNamedItemNS(
      const std::string &namespace_uri,
      const std::string &local_name) const = 0;
  virtual DOMExceptionCode SetNamedItemNS(DOMNodeInterface *arg) = 0;
  virtual DOMExceptionCode RemoveNamedItemNS(const std::string &namespace_uri,
                                             const std::string &local_name) = 0;
  */
};
CLASS_ID_IMPL(DOMNamedNodeMapInterface, ScriptableInterface)

class DOMCharacterDataInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0x199ea7a610e048b9);

  virtual UTF16String GetData() const = 0;
  virtual void SetData(const UTF16String &data) = 0;
  virtual size_t GetLength() const = 0;
  /** Not in DOM spec. IsEmpty() is more lightweight than GetLength() == 0. */
  virtual bool IsEmpty() const = 0;
  /** The caller should free the pointer with delete [] operator after use. */
  virtual DOMExceptionCode SubstringData(size_t offset, size_t count,
                                         UTF16String *result) const = 0;
  virtual void AppendData(const UTF16String &arg) = 0;
  virtual DOMExceptionCode InsertData(size_t offset,
                                      const UTF16String &arg) = 0;
  virtual DOMExceptionCode DeleteData(size_t offset, size_t count) = 0;
  virtual DOMExceptionCode ReplaceData(size_t offset, size_t count,
                                       const UTF16String &arg) = 0;
};
CLASS_ID_IMPL(DOMCharacterDataInterface, DOMNodeInterface)

class DOMAttrInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0xc1c04a2ea6ed45fc);

  virtual std::string GetName() const = 0;
  virtual bool IsSpecified() const = 0;
  virtual std::string GetValue() const = 0;
  virtual void SetValue(const std::string &value) = 0;

  /** DOM2 property, but useful. */
  virtual DOMElementInterface *GetOwnerElement() = 0;
  virtual const DOMElementInterface *GetOwnerElement() const = 0;
};
CLASS_ID_IMPL(DOMAttrInterface, DOMNodeInterface)

class DOMElementInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0x98722c98a65a4801);

  virtual std::string GetTagName() const = 0;
  virtual std::string GetAttribute(const std::string &name) const = 0;
  virtual DOMExceptionCode SetAttribute(const std::string &name,
                                        const std::string &value) = 0;
  virtual void RemoveAttribute(const std::string &name) = 0;
  virtual DOMAttrInterface *GetAttributeNode(const std::string &name) = 0;
  virtual const DOMAttrInterface *GetAttributeNode(
      const std::string &name) const = 0;
  virtual DOMExceptionCode SetAttributeNode(DOMAttrInterface *new_attr) = 0;
  virtual DOMExceptionCode RemoveAttributeNode(DOMAttrInterface *old_attr) = 0;
  // GetElementsByTagName has been declared in DOMNodeInterface.
  virtual DOMNamedNodeMapInterface *GetAttributes() = 0;
  virtual const DOMNamedNodeMapInterface *GetAttributes() const = 0;

  /* TODO: DOM2
  virtual std::string GetAttributeNS(const std::string &namespace_uri,
                                     const std::string &local_name) = 0;
  virtual DOMExceptionCode SetAttributeNS(const std::string &namespace_uri,
                                          const std::string &qualified_name,
                                          const std::string &value) = 0;
  virtual DOMExceptionCode RemoveAttributeNS(const std::string &namespace_uri,
                                             const std::string &local_name) = 0;
  virtual DOMAttrInterface *GetAttributeNodeNS(
      const std::string &namespace_uri,
      const std::string &local_name) = 0;
  virtual const DOMAttrInterface *GetAttributeNodeNS(
      const std::string &namespace_uri,
      const std::string &local_name) const = 0;
  virtual DOMExceptionCode SetAttributeNodeNS(DOMAttrInterface *new_attr) = 0;
  // GetElementsByTagNameNS has been declared in DOMNodeInterface.
  virtual bool HasAttribute(const std::string &name) const = 0;
  virtual bool HasAttributeNS(const std::string &namespace_uri,
                              const std::string &local_name) const = 0;
  */
};
CLASS_ID_IMPL(DOMElementInterface, DOMNodeInterface)

class DOMTextInterface : public DOMCharacterDataInterface {
 public:
  CLASS_ID_DECL(0x401b780c290c4525);
  virtual DOMExceptionCode SplitText(size_t offset,
                                     DOMTextInterface **new_text) = 0;
};
CLASS_ID_IMPL(DOMTextInterface, DOMCharacterDataInterface)

class DOMCommentInterface : public DOMCharacterDataInterface {
 public:
  CLASS_ID_DECL(0x2be4711d5e9b4400);
};
CLASS_ID_IMPL(DOMCommentInterface, DOMCharacterDataInterface)

class DOMCDATASectionInterface : public DOMTextInterface {
 public:
  CLASS_ID_DECL(0x16ce6e727f694f7b);
};
CLASS_ID_IMPL(DOMCDATASectionInterface, DOMTextInterface)

class DOMDocumentFragmentInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0x349f983c7e1c4407);
};
CLASS_ID_IMPL(DOMDocumentFragmentInterface, DOMNodeInterface)

class DOMDocumentTypeInterface;
class DOMEntityReferenceInterface;

class DOMProcessingInstructionInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0xb3f35b20e5854943);

  virtual std::string GetTarget() const = 0;
  virtual std::string GetData() const = 0;
  virtual void SetData(const std::string &data) = 0;
};
CLASS_ID_IMPL(DOMProcessingInstructionInterface, DOMNodeInterface)

class DOMImplementationInterface : public ScriptableInterface {
 public:
  CLASS_ID_DECL(0x92586d525bf34b13);

  virtual bool HasFeature(const char *feature, const char *version) const = 0;

  /* TODO: DOM2
  virtual DOMExceptionCode CreateDocumentType(
      const std::string &qualified_name,
      const std::string &public_id, const std::string &system_id,
      DOMDocumentTypeInterface **result) = 0;
  virtual DOMExceptionCode CreateDocument(
      const std::string &namespace_uri, const std::string &qualified_name,
      const DOMDocumentTypeInterface *doctype,
      DOMDocumentInterface **result) = 0;
  */
};
CLASS_ID_IMPL(DOMImplementationInterface, ScriptableInterface)

class DOMDocumentInterface : public DOMNodeInterface {
 public:
  CLASS_ID_DECL(0x885f4371c0024a79);

  virtual DOMDocumentTypeInterface *GetDoctype() = 0;
  virtual const DOMDocumentTypeInterface *GetDoctype() const = 0;
  virtual DOMImplementationInterface *GetImplementation() = 0;
  virtual const DOMImplementationInterface *GetImplementation() const = 0;
  virtual DOMElementInterface *GetDocumentElement() = 0;
  virtual const DOMElementInterface *GetDocumentElement() const = 0;
  virtual DOMExceptionCode CreateElement(const std::string &tag_name,
                                         DOMElementInterface **result) = 0;
  virtual DOMDocumentFragmentInterface *CreateDocumentFragment() = 0;
  virtual DOMTextInterface *CreateTextNode(const UTF16String &data) = 0;
  virtual DOMTextInterface *CreateTextNodeUTF8(const std::string &data) = 0;
  virtual DOMCommentInterface *CreateComment(const UTF16String &data) = 0;
  virtual DOMCommentInterface *CreateCommentUTF8(const std::string &data) = 0;
  virtual DOMCDATASectionInterface *CreateCDATASection(
      const UTF16String &data) = 0;
  virtual DOMCDATASectionInterface *CreateCDATASectionUTF8(
      const std::string &data) = 0;
  virtual DOMExceptionCode CreateProcessingInstruction(
      const std::string &target, const std::string &data,
      DOMProcessingInstructionInterface **result) = 0;
  virtual DOMExceptionCode CreateAttribute(const std::string &name,
                                           DOMAttrInterface **result) = 0;
  virtual DOMExceptionCode CreateEntityReference(
      const std::string &name, DOMEntityReferenceInterface **result) = 0;
  // GetElementsByName is declared in DOMNodeInterface.

  /**
   * Load from xml string and build up the DOM document.
   * If the document has already contents, they will be removed first.
   * This method is not defined in DOM spec. Defined here for convenience and
   * compatibility with the Windows version.
   * @param xml a string containing XML definition.
   * @return @c true if succeeded.
   */
  virtual bool LoadXML(const std::string &xml) = 0;

  virtual XMLParserInterface *GetXMLParser() const = 0;

  // A useful DOM2 method.
  virtual DOMExceptionCode ImportNode(const DOMNodeInterface *imported_node,
                                      bool deep,
                                      DOMNodeInterface **result) = 0;

  /** Microsoft extension: property preserveWhitespace. */
  virtual bool PreservesWhiteSpace() const = 0;
  virtual void SetPreserveWhiteSpace(bool preserve_whitespace) = 0;

  /* TODO: DOM2
  virtual DOMExceptionCode CreateElementNS(const std::string &namespace_uri,
                                           const std::string &qualified_name,
                                           DOMElementInterface **result) = 0;
  virtual DOMExceptionCode CreateAttributeNS(const std::string &namespace_uri,
                                             const std::string &qualified_name,
                                             DOMAttrInterface **result) = 0;
  // GetElementsByNameNS is declared in DOMNodeInterface.
  virtual DOMElementInterface *GetElementById(
      const std::string &element_id) = 0;
  virtual const DOMElementInterface *GetElementById(
      const std::string &element_id) const = 0;
  */
};
CLASS_ID_IMPL(DOMDocumentInterface, DOMNodeInterface)

/** @} */

} // namespace ggadget

#endif // GGADGET_XML_DOM_INTERFACE_H__
