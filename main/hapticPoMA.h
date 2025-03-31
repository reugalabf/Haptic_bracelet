int GlobalVar = 0;
Topic *topicHead;
Topic *topicTail;


void setterGlobalVar(int sockfd, char *argument)
{
    //ESP_LOGI(TAG, "setterGlobalVar: %s", argument);
    if (argument != NULL)
        GlobalVar = atoi(argument);
    write(sockfd, "done\n", 6);
}

void getterGlobalVar(int sockfd, char *argument)
{
    char response[10];

    //ESP_LOGI(TAG, "GetterGlobalVar");
    sprintf(response, "%d\n", GlobalVar);
    write(sockfd, response, strlen(response));
}

static void do_processQuery(const int sock)
{
    int len;
    char rx_buffer[128];

    do
    {
        len = recv(sock, rx_buffer, sizeof(rx_buffer) - 1, 0);
        if (len < 0)
        {
            ESP_LOGE(TAG, "Error occurred during receiving: errno %d", errno);
        }
        else if (len == 1)
        {
            ESP_LOGW(TAG, "Connection closed");
        }
        else
        {
            rx_buffer[len] = 0; // Null-terminate whatever is received and treat it like a string
            ESP_LOGI(TAG, "Received %d bytes: %s", len, rx_buffer);

            processMessage(sock, rx_buffer, topicHead);
        }
    } while (len > 0);
}