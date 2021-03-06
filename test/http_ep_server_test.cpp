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
#define BOOST_TEST_MODULE http_ep_server
#include <boost/test/unit_test.hpp>

#include <gst/gst.h>
#include <libsoup/soup.h>
#include <KmsHttpEPServer.h>
#include <kmshttpendpointaction.h>

#define GST_CAT_DEFAULT _http_endpoint_server_test_
GST_DEBUG_CATEGORY_STATIC (GST_CAT_DEFAULT);
#define GST_DEFAULT_NAME "http_endpoint_server_test"

#define MAX_REGISTERED_HTTP_END_POINTS 10
#define DISCONNECTION_TIMEOUT 2 /* seconds */

#define HTTP_GET "GET"
#define DEFAULT_PORT 9091
#define DEFAULT_HOST "localhost"

static KmsHttpEPServer *httpepserver;
static SoupSession *session;

static GMainLoop *loop;
static GSList *urls;
static guint urls_registered;
static guint signal_count;
static guint counted;

static SoupKnownStatusCode expected_200 = SOUP_STATUS_OK;
static SoupKnownStatusCode expected_404 = SOUP_STATUS_NOT_FOUND;

SoupSessionCallback session_cb;

BOOST_AUTO_TEST_SUITE (http_ep_server_test)

static void
init_test_case ()
{
  loop = NULL;
  urls = NULL;
  urls_registered = 0;
  signal_count = 0;
  counted = 0;

  setenv ("GST_PLUGIN_PATH", "./plugins", TRUE);
  gst_init (NULL, NULL);

  GST_DEBUG_CATEGORY_INIT (GST_CAT_DEFAULT, GST_DEFAULT_NAME, 0,
                           GST_DEFAULT_NAME);

  loop = g_main_loop_new (NULL, FALSE);
  session = soup_session_async_new();

  /* Start Http End Point Server */
  httpepserver = kms_http_ep_server_new (KMS_HTTP_EP_SERVER_PORT, DEFAULT_PORT,
                                         KMS_HTTP_EP_SERVER_INTERFACE, DEFAULT_HOST, NULL);
}

static void
tear_down_test_case ()
{
  /* check for missed unrefs before exiting */
  // TODO: Enable check again when leak in 32 bits is found
  // BOOST_CHECK ( G_OBJECT (httpepserver)->ref_count == 1 );

  g_object_unref (G_OBJECT (httpepserver) );
  g_object_unref (G_OBJECT (session) );
  g_slist_free_full (urls, g_free);
  g_main_loop_unref (loop);
}

static void
register_http_end_points (gint n)
{
  const gchar *url;
  gint i;

  for (i = 0; i < n; i++) {
    GstElement *httpep = gst_element_factory_make ("httpendpoint", NULL);
    BOOST_CHECK ( httpep != NULL );

    GST_DEBUG ("Registering %s", GST_ELEMENT_NAME (httpep) );
    url = kms_http_ep_server_register_end_point (httpepserver, httpep,
          DISCONNECTION_TIMEOUT);

    BOOST_CHECK (url != NULL);

    if (url == NULL)
      continue;

    /* Leave the last reference to http end point server */
    g_object_unref (G_OBJECT (httpep) );

    GST_DEBUG ("Registered url: %s", url);
    urls = g_slist_prepend (urls, (gpointer *) g_strdup (url) );
  }

  urls_registered = g_slist_length (urls);
}

static void
http_req_callback (SoupSession *session, SoupMessage *msg, gpointer data)
{
  SoupKnownStatusCode *expected = (SoupKnownStatusCode *) data;
  guint status_code;
  gchar *method;
  SoupURI *uri;

  g_object_get (G_OBJECT (msg), "method", &method, "status-code",
                &status_code, "uri", &uri, NULL);

  GST_DEBUG ("%s %s status code: %d, expected %d", method, soup_uri_get_path (uri),
             status_code, *expected);

  BOOST_CHECK_EQUAL (*expected, status_code);

  if (*expected == expected_404 && ++counted == urls_registered)
    g_main_loop_quit (loop);

  soup_uri_free (uri);
  g_free (method);
}

static void
send_get_request (const gchar *uri, gpointer data)
{
  SoupMessage *msg;
  gchar *url;

  url = g_strdup_printf ("http://%s:%d%s", DEFAULT_HOST, DEFAULT_PORT, uri);

  GST_INFO ("Send " HTTP_GET " %s", url);
  msg = soup_message_new (HTTP_GET, url);
  soup_session_queue_message (session, msg, session_cb, data);

  g_free (url);
}

