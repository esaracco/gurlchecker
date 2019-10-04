/*
 * Copyright (C) 2002-2013
 * Emmanuel Saracco <esaracco@users.labs.libre-entreprise.org>
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330,
 * Boston, MA 02111-1307, USA.
 */

#include "utils.h"
#include "check.h"
#include "project.h"

#include "connection.h"


/**
 * UCTcpState:
 * @UC_TCP_STATE_CONNECTING: We are not yet connected.
 * @UC_TCP_STATE_CONNECTED: We are connected.
 * @UC_TCP_STATE_CLOSED: Connection has been closed.
 *
 * The different states of a #UCConn object.
 *
 */
typedef enum _UCTcpState UCTcpState;
enum _UCTcpState
{
  UC_TCP_STATE_CONNECTING,
  UC_TCP_STATE_CONNECTED,
  UC_TCP_STATE_CLOSED
};

/**
 * UCHTTPStatus:
 * @UC_HTTP_STATUS_UNKNOWN: Not defined.
 * @UC_HTTP_STATUS_OK: Host was conntacted without problem.
 * @UC_TCP_STATUS_TIMEOUT: A timeout occured while connecting.
 * @UC_HTTP_STATUS_BAD_HOSTNAME: Bad hostname (can not resolve).
 * @UC_HTTP_STATUS_CONNECTION_REFUSED: Connection refused by remote.
 * @UC_TCP_STATUS_ERROR: A error occured.
 *
 * The different status of a HTTP link
 *
 */
typedef enum _UCTcpStatus UCTcpStatus;
enum _UCTcpStatus
{
  UC_TCP_STATUS_UNKNOWN,
  UC_TCP_STATUS_OK,
  UC_TCP_STATUS_TIMEOUT,
  UC_TCP_STATUS_BAD_HOSTNAME,
  UC_TCP_STATUS_CONNECTION_REFUSED,
  UC_TCP_STATUS_ERROR
};

/**
 * UCConn:
 * @socket: The socket descriptor.
 * @thread_id: current thread.
 * @thread_end: %TRUE when the main thread is not yet running.
 * @thread_ftp_end: %TRUE when the FTP thread is not yet running.
 * @header_request: %TRUE when only header is needed.
 * @timeout: %TRUE if a timeout occured while retreiving/sending data.
 * @tcp_state: A #UCTcpState value to describe the connection state.
 * @tcp_status: A #UCTcpState value to describe the connection status.
 * @hostname: Name of the host.
 * @port: Port of the host.
 * @array: This array is used for saving server response.
 * @request: The request to send to the server.
 * @session_gnutls: GNU TLS session.
 * @use_gnutls: %TRUE if the protocol require GNU TLS (HTTPS).
 * @use_ftp: %TRUE if the protocol is FTP.
 * @ftp_passive: %TRUE if we are reading a FTP response.
 * @ftp_passive_socket: Socket to use if we are managing FTP response.
 * @ftp_passive_port: Port to use to read FTP response.
 *
 * Here we store connection informations.
 */
typedef struct _UCConn UCConn;
struct _UCConn
{
  gint socket;
  pthread_t thread_id;
  gboolean thread_end;
  gboolean thread_ftp_end;
  gboolean timeout;
  gboolean header_request;
  UCTcpState tcp_state;
  UCTcpStatus tcp_status;
  gchar *hostname;
  gchar **ip_addr;
  guint port;
  GArray *array;
  gchar *request;
#ifdef ENABLE_GNUTLS
  gnutls_session session_gnutls;
  gboolean use_gnutls;
#endif
  gboolean use_ftp;
  gboolean ftp_passive;
  gint ftp_passive_socket;
  guint ftp_passive_port;
};

#define UC_THREAD_WAIT(name, value) \
	while ( \
		!uc_check_cancel_get_value () && \
		!uc_check_ignore_item_get_value () && \
		name == value) \
	{ \
		UC_UPDATE_UI; \
		g_usleep (500); \
	}

#define UC_CHECK_CANCELED \
	( \
		uc_check_cancel_get_value () || \
		uc_check_ignore_item_get_value ()\
	)


#ifdef ENABLE_GNUTLS
static gnutls_certificate_client_credentials xcred_gnutls;
#endif

