/*
 * (C) Copyright 2013 Kurento (http://kurento.org/)
 *
 * All rights reserved. This program and the accompanying materials
 * are made available under the terms of the GNU Lesser General Public License
 * (LGPL) version 2.1 which accompanies this distribution, and is available at
 * http://www.gnu.org/licenses/lgpl-2.1.html
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 */

#include "server_test_base.hpp"
#include <boost/test/unit_test.hpp>

#include "KmsMediaServer_constants.h"
#include "KmsMediaDataType_constants.h"
#include "KmsMediaErrorCodes_constants.h"

#include "KmsMediaUriEndPointType_constants.h"
#include "KmsMediaPlayerEndPointType_constants.h"
#include "KmsMediaRecorderEndPointType_constants.h"
#include "KmsMediaHttpEndPointType_constants.h"
#include "KmsMediaZBarFilterType_constants.h"
#include "KmsMediaJackVaderFilterType_constants.h"
#include "KmsMediaPointerDetectorFilterType_constants.h"
#include "KmsMediaWebRtcEndPointType_constants.h"
#include "KmsMediaPlateDetectorFilterType_constants.h"

#include "utils/marshalling.hpp"
#include "utils/utils.hpp"
#include "HandlerTest.hpp"

#include <gst/gst.h>
#include <glibmm/timeval.h>

#include "common/MediaSet.hpp"

#define GST_CAT_DEFAULT _server_test_
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME "server_test"

#define AUTO_RELEASE_INTERVAL 1

using namespace kurento;

static std::map<std::string, KmsMediaParam> emptyParams = std::map<std::string, KmsMediaParam> ();

class ClientHandler : public F
{
public:
  ClientHandler() : F(), handlerTest (new HandlerTest() ) {
    handlerTest->start ();
  };

  ~ClientHandler () {
    handlerTest->stop();
  }

protected:
  boost::shared_ptr<HandlerTest> handlerTest;

  void check_version ();
  void check_use_released_media_pipeline ();
  void check_auto_released_media_pipeline ();
  void check_keep_alive_media_pipeline ();
  void check_exclude_from_gc ();
  void check_parent ();
  void check_get_parent_of_media_pipeline ();
  void check_getMediaPipeline ();
  void check_same_token ();
  void check_get_media_element_from_pad ();

#if 0 /* Temporally disabled */
  void check_type ();
#endif

  void check_player_end_point ();
  void check_player_end_point_signal_errors ();
  void check_recorder_end_point ();
  void check_http_end_point ();
  void check_zbar_filter ();
  void check_jackvader_filter ();
  void check_pointer_detector_filter ();
  void check_web_rtc_end_point ();
  void check_plate_detector_filter();
};

void
ClientHandler::check_version ()
{
  int32_t gotVersion;

  gotVersion = client->getVersion();
  BOOST_CHECK_EQUAL (gotVersion, g_KmsMediaServer_constants.VERSION);
}

#if 0 /* Temporally disabled */
void
ClientHandler::check_type ()
{
  MediaObjectId mediaPipeline = MediaObjectId();
  MediaObjectId mo = MediaObjectId();

  client->createMediaPipeline (mediaPipeline, 0);
  BOOST_CHECK (mediaPipeline.type.__isset.mediaObject);
  BOOST_CHECK_EQUAL (mediaPipeline.type.mediaObject, MediaObjectType::type::MEDIA_PIPELINE);

  client->createSdpEndPoint (mo, mediaPipeline, SdpEndPointType::type::RTP_END_POINT);
  BOOST_CHECK (mo.type.__isset.sdpEndPoint);
  BOOST_CHECK_EQUAL (mo.type.sdpEndPoint, SdpEndPointType::type::RTP_END_POINT);

  client->createSdpEndPoint (mo, mediaPipeline, SdpEndPointType::type::WEBRTC_END_POINT);
  BOOST_CHECK (mo.type.__isset.sdpEndPoint);
  BOOST_CHECK_EQUAL (mo.type.sdpEndPoint, SdpEndPointType::type::WEBRTC_END_POINT);

  client->createUriEndPoint (mo, mediaPipeline, UriEndPointType::type::PLAYER_END_POINT, "");
  BOOST_CHECK (mo.type.__isset.uriEndPoint);
  BOOST_CHECK_EQUAL (mo.type.uriEndPoint, UriEndPointType::type::PLAYER_END_POINT);

  client->createUriEndPoint (mo, mediaPipeline, UriEndPointType::type::RECORDER_END_POINT, "");
  BOOST_CHECK (mo.type.__isset.uriEndPoint);
  BOOST_CHECK_EQUAL (mo.type.uriEndPoint, UriEndPointType::type::RECORDER_END_POINT);

  client->createHttpEndPoint (mo, mediaPipeline);
  BOOST_CHECK (mo.type.__isset.endPoint);
  BOOST_CHECK_EQUAL (mo.type.endPoint, EndPointType::type::HTTP_END_POINT);

  client->createMixer (mo, mediaPipeline, MixerType::type::MAIN_MIXER);
  BOOST_CHECK (mo.type.__isset.mixerType);
  BOOST_CHECK_EQUAL (mo.type.mixerType, MixerType::type::MAIN_MIXER);

  client->release (mediaPipeline);
}
#endif

