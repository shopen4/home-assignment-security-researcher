// cc server.c -o server
#include <bootstrap.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <mach/mach.h>

typedef struct {
	mach_msg_header_t header;
	char body_str[32];
	mach_msg_trailer_t trailer;
} message;

mach_msg_return_t receive_msg(mach_port_name_t);

int main(void) {
	int kr;
    // The mach_task_self system call returns the calling thread's task port.
    
	mach_port_t task = mach_task_self();
    
    // The mach_port_allocate parameters are task, right and name; task parameter is the task acquiring the port right
    // right parameter is the kind of entity to be created. name parameter is the task's name for the port right.
	mach_port_name_t recv_port;
	kr = mach_port_allocate(task, MACH_PORT_RIGHT_RECEIVE, &recv_port);

	if (kr != KERN_SUCCESS)
		return EXIT_FAILURE;

	printf("[*] Created port with MACH_PORT_RIGHT_RECEIVE: 0x%x\n", recv_port);

    // The mach_port_insert_right function inserts into task the caller's right for a port, using a specified name for the right in the target task
	// Adding send right to the port
	kr = mach_port_insert_right(task, recv_port, recv_port, MACH_MSG_TYPE_MAKE_SEND);

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
	kr = bootstrap_check_in(bootstrap_port, "com.echo.macherino.as-a-service", &recv_port);

	if (kr != KERN_SUCCESS)
		return EXIT_FAILURE;

	printf("[*] Registered our service\n");
		

	while(true) {
		mach_msg_return_t ret = receive_msg(recv_port);
	}
}

mach_msg_return_t receive_msg(mach_port_name_t recv_port) {
	message msg = {0};
	// Mach messages are sent and received with the same API function, mach_msg()
	mach_msg_return_t ret = mach_msg(
		(mach_msg_header_t *)&msg,
		MACH_RCV_MSG,
		0,
		1024 * BYTE_SIZE,
		recv_port,
		MACH_MSG_TIMEOUT_NONE,
		MACH_PORT_NULL);

	if (ret != MACH_MSG_SUCCESS)
		return ret;

	printf("got message\n");
	printf("\tid: %d\n", msg.header.msgh_id);
	printf("\tbodys: %s\n", msg.body_str);

	return MACH_MSG_SUCCESS;
}
	