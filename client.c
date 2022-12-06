// cc client.c -o client
#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>
#include <bootstrap.h>
#include <string.h>
#include <unistd.h>

#define MS_IN_S 1000

mach_msg_return_t receive_msg(mach_port_name_t, mach_msg_timeout_t, char *data);

typedef struct
{
	mach_msg_header_t header;
	char body_str[1024];
} message;

typedef struct
{
	message message;
	mach_msg_trailer_t trailer;
} ReceiveMessage;

int main(const int argc, char **argv)
{
	int kr;
	mach_port_name_t task = mach_task_self();

	// Create bootstrap port
	mach_port_t bootstrap_port;
	kr = task_get_special_port(task, TASK_BOOTSTRAP_PORT, &bootstrap_port);

	if (kr != KERN_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	printf("[*] Got special bootstrap port: 0x%x\n", bootstrap_port);

	// Get port to send to the com.nir.ipc.mach and store it in port
	// Any clients wishing to connect to a given service, can then look up the server port using a similar function: bootstrap_look_up
	// first argument: always bootstrap; second argument: name of service; third argument: out: server port
	mach_port_t port;
	kr = bootstrap_look_up(bootstrap_port, "com.nir.ipc.mach", &port);

	if (kr != KERN_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	printf("[*] Port for com.nir.ipc.mach: 0x%x\n", port);

	mach_port_t replyPort;
	if (mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &replyPort) !=
		KERN_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	if (mach_port_insert_right(
			task, replyPort, replyPort, MACH_MSG_TYPE_MAKE_SEND) !=
		KERN_SUCCESS)
	{
		return EXIT_FAILURE;
	}

	// Create the message for sending
	message msg = {0};
	msg.header.msgh_remote_port = port;
	msg.header.msgh_local_port = replyPort;

	// MACH_MSG_TYPE_COPY_SEND: The message will carry a send right, and the caller should supply a send right.
	msg.header.msgh_bits = MACH_MSGH_BITS_SET(
		/* remote */ MACH_MSG_TYPE_COPY_SEND,
		/* local */ MACH_MSG_TYPE_MAKE_SEND_ONCE,
		/* voucher */ 0,
		/* other */ 0);
	msg.header.msgh_id = 4;
	msg.header.msgh_size = sizeof(msg);

	strcpy(msg.body_str, "test");

	// Mach messages are sent and received with the same API function, mach_msg()
	mach_msg_return_t ret = mach_msg(
		/* msg */ (mach_msg_header_t *)&msg,
		/* option */ MACH_SEND_MSG,
		/* send size */ sizeof(msg),
		/* recv size */ 0,
		/* recv_name */ MACH_PORT_NULL,
		/* timeout */ MACH_MSG_TIMEOUT_NONE,
		/* notify port */ MACH_PORT_NULL);

	while (ret == MACH_MSG_SUCCESS)
	{
		ret = receive_msg(replyPort, /* timeout */ 1 * MS_IN_S, msg.body_str);
	}

	if (ret == MACH_RCV_TIMED_OUT)
	{
		printf("Receive timed out, no more messages from server.\n");
	}
	else if (ret != MACH_MSG_SUCCESS)
	{
		printf("Failed to receive a message: %#x\n", ret);
		return 1;
	}

	return ret;
}

mach_msg_return_t receive_msg(mach_port_name_t recvPort, mach_msg_timeout_t timeout, char *data_sent_to_server)
{
	// Message buffer.
	ReceiveMessage receiveMessage = {0};

	mach_msg_return_t ret = mach_msg(
		/* msg */ (mach_msg_header_t *)&receiveMessage,
		/* option */ MACH_RCV_MSG | MACH_RCV_TIMEOUT,
		/* send size */ 0,
		/* recv size */ sizeof(receiveMessage),
		/* recv_name */ recvPort,
		/* timeout */ timeout,
		/* notify port */ MACH_PORT_NULL);

	if (ret != MACH_MSG_SUCCESS)
	{
		return ret;
	}

	printf("got response message!\n");
	printf("\t id: %d\n", receiveMessage.message.header.msgh_id);
	printf("\t bodys: %s\n", receiveMessage.message.body_str);


	if (strcmp(receiveMessage.message.body_str, data_sent_to_server) == 0) {
		printf("\t The data is the same\n");
	} else {
		printf("\t The data is NOT the same\n");
	}

	return ret;
}
