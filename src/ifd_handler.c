/**
 * IFD handler for PCSC-lite.
 */

#include <debuglog.h>
#include <ifdhandler.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdio.h>
#include <string.h>
#include <swicc/swicc.h>
#include <sys/socket.h>

#define IFD_SLOT_COUNT_MAX SWICC_NET_CLIENT_COUNT_MAX
#define IFD_SERVER_PORT_STR "37324"

typedef struct client_icc_s
{
    char atr[MAX_ATR_SIZE];
    uint32_t atr_len;
    uint32_t cont_iface;
    uint32_t cont_icc;
    uint32_t buf_len_exp;
} client_icc_st;

static swicc_net_server_st server_ctx = {.sock_server = -1};
static swicc_net_msg_st msg_tx = {0U};
static swicc_net_msg_st msg_rx = {0U};

/* Keep track of client ICCs. */
static client_icc_st client_icc[IFD_SLOT_COUNT_MAX] = {0U};

#ifdef DEBUG
static char dbg_str[4096U];
#else
static char dbg_str[0U];
#endif
static uint16_t dbg_str_len;

/**
 * @brief Parse the Lun into a reader number and slot number and check that
 * these values are in sane ranges.
 * @param[in] Lun Yhe Lun to parse.
 * @param[out] reader_num Where to write the number of the reader.
 * @param[out] slot_num Where to write the number of the slot.
 * @return 0 on valid Lun (success), -1 on invalid Lun (failure).
 */
static int32_t lun_parse(DWORD const Lun, uint16_t *const reader_num,
                         uint16_t *const slot_num)
{
    /* Safe cast since extracting 2 high bytes. */
    *reader_num = (uint16_t)((Lun & 0xFFFF0000) >> 4U);
    *slot_num = Lun & 0x0000FFFF;

    if (*slot_num >= IFD_SLOT_COUNT_MAX)
    {
        Log2(PCSC_LOG_ERROR,
             "Tried to create an unsupported slot: slot_num=%u.", *slot_num);
        return IFD_COMMUNICATION_ERROR;
    }
    if (*reader_num != 0U)
    {
        Log2(PCSC_LOG_ERROR,
             "Tried to create an unsupported reader: reader_num=%u.",
             *reader_num);
        return IFD_COMMUNICATION_ERROR;
    }
    return 0;
}

/**
 * @brief Send the TX message, and receive the response into the RX message.
 * @param[in] slot_num Communicate with the card in a given slot.
 * @param[in] log_msg_enable If the exchange should be logged.
 * @return 0 on success, -1 on failure.
 */
static int32_t client_msg_io(uint16_t const slot_num, bool const log_msg_enable)
{
    if (log_msg_enable)
    {
        dbg_str_len = sizeof(dbg_str);
        if (swicc_dbg_net_msg_str(dbg_str, &dbg_str_len, "TX:\n", &msg_tx) ==
            SWICC_RET_SUCCESS)
        {
            Log3(PCSC_LOG_DEBUG, "%.*s", dbg_str_len, dbg_str);
        }
        else
        {
            Log1(PCSC_LOG_ERROR, "Failed to print TX message.");
        }
    }

    if (swicc_net_send(server_ctx.sock_client[slot_num], &msg_tx) !=
        SWICC_RET_SUCCESS)
    {
        Log1(PCSC_LOG_ERROR, "Failed to transmit data to ICC.");
        return -1;
    }

    if (swicc_net_recv(server_ctx.sock_client[slot_num], &msg_rx) !=
        SWICC_RET_SUCCESS)
    {
        Log1(PCSC_LOG_ERROR, "Failed to receive data from ICC.");
        return -1;
    }

    if (log_msg_enable)
    {
        dbg_str_len = sizeof(dbg_str);
        if (swicc_dbg_net_msg_str(dbg_str, &dbg_str_len, "RX:\n", &msg_rx) ==
            SWICC_RET_SUCCESS)
        {
            Log3(PCSC_LOG_DEBUG, "%.*s", dbg_str_len, dbg_str);
        }
        else
        {
            Log1(PCSC_LOG_ERROR, "Failed to print RX message.");
        }
    }

    client_icc[slot_num].cont_icc = msg_rx.data.cont_state;
    client_icc[slot_num].buf_len_exp = msg_rx.data.buf_len_exp;
    return 0;
}