static gboolean
checking_registered_urls (gpointer data)
{
  GST_DEBUG ("Sending GET request to all urls registered");

  g_slist_foreach (urls, (GFunc) send_get_request, data);

  return FALSE;
}

static void
url_removed_cb (KmsHttpEPServer *server, const gchar *url, gpointer data)
{
  GST_DEBUG ("URL %s removed", url);

  if (++signal_count == urls_registered) {
    /* Testing removed URLs */
    g_idle_add ( (GSourceFunc) checking_registered_urls, &expected_404);
  }
}

static void
action_requested_cb (KmsHttpEPServer *server, const gchar *uri,
                     KmsHttpEndPointAction action, gpointer data)
{
  GST_DEBUG ("Action %d requested on %s", action, uri);
  BOOST_CHECK ( action == KMS_HTTP_END_POINT_ACTION_GET );

  BOOST_CHECK (kms_http_ep_server_unregister_end_point (httpepserver, uri) );
}

static void
http_server_start_cb (KmsHttpEPServer *self, GError *err)
{
  if (err != NULL) {
    GST_ERROR ("%s, code %d", err->message, err->code);
    return;
  }

  register_http_end_points (MAX_REGISTERED_HTTP_END_POINTS);

  session_cb = http_req_callback;

  g_idle_add ( (GSourceFunc) checking_registered_urls, &expected_200);
}

BOOST_AUTO_TEST_CASE ( register_http_end_point_test )
{
  init_test_case ();

  g_signal_connect (httpepserver, "url-removed", G_CALLBACK (url_removed_cb),
                    NULL);
  g_signal_connect (httpepserver, "action-requested", G_CALLBACK (action_requested_cb),
                    NULL);

  kms_http_ep_server_start (httpepserver, http_server_start_cb);

  g_main_loop_run (loop);

  BOOST_CHECK_EQUAL (signal_count, urls_registered);

  GST_DEBUG ("Test finished");

  /* Stop Http End Point Server and destroy it */
  kms_http_ep_server_stop (httpepserver);

  tear_down_test_case ();
}

/********************************************/
/* Functions and variables used for tests 2 */
/********************************************/

static void
t2_http_req_callback (SoupSession *session, SoupMessage *msg, gpointer data)
{
  SoupKnownStatusCode *expected = (SoupKnownStatusCode *) data;
  guint status_code;
  gchar *method;
  SoupURI *uri;

  g_object_get (G_OBJECT (msg), "method", &method, "status-code",
                &status_code, "uri", &uri, NULL);

  GST_DEBUG ("%s %s status code: %d, expected %d", method, soup_uri_get_path (uri),
             status_code, *expected);

  BOOST_CHECK_EQUAL (*expected, status_code);

  if (++counted == urls_registered)
    g_main_loop_quit (loop);

  soup_uri_free (uri);
  g_free (method);
}

static void
t2_action_requested_cb (KmsHttpEPServer *server, const gchar *uri,
                        KmsHttpEndPointAction action, gpointer data)
{
  GST_DEBUG ("Action %d requested on %s", action, uri);

  /* We unregister httpendpoints when they have already a pending request */
  /* so as to check we don't miss memory leaks */
  BOOST_CHECK (kms_http_ep_server_unregister_end_point (httpepserver, uri) );
}

static void
t2_url_removed_cb (KmsHttpEPServer *server, const gchar *url, gpointer data)
{
  GST_DEBUG ("URL %s removed", url);
}

static void
t2_http_server_start_cb (KmsHttpEPServer *self, GError *err)
{
  if (err != NULL) {
    GST_ERROR ("%s, code %d", err->message, err->code);
    return;
  }

  register_http_end_points (MAX_REGISTERED_HTTP_END_POINTS);

  session_cb = t2_http_req_callback;

  g_idle_add ( (GSourceFunc) checking_registered_urls, &expected_200);
}

BOOST_AUTO_TEST_CASE ( locked_get_request_http_end_point_test )
{
  init_test_case ();
  g_signal_connect (httpepserver, "url-removed", G_CALLBACK (t2_url_removed_cb),
                    NULL);
  g_signal_connect (httpepserver, "action-requested",
                    G_CALLBACK (t2_action_requested_cb), NULL);

  kms_http_ep_server_start (httpepserver, t2_http_server_start_cb);

  g_main_loop_run (loop);

  GST_DEBUG ("Test finished");

  /* Stop Http End Point Server and destroy it */
  kms_http_ep_server_stop (httpepserver);
  tear_down_test_case ();
}

/********************************************/
/* Functions and variables used for tests 3 */
/********************************************/

static guint QUEUED = 2;

