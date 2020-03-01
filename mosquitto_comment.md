

pub_client.c

```c
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

        // 连接成功回调，publish message
        // mosquitto_publish 将消息加入列表，唤醒main_loop线程
		mosquitto_publish(mosq, NULL, "pub/qos1/test", strlen("message"), "message", 1, false);
	}
}

void on_publish(struct mosquitto *mosq, void *obj, int mid)
{
	printf("on_publish get called. mid=%d\n", mid);

	printf("disconnect from server.\n");

    //发布消息回调， 发送mqtt断开连接消息, 连接断开完成后，触发on_disconnect回调
	mosquitto_disconnect(mosq);
}

void on_disconnect(struct mosquitto *mosq, void *obj, int rc)
{
    // 断开连接回调
    // 连接异常断开，也会调用回调
	printf("on_disconnect get called, rc=%d. stop main loop.\n", rc);

    //停止main函数的while循环
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

    /* v5 版本回调，提供更多参数
    mosquitto_connect_v5_callback_set(mosq, my_connect_callback);
	mosquitto_disconnect_v5_callback_set(mosq, my_disconnect_callback);
	mosquitto_publish_v5_callback_set(mosq, my_publish_callback);
    */

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

```