void
ClientHandler::check_use_released_media_pipeline ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef mo = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;

  client->createMediaPipeline (mediaPipeline);
  client->release (mediaPipeline);
  createKmsMediaUriEndPointConstructorParams (params, "file:///tmp/f.webm");

  try {
    client->createMediaElementWithParams (mo, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params);
    BOOST_FAIL ("Use a released MediaPipeline must throw a KmsMediaServerException");
  } catch (const KmsMediaServerException &e) {
    BOOST_CHECK_EQUAL (g_KmsMediaErrorCodes_constants.MEDIA_OBJECT_NOT_FOUND, e.errorCode);
  }
}

void
ClientHandler::check_auto_released_media_pipeline ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef moA = KmsMediaObjectRef();
  KmsMediaObjectRef moB = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;
  std::map<std::string, KmsMediaParam> mediaObjectparams;

  createKmsMediaObjectConstructorParams (mediaObjectparams, false,
                                         AUTO_RELEASE_INTERVAL);
  createKmsMediaUriEndPointConstructorParams (params, "file:///tmp/f.webm");
  params.insert (mediaObjectparams.begin(), mediaObjectparams.end() );

  client->createMediaPipelineWithParams (mediaPipeline, mediaObjectparams);
  g_usleep ( (2 * AUTO_RELEASE_INTERVAL + 1) * G_USEC_PER_SEC);

  try {
    client->keepAlive (mediaPipeline);
    BOOST_FAIL ("Use an auto released MediaPipeline must throw a KmsMediaServerException");
  } catch (const KmsMediaServerException &e) {
    BOOST_CHECK_EQUAL (g_KmsMediaErrorCodes_constants.MEDIA_OBJECT_NOT_FOUND, e.errorCode);
  }

  client->createMediaPipelineWithParams (mediaPipeline, mediaObjectparams);
  g_usleep (AUTO_RELEASE_INTERVAL * G_USEC_PER_SEC);
  BOOST_REQUIRE_NO_THROW (client->createMediaElementWithParams (moA, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params););

  g_usleep (AUTO_RELEASE_INTERVAL * G_USEC_PER_SEC);
  BOOST_REQUIRE_NO_THROW (client->keepAlive (mediaPipeline) );
  BOOST_REQUIRE_NO_THROW (client->keepAlive (moA) );

  g_usleep (AUTO_RELEASE_INTERVAL * G_USEC_PER_SEC);
  BOOST_REQUIRE_NO_THROW (client->createMediaElementWithParams (moB, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params););

  g_usleep ( AUTO_RELEASE_INTERVAL * G_USEC_PER_SEC);
  BOOST_REQUIRE_NO_THROW (client->keepAlive (moB) );
  g_usleep ( AUTO_RELEASE_INTERVAL * G_USEC_PER_SEC);

  try {
    client->keepAlive (moA);
    BOOST_FAIL ("Use an auto released MediaObject must throw a KmsMediaServerException");
  } catch (const KmsMediaServerException &e) {
    BOOST_CHECK_EQUAL (g_KmsMediaErrorCodes_constants.MEDIA_OBJECT_NOT_FOUND, e.errorCode);
  }

  BOOST_REQUIRE_NO_THROW (client->keepAlive (moB) );
  BOOST_REQUIRE_NO_THROW (client->keepAlive (mediaPipeline) );

  g_usleep ( (2 * AUTO_RELEASE_INTERVAL + 1) * G_USEC_PER_SEC);

  try {
    client->keepAlive (moB);
    BOOST_FAIL ("Use an auto released MediaObject must throw a KmsMediaServerException");
  } catch (const KmsMediaServerException &e) {
    BOOST_CHECK_EQUAL (g_KmsMediaErrorCodes_constants.MEDIA_OBJECT_NOT_FOUND, e.errorCode);
  }

  try {
    client->keepAlive (mediaPipeline);
    BOOST_FAIL ("Use an auto released MediaPipeline must throw a KmsMediaServerException");
  } catch (const KmsMediaServerException &e) {
    BOOST_CHECK_EQUAL (g_KmsMediaErrorCodes_constants.MEDIA_OBJECT_NOT_FOUND, e.errorCode);
  }
}

