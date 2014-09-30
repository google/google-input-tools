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

#include <algorithm>
#include <vector>
#include <ggadget/gadget_consts.h>
#include <ggadget/logger.h>
#include <ggadget/scriptable_helper.h>
#include <ggadget/signals.h>
#include <ggadget/string_utils.h>
#include <ggadget/system_utils.h>
#include <ggadget/xml_http_request_interface.h>
#include <ggadget/xml_parser_interface.h>
#include <ggadget/xml_dom_interface.h>

namespace ggadget {
namespace internal {

// Constants for XML pretty printing.
static const size_t kLineLengthThreshold = 70;
static const size_t kIndent = 1;
static const char *kStandardXMLDecl =
    "<?xml version=\"1.0\" encoding=\"utf-8\"?>";

static const char *kExceptionNames[] = {
    "",
    "INDEX_SIZE_ERR",
    "DOMSTRING_SIZE_ERR",
    "HIERARCHY_REQUEST_ERR",
    "WRONG_DOCUMENT_ERR",
    "INVALID_CHARACTER_ERR",
    "NO_DATA_ALLOWED_ERR",
    "NO_MODIFICATION_ALLOWED_ERR",
    "NOT_FOUND_ERR",
    "NOT_SUPPORTED_ERR",
    "INUSE_ATTRIBUTE_ERR",
};

class GlobalException : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x81f363ca1c034f39, ScriptableInterface);
  static GlobalException *Get() {
    static GlobalException global_exception;
    return &global_exception;
  }
 private:
  GlobalException() {
    RegisterConstants(arraysize(kExceptionNames), kExceptionNames, NULL);
  }
};

static const char *kNodeTypeNames[] = {
    ""
    "ELEMENT_NODE",
    "ATTRIBUTE_NODE",
    "TEXT_NODE",
    "CDATA_SECTION_NODE",
    "ENTITY_REFERENCE_NODE",
    "ENTITY_NODE",
    "PROCESSING_INSTRUCTION_NODE",
    "COMMENT_NODE",
    "DOCUMENT_NODE",
    "DOCUMENT_TYPE_NODE",
    "DOCUMENT_FRAGMENT_NODE",
    "NOTATION_NODE",
};

class GlobalNode : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0x2a9d299fb51c4070, ScriptableInterface);
  static GlobalNode *Get() {
    static GlobalNode global_node;
    return &global_node;
  }
 private:
  GlobalNode() {
    RegisterConstants(arraysize(kNodeTypeNames), kNodeTypeNames, NULL);
  }
};

class DOMException : public ScriptableHelperDefault {
 public:
  DEFINE_CLASS_ID(0x6486921444b44784, ScriptableInterface);

  // Use 'int' instead of DOMExceptionCode to allow arbitrary code.
  DOMException(int code) : code_(code) {
    SetInheritsFrom(GlobalException::Get());
  }

  // Required by webkit-script-runtime.
  virtual bool IsStrict() const { return false; }

  virtual void DoClassRegister() {
    RegisterProperty("code", NewSlot(&DOMException::GetCode), NULL);
    RegisterMethod("toString", NewSlot(&DOMException::ToString));
  }

  std::string ToString() const {
    const char *exception_name = code_ >= 0 &&
        static_cast<size_t>(code_) < arraysize(kExceptionNames) ?
            kExceptionNames[code_] : "unknown";
    return StringPrintf("DOMException: %d(%s)", code_, exception_name);
  }

  int GetCode() const { return code_; }

 private:
  int code_;
};

// Used in the methods for script to throw an script exception on errors.
template <typename T>
static bool GlobalCheckException(T *owner, DOMExceptionCode code) {
  if (code != DOM_NO_ERR) {
    DLOG("Throw DOMException: %d", code);
    owner->SetPendingException(new DOMException(code));
    return false;
  }
  return true;
}

// Check if child type is acceptable for Element, DocumentFragment,
// EntityReference and Entity nodes.
static DOMExceptionCode CheckCommonChildType(DOMNodeInterface *new_child) {
  DOMNodeInterface::NodeType type = new_child->GetNodeType();
  if (type != DOMNodeInterface::ELEMENT_NODE &&
      type != DOMNodeInterface::TEXT_NODE &&
      type != DOMNodeInterface::COMMENT_NODE &&
      type != DOMNodeInterface::PROCESSING_INSTRUCTION_NODE &&
      type != DOMNodeInterface::CDATA_SECTION_NODE &&
      type != DOMNodeInterface::ENTITY_REFERENCE_NODE)
    return DOM_HIERARCHY_REQUEST_ERR;
  return DOM_NO_ERR;
}

class DOMNodeListBase : public ScriptableHelper<DOMNodeListInterface> {
 public:
  using DOMNodeListInterface::GetItem;
  DOMNodeListBase() {
    SetArrayHandler(NewSlot(implicit_cast<DOMNodeListInterface *>(this),
                            &DOMNodeListInterface::GetItem),
                    NULL);
  }
  virtual void DoClassRegister() {
    RegisterProperty("length", NewSlot(&DOMNodeListInterface::GetLength), NULL);
    RegisterMethod("item", NewSlot(&DOMNodeListBase::GetItemNotConst));
    // Microsoft compatibility.
    RegisterMethod("", NewSlot(&DOMNodeListBase::GetItemNotConst));
  }
 private:
  DOMNodeInterface *GetItemNotConst(size_t index) { return GetItem(index); }
};

// Append a '\n' and indent to xml.
static void AppendIndentNewLine(size_t indent, std::string *xml) {
  if (!xml->empty() && *(xml->end() - 1) != '\n')
    xml->append(1, '\n');
  xml->append(indent * 2, ' ');
}

// Append indent if the current position is a new line.
static void AppendIndentIfNewLine(size_t indent, std::string *xml) {
  if (xml->empty() || *(xml->end() - 1) == '\n')
    xml->append(indent * 2, ' ');
}

// Interface for DOMNodeImpl callbacks to its node.
class DOMNodeImplCallbacks {
 public:
  virtual ~DOMNodeImplCallbacks() { }
  // Subclasses should implement these methods.
  virtual DOMNodeInterface *CloneSelf(DOMDocumentInterface *owner_document) = 0;
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) = 0;
  // Append the XML string representation to the string.
  virtual void AppendXML(size_t indent, std::string *xml) = 0;
  virtual bool CheckException(DOMExceptionCode code) = 0;
  // Called when any methods related method is called.
  // Subclasses can do lazy children init in its implementation.
  virtual void UpdateChildren() = 0;
};

class DOMNodeImpl : public SmallObject<> {
 public:
  typedef std::vector<DOMNodeInterface *> Children;

  DOMNodeImpl(DOMNodeInterface *node,
              DOMNodeImplCallbacks *callbacks,
              DOMDocumentInterface *owner_document,
              const std::string &name)
      : node_(node),
        callbacks_(callbacks),
        owner_document_(owner_document),
        parent_(NULL),
        owner_node_(NULL),
        previous_sibling_(NULL), next_sibling_(NULL),
        row_(0), column_(0) {
    ASSERT(!name.empty());
    if (!SplitString(name, ":", &prefix_, &local_name_)) {
      ASSERT(local_name_.empty());
      local_name_.swap(prefix_);
    }
    // Pointer comparison is intended here.
    if (name != kDOMDocumentName) {
      ASSERT(owner_document_);
      // Any newly created node has no parent and thus is orphan. Increase the
      // document orphan count.
      owner_document_->Ref();
    }
  }

  virtual ~DOMNodeImpl() {
    if (!owner_node_ && owner_document_) {
      // The node is still an orphan. Remove the document orphan count.
      owner_document_->Unref();
    }

    for (Children::iterator it = children_.begin();
         it != children_.end(); ++it) {
      // At this time, the refcount of all children should have already been
      // reached 0 but not deleted because the last ref was removed transiently.
      delete *it;
    }
    children_.clear();
    ASSERT(on_element_tree_changed_.GetConnectionCount() == 0);
  }

  DOMNodeListInterface *GetChildNodes() {
    callbacks_->UpdateChildren();
    return new ChildrenNodeList(node_, children_);
  }
  DOMNodeInterface *GetFirstChild() {
    callbacks_->UpdateChildren();
    return children_.empty() ? NULL : children_.front();
  }
  DOMNodeInterface *GetLastChild() {
    callbacks_->UpdateChildren();
    return children_.empty() ? NULL : children_.back();
  }
  DOMNodeInterface *GetPreviousSibling() {
    return previous_sibling_ ? previous_sibling_->node_ : NULL;
  }
  DOMNodeInterface *GetNextSibling() {
    return next_sibling_ ? next_sibling_->node_ : NULL;
  }

  DOMExceptionCode InsertBeforeInternal(DOMNodeInterface *new_child,
                                        DOMNodeInterface *ref_child) {
    if (!new_child)
      return DOM_NULL_POINTER_ERR;

    if (ref_child && ref_child->GetParentNode() != node_)
      return DOM_NOT_FOUND_ERR;

    if (new_child->GetNodeType() == DOMNodeInterface::DOCUMENT_FRAGMENT_NODE) {
      const Children &children = new_child->GetImpl()->children_;
      DOMExceptionCode code = DOM_NO_ERR;
      while (children.size() > 0) {
        code = InsertBefore(children[0], ref_child);
        if (code != DOM_NO_ERR)
          break;
      }
      return code;
    }

    DOMExceptionCode code = callbacks_->CheckNewChild(new_child);
    if (code != DOM_NO_ERR)
      return code;

    if (new_child == ref_child)
      return DOM_NO_ERR;

    // To invalidate cached ElementsByName.
    if (new_child->GetNodeType() == DOMNodeInterface::ELEMENT_NODE)
      OnElementTreeChanged();

    // Remove the new_child from its old parent.
    DOMNodeInterface *old_parent = new_child->GetParentNode();
    if (old_parent) {
      // Add a temporary ref to prevent new_child from being deleted.
      new_child->Ref();
      old_parent->RemoveChild(new_child);
      new_child->Unref(true);
    }

    DOMNodeImpl *new_child_impl = new_child->GetImpl();
    DOMNodeImpl *prev_child_impl = NULL;
    if (ref_child) {
      DOMNodeImpl *ref_child_impl = ref_child->GetImpl();
      if (ref_child_impl->previous_sibling_)
        prev_child_impl = ref_child_impl->previous_sibling_;
      new_child_impl->next_sibling_ = ref_child_impl;
      ref_child_impl->previous_sibling_ = new_child_impl;
      children_.insert(FindChild(ref_child), new_child);
    } else {
      if (!children_.empty())
        prev_child_impl = children_.back()->GetImpl();
      children_.push_back(new_child);
    }
    if (prev_child_impl) {
      prev_child_impl->next_sibling_ = new_child_impl;
      new_child_impl->previous_sibling_ = prev_child_impl;
    }

    new_child_impl->SetParent(node_);
    return DOM_NO_ERR;
  }

  DOMExceptionCode InsertBefore(DOMNodeInterface *new_child,
                                DOMNodeInterface *ref_child) {
    callbacks_->UpdateChildren();
    return InsertBeforeInternal(new_child, ref_child);
  }

  DOMExceptionCode ReplaceChild(DOMNodeInterface *new_child,
                                DOMNodeInterface *old_child) {
    if (!new_child || !old_child)
      return DOM_NULL_POINTER_ERR;
    if (old_child->GetParentNode() != node_)
      return DOM_NOT_FOUND_ERR;
    if (new_child == old_child)
      return DOM_NO_ERR;

    DOMExceptionCode code = InsertBefore(new_child, old_child);
    if (code != DOM_NO_ERR)
      return code;

    return RemoveChild(old_child);
  }

