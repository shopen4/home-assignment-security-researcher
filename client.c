// cc client.c -o client
#include <stdio.h>
#include <stdlib.h>
#include <mach/mach.h>
#include <bootstrap.h>
#include <string.h>
#include <unistd.h>

typedef struct {
	mach_msg_header_t header;
	char body_str[32];
} message;

int main(void) {
	int kr;
	mach_port_name_t task = mach_task_self();

	// Create bootstrap port
	mach_port_t bootstrap_port;
	kr = task_get_special_port(task, TASK_BOOTSTRAP_PORT, &bootstrap_port);

	if (kr != KERN_SUCCESS) {
		return -1;
	}

	printf("[*] Got special bootstrap port: 0x%x\n", bootstrap_port);

	// Get port to send to the com.echo.macherino.as-a-service and store it in port
	// Any clients wishing to connect to a given service, can then look up the server port using a similar function: bootstrap_look_up
	// first argument: always bootstrap; second argument: name of service; third argument: out: server port
	mach_port_t port;
	kr = bootstrap_look_up(bootstrap_port, "com.echo.macherino.as-a-service", &port);

	if (kr != KERN_SUCCESS) {
		return -2;
	}

	printf("[*] Port for com.echo.macherino.as-a-service: 0x%x\n", port);

	// Create the message for sending
	message msg = {0};
	msg.header.msgh_remote_port = port;

    // MACH_MSG_TYPE_COPY_SEND: The message will carry a send right, and the caller should supply a send right. 
	msg.header.msgh_bits = MACH_MSGH_BITS_SET(
		MACH_MSG_TYPE_COPY_SEND,
		0,
		0,
		0);
	msg.header.msgh_id = 4;
	msg.header.msgh_size = sizeof(msg);

	strcpy(msg.body_str, "test message");
	// Mach messages are sent and received with the same API function, mach_msg()
	mach_msg_return_t ret = mach_msg(
		(mach_msg_header_t *)&msg,
		MACH_SEND_MSG,
		sizeof(msg),
		0,
		MACH_PORT_NULL,
		MACH_MSG_TIMEOUT_NONE,
		MACH_PORT_NULL);
		
	return ret;
}