void
ClientHandler::check_keep_alive_media_pipeline ()
{
  int i;
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> mediaObjectparams;

  createKmsMediaObjectConstructorParams (mediaObjectparams, false,
                                         AUTO_RELEASE_INTERVAL);

  client->createMediaPipelineWithParams (mediaPipeline, mediaObjectparams);

  for (i = 0; i < 5; i++) {
    g_usleep (AUTO_RELEASE_INTERVAL * G_USEC_PER_SEC);
    client->keepAlive (mediaPipeline);
  }

  client->release (mediaPipeline);
}

void
ClientHandler::check_exclude_from_gc ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;

  createKmsMediaObjectConstructorParams (params, true, AUTO_RELEASE_INTERVAL);

  client->createMediaPipelineWithParams (mediaPipeline, params);
  g_usleep ( (2 * AUTO_RELEASE_INTERVAL + 1) * G_USEC_PER_SEC);
  BOOST_REQUIRE_NO_THROW (client->keepAlive (mediaPipeline) );
  BOOST_REQUIRE_NO_THROW (client->release (mediaPipeline) );
}

void
ClientHandler::check_parent ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef mo = KmsMediaObjectRef();
  KmsMediaObjectRef parent = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;

  client->createMediaPipeline (mediaPipeline);
  createKmsMediaUriEndPointConstructorParams (params, "file:///tmp/f.webm");
  client->createMediaElementWithParams (mo, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params);
  client->getParent (parent, mo);
  BOOST_CHECK_EQUAL (mediaPipeline.id, parent.id);

  client->release (mediaPipeline);
}

void
ClientHandler::check_get_parent_of_media_pipeline ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef parent = KmsMediaObjectRef();

  client->createMediaPipeline (mediaPipeline);

  try {
    client->getParent (parent, mediaPipeline);
    BOOST_FAIL ("Get parent of a MediaPipeline must throw a KmsMediaServerException");
  } catch (const KmsMediaServerException &e) {
    BOOST_CHECK_EQUAL (g_KmsMediaErrorCodes_constants.MEDIA_OBJECT_HAS_NOT_PARENT, e.errorCode);
  }

  client->release (mediaPipeline);
}

void
ClientHandler::check_getMediaPipeline ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef mo = KmsMediaObjectRef();
  KmsMediaObjectRef mediaPipelineGot = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;

  client->createMediaPipeline (mediaPipeline);
  client->getMediaPipeline (mediaPipelineGot, mediaPipeline);
  BOOST_CHECK_EQUAL (mediaPipeline.id, mediaPipelineGot.id);

  createKmsMediaUriEndPointConstructorParams (params, "file:///tmp/f.webm");
  client->createMediaElementWithParams (mo, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params);
  client->getMediaPipeline (mediaPipelineGot, mo);
  BOOST_CHECK_EQUAL (mediaPipeline.id, mediaPipelineGot.id);

  client->release (mediaPipeline);
}

void
ClientHandler::check_same_token ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef mo = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;

  client->createMediaPipeline (mediaPipeline);
  createKmsMediaUriEndPointConstructorParams (params, "file:///tmp/f.webm");
  client->createMediaElementWithParams (mo, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params);
  BOOST_CHECK_EQUAL (mediaPipeline.token, mo.token);

  client->release (mediaPipeline);
}

