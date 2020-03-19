

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




int mosquitto_publish_v5(struct mosquitto *mosq, int *mid, const char *topic, int payloadlen, const void *payload, int qos, ...)
{

	1. 生成mid，递增

	2.1 检查qos类型：
	    if qos == 0 then
			send__publish(mosq, local_mid, topic, payloadlen, payload, qos, retain, false, outgoing_properties, NULL, 0);
			send__publish实际调用send__real_publish，
			int send__publish(.topic,payload..)
			{
				send__real_publish(..mid,topic,payload.)
			}
			send__real_publish（）
			{
				1. 申请 packet = mosquitto__packet结构，将负载拷贝到 packet 新申请的内存， 调用 **packet__queue(mosq, packet)**，
					packet__queue 将packet放入mosq->out_packet队列（锁保护），
									pthread_mutex_lock(&mosq->out_packet_mutex);
									if(mosq->out_packet){
										mosq->out_packet_last->next = packet;
									}else{
										mosq->out_packet = packet;  //头指针
									}
									mosq->out_packet_last = packet; //尾指针
									pthread_mutex_unlock(&mosq->out_packet_mutex);
					通知主线程，有packet需要发送。
					如果当前处于主线程，调用 packet__write 检查mosq->out_packet队列，实际发送数据net__write(mosq, &(packet->payload[packet->pos]), packet->to_process)。
			}

	2.2 else qos为1或2
			申请 message = mosquitto_message_all 结构，拷贝负载到message中, 将message放入msgs_out.inflight队列（锁保护）
			pthread_mutex_lock(&mosq->msgs_out.mutex);
			message__queue(mosq, message, mosq_md_out);
			pthread_mutex_unlock(&mosq->msgs_out.mutex);

			message__queue() 将message放入msgs_out.inflight队列
							```c
								DL_APPEND(mosq->msgs_out.inflight, message)，
							```
						调用message__release_to_inflight(), 循环遍历队列中的message，并调用send__publish发送message内容。
							```c
							DL_FOREACH_SAFE(mosq->msgs_out.inflight, cur, tmp)
								rc = send__publish(mosq, cur->msg.mid, cur->msg.topic, cur->msg.payloadlen,
												cur->msg.payload, cur->msg.qos, cur->msg.retain,
												cur->dup, cur->properties, NULL, 0);
							```

}

msgs_out.inflight 队列中的message在收到puback/rec_complete回应后删除

