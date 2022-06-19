#include "io.h"
#include <assert.h>
#include <debuglog.h>
#include <errno.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

/**
 * @brief A shorthand for getting a TCP socket for listening for client
 * connections.
 * @param port What port to create the listening socket on.
 * @return Socket FD on success, -1 on failure.
 */
static int32_t sock_listen_create(uint16_t const port)
{
    int32_t const sock = socket(AF_INET, SOCK_STREAM, 0U);
    if (sock != -1)
    {
        struct sockaddr_in const sock_addr = {.sin_zero = {0},
                                              .sin_family = AF_INET,
                                              .sin_addr.s_addr = INADDR_ANY,
                                              .sin_port = htons(port)};
        if (bind(sock, (struct sockaddr *)&sock_addr, sizeof(sock_addr)) != -1)
        {
            if (listen(sock, IO_CLIENT_COUNT_LEN) != -1)
            {
                if (fcntl(sock, F_SETFL, O_NONBLOCK) == 0U)
                {
                    Log2(PCSC_LOG_INFO, "Listening on port %u.", port);
                    return sock;
                }
                else
                {
                    Log2(PCSC_LOG_ERROR,
                         "Failed to set listening socket to non-blocking: %s.",
                         strerror(errno));
                }
            }
            else
            {
                Log2(PCSC_LOG_ERROR, "Call to listen() failed: %s.",
                     strerror(errno));
            }
        }
        else
        {
            Log2(PCSC_LOG_ERROR, "Call to bind() failed: %s.", strerror(errno));
        }
        if (close(sock) == -1)
        {
            Log2(PCSC_LOG_ERROR, "Call to close() failed: %s.",
                 strerror(errno));
        }
    }
    else
    {
        Log2(PCSC_LOG_ERROR, "Call to socket() failed: %s.", strerror(errno));
    }
    Log1(PCSC_LOG_ERROR, "Failed to create a server socket.");
    return -1;
}

/**
 * @brief Closes the socket.
 * @param sock Socket FD to close.
 * @return 0 on success, -1 on failure.
 */
static int32_t sock_close(int32_t const sock)
{
    if (sock < 0)
    {
        Log2(PCSC_LOG_ERROR, "Invalid socket FD, expected >=0 got %i.", sock);
        return -1;
    }

    bool success = true;
    if (sock != -1)
    {
        if (shutdown(sock, SHUT_RDWR) == -1)
        {
            Log2(PCSC_LOG_ERROR, "Call to shutdown() failed: %s.",
                 strerror(errno));
            /* A failed shutdown is only a problem for the client. */
        }
        if (close(sock) == -1)
        {
            Log2(PCSC_LOG_ERROR, "Call to close() failed: %s.",
                 strerror(errno));
            success = success && false;
        }
    }
    if (success)
    {
        return 0;
    }
    Log1(PCSC_LOG_ERROR, "Failed to close socket.");
    return -1;
}

/**
 * @brief Send a message to a given (client) socket.
 * @param sock Client socket.
 * @param msg Message to send.
 * @return Number of byte that were sent on success, -1 on failure.
 */
static int64_t msg_send(int32_t const sock, msg_st const *const msg)
{
    if (sock < 0)
    {
        Log2(PCSC_LOG_ERROR, "Invalid socket FD, expected >=0 got %i.", sock);
        return -1;
    }

    if (msg->hdr.size > sizeof(msg->data))
    {
        Log1(PCSC_LOG_ERROR,
             "Message header indicates a data size larger than the buffer "
             "itself.");
        return -1;
    }

    /* Safe cast since the target type can fit the sum of cast ones. */
    uint32_t const size_msg = (uint32_t)sizeof(msg_hdr_st) + msg->hdr.size;
    int64_t const sent_bytes = send(sock, &msg->hdr, size_msg, 0U);
    if (sent_bytes == size_msg)
    {
        /* Success. */
    }
    else if (sent_bytes < 0)
    {
        Log2(PCSC_LOG_ERROR, "Call to send() failed: %s.", strerror(errno));
    }
    else
    {
        Log1(PCSC_LOG_ERROR, "Failed to send message.");
        return -1;
    }

    if (sent_bytes != size_msg)
    {
        Log1(PCSC_LOG_ERROR, "Failed to send all the message bytes.");
        return -1;
    }

    io_dbg_msg_print("TX:", msg);
    return sent_bytes;
}