  DOMExceptionCode RemoveChild(DOMNodeInterface *old_child) {
    if (!old_child)
      return DOM_NULL_POINTER_ERR;
    if (old_child->GetParentNode() != node_)
      return DOM_NOT_FOUND_ERR;

    // To invalidate cached ElementsByName.
    if (old_child->GetNodeType() == DOMNodeInterface::ELEMENT_NODE)
      OnElementTreeChanged();

    children_.erase(FindChild(old_child));
    DOMNodeImpl *old_child_impl = old_child->GetImpl();
    DOMNodeImpl *prev_child_impl = old_child_impl->previous_sibling_;
    DOMNodeImpl *next_child_impl = old_child_impl->next_sibling_;
    if (prev_child_impl)
      prev_child_impl->next_sibling_ = next_child_impl;
    if (next_child_impl)
      next_child_impl->previous_sibling_ = prev_child_impl;
    old_child_impl->previous_sibling_ = NULL;
    old_child_impl->next_sibling_ = NULL;
    old_child->GetImpl()->SetParent(NULL);
    return DOM_NO_ERR;
  }

  DOMNodeInterface *CloneNode(DOMDocumentInterface *owner_document, bool deep) {
    DOMNodeInterface *self_cloned = callbacks_->CloneSelf(owner_document);
    if (self_cloned && deep) {
      for (Children::iterator it = children_.begin();
           it != children_.end(); ++it) {
        // Ignore error returned from AppendChild, since it should not occur.
        self_cloned->AppendChild(
            (*it)->GetImpl()->CloneNode(owner_document, deep));
      }
    }
    return self_cloned;
  }

  void Normalize() {
    for (size_t i = 0; i < children_.size(); i++) {
      DOMNodeInterface *child = children_[i];
      if (child->GetNodeType() == DOMNodeInterface::TEXT_NODE) {
        DOMTextInterface *text = down_cast<DOMTextInterface *>(child);
        if (text->IsEmpty()) {
          // Remove empty text nodes.
          RemoveChild(text);
          i--;
        } else if (i > 0) {
          DOMNodeInterface *last_child = children_[i - 1];
          if (last_child->GetNodeType() == DOMNodeInterface::TEXT_NODE) {
            // Merge the two node into one.
            DOMTextInterface *text0 = down_cast<DOMTextInterface *>(last_child);
            text0->InsertData(text0->GetLength(), text->GetData());
            RemoveChild(text);
            i--;
          }
        }
      } else {
        child->Normalize();
      }
    }
  }

  std::string GetTextContentPreserveWhiteSpace() {
    return node_->AllowsNodeValue() ? node_->GetNodeValue() :
                                      GetChildrenTextContent();
  }

  std::string GetChildrenTextContent() {
    std::string result;
    for (Children::iterator it = children_.begin();
         it != children_.end(); ++it) {
      DOMNodeInterface::NodeType type = (*it)->GetNodeType();
      if (type != DOMNodeInterface::COMMENT_NODE &&
          type != DOMNodeInterface::PROCESSING_INSTRUCTION_NODE) {
        // White spaces in child nodes are preserved.
        result += (*it)->GetImpl()->GetTextContentPreserveWhiteSpace();
      }
    }
    return result;
  }

  void SetChildTextContent(const std::string &text_content) {
    RemoveAllChildren();
    // Don't call InsertBefore because this method may be called from
    // UpdateChildren() which is from InsertBefore().
    // The caller should ensure the correct status of children before calling
    // this method.
    InsertBeforeInternal(owner_document_->CreateTextNodeUTF8(text_content),
                         NULL);
  }

  std::string GetXML() {
    std::string result;
    result.reserve(256);
    callbacks_->AppendXML(0, &result);
    return result;
  }

  std::string GetNodeName() {
    return prefix_.empty() ? local_name_ : prefix_ + ":" + local_name_;
  }

  DOMExceptionCode SetPrefix(const std::string &prefix) {
    if (prefix.empty()) {
      prefix_.clear();
    } else if (owner_document_->GetXMLParser()->CheckXMLName(prefix.c_str())) {
      prefix_ = prefix;
    } else {
      return DOM_INVALID_CHARACTER_ERR;
    }
    return DOM_NO_ERR;
  }

  DOMNodeListInterface *GetElementsByTagName(const std::string &name) {
    return new ElementsByTagName(node_, name);
  }

  DOMNodeInterface *SelectSingleNode(const char *xpath) {
    if (!xpath || !*xpath)
      return NULL;
    DOMNodeInterface *context_node = node_;
    if (*xpath == '/') {
      // If owner_document_ is NULL, the node itself is a document.
      context_node = owner_document_ ? owner_document_ : node_;
      if (!xpath[1])
        return context_node;
      // xpath + 1 is "xpath_tail" as defined in SelectNodesResult.
      xpath++;
    }
    SelectNodesResult nodes(context_node, xpath, true);
    return nodes.GetItem(0);
  }

  DOMNodeListInterface *SelectNodes(const char *xpath) {
    if (!xpath || !*xpath)
      return new EmptyNodeList();
    DOMNodeInterface *context_node = node_;
    if (*xpath == '/') {
      // If owner_document_ is NULL, the node itself is a document.
      context_node = owner_document_ ? owner_document_ : node_;
      if (!xpath[1])
        return new SingleNodeList(context_node);
      // xpath + 1 is "xpath_tail" as defined in SelectNodesResult.
      xpath++;
    }
    return new SelectNodesResult(context_node, xpath, false);
  }

 public:
  // The following public methods are utilities for interface implementations.
  void AppendChildrenXML(size_t indent, std::string *xml) {
    for (Children::iterator it = children_.begin();
         it != children_.end(); ++it) {
      (*it)->GetImpl()->callbacks_->AppendXML(indent, xml);
    }
  }

  void RemoveAllChildren() {
    for (Children::iterator it = children_.begin();
         it != children_.end(); ++it) {
      DOMNodeImpl *child_impl = (*it)->GetImpl();
      child_impl->previous_sibling_ = NULL;
      child_impl->next_sibling_ = NULL;
      (*it)->GetImpl()->SetParent(NULL);
    }
    children_.clear();
  }

  DOMExceptionCode CheckNewChildCommon(DOMNodeInterface *new_child) {
    // The new_child must be in the same document of this node.
    DOMDocumentInterface *new_child_doc = new_child->GetOwnerDocument();
    if ((owner_document_ && new_child_doc != owner_document_) ||
        // In case of the current node is the document.
        (!owner_document_ && new_child_doc != node_)) {
      DLOG("CheckNewChildCommon: Wrong document");
      return DOM_WRONG_DOCUMENT_ERR;
    }

    // The new_child can't be this node itself or one of this node's ancestors.
    for (DOMNodeInterface *ancestor = node_; ancestor != NULL;
         ancestor = ancestor->GetParentNode()) {
      if (ancestor == new_child) {
        DLOG("CheckNewChildCommon: New child is self or ancestor");
        return DOM_HIERARCHY_REQUEST_ERR;
      }
    }

    return DOM_NO_ERR;
  }

  Children::iterator FindChild(DOMNodeInterface *child) {
    ASSERT(child && child->GetParentNode() == node_);
    Children::iterator it = std::find(children_.begin(),
                                      children_.end(), child);
    ASSERT(it != children_.end());
    return it;
  }

  Variant ScriptGetNodeValue() {
    return node_->AllowsNodeValue() ? Variant(node_->GetNodeValue()) :
                                      Variant(static_cast<const char *>(NULL));
  }

  void ScriptSetNodeValue(const Variant &value) {
    std::string value_str;
    value.ConvertToString(&value_str);
    callbacks_->CheckException(node_->SetNodeValue(value_str));
  }

  Variant ScriptGetPrefix() {
    std::string prefix = node_->GetPrefix();
    return prefix.empty() ? Variant(static_cast<const char *>(NULL)) :
                             Variant(prefix);
  }

  void ScriptSetPrefix(const Variant &prefix) {
    std::string prefix_str;
    prefix.ConvertToString(&prefix_str);
    callbacks_->CheckException(node_->SetPrefix(prefix_str));
  }

  DOMNodeInterface *ScriptInsertBefore(DOMNodeInterface *new_child,
                                       DOMNodeInterface *ref_child) {
    return callbacks_->CheckException(InsertBefore(new_child, ref_child))?
           new_child : NULL;
  }

  DOMNodeInterface *ScriptReplaceChild(DOMNodeInterface *new_child,
                                       DOMNodeInterface *old_child) {
    // Add a transient reference to avoid the node from being deleted.
    if (old_child)
      old_child->Ref();
    DOMExceptionCode code = ReplaceChild(new_child, old_child);
    // Remove the transient reference but still keep the node if there is no
    // error.
    if (old_child)
      old_child->Unref(code == DOM_NO_ERR);
    return callbacks_->CheckException(code) ? old_child : NULL;
  }

  DOMNodeInterface *ScriptRemoveChild(DOMNodeInterface *old_child) {
    // Add a transient reference to avoid the node from being deleted.
    if (old_child)
      old_child->Ref();
    DOMExceptionCode code = RemoveChild(old_child);
    // Remove the transient reference but still keep the node.
    if (old_child)
      old_child->Unref(code == DOM_NO_ERR);
    return callbacks_->CheckException(code) ? old_child : NULL;
  }

  DOMNodeInterface *ScriptAppendChild(DOMNodeInterface *new_child) {
    return ScriptInsertBefore(new_child, NULL);
  }

  void SetParent(DOMNodeInterface *new_parent) {
    parent_ = new_parent;
    SetOwnerNode(new_parent);
  }

  // The implementation classes must call this method when the owner node
  // changes. In most cases, the owner node is the parent node, but DOMAttr
  // is an exception, whose owner node is the owner element.
  void SetOwnerNode(DOMNodeInterface *new_owner) {
    if (owner_node_ != new_owner) {
      int ref_count = node_->GetRefCount();
      if (owner_node_) {
        // This node is detached from the old owner node.
        for (int i = 0; i < ref_count; i++)
          owner_node_->Unref();

        if (!new_owner) {
          // This node becomes a new orphan.
          if (node_->GetRefCount() == 0) {
            // This orphan is not referenced, delete it now.
            delete node_;
            return;
          } else {
            // This orphan is still referenced. Increase the document
            // orphan count.
            owner_document_->Ref();
          }
        }
      }

      if (new_owner) {
        // This node is attached to the new owner node.
        for (int i = 0; i < ref_count; i++)
          new_owner->Ref();

        if (!owner_node_) {
          // The node is no longer orphan root, so decrease the document
          // orphan count.
          owner_document_->Unref();
        }
      }
      owner_node_ = new_owner;
    }
  }

  // The DOMNodeList used to enumerate this node's children.
  class ChildrenNodeList : public DOMNodeListBase {
   public:
    DEFINE_CLASS_ID(0x72b1fc54e58041ae, DOMNodeListInterface);

    ChildrenNodeList(DOMNodeInterface *node,
                     const Children &children)
        : node_(node), children_(children) {
      node->Ref();
    }
    virtual ~ChildrenNodeList() {
      node_->Unref();
    }

    DOMNodeInterface *GetItem(size_t index) {
      return index < children_.size() ? children_[index] : NULL;
    }
    const DOMNodeInterface *GetItem(size_t index) const {
      return index < children_.size() ? children_[index] : NULL;
    }
    size_t GetLength() const { return children_.size(); }

   private:
    DOMNodeInterface *node_;
    const Children &children_;
  };

  class EmptyNodeList : public DOMNodeListBase {
   public:
    DEFINE_CLASS_ID(0xd59e03c958194bd8, DOMNodeListInterface);
    virtual DOMNodeInterface *GetItem(size_t index) {
      GGL_UNUSED(index);
      return NULL;
    }
    virtual const DOMNodeInterface *GetItem(size_t index) const {
      GGL_UNUSED(index);
      return NULL;
    }
    virtual size_t GetLength() const { return 0; }
  };