void
ClientHandler::check_get_media_element_from_pad ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef playerEndPoint = KmsMediaObjectRef();
  KmsMediaObjectRef me = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;
  std::vector<KmsMediaObjectRef> pads;
  std::vector<KmsMediaObjectRef>::iterator it;

  client->createMediaPipeline (mediaPipeline);
  createKmsMediaUriEndPointConstructorParams (params, "file:///tmp/f.webm");
  client->createMediaElementWithParams (playerEndPoint, mediaPipeline,
                                        g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params);

  client->getMediaSrcs (pads, playerEndPoint);

  for (it = pads.begin(); it != pads.end(); ++it) {
    client->getMediaElement (me, *it);
    BOOST_CHECK_EQUAL (playerEndPoint.id, me.id);
  }

  client->getMediaSinks (pads, playerEndPoint);

  for (it = pads.begin(); it != pads.end(); ++it) {
    client->getMediaElement (me, *it);
    BOOST_CHECK_EQUAL (playerEndPoint.id, me.id);
  }

  client->release (mediaPipeline);
}

void
ClientHandler::check_player_end_point ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef playerEndPoint = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;
  KmsMediaInvocationReturn ret;
  std::string originalUri = "https://ci.kurento.com/video/small.webm";
  std::string resultUri;
  std::string callbackToken;
  Glib::Mutex mutex;
  Glib::Cond cond;
  Glib::TimeVal timeout;
  gboolean endTimeout;

  client->createMediaPipeline (mediaPipeline);
  createKmsMediaUriEndPointConstructorParams (params, originalUri);
  client->createMediaElementWithParams (playerEndPoint, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params);

  client->invoke (ret, playerEndPoint, g_KmsMediaUriEndPointType_constants.GET_URI, emptyParams);

  BOOST_REQUIRE_NO_THROW (unmarshalStringInvocationReturn (resultUri, ret) );
  BOOST_CHECK_EQUAL (0, originalUri.compare (resultUri) );

  mutex.lock();
  auto f = [&cond, &mutex, this] (std::string cT, KmsMediaEvent e) {
    GST_INFO ("playerEndPoint: %s received", e.type.c_str() );
    mutex.lock();
    cond.signal();
    handlerTest->deleteEventFunction();
    mutex.unlock();
  };
  handlerTest->setEventFunction (f, g_KmsMediaPlayerEndPointType_constants.EVENT_EOS);
  client->subscribeEvent (callbackToken, playerEndPoint,
                          g_KmsMediaPlayerEndPointType_constants.EVENT_EOS,
                          HANDLER_IP, HANDLER_PORT);
  GST_DEBUG ("callbackToken: %s", callbackToken.c_str () );
  params.clear();
  client->invoke (ret, playerEndPoint, g_KmsMediaUriEndPointType_constants.START,
                  params);
  timeout.assign_current_time();
  timeout += 20;
  endTimeout = cond.timed_wait (mutex, timeout);
  mutex.unlock();

  if (!endTimeout) {
    BOOST_FAIL ("check_player_end_point: EOS event timeout reached ");
  }

  client->unsubscribeEvent (playerEndPoint, callbackToken);

  try {
    client->subscribeEvent (callbackToken, playerEndPoint, "BAD_EVENT_TYPE", "", 0);
    BOOST_FAIL ("Subscribe for an event not supported by PlayerEndPoint must throw a KmsMediaServerException");
  } catch (const KmsMediaServerException &e) {
    BOOST_CHECK_EQUAL (g_KmsMediaErrorCodes_constants.MEDIA_OBJECT_EVENT_NOT_SUPPORTED, e.errorCode);
  }

  client->release (mediaPipeline);
}