static void *connect_thread (void *data);
static void *read_thread (void *data);
static UCConn *uc_conn_new (const gchar * proto, gchar * host,
			    guint port, gchar * buffer,
			    gboolean header_request, gchar **ip);
static void uc_conn_disconnect (UCConn * conn);
static void uc_conn_free (UCConn **conn);
static UCTcpStatus connect_nonblock (const gint soc, struct sockaddr *addr,
                                     socklen_t addrlen);
static gboolean uc_conn_timeout_cb (gpointer data);


/**
 * uc_conn_init:
 * 
 * Initialize connections
 */
void
uc_conn_init (void)
{
#ifdef ENABLE_GNUTLS
  gnutls_global_init ();
  gnutls_certificate_allocate_credentials (&xcred_gnutls);
  gnutls_certificate_set_x509_trust_file (xcred_gnutls, "ca.pem",
					  GNUTLS_X509_FMT_PEM);
#endif
}


/**
 * uc_conn_free:
 * @conn: #UCConn object to free
 * 
 * Free a #UCConn structure.
 */
static void
uc_conn_free (UCConn **conn)
{
  uc_conn_disconnect (*conn);

  if ((*conn)->array != NULL)
    g_array_free ((*conn)->array, TRUE), (*conn)->array = NULL;
  if ((*conn)->hostname != NULL)
    g_free ((*conn)->hostname), (*conn)->hostname = NULL;

  g_free (*conn), *conn = NULL;
}


/**
 * uc_conn_new:
 * @proto: The protocol.
 * @host: The host.
 * @port: The port.
 * @buffer: The HTTP request.
 * @header_request: %TRUE if we need only to get the header
 *
 * Create a new #UCConn object.
 * 
 * Returns: A #UCConn node.
 */
static UCConn *
uc_conn_new (const gchar * proto, gchar * host, guint port, gchar * buffer,
	     gboolean header_request, gchar **ip_addr)
{
  UCConn *conn = NULL;


  conn = g_new0 (UCConn, 1);
  conn->thread_end = FALSE;
  conn->thread_ftp_end = FALSE;
  conn->header_request = header_request;
  conn->timeout = FALSE;
  conn->socket = 0;
  conn->port = port;
  conn->use_ftp = FALSE;
  conn->ftp_passive_socket = 0;
  conn->ftp_passive_port = 0;
  conn->ftp_passive = FALSE;
  conn->hostname = g_strdup (host);
  conn->tcp_status = UC_TCP_STATUS_UNKNOWN;
  conn->array = g_array_new (FALSE, TRUE, 1);
  conn->request = buffer;

  // To be fill during connection attempt
  conn->ip_addr = ip_addr;

  if (strcmp (proto, UC_PROTOCOL_FTP) == 0)
  {
    conn->use_ftp = TRUE;
    conn->port = atoi (UC_URL_DEFAULT_FTP_PORT);
  }
#ifdef ENABLE_GNUTLS
  if (strcmp (proto, UC_PROTOCOL_HTTPS) == 0)
  {
    conn->use_gnutls = TRUE;
    conn->port = atoi (UC_URL_DEFAULT_SSL_PORT);

    gnutls_init (&conn->session_gnutls, GNUTLS_CLIENT);
    gnutls_set_default_priority (conn->session_gnutls);
    gnutls_set_default_priority (conn->session_gnutls);
    gnutls_credentials_set (conn->session_gnutls, GNUTLS_CRD_CERTIFICATE,
                            xcred_gnutls);
  }
  else
  {
    conn->session_gnutls = NULL;
    conn->use_gnutls = FALSE;
  }
#endif

  return conn;
}


/**
 * uc_conn_timeout_cb:
 * @data: a #UCConn oject.
 * 
 * Called when a connection timeout occure.
 */
static gboolean
uc_conn_timeout_cb (gpointer data)
{
  ((UCConn *)data)->thread_end = TRUE;

  return FALSE;
}


/**
 * uc_conn_disconnect:
 * @conn: A #UCConn object.
 * 
 * Reset all #UCConn stuff.
 */