  class SingleNodeList : public DOMNodeListBase {
   public:
    DEFINE_CLASS_ID(0x73bbfa5e3ed64537, DOMNodeListInterface);
    SingleNodeList(DOMNodeInterface *node) : node_(node) { node->Ref(); }
    virtual ~SingleNodeList() {  node_->Unref(); }
    virtual DOMNodeInterface *GetItem(size_t index) {
      return index == 0 ? node_ : NULL;
    }
    virtual const DOMNodeInterface *GetItem(size_t index) const {
      return index == 0 ? node_ : NULL;
    }
    virtual size_t GetLength() const { return 1; }
   private:
    DOMNodeInterface *node_;
  };

  class CachedDOMNodeListBase : public DOMNodeListBase {
   public:
    CachedDOMNodeListBase(DOMNodeInterface *node)
        : node_(node),
          valid_(false) {
      node_->Ref();
      on_invalidate_connection_ =
          node->GetImpl()->on_element_tree_changed_.Connect(
              NewSlot(this, &SelectNodesResult::Invalidate));
    }
    virtual ~CachedDOMNodeListBase() {
      on_invalidate_connection_->Disconnect();
      node_->Unref();
    }

    virtual DOMNodeInterface *GetItem(size_t index) {
      EnsureValid();
      return index >= nodes_.size() ? NULL : nodes_[index];
    }
    virtual const DOMNodeInterface *GetItem(size_t index) const {
      EnsureValid();
      return index >= nodes_.size() ? NULL : nodes_[index];
    }

    virtual size_t GetLength() const {
      EnsureValid();
      return nodes_.size();
    }

   protected:
    virtual void Refresh() const = 0;

    void EnsureValid() const {
      if (!valid_) {
        Refresh();
        valid_ = true;
      }
    }

    void Invalidate() {
      valid_ = false;
      nodes_.clear();
    }

    DOMNodeInterface *node_;
    mutable bool valid_;
    mutable std::vector<DOMNodeInterface *> nodes_;
    Connection *on_invalidate_connection_;
  };

  // The DOMNodeList used as the return value of GetElementsByTagName().
  class ElementsByTagName : public CachedDOMNodeListBase {
   public:
    DEFINE_CLASS_ID(0x08b36d84ae044941, DOMNodeListInterface);

    ElementsByTagName(DOMNodeInterface *node, const std::string &name)
        : CachedDOMNodeListBase(node),
          name_(name),
          wildcard_(name == "*") {
    }

   protected:
    virtual void Refresh() const {
      DoRefresh(node_);
    }

    void DoRefresh(DOMNodeInterface *node) const {
      for (DOMNodeInterface *item = node->GetFirstChild(); item;
           item = item->GetNextSibling()) {
        if (item->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
          if (wildcard_ || name_ == item->GetNodeName())
            nodes_.push_back(item);
          DoRefresh(item);
        }
      }
    }

    std::string name_;
    bool wildcard_;
  };

  // The DOMNodeList used as the return value of SelectNodes().
  class SelectNodesResult : public CachedDOMNodeListBase {
   public:
    DEFINE_CLASS_ID(0xefd4339aee1340dc, DOMNodeListInterface);

    // xpath_tail is the xpath part after the leading "/" or "./".
    // For example, if the input xpath is "/title", the caller must pass the
    // document as context_node, and "title" as xpath_tail.
    // If the input xpath is ".//title", the caller must pass the current node
    // as context_node, and "/title" as xpath_tail (so a leading '/' doesn't
    // mean the document context, but means recursive descent selection.
    SelectNodesResult(DOMNodeInterface *context_node, const char *xpath_tail,
                      bool first_only)
        : CachedDOMNodeListBase(context_node),
          xpath_tail_(xpath_tail),
          first_only_(first_only) {
    }

   protected:
    virtual void Refresh() const {
      DoRefresh(xpath_tail_.c_str(), node_);
    }

    bool DoRefresh(const char *xpath_tail, DOMNodeInterface *node) const {
      if (!*xpath_tail)
        return true;
      const char *name = xpath_tail;
      const char *name_end = strchr(name, '/');
      int recursive_descent = 0;
      if (name_end == name) {
        // Leading '/' means descent selection because the other '/' has been
        // removed from xpath_tail.
        name++;
        if (*name == '/')
          return false; // Invalid xpath.
        name_end = strchr(name, '/');
        recursive_descent = 1;
      }
      size_t name_length = name_end ? name_end - name : strlen(name);
      if (name_length == 0)
        return true;

      if (*name == '.') {
        if (!name[1]) {
          nodes_.push_back(node);
          if (first_only_)
            return false;
          if (recursive_descent == 0)
            return true;
          recursive_descent = 2; // Descent only.
        } else if (name[1] != '/' || !name_end) {
          // Unsupported xpath grammar.
          return true;
        } else if (!DoRefresh(name_end + 1, node)) {
          return false;
        }
      }

      // FIXME: for an xpath like "...a//b...", and document <a><a><b></a></a>,
      // the following algorithm will return two instances of element b.
      bool is_wildcard = *name == '*' && name_length == 1;
      for (DOMNodeInterface *item = node->GetFirstChild(); item;
           item = item->GetNextSibling()) {
        if (item->GetNodeType() == DOMNodeInterface::ELEMENT_NODE) {
          bool name_matched = false;
          if (recursive_descent != 2 &&
              (is_wildcard ||
               strncmp(item->GetNodeName().c_str(), name, name_length) == 0)) {
            if (!name_end) {
              // A matching element found.
              nodes_.push_back(item);
              if (first_only_)
                return false;
            }
            name_matched = true;
          }
          if (recursive_descent && !DoRefresh(xpath_tail, item))
            return false;
          if (name_matched && name_end && !DoRefresh(name_end + 1, item))
            return false;
        }
      }
      return true;
    }

    std::string xpath_tail_;
    bool first_only_;
  };

  void OnElementTreeChanged() {
    on_element_tree_changed_();
    // Pass up the message to ancestors.
    if (parent_)
      parent_->GetImpl()->OnElementTreeChanged();
  }

  // In fact, node_ and callbacks_ points to the same object.
  DOMNodeInterface *node_;
  DOMNodeImplCallbacks *callbacks_;
  DOMDocumentInterface *owner_document_;
  std::string prefix_;
  std::string local_name_;
  DOMNodeInterface *parent_;
  // In most cases, owner_node_ == parent_, but for DOMAttr, owner_node_ is the
  // owner element.
  DOMNodeInterface *owner_node_;
  Children children_;
  DOMNodeImpl *previous_sibling_, *next_sibling_;
  int row_, column_;
  Signal0<void> on_element_tree_changed_;
};

