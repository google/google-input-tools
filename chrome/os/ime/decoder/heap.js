// Copyright 2013 The ChromeOS IME Authors. All Rights Reserved.
// limitations under the License.
// See the License for the specific language governing permissions and
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// distributed under the License is distributed on an "AS-IS" BASIS,
// Unless required by applicable law or agreed to in writing, software
//
//      http://www.apache.org/licenses/LICENSE-2.0
//
// You may obtain a copy of the License at
// you may not use this file except in compliance with the License.
// Licensed under the Apache License, Version 2.0 (the "License");
//

/**
 * @fileoverview Datastructure: Heap.
 *
 * This file provides the implementation of a Heap datastructure. Smaller keys
 * rise to the top.
 *
 * The big-O notation for all operations are below:
 * <pre>
 *  Method          big-O
 * ----------------------------------------------------------------------------
 * - insert         O(logn)
 * - remove         O(logn)
 * - peek           O(1)
 * - contains       O(n)
 * </pre>
 */

goog.provide('goog.ime.offline.Heap');

goog.require('goog.array');
goog.require('goog.object');
goog.require('goog.structs.Node');



/**
 * Class for a Heap datastructure.
 *
 * @param {goog.ime.offline.Heap|Object=} opt_heap Optional
 *     goog.ime.offline.Heap or Object to initialize heap with.
 * @constructor
 */
goog.ime.offline.Heap = function(opt_heap) {
  /**
   * The nodes of the heap.
   * @private
   * @type {Array.<goog.structs.Node>}
   */
  this.nodes_ = [];

  if (opt_heap) {
    this.insertAll(opt_heap);
  }
};


/**
 * Insert the given value into the heap with the given key.
 * @param {*} key The key.
 * @param {*} value The value.
 */
goog.ime.offline.Heap.prototype.insert = function(key, value) {
  var node = new goog.structs.Node(key, value);
  var nodes = this.nodes_;
  nodes.push(node);
  this.moveUp_(nodes.length - 1);
};


/**
 * Increases the given value in the heap with the given key.
 * @param {*} key The key.
 * @param {*} value The value.
 */
goog.ime.offline.Heap.prototype.increase = function(key, value) {
  var index = goog.array.findIndex(this.nodes_, function(node) {
    return node.getValue() == value;
  });

  if (index < 0) {
    this.insert(key, value);
  } else {
    var oldKey = this.nodes_[index].getKey();
    if (key > oldKey) {
      this.nodes_[index] = new goog.structs.Node(key, value);
      this.moveDown_(index);
    }
  }
};


/**
 * Decreases the given value in the heap with the given key.
 * @param {*} key The key.
 * @param {*} value The value.
 */
goog.ime.offline.Heap.prototype.decrease = function(key, value) {
  var index = goog.array.findIndex(this.nodes_, function(node) {
    return node.getValue() == value;
  });

  if (index < 0) {
    this.insert(key, value);
  } else {
    var oldKey = this.nodes_[index].getKey();
    if (key < oldKey) {
      this.nodes_[index] = new goog.structs.Node(key, value);
      this.moveUp_(index);
    }
  }
};


/**
 * Sets the given value in the heap with the given key.
 * @param {*} key The key.
 * @param {*} value The value.
 */
goog.ime.offline.Heap.prototype.set = function(key, value) {
  var index = goog.array.findIndex(this.nodes_, function(node) {
    return node.getValue() == value;
  });

  if (index < 0) {
    this.insert(key, value);
  } else {
    var oldKey = this.nodes_[index].getKey();
    this.nodes_[index] = new goog.structs.Node(key, value);
    if (key < oldKey) {
      this.moveUp_(index);
    } else {
      this.moveDown_(index);
    }
  }
};


/**
 * Adds multiple key-value pairs from another goog.ime.offline.Heap or Object
 * @param {goog.ime.offline.Heap|Object} heap Object containing the data to add.
 */
goog.ime.offline.Heap.prototype.insertAll = function(heap) {
  var keys, values;
  if (heap instanceof goog.ime.offline.Heap) {
    keys = heap.getKeys();
    values = heap.getValues();

    // If it is a heap and the current heap is empty, I can realy on the fact
    // that the keys/values are in the correct order to put in the underlying
    // structure.
    if (heap.getCount() <= 0) {
      var nodes = this.nodes_;
      for (var i = 0; i < keys.length; i++) {
        nodes.push(new goog.structs.Node(keys[i], values[i]));
      }
      return;
    }
  } else {
    keys = goog.object.getKeys(heap);
    values = goog.object.getValues(heap);
  }

  for (var i = 0; i < keys.length; i++) {
    this.insert(keys[i], values[i]);
  }
};


/**
 * Retrieves and removes the root value of this heap.
 * @return {*} The value removed from the root of the heap.  Returns
 *     undefined if the heap is empty.
 */
goog.ime.offline.Heap.prototype.remove = function() {
  var nodes = this.nodes_;
  var count = nodes.length;
  var rootNode = nodes[0];
  if (count <= 0) {
    return undefined;
  } else if (count == 1) {
    goog.array.clear(nodes);
  } else {
    nodes[0] = nodes.pop();
    this.moveDown_(0);
  }
  return rootNode.getValue();
};


/**
 * Retrieves but does not remove the root value of this heap.
 * @return {*} The value at the root of the heap. Returns
 *     undefined if the heap is empty.
 */
