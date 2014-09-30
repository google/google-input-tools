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

#ifndef EXTENSIONS_GTKMOZ_BROWSER_CHILD_H__
#define EXTENSIONS_GTKMOZ_BROWSER_CHILD_H__

#ifndef NO_NAMESPACE
namespace ggadget {
namespace gtkmoz {
#endif

/**
 * Notes about data types of format of messages and return values:
 * - normal string: a UTF8 string literal without any encoding. The sender must
 *   ensure there are no line breaks or kEndOfMessage in the string.
 * - encoded string: a UTF8 string encoded by EncodeJavaScriptString().
 * - encoded value: a variant value encoded based on its data type:
 *   - number: in its string representation (%jd for int64_t, %g for double);
 *   - bool: "true" or "false" (without the quotes);
 *   - string: see above "encoded string".
 *   - non-null object and function: "hobj|wobj <object_id>" (without the
 *     quotes). hobj means a host object; wojb means a browser object.
 *     object_id is of type size_t.
 *   - null: "null" (without the quotes);
 *   - undefined: "undefined" (without the quotes).
 *
 * A controller or child can also return result values in format
 * "exception: ...." to tell the other part that a JavaScript exception occurs.
 * The other part should throw an JS exception when it receives such a result.
 */

/**
 * End of a command and feedback message.
 * "\"\"\"" is used to disambiguate from encoded strings, because consecutive
 * three quotes never occur in encoded strings.
 */
const char kEndOfMessage[] = "\"\"\"EOM\"\"\"";
/** End of message tag including the proceeding and succeeding line breaks. */
const char kEndOfMessageFull[] = "\n\"\"\"EOM\"\"\"\n";
const size_t kEOMLength = sizeof(kEndOfMessage) - 1;
const size_t kEOMFullLength = sizeof(kEndOfMessageFull) - 1;

/**
 * The prefix of a reply message. A reply message is a single line starting
 * with kReplyPrefix.
 */
const char kReplyPrefix[] = "R ";
const size_t kReplyPrefixLength = sizeof(kReplyPrefix) - 1;

/**
 * The controller tells the child to open a new browser.
 *
 * Message format:
 * <code>
 * NEW\n
 * Browser ID (size_t)\n
 * Socket ID (GdkNativeWindow in 0x<hex> format)\n
 * """EOM"""\n
 * </code>
 *
 * The browser must immediately reply a message containing only kReplyPrefix.
 */
const char kNewBrowserCommand[] = "NEW";

/**
 * The controller sets the content to display by the browser child.
 *
 * Message format:
 * <code>
 * CONTENT\n
 * Browser ID (size_t)\n
 * Mime type (normal string)\n
 * Contents (encoded string)\n
 * """EOM"""\n
 * </code>
 *
 * The browser must immediately reply a message containing only kReplyPrefix.
 */
const char kSetContentCommand[] = "CONTENT";

/**
 * The controller lets browser child to open a URL.
 *
 * Message format:
 * <code>
 * URL\n
 * Browser ID (size_t)\n
 * The URL (normal string)\n
 * """EOM"""\n
 * </code>
 *
 * The browser must immediately reply a message containing only kReplyPrefix.
 */
const char kOpenURLCommand[] = "URL";

/**
 * The controller wants to close a browser.
 *
 * Message format:
 * <code>
 * CLOSE\n
 * Browser ID (size_t)\n
 * """EOM"""\n
 * </code>
 *
 * The browser must immediately reply a message containing only kReplyPrefix.
 */
const char kCloseBrowserCommand[] = "CLOSE";

/**
 * The controller wants to get a property of a child browser object.
 * The message and return value format is the same as kGetPropertyFeedback.
 */
const char kGetPropertyCommand[] = "GET";
/**
 * The controller wants to set a property of a child browser object.
 * The message and return value format is the same as kSetPropertyFeedback.
 */
const char kSetPropertyCommand[] = "SET";
/**
 * The controller wants to call a child browser function.
 * The message and return value format is the same as kCallFeedback.
 */
const char kCallCommand[] = "CALL";
/**
 * The controller wants to un-reference a child browser object.
 * The message and return value format is the same as kUnrefFeedback.
 */
const char kUnrefCommand[] = "UNREF";

/**
 * The controller sets the always_open_new_window flag.
 * See @c BrowserElement::SetAlwaysOpenNewWindow() for details.
 *
 * Message format:
 * <code>
 * AONW\n
 * Browser ID (size_t)\n
 * 1|0
 * """EOM"""\n
 * </code>
 *
 * The browser must immediately reply a message containing only kReplyPrefix.
 */
const char kSetAlwaysOpenNewWindowCommand[] = "AONW";

/**
 * The controller wants the child browser to quit.
 *
 * Message format:
 * <code>
 * QUIT\n
 * """EOM"""\n
 * </code>
 *
 * No reply is needed.
 */
const char kQuitCommand[] = "QUIT";

/**
 * The browser child tells the controller that the script wants to get the
 * value of a host object property.
 *
 * Message format:
 * <code>
 * GET\n
 * Browser ID (size_t)\n
 * Object ID (size_t)\n
 * Property key (encoded string or int)\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing kReplyPrefix
 * and the return value (encoded value) through the return value channel.
 */
const char kGetPropertyFeedback[] = "GET";

/**
 * The browser child tells the controller that the script has set the value of
 * a host object property.
 *
 * Message format:
 * <code>
 * SET\n
 * Browser ID (size_t)\n
 * Object ID (size_t)\n
 * Property key (encoded string or int)\n
 * Property value (encoded value)\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing only kReplyPrefix.
 */
const char kSetPropertyFeedback[] = "SET";

/**
 * The browser child tells the controller that the script has invoked
 * controller function.
 *
 * Message format:
 * <code>
 * CALL\n
 * Browser ID (size_t)\n
 * Callee object ID (size_t)\n
 * ID of 'this' object (size_t)\n
 * [The first parameter (encoded value)\n
 * ...
 * The last parameter (encoded value)\n]
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing kReplyPrefix and
 * the return value (encoded value) through the return value channel.
 */
const char kCallFeedback[] = "CALL";

/**
 * The browser child tells the controller that the script has finished using
 * a host object.
 *
 * Message format:
 * <code>
 * UNREF\n
 * Browser ID (size_t)\n
 * Object ID (size_t)\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing only kReplyPrefix.
 */
const char kUnrefFeedback[] = "UNREF";

/**
 * The browser child tells the controller that the browser is about to open
 * a URL in a new window.
 *
 * Message format:
 * <code>
 * OPEN\n
 * Browser ID (size_t)\n
 * URL (normal string)\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing kReplyPrefix and
 * the return value "1" or "0" indicating if the URL has been/will be opened or
 * not opened by the controller respectively.
 */
const char kOpenURLFeedback[] = "OPEN";

/**
 * The browser child tells the controller that the browser is about to go to
 * a URL in the current window or some sub-frame.
 *
 * Message format:
 * <code>
 * GOTO\n
 * Browser ID (size_t)\n
 * URL (normal string)\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing kReplyPrefix and
 * the return value "1" or "0" indicating if the URL has been/will be opened or
 * not opened by the controller respectively.
 */
const char kGoToURLFeedback[] = "GOTO";

/**
 * The browser child tells the controller that the browser has encountered a
 * network error.
 *
 * Message format:
 * <code>
 * NETERR\n
 * Browser ID (size_t)\n
 * Error URL (normal string, about:neterror?...)\n
 * """EOM"""\n
 *
 * The controller must immediately reply a message containing kReplyPrefix and
 * the return value "1" or "0" indicating if the error has been/will be handled
 * or not handled by the controller respectively.
 * </code>
 */
const char kNetErrorFeedback[] = "ERR";

/**
 * The browser child periodically pings the controller to check if the
 * controller died.
 *
 * Message format:
 * <code>
 * PING\n
 * """EOM"""\n
 * </code>
 *
 * The controller must immediately reply a message containing "R ACK\n".
 */
const char kPingFeedback[] = "PING";
const char kPingAck[] = "ACK";
const char kPingAckFull[] = "R ACK\n";
const size_t kPingAckFullLength = sizeof(kPingAckFull) - 1;
const int kPingInterval = 30000;  // 30 seconds.

#ifndef NO_NAMESPACE
} // namespace gtkmoz
} // namespace ggadget
#endif

#endif // EXTENSIONS_GTKMOZ_BROWSER_CHILD_H__