static void
uc_conn_disconnect (UCConn * conn)
{
  pthread_cancel (conn->thread_id);

#ifdef ENABLE_GNUTLS
  if (conn->use_gnutls)
  {
    gnutls_bye (conn->session_gnutls, GNUTLS_SHUT_RDWR);
    gnutls_deinit (conn->session_gnutls);
    conn->session_gnutls = NULL;
  }
#endif

  if (conn->socket > 0)
  {
    shutdown (conn->socket, SHUT_RDWR);
    close (conn->socket), conn->socket = 0;
  }

  if (conn->ftp_passive_socket)
  {
    shutdown (conn->ftp_passive_socket, SHUT_RDWR);
    close (conn->ftp_passive_socket), conn->ftp_passive_socket = 0;
  }
}


/**
 * uc_server_get_response:
 * @proto: The protocol.
 * @host: The host.
 * @port: The port.
 * @buffer: The HTTP request to send.
 * @header_request: %TRUE if we need only to get the header
 *
 * Connect to a host:port, send a HTTP request and return the content
 * of the HTTP response.
 * 
 * Returns: Content of the HTTP response.
 */
GArray *
uc_server_get_response (const gchar * proto, gchar * host, const guint port,
			gchar * buffer, gboolean header_request, gchar **ip)
{
  guint port_real = 0;
  gchar *host_real = NULL;
  UCConn *conn = NULL;
  GArray *array = NULL;
  guint timeout_id = 0;


  uc_check_ignore_item_set_value (FALSE);
  if (uc_check_cancel_get_value ())
    return NULL;

  if (strlen (uc_project_get_proxy_host ()) > 0
      && (g_ascii_strcasecmp (host, "localhost") != 0
	  && strstr (host, "127.0.0.") == NULL))
  {
    host_real = uc_project_get_proxy_host ();
    port_real = uc_project_get_proxy_port ();
  }
  else
  {
    host_real = host;
    port_real = port;
  }

  // Connect
  conn = uc_conn_new (proto, host_real, port_real, buffer, header_request,
                     ip);
  conn->thread_end = FALSE;
  pthread_create (&conn->thread_id, NULL, connect_thread, conn);
  pthread_detach (conn->thread_id);

  UC_THREAD_WAIT (conn->thread_end, FALSE);

  // Connection OK : send client request and read server response
  if (!UC_CHECK_CANCELED && conn->tcp_status == UC_TCP_STATUS_OK)
  {
    conn->thread_end = FALSE;
    pthread_create (&conn->thread_id, NULL, read_thread, conn);
    pthread_detach (conn->thread_id);

    timeout_id = g_timeout_add (uc_project_get_check_timeout () * 1000,
                                uc_conn_timeout_cb, conn);

    UC_THREAD_WAIT (conn->thread_end, FALSE);
    if (timeout_id != 0)
      g_source_remove (timeout_id), timeout_id = 0;

    // Read OK : get fill array with the server response
    if (!UC_CHECK_CANCELED && conn->tcp_status == UC_TCP_STATUS_OK)
    {
      array = g_array_new (FALSE, TRUE, 1);
      g_array_append_vals (array, conn->array->data, conn->array->len);
    }
  }

  uc_conn_free (&conn);

  return array;
}


// FIXME rewrite all this function
/**
 * read_thread:
 * @data: Pointer on a #UCConn structure.
 * 
 * The thread used to read incomming data from a server.
 *
 * Returns: Always %NULL (this is a detacheable thread).
 */