template <typename Interface>
class DOMNodeBase : public ScriptableHelper<Interface>,
                    public DOMNodeImplCallbacks {
 public:
  // The following is to overcome template inheritance limits.
  typedef ScriptableHelper<Interface> Super;
  using Super::RegisterConstant;
  using Super::RegisterProperty;
  using Super::RegisterSimpleProperty;
  using Super::RegisterReadonlySimpleProperty;
  using Super::RegisterMethod;
  using Super::SetInheritsFrom;
  using Super::SetDynamicPropertyHandler;
  using Super::SetArrayHandler;

  DOMNodeBase(DOMDocumentInterface *owner_document,
              const std::string &name)
      : impl_(new DOMNodeImpl(this, this, owner_document, name)) {
    SetInheritsFrom(GlobalNode::Get());
  }

  virtual void DoClassRegister() {
    // "baseName" is not in W3C standard. Register it to keep compatibility
    // with the Windows DOM.
    RegisterProperty("baseName", NewSlot(&DOMNodeBase::GetLocalName), NULL);
    // Windows DOM.
    RegisterProperty("xml", NewSlot(&DOMNodeBase::GetXML), NULL);

    RegisterProperty("localName", NewSlot(&DOMNodeBase::GetLocalName), NULL);
    RegisterProperty("nodeName", NewSlot(&DOMNodeBase::GetNodeName), NULL);
    RegisterProperty("nodeValue",
                     NewSlot(&DOMNodeImpl::ScriptGetNodeValue,
                             &DOMNodeBase::impl_),
                     NewSlot(&DOMNodeImpl::ScriptSetNodeValue,
                             &DOMNodeBase::impl_));
    RegisterProperty("nodeType", NewSlot(&DOMNodeInterface::GetNodeType), NULL);
    // Don't register parentNode as a constant to prevent circular refs.
    RegisterProperty("parentNode",
                     NewSlot(&DOMNodeBase::GetParentNodeNotConst), NULL);
    RegisterProperty("childNodes",
                     NewSlot(&DOMNodeImpl::GetChildNodes,
                             &DOMNodeBase::impl_), NULL);
    RegisterProperty("firstChild",
                     NewSlot(&DOMNodeImpl::GetFirstChild,
                             &DOMNodeBase::impl_), NULL);
    RegisterProperty("lastChild",
                     NewSlot(&DOMNodeImpl::GetLastChild,
                             &DOMNodeBase::impl_), NULL);
    RegisterProperty("previousSibling",
                     NewSlot(&DOMNodeImpl::GetPreviousSibling,
                             &DOMNodeBase::impl_), NULL);
    RegisterProperty("nextSibling",
                     NewSlot(&DOMNodeImpl::GetNextSibling,
                             &DOMNodeBase::impl_), NULL);
    RegisterProperty("attributes", NewSlot(&DOMNodeBase::GetAttributesNotConst),
                     NULL);
    // Don't register ownerDocument as a constant to prevent circular refs.
    RegisterProperty("ownerDocument",
                     NewSlot(&DOMNodeBase::GetOwnerDocumentNotConst), NULL);
    RegisterProperty("prefix",
                     NewSlot(&DOMNodeImpl::ScriptGetPrefix,
                             &DOMNodeBase::impl_),
                     NewSlot(&DOMNodeImpl::ScriptSetPrefix,
                             &DOMNodeBase::impl_));
    RegisterProperty("text", NewSlot(&DOMNodeBase::GetTextContent),
                     NewSlot(&DOMNodeBase::SetTextContent));
    RegisterMethod("insertBefore",
                   NewSlot(&DOMNodeImpl::ScriptInsertBefore,
                           &DOMNodeBase::impl_));
    RegisterMethod("replaceChild",
                   NewSlot(&DOMNodeImpl::ScriptReplaceChild,
                           &DOMNodeBase::impl_));
    RegisterMethod("removeChild",
                   NewSlot(&DOMNodeImpl::ScriptRemoveChild,
                           &DOMNodeBase::impl_));
    RegisterMethod("appendChild",
                   NewSlot(&DOMNodeImpl::ScriptAppendChild,
                           &DOMNodeBase::impl_));
    RegisterMethod("hasChildNodes",
                   NewSlot(&DOMNodeBase::HasChildNodes));
    RegisterMethod("cloneNode", NewSlot(&DOMNodeBase::CloneNode));
    RegisterMethod("normalize", NewSlot(&DOMNodeBase::Normalize));
    RegisterMethod("selectSingleNode", NewSlot(&DOMNodeImpl::SelectSingleNode,
                                               &DOMNodeBase::impl_));
    RegisterMethod("selectNodes", NewSlot(&DOMNodeImpl::SelectNodes,
                                          &DOMNodeBase::impl_));
  }

  virtual ~DOMNodeBase() {
    delete impl_;
    impl_ = NULL;
  }

  // Returns the address of impl_, to make it possible to access implementation
  // specific things through DOMNodeInterface pointers.
  virtual DOMNodeImpl *GetImpl() const { return impl_; }

  // Overrides ScriptableHelper::Ref().
  virtual void Ref() const {
    if (impl_->owner_node_) {
      // Increase the reference count along the path to the root.
      impl_->owner_node_->Ref();
    }
    return Super::Ref();
  }
  // Overrides ScriptableHelper::Unref().
  virtual void Unref(bool transient = false) const {
    if (impl_->owner_node_) {
      Super::Unref(true);
      // Decrease the reference count along the path to the root.
      impl_->owner_node_->Unref(transient);
    } else {
      // Only the root can delete the whole tree. Because the reference count
      // are accumulated, root's refcount == 0 means all decendents's
      // refcount == 0, thus the whole tree can be safely deleted.
      Super::Unref(transient);
    }
  }

  virtual std::string GetNodeName() const {
    return impl_->GetNodeName();
  }

  virtual std::string GetNodeValue() const { return std::string(); }
  virtual DOMExceptionCode SetNodeValue(const std::string &node_value) {
    GGL_UNUSED(node_value);
    return DOM_NO_MODIFICATION_ALLOWED_ERR;
  }
  virtual bool AllowsNodeValue() const { return false; }
  // GetNodeType() is still abstract.

  virtual DOMNodeInterface *GetParentNode() { return impl_->parent_; }
  virtual const DOMNodeInterface *GetParentNode() const {
    return impl_->parent_;
  }
  DOMNodeInterface *GetParentNodeNotConst() { return GetParentNode(); }

  virtual DOMNodeListInterface *GetChildNodes() {
    return impl_->GetChildNodes();
  }
  virtual const DOMNodeListInterface *GetChildNodes() const {
    return impl_->GetChildNodes();
  }

  virtual DOMNodeInterface *GetFirstChild() {
    return impl_->GetFirstChild();
  }
  virtual const DOMNodeInterface *GetFirstChild() const {
    return impl_->GetFirstChild();
  }

  virtual DOMNodeInterface *GetLastChild() {
    return impl_->GetLastChild();
  }
  virtual const DOMNodeInterface *GetLastChild() const {
    return impl_->GetLastChild();
  }

  virtual DOMNodeInterface *GetPreviousSibling() {
    return impl_->GetPreviousSibling();
  }
  virtual const DOMNodeInterface *GetPreviousSibling() const {
    return impl_->GetPreviousSibling();
  }

  virtual DOMNodeInterface *GetNextSibling() {
    return impl_->GetNextSibling();
  }
  virtual const DOMNodeInterface *GetNextSibling() const {
    return impl_->GetNextSibling();
  }

  virtual DOMNamedNodeMapInterface *GetAttributes() { return NULL; }
  virtual const DOMNamedNodeMapInterface *GetAttributes() const { return NULL; }
  DOMNamedNodeMapInterface *GetAttributesNotConst() { return GetAttributes(); }

  virtual DOMDocumentInterface *GetOwnerDocument() {
    return impl_->owner_document_;
  }
  virtual const DOMDocumentInterface *GetOwnerDocument() const {
    return impl_->owner_document_;
  }
  DOMDocumentInterface *GetOwnerDocumentNotConst() {
    return GetOwnerDocument();
  }

  virtual DOMExceptionCode InsertBefore(DOMNodeInterface *new_child,
                                        DOMNodeInterface *ref_child) {
    return impl_->InsertBefore(new_child, ref_child);
  }

  virtual DOMExceptionCode ReplaceChild(DOMNodeInterface *new_child,
                                        DOMNodeInterface *old_child) {
    return impl_->ReplaceChild(new_child, old_child);
  }

  virtual DOMExceptionCode RemoveChild(DOMNodeInterface *old_child) {
    return impl_->RemoveChild(old_child);
  }

  virtual DOMExceptionCode AppendChild(DOMNodeInterface *new_child) {
    return impl_->InsertBefore(new_child, NULL);
  }

  virtual bool HasChildNodes() const {
    return !impl_->children_.empty();
  }

  virtual DOMNodeInterface *CloneNode(bool deep) const {
    return impl_->CloneNode(impl_->owner_document_, deep);
  }

  virtual void Normalize() {
    impl_->Normalize();
  }

  virtual DOMNodeListInterface *GetElementsByTagName(const std::string &name) {
    return impl_->GetElementsByTagName(name);
  }

  virtual const DOMNodeListInterface *GetElementsByTagName(
      const std::string &name) const {
    return impl_->GetElementsByTagName(name);
  }

  DOMNodeListInterface *GetElementsByTagNameNotConst(const std::string &name) {
    return GetElementsByTagName(name);
  }

  // Default impl. Comment and CDataSection should override.
  virtual std::string GetTextContent() const {
    std::string result = impl_->GetTextContentPreserveWhiteSpace();
    return impl_->owner_document_->PreservesWhiteSpace() ?
           result : TrimString(result);
  }

  virtual void SetTextContent(const std::string &text_content) {
    if (AllowsNodeValue())
      SetNodeValue(text_content);
    else
      impl_->SetChildTextContent(text_content);
  }

  virtual std::string GetXML() const {
    return impl_->GetXML();
  }

  virtual int GetRow() const {
    return impl_->row_;
  }

  virtual void SetRow(int row) {
    impl_->row_ = row;
  }

  virtual int GetColumn() const {
    return impl_->column_;
  }

  virtual void SetColumn(int column) {
    impl_->column_ = column;
  }

  virtual bool CheckException(DOMExceptionCode code) {
    return GlobalCheckException(this, code);
  }

  virtual std::string GetPrefix() const {
    return impl_->prefix_;
  }

  virtual DOMExceptionCode SetPrefix(const std::string &prefix) {
    return AllowPrefix() ? impl_->SetPrefix(prefix) :
                           DOM_NO_MODIFICATION_ALLOWED_ERR;
  }

  virtual std::string GetLocalName() const {
    return impl_->local_name_;
  }

  virtual DOMNodeInterface *SelectSingleNode(const char *xpath) {
    return impl_->SelectSingleNode(xpath);
  }

  virtual const DOMNodeInterface *SelectSingleNode(const char *xpath) const {
    return impl_->SelectSingleNode(xpath);
  }

  virtual DOMNodeListInterface *SelectNodes(const char *xpath) {
    return impl_->SelectNodes(xpath);
  }

  virtual const DOMNodeListInterface *SelectNodes(const char *xpath) const {
    return impl_->SelectNodes(xpath);
  }

  bool CheckXMLName(const char *name) const {
    return impl_->owner_document_->GetXMLParser()->CheckXMLName(name);
  }

  std::string EncodeXMLString(const std::string &xml) const {
    return impl_->owner_document_->GetXMLParser()->EncodeXMLString(xml.c_str());
  }

  virtual void UpdateChildren() { }

 protected:
  virtual bool AllowPrefix() const { return false; }

 private:
  DOMNodeImpl *impl_;
};

class DOMCharacterDataImpl : public SmallObject<> {
 public:
  DOMCharacterDataImpl(const UTF16String &data)
      : utf16_data_(data) {
  }

  DOMCharacterDataImpl(const std::string &data)
      : utf8_data_(data) {
  }

  std::string GetNodeValue() const {
    EnsureUTF8Data();
    return utf8_data_;
  }
  void SetNodeValue(const std::string &value) {
    utf8_data_ = value;
    utf16_data_.clear();
  }

  UTF16String GetData() const {
    EnsureUTF16Data();
    return utf16_data_;
  }
  void SetData(const UTF16String &data) {
    utf16_data_ = data;
    utf8_data_.clear();
  }

  size_t GetLength() const {
    if (utf16_data_.empty() && utf8_data_.empty())
      return 0;
    EnsureUTF16Data();
    return utf16_data_.length();
  }

  bool IsEmpty() const {
    return utf16_data_.empty() && utf8_data_.empty();
  }

  DOMExceptionCode SubstringData(size_t offset, size_t count,
                                 UTF16String *result) const {
    ASSERT(result);
    EnsureUTF16Data();
    result->clear();
    if (offset > utf16_data_.length())
      return DOM_INDEX_SIZE_ERR;
    count = std::min(utf16_data_.length() - offset, count);
    *result = utf16_data_.substr(offset, count);
    return DOM_NO_ERR;
  }

  void AppendData(const UTF16String &arg) {
    EnsureUTF16Data();
    utf16_data_ += arg;
    utf8_data_.clear();
  }

  DOMExceptionCode InsertData(size_t offset, const UTF16String &arg) {
    EnsureUTF16Data();
    if (offset > utf16_data_.length())
      return DOM_INDEX_SIZE_ERR;

    utf16_data_.insert(offset, arg);
    utf8_data_.clear();
    return DOM_NO_ERR;
  }

  DOMExceptionCode DeleteData(size_t offset, size_t count) {
    EnsureUTF16Data();
    if (offset > utf16_data_.length())
      return DOM_INDEX_SIZE_ERR;
    count = std::min(utf16_data_.length() - offset, count);

    utf16_data_.erase(offset, count);
    utf8_data_.clear();
    return DOM_NO_ERR;
  }

  DOMExceptionCode ReplaceData(size_t offset, size_t count,
                               const UTF16String &arg) {
    EnsureUTF16Data();
    if (offset > utf16_data_.length())
      return DOM_INDEX_SIZE_ERR;
    count = std::min(utf16_data_.length() - offset, count);

    utf16_data_.replace(offset, count, arg);
    utf8_data_.clear();
    return DOM_NO_ERR;
  }

 private:
  void EnsureUTF16Data() const {
    if (utf16_data_.empty() && !utf8_data_.empty())
      ConvertStringUTF8ToUTF16(utf8_data_, &utf16_data_);
  }

  void EnsureUTF8Data() const {
    if (utf8_data_.empty() && !utf16_data_.empty())
      ConvertStringUTF16ToUTF8(utf16_data_, &utf8_data_);
  }

  // data_ and utf8_data_ cache each other.
  mutable UTF16String utf16_data_;
  mutable std::string utf8_data_;
};

template <typename Interface1>
class DOMCharacterData : public DOMNodeBase<Interface1> {
 public:
  typedef DOMNodeBase<Interface1> Super;
  DOMCharacterData(DOMDocumentInterface *owner_document,
                   const std::string &name, const UTF16String &data)
      : Super(owner_document, name),
        impl_(data) {
  }

  DOMCharacterData(DOMDocumentInterface *owner_document,
                   const std::string &name, const std::string &data)
      : Super(owner_document, name),
        impl_(data) {
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    this->RegisterProperty("data", NewSlot(&DOMCharacterData::GetData),
                           NewSlot(&DOMCharacterData::SetData));
    this->RegisterProperty("length",
                           NewSlot(&DOMCharacterData::GetLength),
                           NULL);
    this->RegisterMethod("substringData",
                         NewSlot(&DOMCharacterData::ScriptSubstringData));
    this->RegisterMethod("appendData", NewSlot(&DOMCharacterData::AppendData));
    this->RegisterMethod("insertData",
                         NewSlot(&DOMCharacterData::ScriptInsertData));
    this->RegisterMethod("deleteData",
                         NewSlot(&DOMCharacterData::ScriptDeleteData));
    this->RegisterMethod("replaceData",
                         NewSlot(&DOMCharacterData::ScriptReplaceData));
  }