/**
 * @brief Receive a message from a given (client) socket.
 * @param sock Client socket.
 * @param msg Where to write the received message.
 * @return Number of received bytes on success, -1 on failure.
 */
static int64_t msg_recv(int32_t const sock, msg_st *const msg)
{
    if (sock < 0)
    {
        Log2(PCSC_LOG_ERROR, "Invalid socket FD, expected >=0 got %i.", sock);
        return -1;
    }

    bool recv_failure = false;
    int64_t recvd_bytes;
    recvd_bytes = recv(sock, &msg->hdr, sizeof(msg_hdr_st), 0U);
    do
    {
        /* Check if succeeded. */
        if (recvd_bytes == sizeof(msg_hdr_st))
        {
            /**
             * Check if the indicated size is too large for the static
             * message data buffer.
             */
            if (msg->hdr.size > sizeof(msg->data) ||
                msg->hdr.size < offsetof(msg_data_st, buf))
            {
                Log4(PCSC_LOG_ERROR,
                     "Value of the size field in the message header is too "
                     "large. Got %u, expected %lu >= n <= %lu.",
                     msg->hdr.size, offsetof(msg_data_st, buf),
                     sizeof(msg->data));
                recv_failure = true;
                break;
            }
            recvd_bytes = recv(sock, &msg->data, msg->hdr.size, 0);
            /**
             * Make sure we received the whole message also the cast here is
             * safe.
             */
            if (recvd_bytes != (int64_t)msg->hdr.size)
            {
                recv_failure = true;
                break;
            }
        }
        else
        {
            Log1(PCSC_LOG_ERROR, "Failed to receive message header.");
            recv_failure = true;
            break;
        }
    } while (0U);
    if (recvd_bytes < 0)
    {
        Log2(PCSC_LOG_ERROR, "Call to recv() failed: %s.", strerror(errno));
    }

    if (recv_failure)
    {
        Log1(PCSC_LOG_ERROR, "Failed to receive message.");
        return -1;
    }
    else
    {
        io_dbg_msg_print("RX: ", msg);
        return recvd_bytes;
    }
}

int32_t io_init(io_ctx_st *const io_ctx, uint16_t const port)
{
    io_ctx->sock_listen = sock_listen_create(port);
    if (io_ctx->sock_listen != -1)
    {
        return 0;
    }
    Log2(PCSC_LOG_ERROR, "Failed to open a server socket: sock=%i.",
         io_ctx->sock_listen);
    io_ctx->sock_listen = -1;
    return -1;
}

int32_t io_recv(int32_t const sock, msg_st *const msg)
{
    if (sock < 0)
    {
        Log2(PCSC_LOG_ERROR, "Invalid socket FD, expected >=0 got %i.", sock);
        return -1;
    }

    if (msg_recv(sock, msg) >= 0)
    {
        return 0;
    }

    Log1(PCSC_LOG_ERROR, "Failed to receive message from client.");
    return -1;
}

int32_t io_send(int32_t const sock, msg_st const *const msg)
{
    if (sock < 0)
    {
        Log2(PCSC_LOG_ERROR, "Invalid socket FD, expected >=0 got %i.", sock);
        return -1;
    }

    if (msg_send(sock, msg) >= 0)
    {
        return 0;
    }

    Log1(PCSC_LOG_ERROR, "Failed to send message to client.");
    return -1;
}

int32_t io_client_connect(io_ctx_st const *const io_ctx, int32_t *const sock)
{
    if (*sock != -1)
    {
        Log1(PCSC_LOG_ERROR, "Value of socket must be -1 before accepting.");
        return -1;
    }

    *sock = accept(io_ctx->sock_listen, NULL, NULL);
    if (*sock >= 0)
    {
        Log1(PCSC_LOG_INFO, "Client connected.");
        return 0;
    }
    else if (errno == EAGAIN || errno == EWOULDBLOCK)
    {
        Log1(PCSC_LOG_DEBUG,
             "Tried accepting connections but no client was queued.");
        return 1;
    }
    else if (errno == ECONNABORTED || errno == EPERM || errno == EPROTO)
    {
        Log1(PCSC_LOG_ERROR, "Failed to accept a client connection because of "
                             "client-side problems, retrying...");
        return -1;
    }
    else
    {
        Log2(PCSC_LOG_ERROR,
             "Failed to accept a client connection. Call to accept() "
             "failed: %s.",
             strerror(errno));
        return -1;
    }
}