static void
t3_http_req_callback (SoupSession *session, SoupMessage *msg, gpointer data)
{
  SoupKnownStatusCode *expected = (SoupKnownStatusCode *) data;
  guint status_code;
  gchar *method;
  SoupURI *uri;

  g_object_get (G_OBJECT (msg), "method", &method, "status-code",
                &status_code, "uri", &uri, NULL);

  GST_DEBUG ("%s %s status code: %d, expected %d", method, soup_uri_get_path (uri),
             status_code, *expected);

  BOOST_CHECK_EQUAL (*expected, status_code);

  if (++counted == urls_registered)
    g_main_loop_quit (loop);

  soup_uri_free (uri);
  g_free (method);
}

static void
t3_http_server_start_cb (KmsHttpEPServer *self, GError *err)
{
  if (err != NULL) {
    GST_ERROR ("%s, code %d", err->message, err->code);
    return;
  }

  register_http_end_points (QUEUED);

  session_cb = t3_http_req_callback;

  g_idle_add ( (GSourceFunc) checking_registered_urls, &expected_200);
}

static void
t3_action_requested_cb (KmsHttpEPServer *server, const gchar *uri,
                        KmsHttpEndPointAction action, gpointer data)
{
  GST_DEBUG ("Action %d requested on %s", action, uri);

  if (++signal_count == QUEUED) {
    /* Stop Http End Point Server and destroy it */
    kms_http_ep_server_stop (httpepserver);
  }
}

static void
t3_url_removed_cb (KmsHttpEPServer *server, const gchar *url, gpointer data)
{
  GST_DEBUG ("URL %s removed", url);
}

BOOST_AUTO_TEST_CASE ( shutdown_http_end_point_test )
{
  init_test_case ();
  g_signal_connect (httpepserver, "url-removed", G_CALLBACK (t3_url_removed_cb),
                    NULL);
  g_signal_connect (httpepserver, "action-requested",
                    G_CALLBACK (t3_action_requested_cb), NULL);

  kms_http_ep_server_start (httpepserver, t3_http_server_start_cb);

  g_main_loop_run (loop);

  GST_DEBUG ("Test finished");

  tear_down_test_case ();
}

/********************************************/
/* Functions and variables used for test 4  */
/********************************************/

static void
t4_http_req_callback (SoupSession *session, SoupMessage *msg, gpointer data)
{
  SoupKnownStatusCode *expected = (SoupKnownStatusCode *) data;
  guint status_code;
  gchar *method;
  SoupURI *uri;
  const gchar *cookie_str;

  g_object_get (G_OBJECT (msg), "method", &method, "status-code",
                &status_code, "uri", &uri, NULL);

  GST_DEBUG ("%s %s status code: %d, expected %d", method, soup_uri_get_path (uri),
             status_code, *expected);
  BOOST_CHECK (status_code == *expected);

  /* TODO: Check why soup_cookies_from_response does not work */
  cookie_str = soup_message_headers_get_list (msg->response_headers, "Set-Cookie");
  BOOST_CHECK (cookie_str != NULL);

  if (++counted == urls_registered)
    g_main_loop_quit (loop);

  soup_uri_free (uri);
  g_free (method);
}

static void
t4_http_server_start_cb (KmsHttpEPServer *self, GError *err)
{
  BOOST_CHECK ( err == NULL );

  if (err != NULL) {
    GST_ERROR ("%s, code %d", err->message, err->code);
    g_main_loop_quit (loop);
    return;
  }

  register_http_end_points (MAX_REGISTERED_HTTP_END_POINTS);

  session_cb = t4_http_req_callback;

  g_idle_add ( (GSourceFunc) checking_registered_urls, &expected_200);
}

static void
t4_action_requested_cb (KmsHttpEPServer *server, const gchar *uri,
                        KmsHttpEndPointAction action, gpointer data)
{
  GST_DEBUG ("Action %d requested on %s", action, uri);

  /* We unregister httpendpoints when they have already a pending request */
  /* so as to check we don't miss memory leaks */
  BOOST_CHECK (kms_http_ep_server_unregister_end_point (httpepserver, uri) );
}

BOOST_AUTO_TEST_CASE ( cookie_http_end_point_test )
{
  init_test_case ();

  g_signal_connect (httpepserver, "action-requested",
                    G_CALLBACK (t4_action_requested_cb), NULL);

  kms_http_ep_server_start (httpepserver, t4_http_server_start_cb);

  g_main_loop_run (loop);

  GST_DEBUG ("Test finished");

  /* Stop Http End Point Server and destroy it */
  kms_http_ep_server_stop (httpepserver);
  tear_down_test_case ();
}

/********************************************/
/* Functions and variables used for test 5  */
/********************************************/