  virtual std::string GetNodeValue() const { return impl_.GetNodeValue(); }
  virtual DOMExceptionCode SetNodeValue(const std::string &value) {
    impl_.SetNodeValue(value);
    return DOM_NO_ERR;
  }
  virtual bool AllowsNodeValue() const { return true; }
  virtual UTF16String GetData() const { return impl_.GetData(); }
  virtual void SetData(const UTF16String &data) { impl_.SetData(data); }
  virtual size_t GetLength() const { return impl_.GetLength(); }
  virtual bool IsEmpty() const { return impl_.IsEmpty(); }
  virtual DOMExceptionCode SubstringData(size_t offset, size_t count,
                                         UTF16String *result) const {
    return impl_.SubstringData(offset, count, result);
  }
  virtual void AppendData(const UTF16String &arg) {
    impl_.AppendData(arg);
  }
  virtual DOMExceptionCode InsertData(size_t offset, const UTF16String &arg) {
    return impl_.InsertData(offset, arg);
  }
  virtual DOMExceptionCode DeleteData(size_t offset, size_t count) {
    return impl_.DeleteData(offset, count);
  }
  virtual DOMExceptionCode ReplaceData(size_t offset, size_t count,
                                       const UTF16String &arg) {
    return impl_.ReplaceData(offset, count, arg);
  }

 protected:
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    GGL_UNUSED(new_child);
    // DOMCharacterData doesn't allow adding children to it.
    return DOM_HIERARCHY_REQUEST_ERR;
  }

 private:
  UTF16String ScriptSubstringData(size_t offset, size_t count) {
    UTF16String result;
    this->CheckException(SubstringData(offset, count, &result));
    return result;
  }

  void ScriptInsertData(size_t offset, const UTF16String &arg) {
    this->CheckException(InsertData(offset, arg));
  }

  void ScriptDeleteData(size_t offset, size_t count) {
    this->CheckException(DeleteData(offset, count));
  }

  void ScriptReplaceData(size_t offset, size_t count, const UTF16String &arg) {
    this->CheckException(ReplaceData(offset, count, arg));
  }

  DOMCharacterDataImpl impl_;
};

class DOMElement;
// To improve performance, DOMAttr has two modes:
// 1. local value mode: value is hold by itself.
// 2. children mode: value is hold in child text nodes.
// By default, DOMAttr is in mode 1, and will change to mode 2 if
// UpdateChildren() is called. It will back to mode 1 if SetValue() is called.
// Use HasChildNodes() to check for mode.
class DOMAttr : public DOMNodeBase<DOMAttrInterface> {
 public:
  DEFINE_CLASS_ID(0x5fee553d317b47d9, DOMAttrInterface);
  typedef DOMNodeBase<DOMAttrInterface> Super;

  DOMAttr(DOMDocumentInterface *owner_document, const std::string &name,
          DOMElement *owner_element)
      : Super(owner_document, name),
        owner_element_(NULL) {
    SetOwnerElement(owner_element);
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterProperty("name", NewSlot(&DOMAttr::GetName), NULL);
    // Our DOMAttrs are always specified, because we don't support DTD for now.
    RegisterConstant("specified", true);
    RegisterProperty("value", NewSlot(&DOMAttr::GetValue),
                     NewSlot(&DOMAttr::SetValue));
    // ownerElement is a DOM2 property, so not registered for now.
  }

  virtual DOMNodeInterface *CloneNode(bool deep) const {
    GGL_UNUSED(deep);
    // Attr.cloneNode always clone its children, even if deep is false.
    return Super::CloneNode(true);
  }

  virtual std::string GetNodeValue() const {
    return Super::HasChildNodes() ? GetImpl()->GetChildrenTextContent() :
                                    value_;
  }
  virtual DOMExceptionCode SetNodeValue(const std::string &value) {
    GetImpl()->RemoveAllChildren();
    value_ = value;
    return DOM_NO_ERR;
  }
  virtual bool AllowsNodeValue() const { return true; }
  virtual NodeType GetNodeType() const { return ATTRIBUTE_NODE; }

  // In mode 1, non-empty value_ implies a text node child which is not
  // actually created.
  virtual bool HasChildNodes() const {
    return !value_.empty() || Super::HasChildNodes();
  }

  virtual std::string GetName() const { return GetNodeName(); }
  // Our DOMAttrs are always specified, because we don't support DTD for now.
  virtual bool IsSpecified() const { return true; }
  virtual std::string GetValue() const { return GetNodeValue(); }
  virtual void SetValue(const std::string &value) { SetNodeValue(value); }

  virtual DOMElementInterface *GetOwnerElement(); // Defined later.
  virtual const DOMElementInterface *GetOwnerElement() const;

  void SetOwnerElement(DOMElement *owner_element); // Defined after DOMElement.

  virtual void AppendXML(size_t indent, std::string *xml) {
    GGL_UNUSED(indent);
    // Omit the indent parameter, let the parent deal with it.
    xml->append(GetNodeName());
    xml->append("=\"");
    xml->append(EncodeXMLString(GetNodeValue()));
    xml->append(1, '"');
  }

  virtual void UpdateChildren() {
    if (!Super::HasChildNodes() && !value_.empty()) {
      GetImpl()->SetChildTextContent(value_);
      value_.clear();
    }
  }

 protected:
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    DOMExceptionCode code = GetImpl()->CheckNewChildCommon(new_child);
    if (code == DOM_NO_ERR) {
      NodeType type = new_child->GetNodeType();
      if (type != TEXT_NODE && type != ENTITY_REFERENCE_NODE)
        code = DOM_HIERARCHY_REQUEST_ERR;
    }
    return code;
  }

  virtual DOMNodeInterface *CloneSelf(DOMDocumentInterface *owner_document) {
    DOMAttr *attr = new DOMAttr(owner_document, GetName(), NULL);
    attr->value_ = value_;
    // If in mode 2, the content will be cloned by common CloneNode
    // implementation, because for Attr.cloneNode(), children are always cloned.
    return attr;
  }

  virtual bool AllowPrefix() const { return true; }

 private:
  DOMElement *owner_element_;
  std::string value_;
};

class DOMElement : public DOMNodeBase<DOMElementInterface> {
 public:
  DEFINE_CLASS_ID(0x721f40f59a3f48a9, DOMElementInterface);
  typedef DOMNodeBase<DOMElementInterface> Super;
  typedef std::vector<DOMAttr *> Attrs;
  // Maps attribute name to the index of Attrs.
  typedef LightMap<std::string, size_t> AttrsMap;

  DOMElement(DOMDocumentInterface *owner_document, const std::string &tag_name)
      : Super(owner_document, tag_name) {
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterProperty("tagName", NewSlot(&DOMElement::GetTagName), NULL);
    RegisterMethod("getAttribute", NewSlot(&DOMElement::GetAttribute));
    RegisterMethod("setAttribute",
                   NewSlot(&DOMElement::ScriptSetAttribute));
    RegisterMethod("removeAttribute",
                   NewSlot(&DOMElement::RemoveAttribute));
    RegisterMethod("getAttributeNode",
                   NewSlot(&DOMElement::GetAttributeNodeNotConst));
    RegisterMethod("setAttributeNode",
                   NewSlot(&DOMElement::ScriptSetAttributeNode));
    RegisterMethod("removeAttributeNode",
                   NewSlot(&DOMElement::ScriptRemoveAttributeNode));
    RegisterMethod("getElementsByTagName",
                   NewSlot(&Super::GetElementsByTagNameNotConst));
  }

