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

#include "extensions/google_gadget_manager/gadgets_metadata.h"
#include "ggadget/file_manager_factory.h"
#include "ggadget/logger.h"
#include "ggadget/tests/init_extensions.h"
#include "ggadget/tests/mocked_file_manager.h"
#include "ggadget/tests/mocked_xml_http_request.h"
#include "unittest/gtest.h"

using namespace ggadget;
using namespace ggadget::google;

MockedFileManager g_mocked_fm;

#define GADGET_ID1 "12345678-5274-4C6C-A59F-1CC60A8B778B"

const char *plugin_xml_file =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin author='Author1' id='id1' creation_date='November 17, 2005'"
  " download_url='/url&amp;' guid='" GADGET_ID1 "'/>\n"
  // The following is a bad data because of no uuid or download_url.
  " <plugin author='Author2' id='id2' updated_date='December 1, 2007'/>\n"
  " <bad-tag/>\n"
  " <plugin author='Author3' id='id3' download_url='/uu' creation_date='May 10, 2007'>\n"
  "  <title locale='en'>Title en</title>\n"
  "  <description locale='en'>Description en</description>\n"
  "  <title locale='nl'>Title nl&quot;&lt;&gt;&amp;</title>\n"
  "  <description locale='nl'>Description nl</description>\n"
  " </plugin>\n"
  " <plugin author='Author4' id='id4' download_url='/xx' updated_date='June 1, 2006'/>\n"
  "</plugins>\n";

const char *plugin_xml_network =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin guid='" GADGET_ID1 "' rank='9.9'/>\n"
  " <plugin download_url='/uu' id='id3' updated_date='December 20, 2007'>\n"
  "  <title locale='ja'>Title ja</title>\n"
  "  <description locale='ja'>Description ja</description>\n"
  " </plugin>\n"
  " <plugin download_url='/new' id='id5' updated_date='December 18, 2007'>\n"
  "  <title locale='ja'>New Title ja</title>\n"
  "  <description locale='ja'>New Description ja</description>\n"
  " </plugin>\n"
  "</plugins>\n";

const char *plugin_xml_network_bad =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin guid='" GADGET_ID1 "' rank='9.9'/>\n";

const char *plugin_xml_network_extra_plugin =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin guid='" GADGET_ID1 "' rank='9.9'/>\n"
  " <plugin download_url='/uu' id='id3' updated_date='December 20, 2007'>\n"
  "  <title locale='ja'>Title ja</title>\n"
  "  <description locale='ja'>Description ja</description>\n"
  " </plugin>\n"
  " <plugin download_url='/new' id='id5' updated_date='December 18, 2007'>\n"
  "  <title locale='ja'>New Title ja</title>\n"
  "  <description locale='ja'>New Description ja</description>\n"
  " </plugin>\n"
  " <plugin guid='EXTRA_PLUGIN_GUID' rank='9.9'/>\n"
  "</plugins>\n";

const char *expected_xml_file_merge_network =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin download_url=\"/new\" id=\"id5\" updated_date=\"December 18, 2007\">\n"
  "  <title locale=\"ja\">New Title ja</title>\n"
  "  <description locale=\"ja\">New Description ja</description>\n"
  " </plugin>\n"
  " <plugin download_url=\"/uu\" id=\"id3\" updated_date=\"December 20, 2007\">\n"
  "  <title locale=\"ja\">Title ja</title>\n"
  "  <description locale=\"ja\">Description ja</description>\n"
  " </plugin>\n"
  " <plugin author=\"Author1\" creation_date=\"November 17, 2005\""
  " download_url=\"/url&amp;\" guid=\"" GADGET_ID1 "\" id=\"id1\" rank=\"9.9\"/>\n"
  "</plugins>\n";

const char *xml_from_network =
  "<?xml version=\"1.0\" encoding=\"utf-8\"?>\n"
  "<plugins>\n"
  " <plugin download_url=\"/new\" id=\"id5\" updated_date=\"December 18, 2007\">\n"
  "  <title locale=\"ja\">New Title ja</title>\n"
  "  <description locale=\"ja\">New Description ja</description>\n"
  " </plugin>\n"
  " <plugin download_url=\"/uu\" id=\"id3\" updated_date=\"December 20, 2007\">\n"
  "  <title locale=\"ja\">Title ja</title>\n"
  "  <description locale=\"ja\">Description ja</description>\n"
  " </plugin>\n"
  "</plugins>\n";

