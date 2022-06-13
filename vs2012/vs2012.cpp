// vs2012.cpp : 此文件包含 "main" 函数。程序执行将在此处开始并结束。
//

#include <iostream>
#include "stdio.h"
#include "uJSON.h"

#define JSON_BUFFER_BYTES 512

char json_buffer[JSON_BUFFER_BYTES];

uJsonError on_json(const char* s)
{
    char buf[64];
    int i, numbers, val;
    uJsonError err;

    err = uJson_parse(s);
    if (err != UJSON_OK) {
        return err;
    }

    if (uJson_get_string(buf, 64, "data", "ip", -1) == UJSON_OK) {
        printf("ip:%s\n", buf);
    }

    if (uJson_get_items(&numbers, "data", "nodeInfos", -1) == UJSON_OK) {
        for (i = 0; i < numbers; i++) {
            if (uJson_get_integer(&val, "data", "nodeInfos", i, "id", -1) == UJSON_OK) {
                //
                printf("[%d]=%d\n", i, val);
            }
        }
    }
}

static char* create_command(char* host, char* interface, const char* method)
{
    char url[128];

    sprintf(url, "http://%s/%s", host, interface);

    uJson_create_root_object(json_buffer, JSON_BUFFER_BYTES);

    uJson_add_item(UJSON_STRING, "url", url, -1);
    uJson_add_item(UJSON_STRING, "method", (char*)method, -1);


    return json_buffer;
}

char* build_json()
{
    uJson_create_root_array(json_buffer, JSON_BUFFER_BYTES); // the content of buf is "[]" after this step
    uJson_add_item(UJSON_PRIMITIVE, NULL, "1",  -1); //  the buf changes to "[1]"
    uJson_add_item(UJSON_PRIMITIVE, NULL, "{}", -1); // the buf changes to "[1, {}]"
    uJson_add_item(UJSON_STRING, "key", "val", 1, -1); // the buf changes to "[1, {\"key\":\"val\"}]" finally

    return json_buffer;
}

int main()
{
    on_json("{\"data\":{\"ip\": \"36.152.44.96\", \"nodeInfos\":[{\"id\":10},{\"id\":12},{\"id\":17}]}}");

    printf("%s\n", create_command("fine.com", "api", "get"));

    printf("%s\n", build_json());

    std::cout << "Hello World!\n";
}

// 运行程序: Ctrl + F5 或调试 >“开始执行(不调试)”菜单
// 调试程序: F5 或调试 >“开始调试”菜单

// 入门使用技巧: 
//   1. 使用解决方案资源管理器窗口添加/管理文件
//   2. 使用团队资源管理器窗口连接到源代码管理
//   3. 使用输出窗口查看生成输出和其他消息
//   4. 使用错误列表窗口查看错误
//   5. 转到“项目”>“添加新项”以创建新的代码文件，或转到“项目”>“添加现有项”以将现有代码文件添加到项目
//   6. 将来，若要再次打开此项目，请转到“文件”>“打开”>“项目”并选择 .sln 文件
