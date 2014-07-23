var chrome = {};
chrome.i18n = {};
chrome.i18n.getMessage = function(message) {
  return message;
};
chrome.runtime = {};
chrome.runtime.sendMessage = function(message, opt_callback) {};
chrome.runtime.onMessage = {};
chrome.runtime.onMessage.addListener = function(request, sender, opt_callback) {};
chrome.runtime.getBackgroundPage = function(callback) {};


var inputview = undefined;