static void *
read_thread (void *data)
{
  UCConn *conn = (UCConn *) data;
  gint nb_read = 0;
  gchar *tmp = NULL;
  gchar buffer[UC_BUFFER_LEN + 1];


#ifdef ENABLE_GNUTLS
  // HTTPS
  if (conn->use_gnutls)
  {
    gnutls_record_send (conn->session_gnutls, conn->request,
                        strlen (conn->request));
    while ((nb_read = gnutls_record_recv (conn->session_gnutls, buffer,
                                          UC_BUFFER_LEN)) > 0)
    {
      if (conn->header_request && (tmp = strstr (buffer, "\r\n\r\n")))
      {
        *tmp = '\0';
        g_array_append_vals (conn->array, buffer, strlen (buffer));
        nb_read = 0;
        break;
      }

      g_array_append_vals (conn->array, buffer, nb_read);
    }

    conn->tcp_status = (nb_read == 0) ? UC_TCP_STATUS_OK : UC_TCP_STATUS_ERROR;
    goto handler_exit;
  }
  // FTP
  else if (conn->use_ftp)
#else
  if (conn->use_ftp)
#endif
  {
    guint temp[6];
    gboolean have_data = FALSE;
    gchar r_user[] = "USER anonymous\r\n";
    gchar r_pass[] = "PASS gurlchecker@gurlchecker.labs.libre-entreprise.org\r\n";
    gchar r_pasv[] = "PASV\r\n";
    gchar r_quit[] = "QUIT\r\n";
    gchar *port = NULL;
    gchar *request = NULL;
    pthread_t thread;


    // Read banner
    while ((nb_read = read (conn->socket, buffer, UC_BUFFER_LEN)) > 0)
    {
      buffer[nb_read] = '\0';
      if (strstr (buffer, "220 "))
        break;
    }

    // Send USER command
    write (conn->socket, r_user, strlen (r_user));
    // FIXME g_usleep (G_USEC_PER_SEC);
    nb_read = read (conn->socket, buffer, UC_BUFFER_LEN);

    // 230: User logged in, proceed
    if (uc_utils_ftp_code_search (buffer, "230", nb_read))
      ;
    // 332: Need account for login
    else if (uc_utils_ftp_code_search (buffer, "332", nb_read))
    {
      g_warning ("FTP: Need account for login (%s)", conn->hostname);
      goto handler_error;
    }
    // 331: User name okay, need password
    else if (uc_utils_ftp_code_search (buffer, "331", nb_read))
    {
      // Send PASS command
      write (conn->socket, r_pass, strlen (r_pass));
      while ((nb_read = read (conn->socket, buffer, UC_BUFFER_LEN)) > 0)
      {
        buffer[nb_read] = '\0';
        if (strstr (buffer, "230 "))
          break;
        else if (strstr (buffer, UC_STATUS_CODE_FTP_MAX_CLIENTS) != NULL)
        {
          g_array_append_vals (conn->array, UC_STATUS_CODE_FTP_MAX_CLIENTS,
                               strlen (UC_STATUS_CODE_FTP_MAX_CLIENTS));
          goto handler_exit;
        }
      }
    }
    else
    {
       g_warning ("ftp: problem while reading after USER command (%s)",
                  conn->hostname);
       goto handler_error;
    }

    // We first test with CWD. if this is a directory and if it exist,
    // no need to request go further: it is ok
    request = g_strdup_printf ("CWD %s\n", conn->request);
    write (conn->socket, request, strlen (request));
    // FIXME g_usleep (G_USEC_PER_SEC);
    g_free (request), request = NULL;

    nb_read = read (conn->socket, buffer, UC_BUFFER_LEN);
    if (uc_utils_ftp_code_search (buffer, "250", nb_read))
    {
       g_array_append_vals (conn->array, UC_STATUS_CODE_FTP_OK,
                            strlen (UC_STATUS_CODE_FTP_OK));
       goto handler_exit;
    }

    // Send PASV command
    write (conn->socket, r_pasv, strlen (r_pasv));
    // FIXME g_usleep (G_USEC_PER_SEC); 
    nb_read = read (conn->socket, buffer, UC_BUFFER_LEN);
    buffer[nb_read] = '\0';

    if (!uc_utils_ftp_code_search (buffer, "227", nb_read) || nb_read <= 4)
    {
      g_warning ("FTP: problem with passive mode (%s)", conn->hostname);
      goto handler_error;
    }

    buffer[nb_read - 3] = '\0';
    if ((port = strchr (buffer, '(')) != NULL)
      port++;
    else
    {
      g_warning ("FTP: problem reading passive mode line (%s)", conn->hostname);
      goto handler_error;
    }

    if (sscanf (port, "%u,%u,%u,%u,%u,%u",
                &temp[0], &temp[1], &temp[2], &temp[3], &temp[4],
                &temp[5]) != 6)
    {
      g_warning ("FTP: problem building passive port (%s)", conn->hostname);
      goto handler_error;
    }

    request = g_strdup_printf ("LIST %s\n", conn->request);

    write (conn->socket, request, strlen (request));
    g_free (request), request = NULL;

    // FTP passive connection
    conn->ftp_passive_port = (temp[4] * 256) + temp[5];
    conn->ftp_passive = TRUE;
    conn->thread_ftp_end = FALSE;
    pthread_create (&thread, NULL, connect_thread, conn);
    pthread_detach (thread);

    UC_THREAD_WAIT (conn->thread_ftp_end, FALSE);
    pthread_cancel (thread);
    conn->thread_ftp_end = FALSE;
    conn->ftp_passive = FALSE;

    if (conn->tcp_status != UC_TCP_STATUS_OK)
    {
      g_warning ("FTP: problem while connecting on passive port (%s)",
                 conn->hostname);
       goto handler_error;
    }
    else
    {
      gchar *state = NULL;


       while ((nb_read =
                 read (conn->ftp_passive_socket, buffer, UC_BUFFER_LEN)) > 0)
         have_data = TRUE;

       shutdown (conn->ftp_passive_socket, SHUT_RDWR);
       pthread_cancel (thread);
       close (conn->ftp_passive_socket);

       state = (have_data) ? UC_STATUS_CODE_FTP_OK : UC_STATUS_CODE_FTP_OK;
       g_array_append_vals (conn->array, state, strlen (state));

       // Send QUIT command
       write (conn->socket, r_quit, strlen (r_quit));
    }

    goto handler_exit;
  }
  // HTTP
  else if (conn->socket > 0)
  {
    write (conn->socket, conn->request, strlen (conn->request));
    while (!tmp && (nb_read = read (conn->socket, buffer, UC_BUFFER_LEN)) > 0)
    {
      if (conn->header_request)
      {
        gchar b = buffer[nb_read];


        buffer[nb_read] = '\0';
        if ((tmp = strstr (buffer, "\r\n\r\n")))
        {
          *tmp = '\0';
          g_array_append_vals (conn->array, buffer, strlen (buffer));
          nb_read = 0;
        }
        else
          buffer[nb_read] = b;
      }

      if (nb_read > 0)
        g_array_append_vals (conn->array, buffer, nb_read);
    }

    conn->tcp_status = (nb_read == 0) ? UC_TCP_STATUS_OK : UC_TCP_STATUS_ERROR;

    goto handler_exit;
  }

handler_error:
  conn->tcp_status = UC_TCP_STATUS_ERROR;

handler_exit:
  conn->thread_end = TRUE;

  pthread_exit (NULL);
}