static GstElement *pipeline, *httpep;
static const gchar *t5_uri;
static SoupCookie *cookie;

static void
bus_msg_cb (GstBus *bus, GstMessage *msg, gpointer pipeline)
{
  switch (msg->type) {
  case GST_MESSAGE_ERROR: {
    GST_ERROR ("%s bus error: %" GST_PTR_FORMAT, GST_ELEMENT_NAME (pipeline),
               msg);
    BOOST_FAIL ("Error received on the bus");
    break;
  }

  case GST_MESSAGE_WARNING: {
    GST_WARNING ("%s bus: %" GST_PTR_FORMAT, GST_ELEMENT_NAME (pipeline),
                 msg);
    break;
  }

  default:
    break;
  }
}

static void
t5_request_no_cookie_cb (SoupSession *session, SoupMessage *msg,
                         gpointer user_data)
{
  GST_DEBUG ("status code: %d", msg->status_code);

  /* Request should not be attended without the proper cookie */
  if (msg->status_code != SOUP_STATUS_BAD_REQUEST)
    BOOST_FAIL ("Get request without cookie failed");

  g_main_loop_quit (loop);
}

static void
t5_make_request_without_cookie()
{
  SoupMessage *msg;
  gchar *url;

  url = g_strdup_printf ("http://%s:%d%s", DEFAULT_HOST, DEFAULT_PORT, t5_uri);

  GST_INFO ("Send " HTTP_GET " %s", url);

  msg = soup_message_new (HTTP_GET, url);
  soup_session_queue_message (session, msg, t5_request_no_cookie_cb, NULL);

  g_free (url);
}

static gboolean
t5_cancel_cb (gpointer data)
{
  SoupMessage *msg = (SoupMessage *) data;
  GST_DEBUG ("Cancel Message.");
  soup_session_cancel_message (session, msg, SOUP_STATUS_CANCELLED);

  return FALSE;
}

static void
t5_request_with_cookie_cb (SoupSession *session, SoupMessage *msg,
                           gpointer user_data)
{
  guint status_code;
  gchar *method;
  SoupURI *uri;

  GST_DEBUG ("Request with cookie");

  g_object_get (G_OBJECT (msg), "method", &method, "status-code",
                &status_code, "uri", &uri, NULL);

  GST_WARNING ("%s %s status code: %d, expected %d", method,
               soup_uri_get_path (uri), status_code, SOUP_STATUS_CANCELLED);

  BOOST_CHECK (status_code == SOUP_STATUS_CANCELLED);

  t5_make_request_without_cookie();

  g_free (method);
  soup_uri_free (uri);
}

static void
t5_send_get_request_2 ()
{
  SoupMessage *msg;
  gchar *url, *header;

  url = g_strdup_printf ("http://%s:%d%s", DEFAULT_HOST, DEFAULT_PORT, t5_uri);

  GST_INFO ("Send " HTTP_GET " %s", url);
  msg = soup_message_new (HTTP_GET, url);

  header = soup_cookie_to_cookie_header (cookie);
  soup_message_headers_append (msg->request_headers, "Cookie", header);
  g_free (header);

  soup_session_queue_message (session, msg, t5_request_with_cookie_cb, NULL);

  g_timeout_add_full (G_PRIORITY_DEFAULT, 1000, t5_cancel_cb,
                      g_object_ref (G_OBJECT (msg) ), g_object_unref);
  g_free (url);
}

static void
t5_http_req_callback (SoupSession *session, SoupMessage *msg, gpointer data)
{
  guint status_code;
  gchar *method;
  SoupURI *uri;
  const gchar *header;

  g_object_get (G_OBJECT (msg), "method", &method, "status-code",
                &status_code, "uri", &uri, NULL);

  GST_WARNING ("%s %s status code: %d, expected %d", method, soup_uri_get_path (uri),
               status_code, SOUP_STATUS_CANCELLED);
  BOOST_CHECK (status_code == SOUP_STATUS_CANCELLED);

  /* TODO: Check why soup_cookies_from_response does not work */
  header = soup_message_headers_get_list (msg->response_headers, "Set-Cookie");

  BOOST_CHECK (header != NULL);

  cookie = soup_cookie_parse (header, NULL);
  BOOST_CHECK (cookie != NULL);

  t5_send_get_request_2();

  soup_uri_free (uri);
  g_free (method);
}