void
ClientHandler::check_player_end_point_signal_errors()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef playerEndPoint = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;
  KmsMediaInvocationReturn ret;
  std::string originalUri = "file:///tmp/f.webm";
  std::string resultUri;
  std::string callbackToken;
  Glib::Mutex mutex;
  Glib::Cond cond;
  Glib::TimeVal timeout;
  gboolean endTimeout;

  client->createMediaPipeline (mediaPipeline);
  createKmsMediaUriEndPointConstructorParams (params, originalUri);
  client->createMediaElementWithParams (playerEndPoint, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params);

  mutex.lock();
  auto f = [&cond, &mutex, this] (std::string cT, KmsMediaError e) {
    mutex.lock();
    cond.signal();
    handlerTest->deleteErrorFunction();
    mutex.unlock();
  };
  client->subscribeError (callbackToken, playerEndPoint, HANDLER_IP, HANDLER_PORT);
  handlerTest->setErrorFunction (f, "invalid-uri");
  params.clear();
  client->invoke (ret, playerEndPoint, g_KmsMediaUriEndPointType_constants.START,
                  params);
  timeout.assign_current_time();
  timeout += 20;
  endTimeout = cond.timed_wait (mutex, timeout);
  mutex.unlock();

  if (!endTimeout) {
    BOOST_FAIL ("check_player_end_point_signal_errors: timeout reached");
  }

  client->unsubscribeError (playerEndPoint, callbackToken);
  client->release (mediaPipeline);
}

void
ClientHandler::check_recorder_end_point ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef recorderEndPoint = KmsMediaObjectRef();
  KmsMediaObjectRef playerEndPoint = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;
  std::string callbackToken;
  KmsMediaInvocationReturn ret;
  Glib::Mutex mutex;
  Glib::Cond cond;
  Glib::TimeVal timeout;
  gboolean endTimeout;
  const std::string uriSrc = "https://ci.kurento.com/video/small.webm";
  const std::string uriDst = "file:///tmp/test/player_end_point_test.webm";
  std::string resultUri;

  client->createMediaPipeline (mediaPipeline);
  createKmsMediaUriEndPointConstructorParams (params, uriSrc);
  client->createMediaElementWithParams (playerEndPoint, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params);
  params.clear();
  createKmsMediaUriEndPointConstructorParams (params, uriDst);
  client->createMediaElementWithParams (recorderEndPoint, mediaPipeline, g_KmsMediaRecorderEndPointType_constants.TYPE_NAME, params);

  client->connectElements (playerEndPoint, recorderEndPoint);

  client->subscribeError (callbackToken, mediaPipeline, HANDLER_IP, HANDLER_PORT);

  client->invoke (ret, recorderEndPoint, g_KmsMediaUriEndPointType_constants.GET_URI, emptyParams);
  BOOST_REQUIRE_NO_THROW (unmarshalStringInvocationReturn (resultUri, ret) );
  BOOST_CHECK_EQUAL (0, uriDst.compare (resultUri) );


  mutex.lock();
  auto f = [&cond, &mutex, this] (std::string cT, KmsMediaError e) {
    mutex.lock();
    cond.signal();
    handlerTest->deleteErrorFunction();
    mutex.unlock();
  };

  handlerTest->setErrorFunction (f, "UNEXPECTED_ERROR");
  params.clear();
  client->invoke (ret, playerEndPoint, g_KmsMediaUriEndPointType_constants.START, params);

  params.clear();
  client->invoke (ret, recorderEndPoint, g_KmsMediaUriEndPointType_constants.START, params);

  timeout.assign_current_time();
  timeout += 120;
  endTimeout = cond.timed_wait (mutex, timeout);
  mutex.unlock();

  if (!endTimeout) {
    BOOST_FAIL ("check_player_end_point_signal_errors: timeout reached");
  }

  client->unsubscribeError (mediaPipeline, callbackToken);

  client->release (mediaPipeline);
}

void
ClientHandler::check_http_end_point ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef httpEp = KmsMediaObjectRef();
  KmsMediaInvocationReturn ret;
  std::string url;

  client->createMediaPipeline (mediaPipeline);
  client->createMediaElement (httpEp, mediaPipeline, g_KmsMediaHttpEndPointType_constants.TYPE_NAME);
  client->invoke (ret, httpEp, g_KmsMediaHttpEndPointType_constants.GET_URL, emptyParams);

  unmarshalStringInvocationReturn (url, ret);

  GST_INFO ("HttpEndPoint URL: %s", url.c_str () );

  client->release (mediaPipeline);
}