/**
 * connect_nonblock:
 * @soc: Socket to connect.
 * @addr: #sockaddr_in structure with socket's informations.
 *
 * Do a non-blocking connect.
 *  
 * Returns: #UCTcpStatus of the operation.
 */
static UCTcpStatus
connect_nonblock (const gint soc, struct sockaddr *addr, socklen_t addrlen)
{
  gint res;
  glong arg;
  fd_set myset;
  struct timeval tv;
  gint valopt;
  socklen_t lon;


  if ((arg = fcntl (soc, F_GETFL, NULL)) < 0)
    {
      /* g_warning ("Error fcntl(..., F_GETFL) (%s)\n", strerror (errno)); */
      return UC_TCP_STATUS_ERROR;
    }

  arg |= O_NONBLOCK;
  if (fcntl (soc, F_SETFL, arg) < 0)
    {
      /* g_warning ("Error fcntl(..., F_SETFL) (%s)\n", strerror (errno)); */
      return UC_TCP_STATUS_ERROR;
    }

  res = connect (soc, addr, addrlen);
  if (res < 0)
    {
      if (errno == EINPROGRESS)
	{
	  do
	    {
	      tv.tv_sec = uc_project_get_check_timeout ();
	      tv.tv_usec = 0;
	      FD_ZERO (&myset);
	      FD_SET (soc, &myset);
	      res = select (soc + 1, NULL, &myset, NULL, &tv);
	      if (res < 0 && errno != EINTR)
		{
		  /* g_warning ("Error connecting %d - %s\n", errno,
		     strerror (errno)); */
		  return UC_TCP_STATUS_ERROR;
		}
	      else if (res > 0)
		{
		  lon = sizeof (int);
		  if (getsockopt
		      (soc, SOL_SOCKET, SO_ERROR, (void *) (&valopt),
		       &lon) < 0)
		    {
		      /* g_warning ("Error in getsockopt() %d - %s\n",
		         errno, strerror (errno)); */
		      return UC_TCP_STATUS_ERROR;
		    }

		  if (valopt)
		    {
		      /* g_warning ("Error in delayed connection() %d - %s\n",
		         valopt, strerror (valopt)); */
		      return UC_TCP_STATUS_ERROR;
		    }
		  break;
		}
	      else
		{
		  /* g_warning ("Timeout in select() - Cancelling!\n"); */
		  return UC_TCP_STATUS_TIMEOUT;
		}
	    }
	  while (1);
	}
      else
	{
	  /* g_warning ("Error connecting %d - %s\n", errno, strerror (errno));
	   */
	  return UC_TCP_STATUS_ERROR;
	}
    }

  if ((arg = fcntl (soc, F_GETFL, NULL)) < 0)
    {
      /* g_warning ("Error fcntl(..., F_GETFL) (%s)\n", strerror (errno)); */
      return UC_TCP_STATUS_ERROR;
    }

  arg &= (~O_NONBLOCK);
  if (fcntl (soc, F_SETFL, arg) < 0)
    {
      /* g_warning ("Error fcntl(..., F_SETFL) (%s)\n", strerror (errno)); */
      return UC_TCP_STATUS_ERROR;
    }

  return UC_TCP_STATUS_OK;
}


