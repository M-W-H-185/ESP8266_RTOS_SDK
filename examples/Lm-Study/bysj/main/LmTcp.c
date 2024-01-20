#include "LmTcp.h"
#include "driver/gpio.h"

static const char *TAG_TCP = "tcp->";

#define GPIO_OUTPUT_PIN_SEL  (1ULL<<5) // IO5

#define LM_CLOUD_CONNECT_ADDRESS  "192.168.0.149"
#define LM_CLOUD_CONNECT_PORT  8859

#define LM_CLOUD_CONNECT_SN "sdg345ghdgh345gweqer23"
#define LM_CLOUD_CONNECT_SECRET_KEY "gwb24tg3qwrg234f134fg"
static bool is_cloud_auth_success = false;
// sock服务句柄 初始化为-1 -1代表没连接
static int lm_sock_handle = -1;
// sock 读取缓冲区 
static char rx_buffer[500];

// tcp 发送函数
void lm_tcp_send(char* sdata){
    if(lm_sock_handle != -1){
        int err = send(lm_sock_handle, sdata, strlen(sdata), 0);
        free(sdata);
        if (err < 0) {
            ESP_LOGE(TAG_TCP, "Error occured during sending: errno %d", errno);
        }
    }
}
// 这个必须是要云平台鉴权成功才能发送tcp信息
void lm_cloud_tcp_send(char* sdata){
    if(is_cloud_auth_success == true){
        lm_tcp_send(sdata);
        free(sdata);
    }
    else{
        ESP_LOGE(TAG_TCP,"设备未鉴权");
    }
}
// 解析函数
void lm_tcp_data_parse(){
    static struct cloud_request cr = {-1,-1,"","",-1};
    cr.t = -1;
    cr.status = -1;
    cr.cmdid = "";
    cr.apitag = "";
    cr.data = -1;
    request_data_json_parse(rx_buffer,&cr);
    ESP_LOGE(TAG_TCP,"t:%d status:%d cmdId:%s apitag:%s data:%d",
        cr.t,cr.status,cr.cmdid,cr.apitag,cr.data
    );
    switch(cr.t){
        // 连接响应
        case 2:
            if(cr.status == 0){
                is_cloud_auth_success = true;
                ESP_LOGE(TAG_TCP,"设备授权成功！");
            }
            break;
        // 上报数据确认
        case 4:
          
            break;
        // 命令请求
        case 5:
            if(cr.data == 1){
                // 执行 开关
                ESP_LOGE(TAG_TCP,"云平台命令请求111111111");
                gpio_set_level(GPIO_NUM_5, 0);
            }
            else if (cr.data == 0){
                ESP_LOGE(TAG_TCP,"云平台命令请求000000000000");
                gpio_set_level(GPIO_NUM_5, 1);
            }
            // 到时候把那个命令回调放上去咯
            ESP_LOGE(TAG_TCP,"云平台命令请求");

            break;
    }
}


//域名转ip
//host 域名字符串
//hostlen 长度
static char* resolve_host_name_turn_ip(const char *host, size_t hostlen)
{
    struct addrinfo hints;
    memset(&hints, 0, sizeof(hints));
    hints.ai_family = AF_UNSPEC;
    hints.ai_socktype = SOCK_STREAM;
 
    char *use_host = strndup(host, hostlen);
    if (!use_host)
    {
        return NULL;
    }
 
    // ESP_LOGD(TAG_TCP, "host:%s: strlen %lu", use_host, (unsigned long)hostlen);
    struct addrinfo *res;
    if (getaddrinfo(use_host, NULL, &hints, &res))
    {
        // ESP_LOGE(TAG_TCP, "couldn't get hostname for :%s:", use_host);
        free(use_host);
        return NULL;
    }
    free(use_host);
    // ----
    struct addrinfo *cur;
    static char ip[128];

    if (!res)
    {
        //DNS解析失败
        ESP_LOGI(TAG_TCP, "DNS resolve host name failed!");
        return NULL;
    }
    //解析ip
    for (cur = res; cur != NULL; cur = cur->ai_next)
    {
        inet_ntop(AF_INET, &(((struct sockaddr_in *)cur->ai_addr)->sin_addr), ip, sizeof(ip));
        // ESP_LOGI(TAG_TCP, "DNS IP:[%s]", ip);
    }
    free(cur);
    free(res);
    return ip;
}