void
ClientHandler::check_zbar_filter ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef zbarFilter = KmsMediaObjectRef();
  KmsMediaObjectRef playerEndPoint = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;
  KmsMediaInvocationReturn ret;
  std::string originalUri = "https://ci.kurento.com/video/barcodes.webm";
  std::string resultUri;
  std::string callbackToken;
  Glib::Mutex mutex;
  Glib::Cond cond;
  Glib::TimeVal timeout;
  gboolean endTimeout;

  client->createMediaPipeline (mediaPipeline);
  createKmsMediaUriEndPointConstructorParams (params, originalUri);
  client->createMediaElementWithParams (playerEndPoint, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params);
  client->createMediaElement (zbarFilter, mediaPipeline, g_KmsMediaZBarFilterType_constants.TYPE_NAME);
  client->connectElements (playerEndPoint, zbarFilter);

  mutex.lock();
  auto f = [&cond, &mutex, this] (std::string cT, KmsMediaEvent e) {
    GST_INFO ("zBarFilter: %s received", e.type.c_str() );
    mutex.lock();
    cond.signal();
    handlerTest->deleteEventFunction();
    mutex.unlock();
  };
  handlerTest->setEventFunction (f, g_KmsMediaZBarFilterType_constants.EVENT_CODE_FOUND);

  client->subscribeEvent (callbackToken, zbarFilter,
                          g_KmsMediaZBarFilterType_constants.EVENT_CODE_FOUND,
                          HANDLER_IP, HANDLER_PORT);

  GST_DEBUG ("callbackToken: %s", callbackToken.c_str () );
  params.clear();
  client->invoke (ret, playerEndPoint, g_KmsMediaUriEndPointType_constants.START,
                  params);
  timeout.assign_current_time();
  timeout += 20;
  endTimeout = cond.timed_wait (mutex, timeout);
  mutex.unlock();

  if (!endTimeout) {
    BOOST_FAIL ("check_zbar_filter_event: Not barcodes detected until timeout");
  }

  client->unsubscribeEvent (zbarFilter, callbackToken);
  client->release (mediaPipeline);
}

void
ClientHandler::check_jackvader_filter ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef jackVaderFilter = KmsMediaObjectRef();

  client->createMediaPipeline (mediaPipeline);
  client->createMediaElement (jackVaderFilter, mediaPipeline, g_KmsMediaJackVaderFilterType_constants.TYPE_NAME);
  client->release (mediaPipeline);
}

void
ClientHandler::check_pointer_detector_filter ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef pointerDetectorFilter = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;
  KmsMediaParam param;
  KmsMediaPointerDetectorWindowSet windowSet;
  KmsMediaPointerDetectorWindow window;
  KmsMediaInvocationReturn ret;
  std::string name;

  gint32 numElements = 5;

  //create window layouts
  for (int i = 0; i < numElements ; i++) {
    KmsMediaPointerDetectorWindow aux;
    aux.topRightCornerX = i * 3;
    aux.topRightCornerY = i * 4;
    aux.width = i * 5;
    aux.height = i * 6;
    aux.id = "window" + std::to_string (i);
    windowSet.windows.insert (aux);
  }

  //marshalling data
  createStructParam (param, windowSet, g_KmsMediaPointerDetectorFilterType_constants.CONSTRUCTOR_PARAMS_DATA_TYPE);
  params[g_KmsMediaPointerDetectorFilterType_constants.CONSTRUCTOR_PARAMS_DATA_TYPE] = param;

  //create elements
  client->createMediaPipeline (mediaPipeline);
  client->createMediaElementWithParams (pointerDetectorFilter, mediaPipeline,
                                        g_KmsMediaPointerDetectorFilterType_constants.TYPE_NAME,
                                        params);

  params.clear();

  //test functions
  //add new Window
  window.topRightCornerX = 15;
  window.topRightCornerY = 15;
  window.width = 15;
  window.height = 15;
  window.id = "new";

  //marshalling data
  createStructParam (param, window, g_KmsMediaPointerDetectorFilterType_constants.ADD_NEW_WINDOW_PARAM_WINDOW);
  params[g_KmsMediaPointerDetectorFilterType_constants.ADD_NEW_WINDOW_PARAM_WINDOW] = param;

  BOOST_REQUIRE_NO_THROW (client->invoke (ret, pointerDetectorFilter,
                                          g_KmsMediaPointerDetectorFilterType_constants.ADD_NEW_WINDOW, params);)
  //remove window
  name = "new";
  setStringParam (params, g_KmsMediaPointerDetectorFilterType_constants.REMOVE_WINDOW_PARAM_WINDOW_ID, name);
  client->invoke (ret, pointerDetectorFilter, g_KmsMediaPointerDetectorFilterType_constants.REMOVE_WINDOW, params);

  //clear windows
  client->invoke (ret, pointerDetectorFilter, g_KmsMediaPointerDetectorFilterType_constants.CLEAR_WINDOWS, emptyParams);

  client->release (mediaPipeline);
}