/**
 * connect_thread:
 * @data: a #UCConn object.
 * 
 * The thread used to connect to a server.
 *
 * Returns: Always %NULL (this is a detacheable thread).
 */
static void *
connect_thread (void *data)
{
  UCConn *conn = (UCConn *) data;
  struct addrinfo hints; 
  struct addrinfo *res = NULL; 
  gint *sock = NULL;
  gchar *port = NULL;
  gint ret = 0;


  if (conn->ftp_passive)
  {
    sock = &conn->ftp_passive_socket;
    port = g_strdup_printf ("%u", conn->ftp_passive_port);
  }
  else
  {
    sock = &conn->socket;
    port = g_strdup_printf ("%u", conn->port);
  }

  // Free port at thread exit
  pthread_cleanup_push (g_free, port);

  memset (&hints, 0, sizeof (hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_family = PF_UNSPEC;

  if ((ret = getaddrinfo (conn->hostname, port, &hints, &res)) != 0 ||
      res == NULL)
  {
    /* FIXME conn->tcp_status = UC_TCP_STATUS_BAD_HOSTNAME; */
    g_warning ("Problem with getaddrinfo(): %s:%s\n\t-> %s",
               conn->hostname, port, gai_strerror (ret));
    conn->tcp_status = UC_TCP_STATUS_ERROR;
  }
  // TODO check all returned addresses for the hostname
  else if ((*sock = socket (res->ai_family, res->ai_socktype,
                            res->ai_protocol)) >= 0)
  {
    // Get IP addr if caller is OK for that (header request only)
    if (conn->ip_addr)
    {
      struct sockaddr_in *sa = (struct sockaddr_in *) res->ai_addr;
      *conn->ip_addr = g_strdup (inet_ntoa (sa->sin_addr));
    }

    // Free getaddrinfo result at thread exit
    pthread_cleanup_push ((void *) freeaddrinfo, res);

    /* connect socket */
    if ((conn->tcp_status =
           connect_nonblock (*sock, res->ai_addr,
                             res->ai_addrlen)) == UC_TCP_STATUS_OK)
    {
#ifdef ENABLE_GNUTLS
      if (conn->use_gnutls)
      {
        gnutls_transport_set_ptr (conn->session_gnutls,
                                   (gnutls_transport_ptr_t) conn->socket);
        do
          ret = gnutls_handshake (conn->session_gnutls);
        while ((ret == GNUTLS_E_AGAIN) || (ret == GNUTLS_E_INTERRUPTED));

        if (ret < 0)
        {
          g_warning ("Problem with SSL handshake: %s:%s", conn->hostname, port);
          gnutls_perror (ret);
          conn->tcp_status = UC_TCP_STATUS_ERROR;
        }
      }
#endif
    }

    pthread_cleanup_pop (1); // freeaddrinfo (res)
  }

  pthread_cleanup_pop (1); // g_free (port)

  if (conn->ftp_passive)
    conn->thread_ftp_end = TRUE;
  else
    conn->thread_end = TRUE;

  pthread_exit (NULL);
}