static void tcp_client_task(void *pvParameters){
    char addr_str[128];
    int addr_family;
    int ip_protocol;
    while (1) {
        // 域名转ip ip还是ip
        char* ip = resolve_host_name_turn_ip(LM_CLOUD_CONNECT_ADDRESS,strlen(LM_CLOUD_CONNECT_ADDRESS));
        ESP_LOGI(TAG_TCP, "Server IP: %s ", ip);
        if(ip == NULL){
            break;
        }
        struct sockaddr_in destAddr;
        destAddr.sin_addr.s_addr = inet_addr(ip);
        destAddr.sin_family = AF_INET;
        destAddr.sin_port = htons(LM_CLOUD_CONNECT_PORT);
        addr_family = AF_INET;
        ip_protocol = IPPROTO_IP;
        inet_ntoa_r(destAddr.sin_addr, addr_str, sizeof(addr_str) - 1);

        // 初始化sock句柄
        lm_sock_handle = socket(addr_family, SOCK_STREAM, ip_protocol);
        if (lm_sock_handle < 0) {
            ESP_LOGE(TAG_TCP, "Unable to create socket: errno %d", errno);
            break;
        }
        ESP_LOGI(TAG_TCP, "Socket created");
        // 连接
        int err = connect(lm_sock_handle, (struct sockaddr *)&destAddr, sizeof(destAddr));
        if (err != 0) {
            ESP_LOGE(TAG_TCP, "Socket unable to connect: errno %d", errno);
            close(lm_sock_handle);
            continue;
        }
        ESP_LOGI(TAG_TCP, "Successfully connected");
        // 连接成功后 发送连接连接鉴权秘钥
        lm_tcp_send(
            get_connect_request_auth_param(LM_CLOUD_CONNECT_SN,LM_CLOUD_CONNECT_SECRET_KEY)
            );
        while (1) {
            // 读取
            int len = recv(lm_sock_handle, rx_buffer, sizeof(rx_buffer) - 1, 0);
            // Error occured during receiving
            if (len < 0) {
                ESP_LOGE(TAG_TCP, "recv failed: errno %d", errno);
                break;
            }
            // Data received
            else {
                rx_buffer[len] = 0; // Null-terminate whatever we received and treat like a string
                // ESP_LOGI(TAG_TCP, "Received %d bytes from %s:", len, addr_str);
                ESP_LOGI(TAG_TCP, "%s", rx_buffer);
                lm_tcp_data_parse();
            }
            vTaskDelay(500 / portTICK_PERIOD_MS);
        }
        // 异常 或者 服务器断开 连接后的事件
        if (lm_sock_handle != -1) {
            is_cloud_auth_success = false;
            ESP_LOGE(TAG_TCP, "Shutting down socket and restarting...");
            shutdown(lm_sock_handle, 0);
            close(lm_sock_handle);
        }
    }
    vTaskDelete(NULL);
}
#include "cJSON.h"
 
// 采集传感器、执行器并将数据发送到云平台的任务
static void gather_task(void *pvParameters){
    char* ret = "";
    int aa = 0;
    while(1){
        cJSON *cjson_body = cJSON_CreateObject();

        cJSON_AddNumberToObject(cjson_body, "t", 3);

        cJSON *cjson_data = cJSON_CreateObject();
        int data = gpio_get_level(GPIO_NUM_5);
        if(data == 0){
            cJSON_AddNumberToObject(cjson_data,"led",1);
        }
        else if(data == 1){
            cJSON_AddNumberToObject(cjson_data,"led",0);
        }
        // 存入led的状态

        // 将data添加到json主体里面
        cJSON_AddItemToObject(cjson_body, "data", cjson_data);

        ret = cJSON_Print(cjson_body);

        cJSON_Delete(cjson_body);
        cJSON_Delete(cjson_data);
        ESP_LOGI(TAG_TCP,"-------------->%s",ret);

        lm_cloud_tcp_send(ret);

        free(ret);
        vTaskDelay(1500 / portTICK_PERIOD_MS);
    }
    vTaskDelete(NULL);
}
void lm_tcp_init(void){
    tcpip_adapter_init();//初始化tpc堆栈
    xTaskCreate(tcp_client_task, "tcp_client", 4096, NULL, 5, NULL);

    // cscs 
    /* 1.定义一个gpio配置结构体 */
	gpio_config_t gpio_config_structure;

    /* 2.初始化gpio配置结构体*/
    gpio_config_structure.pin_bit_mask = GPIO_OUTPUT_PIN_SEL ;/* 选择IO5 */
    gpio_config_structure.mode = GPIO_MODE_OUTPUT;              /* 输出模式 */
    gpio_config_structure.pull_up_en = 0;                       /* 不上拉 */
    gpio_config_structure.pull_down_en = 0;                     /* 不下拉 */
    gpio_config_structure.intr_type = GPIO_INTR_DISABLE;        /* 禁止中断 */ 

    /* 3.根据设定参数初始化并使能 */  
	gpio_config(&gpio_config_structure);

    /* 4.输出高电平，继电器不吸合*/
    gpio_set_level(GPIO_NUM_5, 1);

    

    // cscs

    xTaskCreate(gather_task, "gather_task", 4096, NULL, 6, NULL);
}



