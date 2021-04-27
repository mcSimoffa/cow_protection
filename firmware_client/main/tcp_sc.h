
#ifndef _TCP_SC_H_
#define _TCP_SC_H_

typedef struct
{
	uint8_t		*data;
	uint32_t	size;
} tcp_packet_t;

typedef void (*rec_cb_t)(void *data, uint32_t size);

void tcp_client_task(void *pvParameters);

esp_err_t msg_send_socket(void *data, uint32_t len);

void tcp_receiver_cb_register(void *_rec_cb);

#endif //_TCP_SC_H_