TEST(GadgetsMetadata, InitialLoadNull) {
  g_mocked_fm.data_.clear();
  GadgetsMetadata gmd;
  EXPECT_EQ(0U, gmd.GetAllGadgetInfo()->size());
  EXPECT_EQ(std::string(kPluginsXMLLocation), g_mocked_fm.requested_file_);
  g_mocked_fm.requested_file_.clear();
}

TEST(GadgetsMetadata, InitialLoadFail) {
  g_mocked_fm.should_fail_ = true;
  GadgetsMetadata gmd;
  EXPECT_EQ(0U, gmd.GetAllGadgetInfo()->size());
  g_mocked_fm.should_fail_ = false;
  EXPECT_EQ(std::string(kBuiltinGadgetsXMLLocation),
            g_mocked_fm.requested_file_);
  g_mocked_fm.requested_file_.clear();
}

void ExpectFileData(const GadgetsMetadata &data) {
  const GadgetInfoMap &map = *data.GetAllGadgetInfo();
  ASSERT_EQ(3U, map.size());
  const GadgetInfo &info = map.find(GADGET_ID1)->second;
  ASSERT_EQ(5U, info.attributes.size());
  EXPECT_EQ(std::string("Author1"), info.attributes.find("author")->second);
  EXPECT_EQ(std::string("/url&"), info.attributes.find("download_url")->second);
  EXPECT_EQ(0U, info.titles.size());
  EXPECT_EQ(0U, info.descriptions.size());

  const GadgetInfo &info1 = map.find("/uu")->second;
  ASSERT_EQ(4U, info1.attributes.size());
  ASSERT_EQ(2U, info1.titles.size());
  ASSERT_EQ(2U, info1.descriptions.size());
  EXPECT_EQ(std::string("Title en"), info1.titles.find("en")->second);
  EXPECT_EQ(std::string("Title nl\"<>&"), info1.titles.find("nl")->second);
  EXPECT_EQ(std::string("Description en"),
            info1.descriptions.find("en")->second);
  EXPECT_EQ(std::string("Description nl"),
            info1.descriptions.find("nl")->second);
}

TEST(GadgetsMetadata, InitialLoadData) {
  g_mocked_fm.data_[kPluginsXMLLocation] = plugin_xml_file;
  GadgetsMetadata data;
  ExpectFileData(data);
  data.FreeMemory();
  ExpectFileData(data);
}

TEST(GadgetsMetadata, IncrementalUpdateNULLCallback) {
  g_mocked_fm.data_[kPluginsXMLLocation] = plugin_xml_file;
  GadgetsMetadata data;
  EXPECT_EQ(std::string(kBuiltinGadgetsXMLLocation),
            g_mocked_fm.requested_file_);
  g_mocked_fm.requested_file_.clear();
  MockedXMLHttpRequest request(200, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  data.UpdateFromServer(false, &request, NULL);
  EXPECT_EQ(std::string(kPluginsXMLLocation), g_mocked_fm.requested_file_);
  g_mocked_fm.requested_file_.clear();
  EXPECT_EQ(std::string(expected_xml_file_merge_network),
            g_mocked_fm.data_[kPluginsXMLLocation]);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=05092007",
            request.requested_url_);
}

bool g_callback_called = false;
bool g_request_success = false;
bool g_parsing_success = false;
void Callback(bool request_success, bool parsing_success) {
  g_callback_called = true;
  g_request_success = request_success;
  g_parsing_success = parsing_success;
}