/**
 * @brief Disconnect a client making sure to cleanup any state realted to it.
 * @param[in] slot_num
 */
static void client_disconnect(uint16_t const slot_num)
{
    swicc_net_server_client_disconnect(&server_ctx, (uint16_t)slot_num);
    client_icc[slot_num].atr_len = 0U;
    client_icc[slot_num].cont_iface = 0U;
    client_icc[slot_num].cont_icc = 0U;
}

/**
 * @brief Perform an ICC powerup (cold reset with PPS exchange).
 * @param[in] slot_num
 * @return 0 on success, -1 on failure.
 */
static int32_t icc_powerup(uint16_t const slot_num)
{
    /* All contact states are set to valid. */
    msg_tx.data.cont_state = 0U;
    msg_tx.data.ctrl = SWICC_NET_MSG_CTRL_MOCK_RESET_COLD_PPS_Y;
    msg_tx.data.buf_len_exp = 0U;
    msg_tx.hdr.size = offsetof(swicc_net_msg_data_st, buf);

    if (client_msg_io(slot_num, true) != 0 ||
        msg_rx.data.ctrl != SWICC_NET_MSG_CTRL_SUCCESS)
    {
        return -1;
    }

    /**
     * At this point the ICC has been mock initialized and the interface
     * contacts shall be in the 'ready' state.
     */
    client_icc[slot_num].cont_iface = FSM_STATE_CONT_READY;

    /**
     * Make sure that the response contains an ATR (that's non-zero in length).
     */
    if (msg_rx.hdr.size <= offsetof(swicc_net_msg_data_st, buf))
    {
        Log1(PCSC_LOG_ERROR, "ICC ATR is invalid.");
        return -1;
    }
    client_icc[slot_num].atr_len =
        (uint32_t)(msg_rx.hdr.size - offsetof(swicc_net_msg_data_st, buf));
    memcpy(client_icc[slot_num].atr, msg_rx.data.buf,
           client_icc[slot_num].atr_len);
    return 0;
}

/**
 * @brief The logger that is used with the swICC network module. This is used so
 * that the PC/SC-lite middleware logging utilities can be used.
 */
static swicc_net_logger_ft net_logger;
static void net_logger(char const *const fmt, ...)
{
#ifdef DEBUG
    va_list argptr;
    va_start(argptr, fmt);
    log_msg(PCSC_LOG_DEBUG, fmt, argptr);
    va_end(argptr);
#endif
}

/**
 * @brief Check if ann ICC is present. This does not actually send any data to
 * the card, just relies on the most recent information (last keep-alive
 * message).
 * @param[in] slot_num
 * @return true if present, false if not.
 */
static bool icc_present(uint16_t const slot_num)
{
    return server_ctx.sock_client[slot_num] >= 0;
}

/**
 * @brief Check if the reader is present, i.e., if it has been initialized.
 * @return true if present, false if not.
 */
static bool reader_present()
{
    return server_ctx.sock_server >= 0;
}

RESPONSECODE IFDHCreateChannelByName(DWORD const Lun, LPSTR const DeviceName)
{
    Log3(PCSC_LOG_DEBUG, "Lun=0x%04lX, DeviceName='%s'.", Lun, DeviceName);

    uint16_t reader_num;
    uint16_t slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    if (strncmp(DeviceName, DIR_PCSC_DEV, strlen(DIR_PCSC_DEV)) != 0)
    {
        Log3(PCSC_LOG_ERROR, "Expected device '%s' but got DeviceName='%s'.",
             DIR_PCSC_DEV, DeviceName);
        return IFD_COMMUNICATION_ERROR;
    }

    /* Channel is ignored. */
    return IFDHCreateChannel(Lun, 0U);
}

