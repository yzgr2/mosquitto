#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <mosquitto.h>

static int run = -1;

void on_connect(struct mosquitto *mosq, void *obj, int rc)
{
	if(rc){
		exit(1);
	}else{
		printf("on_connect get called, begin publish test message\n");

		mosquitto_publish(mosq, NULL, "pub/qos1/test", strlen("message"), "message", 1, false);
	}
}

void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
	printf("on_publish get called. mid=%d\n", mid);

	printf("disconnect from server.\n");

	mosquitto_disconnect(mosq);
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
	printf("on_disconnect get called, rc=%d. stop main loop.\n", rc);

	run = 0;
}

int main(int argc, char *argv[])
{
	int rc;
	struct mosquitto *mosq;

	int port = 1883;	
	if( argc > 1)
	{
		port = atoi(argv[1]);
	}

	mosquitto_lib_init();

	//1. 申请并初始化结构
	mosq = mosquitto_new("publish-qos1-test", true, NULL);
	mosquitto_int_option(mosq, MOSQ_OPT_PROTOCOL_VERSION, MQTT_PROTOCOL_V5);

	//2.设置回调函数
	mosquitto_connect_callback_set(mosq, on_connect);
	mosquitto_disconnect_callback_set(mosq, on_disconnect);
	mosquitto_publish_callback_set(mosq, on_publish);
	mosquitto_message_retry_set(mosq, 3);

	//3.连接服务器 -阻塞方式
	rc = mosquitto_connect(mosq, "localhost", port, 60);
	printf("connected result : rc=%d\n", rc);

	while(run == -1)
	{
		//4. 接受并处理一个报文, 回调函数在内部被调用
		mosquitto_loop(mosq, 300, 1);
	}

	//释放连接
	mosquitto_destroy(mosq);

	//5. 以上while循环退出，清理库
	mosquitto_lib_cleanup();
	return run;
}