  ~DOMElement() {
    ASSERT(attrs_.size() == attrs_map_.size());
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it)
      delete (*it);
  }

  virtual NodeType GetNodeType() const { return ELEMENT_NODE; }
  virtual std::string GetTagName() const { return GetNodeName(); }

  virtual void Normalize() {
    Super::Normalize();
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it)
      (*it)->Normalize();
  }

  virtual std::string GetAttribute(const std::string &name) const {
    const DOMAttrInterface *attr = GetAttributeNode(name);
    // TODO: Default value logic.
    return attr ? attr->GetValue() : "";
  }

  virtual DOMExceptionCode SetAttribute(const std::string &name,
                                        const std::string &value) {
    if (!CheckXMLName(name.c_str()))
      return DOM_INVALID_CHARACTER_ERR;

    AttrsMap::iterator it = attrs_map_.find(name);
    if (it == attrs_map_.end()) {
      DOMAttr *attr = new DOMAttr(GetOwnerDocument(), name, this);
      attrs_map_[attr->GetName()] = attrs_.size();
      attrs_.push_back(attr);
      attr->SetValue(value);
      attr->SetRow(GetRow());
      // Don't set column, because it is inaccurate.
      ASSERT(attrs_map_.size() == attrs_.size());
    } else {
      ASSERT(it->second < attrs_.size());
      attrs_[it->second]->SetValue(value);
    }
    return DOM_NO_ERR;
  }

  virtual void RemoveAttribute(const std::string &name) {
    RemoveAttributeInternal(name);
  }

  virtual DOMAttrInterface *GetAttributeNode(const std::string &name) {
    return const_cast<DOMAttrInterface *>(
        static_cast<const DOMElement *>(this)->GetAttributeNode(name));
  }

  virtual const DOMAttrInterface *GetAttributeNode(
      const std::string &name) const {
    AttrsMap::const_iterator it = attrs_map_.find(name);
    ASSERT(it == attrs_map_.end() || it->second < attrs_.size());
    return it == attrs_map_.end() ? NULL : attrs_[it->second];
  }

  virtual DOMExceptionCode SetAttributeNode(DOMAttrInterface *new_attr) {
    if (!new_attr)
      return DOM_NULL_POINTER_ERR;
    if (new_attr->GetOwnerDocument() != GetOwnerDocument())
      return DOM_WRONG_DOCUMENT_ERR;
    if (new_attr->GetOwnerElement()) {
      return new_attr->GetOwnerElement() != this ?
             DOM_INUSE_ATTRIBUTE_ERR : DOM_NO_ERR;
    }

    DOMAttr *new_attr_internal = down_cast<DOMAttr *>(new_attr);
    new_attr_internal->SetOwnerElement(this);
    AttrsMap::iterator it = attrs_map_.find(new_attr->GetName());
    if (it != attrs_map_.end()) {
      ASSERT(it->second < attrs_.size());
      attrs_[it->second]->SetOwnerElement(NULL);
      attrs_[it->second] = new_attr_internal;
      // No need to change attr_map_.
    } else {
      attrs_map_[new_attr->GetName()] = attrs_.size();
      attrs_.push_back(new_attr_internal);
      ASSERT(attrs_map_.size() == attrs_.size());
    }
    return DOM_NO_ERR;
  }

  virtual DOMExceptionCode RemoveAttributeNode(DOMAttrInterface *old_attr) {
    if (!old_attr)
      return DOM_NULL_POINTER_ERR;
    if (old_attr->GetOwnerElement() != this)
      return DOM_NOT_FOUND_ERR;

    bool result = RemoveAttributeInternal(old_attr->GetName());
    ASSERT(result);
    return result ? DOM_NO_ERR : DOM_NOT_FOUND_ERR;
  }

  virtual DOMNamedNodeMapInterface *GetAttributes() {
    return new AttrsNamedMap(this);
  }
  virtual const DOMNamedNodeMapInterface *GetAttributes() const {
    return new AttrsNamedMap(const_cast<DOMElement *>(this));
  }

  virtual void AppendXML(size_t indent, std::string *xml) {
    std::string::size_type line_begin = xml->length();
    AppendIndentNewLine(indent, xml);
    xml->append(1, '<');
    xml->append(GetNodeName());
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it) {
      xml->append(1, ' ');
      (*it)->AppendXML(indent, xml);
      if (xml->size() - line_begin > kLineLengthThreshold) {
        line_begin = xml->length();
        AppendIndentNewLine(indent + kIndent, xml);
      }
    }
    if (HasChildNodes()) {
      xml->append(1, '>');
      GetImpl()->AppendChildrenXML(indent + kIndent, xml);
      AppendIndentIfNewLine(indent, xml);
      xml->append("</");
      xml->append(GetNodeName());
      xml->append(">\n");
    } else {
      xml->append("/>\n");
    }
  }

 protected:
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    DOMExceptionCode code = GetImpl()->CheckNewChildCommon(new_child);
    if (code == DOM_NO_ERR)
      code = CheckCommonChildType(new_child);
    return code;
  }

  virtual DOMNodeInterface *CloneSelf(DOMDocumentInterface *owner_document) {
    DOMElement *element = new DOMElement(owner_document, GetTagName());
    for (Attrs::iterator it = attrs_.begin(); it != attrs_.end(); ++it) {
      DOMAttrInterface *cloned_attr = down_cast<DOMAttrInterface *>(
          (*it)->GetImpl()->CloneNode(owner_document, true));
      element->SetAttributeNode(cloned_attr);
    }
    return element;
  }

  virtual bool AllowPrefix() const { return true; }

 private:
  DOMAttrInterface *GetAttributeNodeNotConst(const std::string &name) {
    return GetAttributeNode(name);
  }

  class AttrsNamedMap : public ScriptableHelper<DOMNamedNodeMapInterface> {
   public:
    DEFINE_CLASS_ID(0xbe2998ee79754343, DOMNamedNodeMapInterface)
    AttrsNamedMap(DOMElement *element) : element_(element) {
      element->Ref();
      SetArrayHandler(NewSlot(this, &AttrsNamedMap::GetItemNotConst), NULL);
    }
    virtual ~AttrsNamedMap() {
      element_->Unref();
    }

    virtual void DoClassRegister() {
      RegisterProperty("length", NewSlot(&AttrsNamedMap::GetLength), NULL);
      RegisterMethod("getNamedItem",
                     NewSlot(&AttrsNamedMap::GetNamedItemNotConst));
      RegisterMethod("setNamedItem",
                     NewSlot(&AttrsNamedMap::ScriptSetNamedItem));
      RegisterMethod("removeNamedItem",
                     NewSlot(&AttrsNamedMap::ScriptRemoveNamedItem));
      RegisterMethod("item", NewSlot(&AttrsNamedMap::GetItemNotConst));
      // Microsoft compatibility.
      RegisterMethod("", NewSlot(&AttrsNamedMap::GetItemNotConst));
    }

    virtual DOMNodeInterface *GetNamedItem(const std::string &name) {
      return element_->GetAttributeNode(name);
    }
    virtual const DOMNodeInterface *GetNamedItem(
        const std::string &name) const {
      return element_->GetAttributeNode(name);
    }
    virtual DOMExceptionCode SetNamedItem(DOMNodeInterface *arg) {
      if (!arg)
        return DOM_NULL_POINTER_ERR;
      if (arg->GetNodeType() != ATTRIBUTE_NODE)
        return DOM_HIERARCHY_REQUEST_ERR;
      return element_->SetAttributeNode(down_cast<DOMAttrInterface *>(arg));
    }
    virtual DOMExceptionCode RemoveNamedItem(const std::string &name) {
      return element_->RemoveAttributeInternal(name) ?
             DOM_NO_ERR : DOM_NOT_FOUND_ERR;
    }
    virtual DOMNodeInterface *GetItem(size_t index) {
      return index < element_->attrs_.size() ? element_->attrs_[index] : NULL;
    }
    virtual const DOMNodeInterface *GetItem(size_t index) const {
      return index < element_->attrs_.size() ? element_->attrs_[index] : NULL;
    }
    virtual size_t GetLength() const {
      return element_->attrs_.size();
    }

   private:
    DOMNodeInterface *GetItemNotConst(size_t index) { return GetItem(index); }
    DOMNodeInterface *GetNamedItemNotConst(const std::string &name) {
      return GetNamedItem(name);
    }

    DOMNodeInterface *ScriptSetNamedItem(DOMNodeInterface *arg) {
      if (!arg) {
        GlobalCheckException(this, DOM_NULL_POINTER_ERR);
        return NULL;
      } else if (arg->GetNodeType() != ATTRIBUTE_NODE) {
        GlobalCheckException(this, DOM_HIERARCHY_REQUEST_ERR);
        return NULL;
      } else {
        DOMAttrInterface *new_attr = down_cast<DOMAttrInterface *>(arg);
        DOMAttrInterface *replaced_attr = NULL;
        if (arg) {
          // Add a temporary reference to the replaced attr to prevent it from
          // being deleted in SetAttributeNode().
          replaced_attr = element_->GetAttributeNode(new_attr->GetName());
          if (replaced_attr)
            replaced_attr->Ref();
        }
        DOMExceptionCode code = element_->SetAttributeNode(new_attr);
        if (replaced_attr)
          replaced_attr->Unref(code == DOM_NO_ERR);
        return GlobalCheckException(this, code) ? replaced_attr : NULL;
      }
    }

    DOMNodeInterface *ScriptRemoveNamedItem(const std::string &name) {
      DOMNodeInterface *removed_node = GetNamedItem(name);
      if (removed_node)
        removed_node->Ref();
      DOMExceptionCode code = RemoveNamedItem(name);
      if (removed_node)
        removed_node->Unref(code == DOM_NO_ERR);
      return GlobalCheckException(this, code) ? removed_node : NULL;
    }

    DOMElement *element_;
  };

  void ScriptSetAttribute(const std::string &name, const std::string &value) {
    CheckException(SetAttribute(name, value));
  }

  DOMAttrInterface *ScriptSetAttributeNode(DOMAttrInterface *new_attr) {
    DOMAttrInterface *replaced_attr = NULL;
    if (new_attr) {
      replaced_attr = GetAttributeNode(new_attr->GetName());
      // Add a temporary reference to the replaced attr to prevent it from
      // being deleted in SetAttributeNode().
      if (replaced_attr)
        replaced_attr->Ref();
    }
    DOMExceptionCode code = SetAttributeNode(new_attr);
    if (replaced_attr)
      replaced_attr->Unref(code == DOM_NO_ERR);
    return CheckException(code) ? replaced_attr : NULL;
  }

  DOMAttrInterface *ScriptRemoveAttributeNode(DOMAttrInterface *old_attr) {
    return CheckException(RemoveAttributeNode(old_attr)) ? old_attr : NULL;
  }

  bool RemoveAttributeInternal(const std::string &name) {
    AttrsMap::iterator it = attrs_map_.find(name);
    if (it != attrs_map_.end()) {
      size_t index = it->second;
      attrs_[index]->SetOwnerElement(NULL);
      if (index < attrs_.size() - 1) {
        // Move the last element to the new blank slot and update the index,
        // ensuring that attrs_ contains no blank slot.
        DOMAttr *last_attr = attrs_.back();
        attrs_[index] = last_attr;
        attrs_map_[last_attr->GetName()] = index;
      }
      attrs_.pop_back();
      attrs_map_.erase(it);
      return true;
    }
    return false;
    // TODO: Deal with default values if we support DTD.
  }

  std::string tag_name_;
  Attrs attrs_;
  AttrsMap attrs_map_;
};

DOMElementInterface *DOMAttr::GetOwnerElement() {
  return owner_element_;
}
const DOMElementInterface *DOMAttr::GetOwnerElement() const {
  return owner_element_;
}

void DOMAttr::SetOwnerElement(DOMElement *owner_element) {
  if (owner_element_ != owner_element) {
    owner_element_ = owner_element;
    GetImpl()->SetOwnerNode(owner_element);
  }
}

static DOMExceptionCode DoSplitText(DOMTextInterface *text,
                                    size_t offset,
                                    DOMTextInterface **new_text) {
  ASSERT(new_text);
  *new_text = NULL;
  if (offset > text->GetLength())
    return DOM_INDEX_SIZE_ERR;

  size_t tail_size = text->GetLength() - offset;
  UTF16String tail_data;
  text->SubstringData(offset, tail_size, &tail_data);
  *new_text = down_cast<DOMTextInterface *>(text->CloneNode(false));
  (*new_text)->SetData(tail_data);
  text->DeleteData(offset, tail_size);

  if (text->GetParentNode())
    text->GetParentNode()->InsertBefore(*new_text, text->GetNextSibling());
  return DOM_NO_ERR;
}

class DOMText : public DOMCharacterData<DOMTextInterface> {
 public:
  DEFINE_CLASS_ID(0xdcd93e1ac43b49d2, DOMTextInterface);
  typedef DOMCharacterData<DOMTextInterface> Super;

  DOMText(DOMDocumentInterface *owner_document, const UTF16String &data)
      : Super(owner_document, kDOMTextName, data) {
  }
  DOMText(DOMDocumentInterface *owner_document, const std::string &data)
      : Super(owner_document, kDOMTextName, data) {
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterMethod("splitText", NewSlot(&DOMText::ScriptSplitText));
  }

  virtual NodeType GetNodeType() const { return TEXT_NODE; }

  virtual DOMExceptionCode SplitText(size_t offset,
                                     DOMTextInterface **new_text) {
    return DoSplitText(this, offset, new_text);
  }

  virtual void AppendXML(size_t indent, std::string *xml) {
    GGL_UNUSED(indent);
    // Omit the indent parameter, let the parent deal with it.
    std::string node_value(GetNodeValue());
    std::string trimmed = TrimString(EncodeXMLString(GetNodeValue()));
    if (!node_value.empty() &&
        (trimmed.empty() ||
         *(node_value.end() - 1) != *(trimmed.end() - 1))) {
      // The tail of the text has been trimmed.
      NodeType next_type = GetNextSibling() ?
                           GetNextSibling()->GetNodeType() : ELEMENT_NODE;
      if (next_type == TEXT_NODE || next_type == ENTITY_REFERENCE_NODE)
        // Preserve one space.
        trimmed += ' ';
    }
    xml->append(trimmed);
  }

 protected:
  virtual DOMNodeInterface *CloneSelf(DOMDocumentInterface *owner_document) {
    return new DOMText(owner_document, GetData());
  }

 private:
  DOMTextInterface *ScriptSplitText(size_t offset) {
    DOMTextInterface *result = NULL;
    return CheckException(SplitText(offset, &result)) ? result : NULL;
  }
};

class DOMComment : public DOMCharacterData<DOMCommentInterface> {
 public:
  DEFINE_CLASS_ID(0x8f177233373d4015, DOMCommentInterface);
  typedef DOMCharacterData<DOMCommentInterface> Super;

  DOMComment(DOMDocumentInterface *owner_document, const UTF16String &data)
      : Super(owner_document, kDOMCommentName, data) {
  }
  DOMComment(DOMDocumentInterface *owner_document, const std::string &data)
      : Super(owner_document, kDOMCommentName, data) {
  }

  virtual NodeType GetNodeType() const { return COMMENT_NODE; }

  virtual std::string GetTextContent() const {
    return GetImpl()->GetTextContentPreserveWhiteSpace();
  }

  virtual void AppendXML(size_t indent, std::string *xml) {
    // Omit the indent parameter, let the parent deal with it.
    AppendIndentNewLine(indent, xml);
    xml->append("<!--");
    // Replace all '--'s in the comment into '- -'s.
    std::string value_str = GetNodeValue();
    const char *value = value_str.c_str();
    bool last_dash = false;
    while (*value) {
      char c = *value++;
      if (c == '-') {
        if (last_dash)
          xml->append(1, ' ');
        last_dash = true;
      } else {
        last_dash = false;
      }
      xml->append(1, c);
    }
    if (last_dash)
      xml->append(1, ' ');
    xml->append("-->\n");
  }

 protected:
  virtual DOMNodeInterface *CloneSelf(DOMDocumentInterface *owner_document) {
    return new DOMComment(owner_document, GetData());
  }
};

class DOMCDATASection : public DOMCharacterData<DOMCDATASectionInterface> {
 public:
  DEFINE_CLASS_ID(0xe6b4c9779b3d4127, DOMCDATASectionInterface);
  typedef DOMCharacterData<DOMCDATASectionInterface> Super;

