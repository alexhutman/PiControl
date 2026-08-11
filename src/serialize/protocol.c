#include "serialize/protocol.h"

#include "model/protocol.h"

#include <stdlib.h>
#include <string.h>

int initialize_deserializer(PiCtrlMsgDeserializer *des) {
    if (!des) return -1;
    des->in.rx_buffer = calloc(1, MAX_PICTRL_MSG_SIZE);
    des->in.rx_buffered_bytes = 0;
    if (!des->in.rx_buffer) {
        // TODO: Do something with these logs
        //lwsl_err("Could not allocate rx_buffer\n");
        return -2;
    }

    const size_t num_initial_bytes = 1;
    des->out.msg.payload = calloc(num_initial_bytes, sizeof(*(des->out.msg.payload)));
    if (!des->out.msg.payload) {
        //lwsl_err("Could not allocate RawPiCtrlMessage payload\n");
        return -3;
    }
    return 0;
}

int destroy_deserializer(PiCtrlMsgDeserializer *des) {
    if (!des) return -1;
    if (des->in.rx_buffer) {
        memset(des->in.rx_buffer, 0, MAX_PICTRL_MSG_SIZE);
        free(des->in.rx_buffer);
        des->in.rx_buffer = NULL;
    }
    if (des->out.msg.payload) {
        memset(des->out.msg.payload, 0, des->out.payload_buf_size);
        free(des->out.msg.payload);
        des->out.msg.payload = NULL;
    }
    return 0;
}

int deserialize_network_data(PiCtrlMsgDeserializer *des) {
    if (!des->in.rx_buffer || !des->out.msg.payload) return -1;

    const size_t payload_size = (size_t)(des->in.rx_buffer[1]);
    const size_t expected_wire_size = sizeof(des->out.msg.header.cmd) + sizeof(des->out.msg.header.payload_size)
                                    + payload_size;
    if (des->in.rx_buffered_bytes != expected_wire_size) {
        //lwsl_warn("Transmission struct size mismatch. Got %zu bytes, expected %zu\n",
                  //des->in.rx_buffered_bytes, expected_wire_size);
        return -2;
    }
    if (payload_size > MAX_PAYLOAD_SIZE) {
        //lwsl_debug("This shouldn't be possible");
        return -3;
    }

    size_t offset = 0;

    uint8_t cmd;
    memcpy(&cmd, &des->in.rx_buffer[offset], sizeof(cmd));
    // Handle endianness here if struct winds up containing multi-byte members
    // (e.g. out->some_uint32_t_member = ntohl(some_uint32_t_member);
    des->out.msg.header.cmd = cmd;
    offset += sizeof(des->out.msg.header.cmd);

    des->out.msg.header.payload_size = payload_size;
    offset += sizeof(des->out.msg.header.payload_size);

    if (payload_size > des->out.payload_buf_size) {
        uint8_t *new_region = realloc(des->out.msg.payload, payload_size);
        if (!new_region) {
            // TODO: Free payload?
            //lwsl_err("Couldn't realloc payload: %s\n", strerror(errno));
            return -4;
        }
        des->out.msg.payload = new_region;
        des->out.payload_buf_size = payload_size;
    }
    memcpy(des->out.msg.payload, &des->in.rx_buffer[offset], payload_size);

    return 0;
}
