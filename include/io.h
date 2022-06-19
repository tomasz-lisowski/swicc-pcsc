#pragma once

#include <stdint.h>
#include <swicc/swicc.h>

#define IO_CLIENT_COUNT_LEN 64U

typedef struct io_ctx_s
{
    int32_t sock_listen;
    int32_t sock_client[IO_CLIENT_COUNT_LEN];
} io_ctx_st;

typedef struct msg_hdr_s
{
    uint32_t size;
} __attribute__((packed)) msg_hdr_st;

typedef struct msg_data_s
{
    uint32_t cont_state;

    /**
     * When sent to swICC, this is unused, when being received by a client, this
     * indicates how many bytes to read from the interface (i.e. the expected
     * buffer length).
     */
    uint32_t buf_len_exp;

    uint8_t buf[SWICC_DATA_MAX];
} __attribute__((packed)) msg_data_st;

/**
 * Expected message format as seen coming from the client.
 * @note Might want to create a raw and internal representation of the message
 * if using sizeof(msg.data) feels unsafe.
 */
typedef struct msg_s
{
    msg_hdr_st hdr;
    msg_data_st data;
} __attribute__((packed)) msg_st;

/**
 * @brief Initialize IO before receiving data for SIM.
 * @param io_ctx I/O module context.
 * @param port Port that clients will be able to connect to for sending
 * data.
 * @return 0 on success, -1 on failure.
 */
int32_t io_init(io_ctx_st *const io_ctx, uint16_t const port);

/**
 * @brief Receive a message.
 * @param sock Client socket to receive from.
 * @param msg Where the received message will be written.
 * @return 0 on success, -1 on failure.
 */
int32_t io_recv(int32_t const sock, msg_st *const msg);

/**
 * @brief Send a message.
 * @param sock Client socket to send message to.
 * @param msg Message to send.
 * @return 0 on success, -1 on failure.
 */
int32_t io_send(int32_t const sock, msg_st const *const msg);

/**
 * @brief If a client is queued, accepts the connection.
 * @param io_ctx I/O module context.
 * @param sock Where the file descriptor of the client socket will be written.
 * @return 0 if client connection has been accepted, 1 if no client was queued,
 * -1 on failure.
 */
int32_t io_client_connect(io_ctx_st const *const io_ctx, int32_t *const sock);

/**
 * @brief Disconnect a client.
 * @param sock Shall contain the FD of the client socket that shall be closed.
 * This will get reset to -1.
 * @return 0 success, -1 on failure.
 */
int32_t io_client_disconnect(int32_t *const sock);

/**
 * @brief De-initialize the IO. On success, a call to any of the IO functions is
 * undefined behavior.
 * @param io_ctx I/O module context.
 * @return 0 on success, -1 on failure.
 */
int32_t io_fini(io_ctx_st *const io_ctx);

/**
 * @brief Print out the message contents.
 * @param prestr A string to print before the debug message. It can be NULL.
 * @param msg Message to print out.
 */
void io_dbg_msg_print(char const *const prestr, msg_st const *const msg);