RESPONSECODE IFDHControl(DWORD Lun, DWORD dwControlCode, PUCHAR TxBuffer,
                         DWORD TxLength, PUCHAR RxBuffer, DWORD RxLength,
                         LPDWORD pdwBytesReturned)
{
    Log9(PCSC_LOG_DEBUG,
         "Lun=0x%04lX, dwControlCode=%lu, TxBuffer=%p, TxLength=%lu, "
         "RxBuffer=%p, RxLength=%lu, pdwBytesReturned=%p.%c",
         Lun, dwControlCode, TxBuffer, TxLength, RxBuffer, RxLength,
         pdwBytesReturned, '\0');

    uint16_t reader_num;
    uint16_t slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE IFDHCreateChannel(DWORD Lun, DWORD Channel)
{
    /* Channel is ignored. */
    Log3(PCSC_LOG_DEBUG, "Lun=0x%04lX, Channel=%lu.", Lun, Channel);

    uint16_t reader_num;
    uint16_t slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    if (!reader_present())
    {
        /* Initialize the server context. */
        server_ctx.sock_server = -1;
        for (uint16_t client_sock_idx = 0U;
             client_sock_idx < IFD_SLOT_COUNT_MAX; ++client_sock_idx)
        {
            server_ctx.sock_client[client_sock_idx] = -1;
        }

        /* Use the PC/SC-lite logging functions. */
        swicc_net_logger_register(net_logger);

        if (swicc_net_server_create(&server_ctx, IFD_SERVER_PORT_STR) !=
            SWICC_RET_SUCCESS)
        {
            return IFD_COMMUNICATION_ERROR;
        }
    }
    else
    {
        if (icc_present(slot_num))
        {
            /* Already present. */
            return IFD_COMMUNICATION_ERROR;
        }
    }

    return IFD_SUCCESS;
}

RESPONSECODE IFDHCloseChannel(DWORD Lun)
{
    Log2(PCSC_LOG_DEBUG, "Lun=0x%04lX.", Lun);

    uint16_t reader_num;
    uint16_t slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    /* Destroying channel 0 leads to destruction of the whole reader. */
    if (reader_present())
    {
        /**
         * @warning This assumes that slot 0 will be destroyed first.
         */
        if (slot_num == 0)
        {
            swicc_net_server_destroy(&server_ctx);
        }
        else
        {
            swicc_net_server_client_disconnect(&server_ctx, slot_num);
        }
    }
    return IFD_SUCCESS;
}

RESPONSECODE IFDHGetCapabilities(DWORD Lun, DWORD Tag, PDWORD Length,
                                 PUCHAR Value)
{
    Log5(PCSC_LOG_DEBUG, "Lun=0x%04lX, Tag=0x%04lX, Length=%p, Value=%p.", Lun,
         Tag, Length, Value);

    uint16_t reader_num;
    uint16_t slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    if (!reader_present())
    {
        return IFD_NO_SUCH_DEVICE;
    }

    switch (Tag)
    {
    case TAG_IFD_ATR:
        if (icc_present(slot_num))
        {
            if (*Length < client_icc->atr_len)
            {
                return IFD_ERROR_INSUFFICIENT_BUFFER;
            }
            memcpy(Value, client_icc->atr, client_icc->atr_len);
            *Length = client_icc->atr_len;
            return IFD_SUCCESS;
        }
        return IFD_COMMUNICATION_ERROR;
    case TAG_IFD_SIMULTANEOUS_ACCESS:
        /* The driver can handle only 1 reader at any time. */
        Value[0U] = 1U;
        Log2(PCSC_LOG_INFO, "Supported reader count: %u.", Value[0U]);
        return IFD_SUCCESS;
    case TAG_IFD_THREAD_SAFE:
        /* Reader slots cannot be accessed simultaneously. */
        Value[0U] = 0U;
        Log2(PCSC_LOG_INFO, "Supporting thread-safe readers: %u.", Value[0U]);
        return IFD_SUCCESS;
    case TAG_IFD_SLOTS_NUMBER:
        /* Number of slots in this reader. */
        Value[0U] = IFD_SLOT_COUNT_MAX;
        Log2(PCSC_LOG_INFO, "Supported slot count per reader: %u.", Value[0U]);
        return IFD_SUCCESS;
    case TAG_IFD_SLOT_THREAD_SAFE:
        /* Multiple slots can't be accessed simultaneously. */
        Value[0U] = 0U;
        Log2(PCSC_LOG_INFO, "Supporting thread-safe slots: %u.", Value[0U]);
        return IFD_SUCCESS;
    case TAG_IFD_STOP_POLLING_THREAD:
    case TAG_IFD_POLLING_THREAD_KILLABLE:
    case TAG_IFD_POLLING_THREAD_WITH_TIMEOUT:
        Log1(PCSC_LOG_INFO, "Capability not supported.");
        return IFD_NOT_SUPPORTED;
    default:
        return IFD_ERROR_TAG;
    }
}

RESPONSECODE IFDHSetCapabilities(DWORD Lun, DWORD Tag, DWORD Length,
                                 PUCHAR Value)
{
    Log5(PCSC_LOG_DEBUG, "Lun=0x%04lX, Tag=0x%04lX, Length=%lu, Value=%p.", Lun,
         Tag, Length, Value);

    uint16_t reader_num;
    uint16_t slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    switch (Tag)
    {
    default:
        return IFD_ERROR_TAG;
    }
}

RESPONSECODE IFDHSetProtocolParameters(DWORD Lun, DWORD Protocol, UCHAR Flags,
                                       UCHAR PTS1, UCHAR PTS2, UCHAR PTS3)
{
    Log9(PCSC_LOG_DEBUG,
         "Lun=0x%04lX, Protocol=%lu, Flags=%u, PTS1=%u, PTS2=%u, PTS3=%u.%c%c",
         Lun, Protocol, Flags, PTS1, PTS2, PTS3, '\0', '\0');

    uint16_t reader_num;
    uint16_t slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    /* Only protocol selection is supported, PTS negotiation is not. */
    if (Flags != 0 || PTS1 != 0 || PTS2 != 0 || PTS3 != 0)
    {
        return IFD_NOT_SUPPORTED;
    }

    /* Check if ICC is present. */
    if (icc_present(slot_num))
    {
        switch (Protocol)
        {
        case SCARD_PROTOCOL_T0:
            return IFD_SUCCESS;
        case SCARD_PROTOCOL_T1:
            /* swICC does not support T=1. */
            return IFD_PROTOCOL_NOT_SUPPORTED;
        default:
            /* Unexpected. */
            return IFD_COMMUNICATION_ERROR;
        }
    }
    else
    {
        return IFD_COMMUNICATION_ERROR;
    }
}

RESPONSECODE IFDHPowerICC(DWORD Lun, DWORD Action, PUCHAR Atr, PDWORD AtrLength)
{
    Log5(PCSC_LOG_DEBUG, "Lun=0x%04lX, Action=%lu, Atr=%p, AtrLength=%p.", Lun,
         Action, Atr, AtrLength);

    uint16_t reader_num;
    uint16_t slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    /* Check if ICC is present. */
    if (!icc_present(slot_num))
    {
        return IFD_COMMUNICATION_ERROR;
    }

    switch (Action)
    {
    case IFD_RESET:
        /**
         * A warm reset is not a cold reset but functionally they
         * are the same.
         */
        if (icc_powerup(slot_num) != 0)
        {
            return IFD_ERROR_POWER_ACTION;
        }
        __attribute__((fallthrough));
    case IFD_POWER_UP:
        /**
         * If ICC is already powered-up, give back the ATR, otherwise power up
         * the ICC.
         */
        if (client_icc[slot_num].atr_len <= 0)
        {
            if (icc_powerup(slot_num) != 0)
            {
                return IFD_ERROR_POWER_ACTION;
            }
        }

        /* Check if the AtrBuffer can contain the ICC ATR. */
        if (*AtrLength < client_icc[slot_num].atr_len)
        {
            Log1(PCSC_LOG_ERROR,
                 "Supplied ATR buffer is too small to contain ICC ATR.");
            return IFD_COMMUNICATION_ERROR;
        }

        *AtrLength = client_icc[slot_num].atr_len;
        memcpy(Atr, client_icc[slot_num].atr, client_icc[slot_num].atr_len);
        return IFD_SUCCESS;
    case IFD_POWER_DOWN:
        /* No need to do power management so do nothing on power-down. */
        return IFD_SUCCESS;
    default:
        break;
    }
    return IFD_ERROR_NOT_SUPPORTED;
}

RESPONSECODE IFDHTransmitToICC(DWORD Lun, SCARD_IO_HEADER SendPci,
                               PUCHAR TxBuffer, DWORD TxLength, PUCHAR RxBuffer,
                               PDWORD RxLength, PSCARD_IO_HEADER RecvPci)
{
    Log9(PCSC_LOG_DEBUG,
         "Lun=0x%04lX, SendPci=%p, TxBuffer=%p, TxLength=%lu, RxBuffer=%p, "
         "RxLength=%p, RecvPci=%p%c.",
         Lun, &SendPci, TxBuffer, TxLength, RxBuffer, RxLength, RecvPci, '\0');

    uint64_t const rx_buf_len = *RxLength;
    /* Driver shall set RxLength to 0 on error. We do this ahead of time. */
    *RxLength = 0;

    uint16_t reader_num;
    uint16_t slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    /**
     * @warning SendPci is not used (stated in PC/SC-lite docs).
     */

    /* Check if ICC is present. */
    if (icc_present(slot_num))
    {
        /* APDU must contain a header. */
        if (TxLength < 5U)
        {
            Log1(PCSC_LOG_ERROR, "APDU is missing a header.");
            return IFD_COMMUNICATION_ERROR;
        }
        else if (TxLength > 5U)
        {
            /**
             * This makes sure that any extra data in the TxBuffer is ignored
             * and only the APDU is transmitted.
             */
            Log2(PCSC_LOG_DEBUG, "APDU data length is %uB.", TxBuffer[4U]);
            TxLength = 5U + TxBuffer[4U]; /* 5 + Lc = header_len + data_len. */
        }

        uint8_t const apdu_ins = TxBuffer[1U];
        uint8_t const apdu_ins_xor_ff = apdu_ins ^ 0xFF;
        uint64_t len_rem = TxLength;

        /**
         * Iterate as many times as are needed to transfer all data from the
         * buffer and until ICC needs more data than can be provided.
         */
        while (len_rem > 0 || client_icc[slot_num].buf_len_exp == 0)
        {
            /* How much data was requested by the ICC. */
            uint32_t const icc_buf_len_exp = client_icc[slot_num].buf_len_exp;

            Log3(PCSC_LOG_DEBUG, "ICC expects %uB. %luB remaining in TxBuffer.",
                 icc_buf_len_exp, len_rem);
            if (len_rem < icc_buf_len_exp)
            {
                Log3(PCSC_LOG_ERROR,
                     "ICC expects %uB, have only %luB to transmit.",
                     icc_buf_len_exp, len_rem);
                return IFD_COMMUNICATION_ERROR;
            }

            memset(&msg_tx, 0U, sizeof(msg_tx));
            msg_tx.data.cont_state = client_icc[slot_num].cont_iface;
            memcpy(msg_tx.data.buf, &TxBuffer[TxLength - len_rem],
                   icc_buf_len_exp);
            msg_tx.hdr.size =
                offsetof(swicc_net_msg_data_st, buf) + icc_buf_len_exp;
            if (client_msg_io(slot_num, true) != 0 ||
                msg_rx.data.ctrl != SWICC_NET_MSG_CTRL_SUCCESS)
            {
                return IFD_COMMUNICATION_ERROR;
            }

            /**
             * After I/O, if the remaining length is 0, it means the command was
             * sent and ICC should have responsed with a response TPDU.
             */
            len_rem -= icc_buf_len_exp;
            Log2(PCSC_LOG_DEBUG, "TxBuffer contains %luB after transmission.",
                 len_rem);

            /* Safe cast since the swICC net functions validated the message. */
            uint32_t const msg_rx_buf_len =
                (uint32_t)(msg_rx.hdr.size -
                           offsetof(swicc_net_msg_data_st, buf));
            Log2(PCSC_LOG_DEBUG, "Received %uB from ICC.", msg_rx_buf_len);

            /**
             * While transmitting the APDU, shall not receive any data in
             * responses, procedure bytes are ok.
             */
            if (msg_rx_buf_len == 1U)
            {
                /* Got a procedure byte. */
                uint8_t const procedure = msg_rx.data.buf[0U];
                if (procedure == 0x60) /* NACK */
                {
                    /* Stop processing command here. */
                    /**
                     * @todo What should the response APDU be for NACK?
                     */
                    break;
                }
                else if (procedure == apdu_ins ||
                         procedure == apdu_ins_xor_ff) /* ACK */
                {
                    /* Continue sending data. */
                }
                else
                {
                    Log2(PCSC_LOG_ERROR,
                         "Received an invalid procedure: 0x%02X.", procedure);
                    return IFD_COMMUNICATION_ERROR;
                }
            }
            else if (msg_rx_buf_len == 2U)
            {
                /**
                 * Got a status before transmitting the whole message. This is
                 * our response to the APDU.
                 */
                break;
            }
            else if (msg_rx_buf_len == 0U)
            {
                /**
                 * 0 means the ICC is most likely changing state, this is okay.
                 */
                /* Continue sending data. */
            }
            else
            {
                /**
                 * This would mean we got a response because there is no more
                 * data to send.
                 */
                if (len_rem == 0)
                {
                    break;
                }

                /* Didn't send all the data but got more data than expected. */
                Log1(PCSC_LOG_ERROR,
                     "Received too much or too little data from ICC.");
                return IFD_COMMUNICATION_ERROR;
            }
        }

        /* Safe cast since the swICC net functions validated the message. */
        uint32_t const tpdu_len =
            (uint32_t)(msg_rx.hdr.size - offsetof(swicc_net_msg_data_st, buf));
        Log2(PCSC_LOG_DEBUG, "TPDU response length is %u.", tpdu_len);

        /* Expecting at least a status word or a TPDU header. */
        if (tpdu_len >= 2U)
        {
            /* RxBuffer is too small to contain the APDU response. */
            if (rx_buf_len < tpdu_len)
            {
                return IFD_COMMUNICATION_ERROR;
            }

            /**
             * Write the response TPDU to the RX buffer (and set length
             * accordingly).
             */
            memcpy(RxBuffer, msg_rx.data.buf, tpdu_len);
            *RxLength = tpdu_len;

            /**
             * @warning RecvPci is not used (stated in PC/SC-lite docs).
             */

            return IFD_SUCCESS;
        }
        else
        {
            Log2(PCSC_LOG_ERROR,
                 "ICC sent an invalid TPDU: tpdu_len=%u, expected =2 or >=5.",
                 tpdu_len);
            return IFD_COMMUNICATION_ERROR;
        }
    }
    else
    {
        return IFD_ICC_NOT_PRESENT;
    }
}

RESPONSECODE IFDHICCPresence(DWORD Lun)
{
    Log2(PCSC_LOG_DEBUG, "Lun=0x%04lX.", Lun);

    uint16_t reader_num;
    uint16_t slot_num;
    if (lun_parse(Lun, &reader_num, &slot_num) != 0)
    {
        return IFD_COMMUNICATION_ERROR;
    }

    /* Check if ICC is already thought to be present. */
    if (reader_present() && icc_present(slot_num))
    {
        /* Send a keep-alive message to ICC to see if it's still connected. */
        memset(&msg_tx, 0U, sizeof(msg_tx));
        msg_tx.data.cont_state = client_icc[slot_num].cont_iface;
        msg_tx.data.ctrl = SWICC_NET_MSG_CTRL_KEEPALIVE;
        msg_tx.hdr.size = offsetof(swicc_net_msg_data_st, buf);

        if (client_msg_io(slot_num, false) == 0 &&
            msg_rx.data.ctrl == SWICC_NET_MSG_CTRL_SUCCESS)
        {
            return IFD_ICC_PRESENT;
        }

        /* Sending or receiving errors are treated as a missing ICC. */
        Log1(PCSC_LOG_INFO, "Client keep-alive failed. Disconnecting it.");
        client_disconnect(slot_num);
        return IFD_ICC_NOT_PRESENT;
    }
    else
    {
        if (reader_present())
        {
            /* Safe cast since parsing Lun rejects invalid slots. */
            if (swicc_net_server_client_connect(&server_ctx,
                                                (uint16_t)slot_num) == 0)
            {
                return IFD_ICC_PRESENT;
            }
        }
        return IFD_ICC_NOT_PRESENT;
    }
}