  DOMCDATASection(DOMDocumentInterface *owner_document, const UTF16String &data)
      : Super(owner_document, kDOMCDATASectionName, data) {
    // TODO:
  }
  DOMCDATASection(DOMDocumentInterface *owner_document, const std::string &data)
      : Super(owner_document, kDOMCDATASectionName, data) {
    // TODO:
  }

  virtual NodeType GetNodeType() const { return CDATA_SECTION_NODE; }

  virtual std::string GetTextContent() const {
    return GetImpl()->GetTextContentPreserveWhiteSpace();
  }

  virtual DOMExceptionCode SplitText(size_t offset,
                                     DOMTextInterface **new_text) {
    return DoSplitText(this, offset, new_text);
  }

  virtual void AppendXML(size_t indent, std::string *xml) {
    AppendIndentNewLine(indent, xml);
    std::string value_str = GetNodeValue();
    const char *value = value_str.c_str();
    while (true) {
      const char *pos = strstr(value, "]]>");
      if (pos) {
        xml->append("<![CDATA[");
        // Output until "]]" and leave ">" to the next section.
        xml->append(value, pos - value + 2);
        xml->append("]]>");
        value = pos + 2;
      } else {
        xml->append("<![CDATA[");
        xml->append(value);
        xml->append("]]>\n");
        break;
      }
    }
  }

 protected:
  virtual DOMNodeInterface *CloneSelf(DOMDocumentInterface *owner_document) {
    return new DOMCDATASection(owner_document, GetData());
  }
};

class DOMDocumentFragment : public DOMNodeBase<DOMDocumentFragmentInterface> {
 public:
  DEFINE_CLASS_ID(0x6ba54beef94643d4, DOMDocumentFragmentInterface);
  typedef DOMNodeBase<DOMDocumentFragmentInterface> Super;

  DOMDocumentFragment(DOMDocumentInterface *owner_document)
      : Super(owner_document, kDOMDocumentFragmentName) { }

  virtual NodeType GetNodeType() const { return DOCUMENT_FRAGMENT_NODE; }

  virtual void AppendXML(size_t indent, std::string *xml) {
    // Because DOMDocumentFragment can't be child of any node, the indent
    // should always be zero.
    ASSERT(indent == 0);
    GetImpl()->AppendChildrenXML(indent, xml);
  }

 protected:
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    DOMExceptionCode code = GetImpl()->CheckNewChildCommon(new_child);
    if (code == DOM_NO_ERR)
      code = CheckCommonChildType(new_child);
    return code;
  }

  virtual DOMNodeInterface *CloneSelf(DOMDocumentInterface *owner_document) {
    return new DOMDocumentFragment(owner_document);
  }
};

class DOMProcessingInstruction
    : public DOMNodeBase<DOMProcessingInstructionInterface> {
 public:
  DEFINE_CLASS_ID(0x54e1e0de36a2464f, DOMProcessingInstructionInterface);
  typedef DOMNodeBase<DOMProcessingInstructionInterface> Super;

  DOMProcessingInstruction(DOMDocumentInterface *owner_document,
                           const std::string &target, const std::string &data)
      : Super(owner_document, target),
        target_(target),
        data_(data) {
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterProperty("target",
                     NewSlot(&DOMProcessingInstruction::GetTarget), NULL);
    RegisterProperty("data", NewSlot(&DOMProcessingInstruction::GetData),
                     NewSlot(&DOMProcessingInstruction::SetData));
  }

  virtual std::string GetNodeValue() const { return data_; }
  virtual DOMExceptionCode SetNodeValue(const std::string &value) {
    SetData(value);
    return DOM_NO_ERR;
  }
  virtual bool AllowsNodeValue() const { return true; }
  virtual NodeType GetNodeType() const { return PROCESSING_INSTRUCTION_NODE; }

  virtual std::string GetTarget() const { return target_; }
  virtual std::string GetData() const { return data_; }
  virtual void SetData(const std::string &data) { data_ = data; }

  virtual void AppendXML(size_t indent, std::string *xml) {
    AppendIndentNewLine(indent, xml);
    xml->append("<?");
    xml->append(GetNodeName());
    xml->append(1, ' ');
    xml->append(GetData());
    xml->append("?>\n");
  }

 protected:
  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    GGL_UNUSED(new_child);
    // DOMProcessingInstruction doesn't allow adding children to it.
    return DOM_HIERARCHY_REQUEST_ERR;
  }

  virtual DOMNodeInterface *CloneSelf(DOMDocumentInterface *owner_document) {
    return new DOMDocumentFragment(owner_document);
  }

 private:
  std::string target_;
  std::string data_;
};

class DOMImplementation
    : public ScriptableHelperNativeOwned<DOMImplementationInterface> {
 public:
  DEFINE_CLASS_ID(0xd23149a89cf24e12, DOMImplementationInterface);

  virtual void DoClassRegister() {
    RegisterMethod("hasFeature", NewSlot(&DOMImplementation::HasFeature));
  }

  virtual bool HasFeature(const char *feature, const char *version) const {
    return feature && strcasecmp(feature, "XML") == 0 &&
          (!version || !version[0] || strcmp(version, "1.0") == 0);
  }
};

// This class is not a complete implementation, just to make some script
// containing Microsoft-specific code run without errors.
class ParseError : public ScriptableHelperNativeOwnedDefault {
 public:
  DEFINE_CLASS_ID(0xc494c55756dc46a6, ScriptableInterface);
  ParseError() : code_(0) {
  }
  int GetCode() const { return code_; }
  void SetCode(int code) { code_ = code; }

  virtual void DoClassRegister() {
    RegisterProperty("errorCode", NewSlot(&ParseError::GetCode), NULL);
    RegisterConstant("filepos", 0);
    RegisterConstant("line", 0);
    RegisterConstant("linepos", 0);
    RegisterConstant("reason", "");
    RegisterConstant("srcText", "");
    RegisterConstant("url", "");
  }

 private:
  int code_;
};

// Note about reference counting of DOMDocument: the reference count is the sum
// of the following two counts:
// 1. Normal reference count same as DOMNodeBase, that is, the accumulated
//    reference counts of all descendants;
// 2. Count of all orphan trees. This count will be increased when a new
//    orphan node is created, and be decreased when a root of an orphan tree
//    is added into another tree, or an orphan is deleted.
class DOMDocument : public DOMNodeBase<DOMDocumentInterface> {
 public:
  DEFINE_CLASS_ID(0x23dffa4b4f234226, DOMDocumentInterface);
  typedef DOMNodeBase<DOMDocumentInterface> Super;

  DOMDocument(XMLParserInterface *xml_parser,
              bool allow_load_http, bool allow_load_file)
      : Super(NULL, kDOMDocumentName),
        xml_parser_(xml_parser),
        http_request_(NULL),
        onreadystatechange_connection_(NULL),
        ready_state_(XMLHttpRequestInterface::UNSENT),
        allow_load_http_(allow_load_http),
        allow_load_file_(allow_load_file),
        preserve_whitespace_(false),
        async_(true) {
  }

  virtual ~DOMDocument() {
    if (http_request_) {
      if (onreadystatechange_connection_) {
        onreadystatechange_connection_->Disconnect();
        onreadystatechange_connection_ = NULL;
      }
      http_request_->Unref();
      http_request_ = NULL;
    }
  }

  virtual void DoClassRegister() {
    Super::DoClassRegister();
    RegisterConstant("doctype", static_cast<ScriptableInterface *>(NULL));
    RegisterConstant("implementation", &dom_implementation_);
    RegisterProperty("documentElement",
                     NewSlot(&DOMDocument::GetDocumentElementNotConst), NULL);
    RegisterMethod("loadXML", NewSlot(&DOMDocument::LoadXML));
    RegisterMethod("createElement",
                   NewSlot(&DOMDocument::ScriptCreateElement));
    RegisterMethod("createDocumentFragment",
                   NewSlot(&DOMDocument::CreateDocumentFragment));
    RegisterMethod("createTextNode",
                   NewSlot(&DOMDocument::CreateTextNode));
    RegisterMethod("createComment", NewSlot(&DOMDocument::CreateComment));
    RegisterMethod("createCDATASection",
                   NewSlot(&DOMDocument::CreateCDATASection));
    RegisterMethod("createProcessingInstruction",
                   NewSlot(&DOMDocument::ScriptCreateProcessingInstruction));
    RegisterMethod("createAttribute",
                   NewSlot(&DOMDocument::ScriptCreateAttribute));
    RegisterMethod("createEntityReference",
                   NewSlot(&DOMDocument::ScriptCreateEntityReference));
    RegisterMethod("getElementsByTagName",
                   NewSlot(&Super::GetElementsByTagNameNotConst));
    RegisterMethod("importNode", NewSlot(&DOMDocument::ScriptImportNode));
    // Compatibility with Microsoft DOM.
    RegisterProperty("parsed", NewFixedGetterSlot(true), NULL);
    RegisterProperty("parseError", NewSlot(&DOMDocument::GetParseError), NULL);
    RegisterProperty("resolveExternals", NULL, NewSlot(&DummySetter));
    RegisterProperty("validateOnParse", NULL, NewSlot(&DummySetter));
    RegisterProperty("preserveWhiteSpace",
                     NewSlot(&DOMDocument::PreservesWhiteSpace),
                     NewSlot(&DOMDocument::SetPreserveWhiteSpace));
    RegisterMethod("getProperty", NewSlot(DummyGetProperty));
    RegisterMethod("setProperty", NewSlot(DummySetProperty));
    // Compatibility with Microsoft DOM: XMLHttpRequest functions.
    RegisterProperty("async", NewSlot(&DOMDocument::IsAsync),
                     NewSlot(&DOMDocument::SetAsync));
    RegisterProperty("readyState", NewSlot(&DOMDocument::GetReadyState), NULL);
    RegisterMethod("load", NewSlot(&DOMDocument::Load));
    RegisterClassSignal("onreadystatechange",
                        &DOMDocument::onreadystatechange_signal_);
  }

  ParseError *GetParseError() {
    return &parse_error_;
  }

  virtual bool LoadXML(const std::string &xml) {
    GetImpl()->RemoveAllChildren();
    bool result = xml_parser_->ParseContentIntoDOM(xml, NULL, "NONAME", NULL,
                                                   NULL, kEncodingFallback,
                                                   this, NULL, NULL);
    parse_error_.SetCode(result ? 0 : 1);
    return result;
  }

  virtual NodeType GetNodeType() const { return DOCUMENT_NODE; }
  virtual DOMDocumentTypeInterface *GetDoctype() {
    return NULL;
    // TODO: If we support DTD.
    // return down_cast<DOMDocumentTypeInterface *>(
    //     FindNodeOfType(DOCUMENT_TYPE_NODE));
  }
  virtual const DOMDocumentTypeInterface *GetDoctype() const {
    return NULL;
    // TODO: If we support DTD.
    // return down_cast<DOMDocumentTypeInterface *>(
    //     FindNodeOfType(DOCUMENT_TYPE_NODE));
  }

  virtual DOMImplementationInterface *GetImplementation() {
    return &dom_implementation_;
  }
  virtual const DOMImplementationInterface *GetImplementation() const {
    return &dom_implementation_;
  }

  virtual DOMElementInterface *GetDocumentElement() {
    return const_cast<DOMElementInterface *>(
        down_cast<const DOMElementInterface *>(FindNodeOfType(ELEMENT_NODE)));
  }

  virtual const DOMElementInterface *GetDocumentElement() const {
    return down_cast<const DOMElementInterface *>(FindNodeOfType(ELEMENT_NODE));
  }

  virtual DOMExceptionCode CreateElement(const std::string &tag_name,
                                         DOMElementInterface **result) {
    ASSERT(result);
    *result = NULL;
    if (!xml_parser_->CheckXMLName(tag_name.c_str()))
      return DOM_INVALID_CHARACTER_ERR;
    *result = new DOMElement(this, tag_name);
    return DOM_NO_ERR;
  }