static void
t5_send_get_request_1 ()
{
  SoupMessage *msg;
  gchar *url;

  url = g_strdup_printf ("http://%s:%d%s", DEFAULT_HOST, DEFAULT_PORT, t5_uri);

  GST_INFO ("Send " HTTP_GET " %s", url);
  msg = soup_message_new (HTTP_GET, url);
  soup_session_queue_message (session, msg, t5_http_req_callback, NULL);

  g_timeout_add_full (G_PRIORITY_DEFAULT, 1000, t5_cancel_cb,
                      g_object_ref (G_OBJECT (msg) ), g_object_unref);
  g_free (url);
}

static void
t5_http_server_start_cb (KmsHttpEPServer *self, GError *err)
{
  if (err != NULL) {
    GST_ERROR ("%s, code %d", err->message, err->code);
    g_main_loop_quit (loop);
    return;
  }

  GST_DEBUG ("Registering %s", GST_ELEMENT_NAME (httpep) );
  t5_uri = kms_http_ep_server_register_end_point (httpepserver, httpep,
           DISCONNECTION_TIMEOUT);
  BOOST_CHECK (t5_uri != NULL);

  if (t5_uri == NULL) {
    g_main_loop_quit (loop);
    return;
  }

  GST_DEBUG ("Registered url: %s", t5_uri);
  urls = g_slist_prepend (urls, (gpointer *) g_strdup (t5_uri) );

  t5_send_get_request_1 ();
}

static void
t5_url_removed_cb (KmsHttpEPServer *server, const gchar *url, gpointer data)
{
  GST_DEBUG ("URL %s removed", url);
}

static void
t5_action_requested_cb (KmsHttpEPServer *server, const gchar *uri,
                        KmsHttpEndPointAction action, gpointer data)
{
  GST_DEBUG ("Action %d requested on %s", action, uri);
  BOOST_CHECK ( action == KMS_HTTP_END_POINT_ACTION_GET );

  if (++counted == 1) {
    /* First time */
    GST_DEBUG ("Starting pipeline");
    gst_element_set_state (pipeline, GST_STATE_PLAYING);
  }
}

BOOST_AUTO_TEST_CASE ( expired_cookie_http_end_point_test )
{
  GstElement *videotestsrc, *timeoverlay, *encoder, *agnosticbin;
  guint bus_watch_id1;
  GstBus *srcbus;

  init_test_case ();

  GST_DEBUG ("Preparing pipeline");

  /* Create gstreamer elements */
  pipeline = gst_pipeline_new ("src-pipeline");
  videotestsrc = gst_element_factory_make ("videotestsrc", NULL);
  encoder = gst_element_factory_make ("vp8enc", NULL);
  agnosticbin = gst_element_factory_make ("agnosticbin", NULL);
  timeoverlay = gst_element_factory_make ("timeoverlay", NULL);
  httpep = gst_element_factory_make ("httpendpoint", NULL);

  GST_DEBUG ("Adding watcher to the pipeline");
  srcbus = gst_pipeline_get_bus (GST_PIPELINE (pipeline) );

  bus_watch_id1 = gst_bus_add_watch (srcbus, gst_bus_async_signal_func, NULL);
  g_signal_connect (srcbus, "message", G_CALLBACK (bus_msg_cb), pipeline);
  g_object_unref (srcbus);

  GST_DEBUG ("Configuring source pipeline");
  gst_bin_add_many (GST_BIN (pipeline), videotestsrc, timeoverlay,
                    encoder, agnosticbin, httpep, NULL);
  gst_element_link (videotestsrc, timeoverlay);
  gst_element_link (timeoverlay, encoder);
  gst_element_link (encoder, agnosticbin);
  gst_element_link_pads (agnosticbin, NULL, httpep, "video_sink");

  GST_DEBUG ("Configuring elements");
  g_object_set (G_OBJECT (videotestsrc), "is-live", TRUE, "do-timestamp", TRUE,
                "pattern", 18, NULL);
  g_object_set (G_OBJECT (timeoverlay), "font-desc", "Sans 28", NULL);

  g_signal_connect (httpepserver, "url-removed", G_CALLBACK (t5_url_removed_cb),
                    NULL);
  g_signal_connect (httpepserver, "action-requested",
                    G_CALLBACK (t5_action_requested_cb), NULL);

  kms_http_ep_server_start (httpepserver, t5_http_server_start_cb);

  g_main_loop_run (loop);

  BOOST_CHECK_EQUAL (signal_count, urls_registered);

  GST_DEBUG ("Test finished");

  /* Stop Http End Point Server and destroy it */
  kms_http_ep_server_stop (httpepserver);

  gst_element_set_state (pipeline, GST_STATE_NULL);
  gst_object_unref (GST_OBJECT (pipeline) );
  g_source_remove (bus_watch_id1);

  tear_down_test_case ();
}

BOOST_AUTO_TEST_SUITE_END()
