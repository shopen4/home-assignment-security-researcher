// cc server.c -o server
#include <bootstrap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mach/mach.h>
#include <stdio.h>
#include <stdlib.h>

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

mach_msg_return_t receive_msg(mach_port_name_t, ReceiveMessage *inMessage);
mach_msg_return_t send_reply(mach_port_name_t port, const message *inMessage);

int main(void)
{
	int kr;
	// The mach_task_self system call returns the calling thread's task port.

	mach_port_t task = mach_task_self();

	// The mach_port_allocate parameters are task, right and name; task parameter is the task acquiring the port right
	// right parameter is the kind of entity to be created. name parameter is the task's name for the port right.
	mach_port_name_t recvPort;
	kr = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &recvPort);

	if (kr != KERN_SUCCESS)
		return EXIT_FAILURE;

	printf("[*] Created port with MACH_PORT_RIGHT_RECEIVE: 0x%x\n", recvPort);

	// The mach_port_insert_right function inserts into task the caller's right for a port, using a specified name for the right in the target task
	// Adding send right to the port
	kr = mach_port_insert_right(task, recvPort, recvPort, MACH_MSG_TYPE_MAKE_SEND);

	if (kr != KERN_SUCCESS)
		return EXIT_FAILURE;

	printf("[*] Added send right to the port\n");

	// This server is accessible to all processes on the system, which may communicate with it over a given port — the bootstrap_port
	mach_port_t bootstrap_port;
	kr = task_get_special_port(task, TASK_BOOTSTRAP_PORT, &bootstrap_port);

	printf("[*] Got bootstrap port: 0x%x\n", bootstrap_port);

	if (kr != KERN_SUCCESS)
		return EXIT_FAILURE;
	// the server is visible by other processes
	// kr = bootstrap_check_in(bootstrap_port, "com.echo.macherino.as-a-service", &recvPort);
	kr = bootstrap_check_in(bootstrap_port, "com.nir.ipc.mach", &recvPort);

	if (kr != KERN_SUCCESS)
		return EXIT_FAILURE;

	printf("[*] Registered our service\n");

	while (true)
	{

		ReceiveMessage receiveMessage = {0};
		mach_msg_return_t ret = receive_msg(recvPort, &receiveMessage);
		if (ret != MACH_MSG_SUCCESS)
		{
			if (ret == MACH_RCV_TOO_LARGE)
			{
				printf("Failed to receive a message because the message size is more than 1024 bytes:  %#x\n", ret);
				continue;
			}
			printf("Failed to receive a message: %#x\n", ret);
			continue;
		}

		// Continue if there's no reply port.
		if (receiveMessage.message.header.msgh_remote_port == MACH_PORT_NULL)
		{
			continue;
		}

		ret = send_reply(
			receiveMessage.message.header.msgh_remote_port,
			&receiveMessage.message);

		if (ret != MACH_MSG_SUCCESS)
		{
			printf("Failed to respond: %#x\n", ret);
		}
	}
}

mach_msg_return_t send_reply(mach_port_name_t port, const message *inMessage)
{

	message response = {0};
	response.header.msgh_bits =
		inMessage->header.msgh_bits &
		MACH_MSGH_BITS_REMOTE_MASK;

	response.header.msgh_remote_port = port;
	response.header.msgh_id = 1;
	response.header.msgh_size = sizeof(response);

	strcpy(response.body_str, "aa");

	mach_msg_return_t ret = mach_msg(
		/* msg */ (mach_msg_header_t *)&response,
		/* option */ MACH_SEND_MSG,
		/* send size */ sizeof(response),
		/* recv size */ 0,
		/* recv_name */ MACH_PORT_NULL,
		/* timeout */ MACH_MSG_TIMEOUT_NONE,
		/* notify port */ MACH_PORT_NULL);

	return ret;
}

mach_msg_return_t receive_msg(mach_port_name_t recvPort, ReceiveMessage *inMessage)
{

	mach_msg_return_t ret = mach_msg(
		/* msg */ (mach_msg_header_t *)inMessage,
		/* option */ MACH_RCV_MSG,
		/* send size */ 0,
		/* recv size */ sizeof(*inMessage),
		/* recv_name */ recvPort,
		/* timeout */ MACH_MSG_TIMEOUT_NONE,
		/* notify port */ MACH_PORT_NULL);

	if (ret != MACH_MSG_SUCCESS)
		return ret;

	FILE *fp;

	fp = fopen("storedData.txt", "a+");
	if (fp == NULL)
	{
		printf("cannot create a file\n");
	}
	else
	{
		fputs(inMessage->message.body_str, fp);
		fputs("\n", fp);
	}

	fclose(fp);

	printf("got message!\n");
	printf("\t id: %d\n", inMessage->message.header.msgh_id);
	printf("\t bodys: %s\n", inMessage->message.body_str);

	return MACH_MSG_SUCCESS;
}