goog.ime.offline.Heap.prototype.peek = function() {
  var nodes = this.nodes_;
  if (nodes.length == 0) {
    return undefined;
  }
  return nodes[0].getValue();
};


/**
 * Retrieves but does not remove the key of the root node of this heap.
 * @return {*} The key at the root of the heap. Returns undefined if the
 *     heap is empty.
 */
goog.ime.offline.Heap.prototype.peekKey = function() {
  return this.nodes_[0] && this.nodes_[0].getKey();
};


/**
 * Moves the node at the given index down to its proper place in the heap.
 * @param {number} index The index of the node to move down.
 * @private
 */
goog.ime.offline.Heap.prototype.moveDown_ = function(index) {
  var nodes = this.nodes_;
  var count = nodes.length;

  // Save the node being moved down.
  var node = nodes[index];
  // While the current node has a child.
  while (index < (count >> 1)) {
    var leftChildIndex = this.getLeftChildIndex_(index);
    var rightChildIndex = this.getRightChildIndex_(index);

    // Determine the index of the smaller child.
    var smallerChildIndex = rightChildIndex < count &&
        nodes[rightChildIndex].getKey() < nodes[leftChildIndex].getKey() ?
        rightChildIndex : leftChildIndex;

    // If the node being moved down is smaller than its children, the node
    // has found the correct index it should be at.
    if (nodes[smallerChildIndex].getKey() > node.getKey()) {
      break;
    }

    // If not, then take the smaller child as the current node.
    nodes[index] = nodes[smallerChildIndex];
    index = smallerChildIndex;
  }
  nodes[index] = node;
};


/**
 * Moves the node at the given index up to its proper place in the heap.
 * @param {number} index The index of the node to move up.
 * @private
 */
goog.ime.offline.Heap.prototype.moveUp_ = function(index) {
  var nodes = this.nodes_;
  var node = nodes[index];

  // While the node being moved up is not at the root.
  while (index > 0) {
    // If the parent is less than the node being moved up, move the parent down.
    var parentIndex = this.getParentIndex_(index);
    if (nodes[parentIndex].getKey() > node.getKey()) {
      nodes[index] = nodes[parentIndex];
      index = parentIndex;
    } else {
      break;
    }
  }
  nodes[index] = node;
};


/**
 * Gets the index of the left child of the node at the given index.
 * @param {number} index The index of the node to get the left child for.
 * @return {number} The index of the left child.
 * @private
 */
goog.ime.offline.Heap.prototype.getLeftChildIndex_ = function(index) {
  return index * 2 + 1;
};


/**
 * Gets the index of the right child of the node at the given index.
 * @param {number} index The index of the node to get the right child for.
 * @return {number} The index of the right child.
 * @private
 */
goog.ime.offline.Heap.prototype.getRightChildIndex_ = function(index) {
  return index * 2 + 2;
};


/**
 * Gets the index of the parent of the node at the given index.
 * @param {number} index The index of the node to get the parent for.
 * @return {number} The index of the parent.
 * @private
 */
goog.ime.offline.Heap.prototype.getParentIndex_ = function(index) {
  return (index - 1) >> 1;
};


/**
 * Gets the values of the heap.
 * @return {Array} The values in the heap.
 */
goog.ime.offline.Heap.prototype.getValues = function() {
  var nodes = this.nodes_;
  var rv = [];
  var l = nodes.length;
  for (var i = 0; i < l; i++) {
    rv.push(nodes[i].getValue());
  }
  return rv;
};


/**
 * Gets the keys of the heap.
 * @return {Array} The keys in the heap.
 */
goog.ime.offline.Heap.prototype.getKeys = function() {
  var nodes = this.nodes_;
  var rv = [];
  var l = nodes.length;
  for (var i = 0; i < l; i++) {
    rv.push(nodes[i].getKey());
  }
  return rv;
};


/**
 * Whether the heap contains the given value.
 * @param {Object} val The value to check for.
 * @return {boolean} Whether the heap contains the value.
 */
goog.ime.offline.Heap.prototype.containsValue = function(val) {
  return goog.array.some(this.nodes_, function(node) {
    return node.getValue() == val;
  });
};


/**
 * Whether the heap contains the given key.
 * @param {Object} key The key to check for.
 * @return {boolean} Whether the heap contains the key.
 */
goog.ime.offline.Heap.prototype.containsKey = function(key) {
  return goog.array.some(this.nodes_, function(node) {
    return node.getKey() == key;
  });
};


/**
 * Clones a heap and returns a new heap
 * @return {goog.ime.offline.Heap} A new goog.ime.offline.Heap with the same
 *     key-value pairs.
 */
goog.ime.offline.Heap.prototype.clone = function() {
  return new goog.ime.offline.Heap(this);
};


/**
 * The number of key-value pairs in the map
 * @return {number} The number of pairs.
 */
goog.ime.offline.Heap.prototype.getCount = function() {
  return this.nodes_.length;
};


/**
 * Returns true if this heap contains no elements.
 * @return {boolean} Whether this heap contains no elements.
 */
goog.ime.offline.Heap.prototype.isEmpty = function() {
  return goog.array.isEmpty(this.nodes_);
};


/**
 * Removes all elements from the heap.
 */
goog.ime.offline.Heap.prototype.clear = function() {
  goog.array.clear(this.nodes_);
};