int32_t io_client_disconnect(int32_t *const sock)
{
    if (*sock < 0)
    {
        Log2(PCSC_LOG_ERROR, "Invalid socket FD, expected >=0 got %i.", *sock);
        return -1;
    }

    if (sock_close(*sock) == 0)
    {
        *sock = -1;
        return 0;
    }
    else
    {
        Log1(PCSC_LOG_ERROR, "Failed to close client socket.");
        return -1;
    }
}

int32_t io_fini(io_ctx_st *const io_ctx)
{
    if (sock_close(io_ctx->sock_listen) == 0)
    {
        for (uint32_t client_idx = 0U;
             client_idx <
             sizeof(io_ctx->sock_client) / sizeof(io_ctx->sock_client[0U]);
             ++client_idx)
        {
            if (sock_close(io_ctx->sock_client[client_idx]) == 0)
            {
                return 0;
            }
            else
            {
                Log2(PCSC_LOG_ERROR, "Failed to close client socket: id=%u.",
                     client_idx);
            }
        }
    }
    return -1;
}

void io_dbg_msg_print(char const *const prestr, msg_st const *const msg)
{
    static char dbg_str[2048U];

    int32_t const len_base = snprintf(
        dbg_str, sizeof(dbg_str),
        "%s"
        "(Message"
        "\n    (Header (Size %u))"
        "\n    (Data"
        "\n        (Cont 0x%08X)"
        "\n        (BufLenExp %u)"
        "\n        (Buf [",
        prestr, msg->hdr.size, msg->data.cont_state, msg->data.buf_len_exp);
    if (len_base < 0)
    {
        Log2(PCSC_LOG_ERROR, "snprintf() failed with %i.", len_base);
        return;
    }
    else if (len_base > (int32_t)sizeof(dbg_str))
    {
        Log3(PCSC_LOG_ERROR, "snprintf() needed %i bytes but only got %lu.",
             len_base, sizeof(dbg_str));
        return;
    }
    uint32_t len = (uint32_t)len_base;

    if (msg->hdr.size > sizeof(msg->data.buf) ||
        msg->hdr.size < offsetof(msg_data_st, buf))
    {
        char const str_inv[] = " invalid";
        if (len + strlen(str_inv) > sizeof(dbg_str))
        {
            Log3(PCSC_LOG_ERROR, "Needed %lu bytes but only got %lu.",
                 len + strlen(str_inv), sizeof(dbg_str));
            return;
        }
        memcpy(&dbg_str[len], str_inv, strlen(str_inv));
        len += strlen(str_inv);
    }
    else
    {
        for (uint32_t data_idx = 0U;
             data_idx < msg->hdr.size - offsetof(msg_data_st, buf); ++data_idx)
        {
            int32_t const len_extra =
                snprintf(&dbg_str[len], sizeof(dbg_str) - len, " %02X",
                         msg->data.buf[data_idx]);
            if (len_extra < 0)
            {
                Log2(PCSC_LOG_ERROR, "snprintf() failed with %i.", len_extra);
                return;
            }
            else if (len + (uint32_t)len_extra > sizeof(dbg_str))
            {
                Log3(PCSC_LOG_ERROR,
                     "snprintf() needed %i bytes but only got %lu.",
                     len + (uint32_t)len_extra, sizeof(dbg_str));
                return;
            }
            len += (uint32_t)len_extra;
        }
    }
    char const str_end[] = " ])))";
    if (len + strlen(str_end) > sizeof(dbg_str))
    {
        Log3(PCSC_LOG_ERROR, "Needed %lu bytes but only got %lu.",
             len + strlen(str_end), sizeof(dbg_str));
        return;
    }
    memcpy(&dbg_str[len], str_end, sizeof(str_end));
    len += sizeof(str_end);
    Log3(PCSC_LOG_DEBUG, "%.*s", len, dbg_str);
}