  virtual DOMDocumentFragmentInterface *CreateDocumentFragment() {
    return new DOMDocumentFragment(this);
  }

  virtual DOMTextInterface *CreateTextNode(const UTF16String &data) {
    return new DOMText(this, data);
  }

  virtual DOMCommentInterface *CreateComment(const UTF16String &data) {
    return new DOMComment(this, data);
  }

  virtual DOMCDATASectionInterface *CreateCDATASection(
      const UTF16String &data) {
    return new DOMCDATASection(this, data);
  }

  virtual DOMTextInterface *CreateTextNodeUTF8(const std::string &data) {
    return new DOMText(this, data);
  }

  virtual DOMCommentInterface *CreateCommentUTF8(const std::string &data) {
    return new DOMComment(this, data);
  }

  virtual DOMCDATASectionInterface *CreateCDATASectionUTF8(
      const std::string &data) {
    return new DOMCDATASection(this, data);
  }

  virtual DOMExceptionCode CreateProcessingInstruction(
      const std::string &target, const std::string &data,
      DOMProcessingInstructionInterface **result) {
    ASSERT(result);
    *result = NULL;
    if (!xml_parser_->CheckXMLName(target.c_str()))
      return DOM_INVALID_CHARACTER_ERR;
    *result = new DOMProcessingInstruction(this, target, data);
    return DOM_NO_ERR;
  }

  virtual DOMExceptionCode CreateAttribute(const std::string &name,
                                           DOMAttrInterface **result) {
    ASSERT(result);
    *result = NULL;
    if (!xml_parser_->CheckXMLName(name.c_str()))
      return DOM_INVALID_CHARACTER_ERR;
    *result = new DOMAttr(this, name, NULL);
    return DOM_NO_ERR;
  }

  virtual DOMExceptionCode CreateEntityReference(
      const std::string &name, DOMEntityReferenceInterface **result) {
    GGL_UNUSED(name);
    ASSERT(result);
    *result = NULL;
    return DOM_NOT_SUPPORTED_ERR;
  }

  virtual void AppendXML(size_t indent, std::string *xml) {
    ASSERT(indent == 0);
    xml->append(kStandardXMLDecl);
    GetImpl()->AppendChildrenXML(indent, xml);
  }

  virtual XMLParserInterface *GetXMLParser() const {
    return xml_parser_;
  }

  virtual DOMExceptionCode ImportNode(const DOMNodeInterface *imported_node,
                                      bool deep,
                                      DOMNodeInterface **result) {
    if (!imported_node)
      return DOM_NULL_POINTER_ERR;
    NodeType type = imported_node->GetNodeType();
    if (type == DOCUMENT_NODE || type == DOCUMENT_TYPE_NODE)
      return DOM_NOT_SUPPORTED_ERR;
    *result = imported_node->GetImpl()->CloneNode(this, deep);
    return *result ? DOM_NO_ERR : DOM_NOT_SUPPORTED_ERR;
  }

  virtual bool PreservesWhiteSpace() const {
    return preserve_whitespace_;
  }

  virtual void SetPreserveWhiteSpace(bool preserve_whitespace) {
    preserve_whitespace_ = preserve_whitespace;
  }

 protected:
  virtual DOMNodeInterface *CloneSelf(DOMDocumentInterface *owner_document) {
    GGL_UNUSED(owner_document);
    return NULL;
  }

  virtual DOMExceptionCode CheckNewChild(DOMNodeInterface *new_child) {
    DOMExceptionCode code = GetImpl()->CheckNewChildCommon(new_child);
    if (code == DOM_NO_ERR) {
      NodeType type = new_child->GetNodeType();
      if (type == ELEMENT_NODE) {
        // Only one element node is allowed.
        if (GetDocumentElement()) {
          DLOG("DOMDocument::CheckNewChild: Duplicated document element");
          code = DOM_HIERARCHY_REQUEST_ERR;
        }
      } else if (type == DOCUMENT_TYPE_NODE) {
        // Only one doc type node is allowed.
        if (GetDoctype()) {
          DLOG("DOMDocument::CheckNewChild: Duplicated doctype");
          code = DOM_HIERARCHY_REQUEST_ERR;
        }
      } else if (type != PROCESSING_INSTRUCTION_NODE && type != COMMENT_NODE) {
        DLOG("DOMDocument::CheckNewChild: Invalid type of document child: %d",
             type);
        code = DOM_HIERARCHY_REQUEST_ERR;
      }
    }
    return code;
  }

 private:
  DOMElementInterface *GetDocumentElementNotConst() {
    return GetDocumentElement();
  }

  const DOMNodeInterface *FindNodeOfType(NodeType type) const {
    const DOMNodeInterface *result = NULL;
    for (const DOMNodeInterface *item = GetFirstChild(); item;
         item = item->GetNextSibling()) {
      if (item->GetNodeType() == type) {
        result = item;
        break;
      }
    }
    return result;
  }

  DOMElementInterface *ScriptCreateElement(const std::string &tag_name) {
    DOMElementInterface *result = NULL;
    return CheckException(CreateElement(tag_name, &result)) ? result : NULL;
  }

  DOMProcessingInstructionInterface *ScriptCreateProcessingInstruction(
      const std::string &target, const std::string &data) {
    DOMProcessingInstructionInterface *result = NULL;
    return CheckException(CreateProcessingInstruction(target, data, &result)) ?
           result : NULL;
  }

  DOMAttrInterface *ScriptCreateAttribute(const std::string &name) {
    DOMAttrInterface *result = NULL;
    return CheckException(CreateAttribute(name, &result)) ? result : NULL;
  }

  ScriptableInterface *ScriptCreateEntityReference(const std::string &name) {
    GGL_UNUSED(name);
    // TODO: if we support DTD.
    return NULL;
  }

  DOMNodeInterface *ScriptImportNode(const DOMNodeInterface *imported_node,
                                     bool deep) {
    DOMNodeInterface *result = NULL;
    return CheckException(ImportNode(imported_node, deep, &result)) ?
           result : NULL;
  }

  static void DummySetProperty(const std::string &name, const Variant &value) {
    GGL_UNUSED(name);
    GGL_UNUSED(value);
  }
  static Variant DummyGetProperty(const std::string &name) {
    GGL_UNUSED(name);
    return Variant();
  }

 private: // Microsoft DOM XMLHttp functions.
  bool IsAsync() const {
    return async_;
  }
  void SetAsync(bool async) {
    async_ = async;
  }

  XMLHttpRequestInterface::State GetReadyState() {
    return http_request_ ? http_request_->GetReadyState() : ready_state_;
  }

  // For Microsoft compatibility.
  bool Load(const Variant &source) {
    GetImpl()->RemoveAllChildren();
    if (source.type() == Variant::TYPE_SCRIPTABLE) {
      DOMDocumentInterface *doc =
          VariantValue<DOMDocumentInterface *>()(source);
      return doc && doc != this && ImportDocument(doc);
    }

    if (source.type() != Variant::TYPE_STRING)
      return false;

    std::string source_str = VariantValue<std::string>()(source);
    if (source_str.empty())
      return false;

    ready_state_ = XMLHttpRequestInterface::UNSENT;
    parse_error_.SetCode(0);

    if (IsAbsolutePath(source_str.c_str())) {
      if (!allow_load_file_) {
        LOG("DOMDocument has no permission to loading from file");
        return false;
      }
      std::string xml;
      ready_state_ = XMLHttpRequestInterface::OPENED;
      onreadystatechange_signal_();
      ready_state_ = XMLHttpRequestInterface::HEADERS_RECEIVED;
      onreadystatechange_signal_();
      if (ReadFileContents(source_str.c_str(), &xml)) {
        ready_state_ = XMLHttpRequestInterface::LOADING;
        onreadystatechange_signal_();
        LoadXML(xml);
      } else {
        parse_error_.SetCode(1);
      }
      ready_state_ = XMLHttpRequestInterface::DONE;
      onreadystatechange_signal_();
      return parse_error_.GetCode() == 0;
    }

    if (!allow_load_http_) {
      LOG("DOMDocument has no permission to loading from network");
      return false;
    }
    if (!http_request_) {
      XMLHttpRequestFactoryInterface *factory = GetXMLHttpRequestFactory();
      if (factory)
        http_request_ = factory->CreateXMLHttpRequest(0, xml_parser_);
      if (!http_request_)
        return false;

      http_request_->Ref();
      onreadystatechange_connection_ = http_request_->ConnectOnReadyStateChange(
          NewSlot(this, &DOMDocument::OnReadyStateChange));
    }

    XMLHttpRequestInterface::ExceptionCode code =
        http_request_->Open("GET", source_str.c_str(), async_, NULL, NULL);
    if (code != XMLHttpRequestInterface::NO_ERR) {
      LOG("DOMDOcument.load XMLHttpRequest exception: %d", code);
      return false;
    }

    code = http_request_->Send(NULL);
    if (code != XMLHttpRequestInterface::NO_ERR) {
      LOG("DOMDOcument.load XMLHttpRequest exception: %d", code);
      return false;
    }
    return parse_error_.GetCode() == 0;
  }

  void OnReadyStateChange() {
    ASSERT(http_request_);
    if (http_request_->GetReadyState() == XMLHttpRequestInterface::DONE) {
      unsigned short status = 0;
      XMLHttpRequestInterface::ExceptionCode code =
          http_request_->GetStatus(&status);
      if (code != XMLHttpRequestInterface::NO_ERR || status != 200 ||
          !http_request_->IsSuccessful()) {
        parse_error_.SetCode(1);
      } else {
        DOMDocumentInterface *response_xml = NULL;
        code = http_request_->GetResponseXML(&response_xml);
        parse_error_.SetCode(code == XMLHttpRequestInterface::NO_ERR &&
                             response_xml && ImportDocument(response_xml) ?
                             0 : 1);
      }
    }
    onreadystatechange_signal_();
  }

  // Copy all contents of another document to this document.
  bool ImportDocument(DOMDocumentInterface *doc) {
    for (DOMNodeInterface *child = doc->GetFirstChild();
         child; child = child->GetNextSibling()) {
      DOMNodeInterface *imported_child;
      if (ImportNode(child, true, &imported_child) != DOM_NO_ERR) {
        LOG("Failed to import node %s(%s) from document",
            child->GetNodeName().c_str(), child->GetNodeValue().c_str());
        GetImpl()->RemoveAllChildren();
        return false;
      }
      imported_child->Ref();
      if (AppendChild(imported_child) != DOM_NO_ERR) {
        GetImpl()->RemoveAllChildren();
        imported_child->Unref();
        return false;
      }
      imported_child->Unref();
    }
    return true;
  }

 private:
  XMLParserInterface *xml_parser_;
  XMLHttpRequestInterface *http_request_;
  Connection *onreadystatechange_connection_;
  Signal0<void> onreadystatechange_signal_;
  ParseError parse_error_;

  // Only used when load from file.
  XMLHttpRequestInterface::State ready_state_ : 4;
  bool allow_load_http_         : 1;
  bool allow_load_file_         : 1;
  bool preserve_whitespace_     : 1;
  bool async_                   : 1;

  static DOMImplementation dom_implementation_;
};

DOMImplementation DOMDocument::dom_implementation_;

} // namespace internal

DOMDocumentInterface *CreateDOMDocument(XMLParserInterface *xml_parser,
                                        bool allow_load_http,
                                        bool allow_load_file) {
  ASSERT(xml_parser);
  return new ::ggadget::internal::DOMDocument(xml_parser,
                                              allow_load_http, allow_load_file);
}

} // namespace ggadget