void
ClientHandler::check_web_rtc_end_point ()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef webRtcEndPoint = KmsMediaObjectRef();

  client->createMediaPipeline (mediaPipeline);
  client->createMediaElement (webRtcEndPoint, mediaPipeline, g_KmsMediaWebRtcEndPointType_constants.TYPE_NAME);
  client->release (mediaPipeline);
}

void
ClientHandler::check_plate_detector_filter()
{
  KmsMediaObjectRef mediaPipeline = KmsMediaObjectRef();
  KmsMediaObjectRef plateDetector = KmsMediaObjectRef();
  KmsMediaObjectRef playerEndPoint = KmsMediaObjectRef();
  std::map<std::string, KmsMediaParam> params;
  KmsMediaInvocationReturn ret;
  std::string originalUri = "https://ci.kurento.com/video/plates.webm";
  std::string resultUri;
  std::string callbackToken;
  Glib::Mutex mutex;
  Glib::Cond cond;
  Glib::TimeVal timeout;
  gboolean endTimeout;

  client->createMediaPipeline (mediaPipeline);
  createKmsMediaUriEndPointConstructorParams (params, originalUri);
  client->createMediaElementWithParams (playerEndPoint, mediaPipeline, g_KmsMediaPlayerEndPointType_constants.TYPE_NAME, params);
  client->createMediaElement (plateDetector, mediaPipeline, g_KmsMediaPlateDetectorFilterType_constants.TYPE_NAME);
  client->connectElements (playerEndPoint, plateDetector);

  mutex.lock();
  auto f = [&cond, &mutex, this] (std::string cT, KmsMediaEvent e) {
    GST_INFO ("PlateDetector: %s received", e.type.c_str() );
    mutex.lock();
    cond.signal();
    handlerTest->deleteEventFunction();
    mutex.unlock();
  };
  handlerTest->setEventFunction (f, g_KmsMediaPlateDetectorFilterType_constants.EVENT_PLATE_DETECTED);

  client->subscribeEvent (callbackToken, plateDetector,
                          g_KmsMediaPlateDetectorFilterType_constants.EVENT_PLATE_DETECTED,
                          HANDLER_IP, HANDLER_PORT);

  GST_DEBUG ("callbackToken: %s", callbackToken.c_str () );
  params.clear();
  client->invoke (ret, playerEndPoint, g_KmsMediaUriEndPointType_constants.START,
                  params);
  timeout.assign_current_time();
  timeout += 20;
  endTimeout = cond.timed_wait (mutex, timeout);
  mutex.unlock();

  if (!endTimeout) {
    BOOST_FAIL ("check_plate_detector_filter_event: Not license plates detected until timeout");
  }

  client->unsubscribeEvent (plateDetector, callbackToken);
  client->release (mediaPipeline);
}

BOOST_FIXTURE_TEST_SUITE ( server_test_suite, ClientHandler)

BOOST_AUTO_TEST_CASE ( server_test )
{
  BOOST_REQUIRE_MESSAGE (initialized, "Cannot connect to the server");
  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0, GST_DEFAULT_NAME);

  check_version();
  check_use_released_media_pipeline ();
  check_auto_released_media_pipeline ();
  check_keep_alive_media_pipeline ();
  check_exclude_from_gc ();
  check_parent ();
  check_get_parent_of_media_pipeline ();
  check_getMediaPipeline ();
  check_same_token ();
  check_get_media_element_from_pad ();

#if 0 /* Temporally disabled */
  check_type ();
#endif

  check_player_end_point ();
  check_player_end_point_signal_errors ();
  check_recorder_end_point ();
  check_http_end_point ();
  check_zbar_filter ();
  check_jackvader_filter ();
  check_pointer_detector_filter ();
  check_web_rtc_end_point ();
  check_plate_detector_filter();
}

BOOST_AUTO_TEST_SUITE_END()