TEST(GadgetsMetadata, IncrementalUpdateWithCallback) {
  g_mocked_fm.data_[kPluginsXMLLocation] = plugin_xml_file;
  GadgetsMetadata data;
  MockedXMLHttpRequest request(200, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = g_request_success = g_parsing_success = false;
  data.UpdateFromServer(false, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_TRUE(g_request_success);
  EXPECT_TRUE(g_parsing_success);
  EXPECT_EQ(std::string(expected_xml_file_merge_network),
            g_mocked_fm.data_[kPluginsXMLLocation]);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=05092007",
            request.requested_url_);
}

TEST(GadgetsMetadata, IncrementalUpdateWithCallbackAfterFreeMemory) {
  g_mocked_fm.data_[kPluginsXMLLocation] = plugin_xml_file;
  GadgetsMetadata data;
  MockedXMLHttpRequest request(200, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = g_request_success = g_parsing_success = false;
  data.FreeMemory();
  data.UpdateFromServer(false, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_TRUE(g_request_success);
  EXPECT_TRUE(g_parsing_success);
  EXPECT_EQ(std::string(expected_xml_file_merge_network),
            g_mocked_fm.data_[kPluginsXMLLocation]);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=05092007",
            request.requested_url_);
}

TEST(GadgetsMetadata, IncrementalUpdateRequestFail) {
  g_mocked_fm.data_[kPluginsXMLLocation] = plugin_xml_file;
  GadgetsMetadata data;
  MockedXMLHttpRequest request(404, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = false;
  g_request_success = g_parsing_success = true;
  data.UpdateFromServer(false, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_FALSE(g_request_success);
  EXPECT_FALSE(g_parsing_success);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=05092007",
            request.requested_url_);
  // data should remain unchanged.
  ExpectFileData(data);
}

TEST(GadgetsMetadata, IncrementalUpdateParsingFail1) {
  g_mocked_fm.data_[kPluginsXMLLocation] = plugin_xml_file;
  GadgetsMetadata data;
  MockedXMLHttpRequest request(200, plugin_xml_network_bad);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = g_request_success = false;
  g_parsing_success = true;
  data.UpdateFromServer(false, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_TRUE(g_request_success);
  EXPECT_FALSE(g_parsing_success);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=05092007",
            request.requested_url_);
  // data should remain unchanged.
  ExpectFileData(data);
}

TEST(GadgetsMetadata, IncrementalUpdateParsingFail2) {
  g_mocked_fm.data_[kPluginsXMLLocation] = plugin_xml_file;
  GadgetsMetadata data;
  MockedXMLHttpRequest request(200, plugin_xml_network_extra_plugin);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = g_request_success = false;
  g_parsing_success = true;
  data.UpdateFromServer(false, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_TRUE(g_request_success);
  EXPECT_FALSE(g_parsing_success);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=05092007",
            request.requested_url_);
  // data should remain unchanged.
  ExpectFileData(data);
}

TEST(GadgetsMetadata, FullDownload) {
  g_mocked_fm.data_[kPluginsXMLLocation] = plugin_xml_file;
  GadgetsMetadata data;
  MockedXMLHttpRequest request(200, xml_from_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = g_request_success = g_parsing_success = false;
  data.UpdateFromServer(true, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_TRUE(g_request_success);
  EXPECT_TRUE(g_parsing_success);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=01011980",
            request.requested_url_);
  EXPECT_EQ(std::string(xml_from_network),
            g_mocked_fm.data_[kPluginsXMLLocation]);
}

TEST(GadgetsMetadata, FullDownloadRequestFail) {
  g_mocked_fm.data_[kPluginsXMLLocation] = plugin_xml_file;
  GadgetsMetadata data;
  MockedXMLHttpRequest request(404, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = false;
  g_request_success = g_parsing_success = true;
  data.UpdateFromServer(true, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_FALSE(g_request_success);
  EXPECT_FALSE(g_parsing_success);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=01011980",
            request.requested_url_);
  // data should remain unchanged.
  ExpectFileData(data);
}

TEST(GadgetsMetadata, FullDownloadParsingFail) {
  g_mocked_fm.data_[kPluginsXMLLocation] = plugin_xml_file;
  GadgetsMetadata data;
  MockedXMLHttpRequest request(200, plugin_xml_network);
  // Different from real impl, the following UpdateFromServer will finish
  // synchronously.
  g_callback_called = g_request_success = false;
  g_parsing_success = true;
  data.UpdateFromServer(true, &request, NewSlot(&Callback));
  EXPECT_TRUE(g_callback_called);
  EXPECT_TRUE(g_request_success);
  EXPECT_FALSE(g_parsing_success);
  EXPECT_EQ(std::string(kPluginsXMLRequestPrefix) + "&diff_from_date=01011980",
            request.requested_url_);
  // data should remain unchanged.
  ExpectFileData(data);
}

int main(int argc, char **argv) {
  testing::ParseGTestFlags(&argc, argv);

  static const char *kExtensions[] = {
    "libxml2_xml_parser/libxml2-xml-parser",
  };
  INIT_EXTENSIONS(argc, argv, kExtensions);
  SetGlobalFileManager(&g_mocked_fm);

  return RUN_ALL_TESTS();
}
