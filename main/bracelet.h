
#include "driver/ledc.h"

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE // or LEDC_HIGH_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_11_BIT // LEDC_TIMER_13_BIT // Adjust resolution as needed
#define LEDC_FREQUENCY (200)            // 200 Hz
#define LEDC_OUTPUT_IO (2)              //(GPIO_NUM_YOUR_MOTOR_PIN) // Replace with your motor pin

static const char *TAGb = "bracelet";

typedef struct Bracelet
{
    ledc_timer_config_t timer;
    ledc_channel_config_t *(*motor);
    uint32_t max_duty;
    unsigned int motor_count;
} Bracelet;

Bracelet bracelet;

uint32_t dutyCycleFromIntensity(Bracelet brclt, int intensity)
{
    return 0 == intensity ? 0 : bracelet.max_duty * intensity / 100;
}

int intensityFromDutyCycle(Bracelet brclt, uint32_t dutyCycle)
{
    ESP_LOGI(TAGb, "intensityFromDuty:%d \n", brclt.motor_count);
    if(dutyCycle ==0) ESP_LOGI(TAGb, "intensityFromDuty:0 \n");
    return (int)(0 == dutyCycle ? 0 : dutyCycle * 100 / bracelet.max_duty);
}
void getMotors (int sockfd, char *parameters){}

void setMotors (int sockfd, char *parameters){}

void setIntensity(int sockfd, char *parameters)
{
    // params is a list of comma separated integers
    char **params;
    int idx = 0;

    params = (char **)malloc(1 * sizeof(int *));
    char *token = strtok(parameters, ",");
    
    while (token != NULL)
    {    
        ESP_LOGI(TAGb, "setIntensity:%s", token);
        params[idx] = (char *)malloc(sizeof(char) * strlen(token));
        strcpy(params[idx++], token);
        token = strtok(NULL, ",");
    }
    // set intensity for each motor
    if (idx != bracelet.motor_count)
        write(sockfd, "Error: parameters do not match motor count\n", 6);
    else
    {
        for (int i = 0; i < idx; i++)
        {
            ESP_LOGI(TAGb, "setIntensity for motor:%d", i);
            bracelet.motor[i]->duty = dutyCycleFromIntensity(bracelet, atoi(params[i]));
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, bracelet.motor[i]->channel, bracelet.motor[i]->duty));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, bracelet.motor[i]->channel));
        }
        write(sockfd, "done\n", 6);
    }
    free(params);
}

void getIntensity(int sockfd, char *parameters)
{
    char datum[10];
    int i;
    ESP_LOGI(TAGb, "getIntensity");
    // params is a list of comma separated integers
    for (i = 0; i < bracelet.motor_count - 1; i++)
    {
        sprintf(datum, "%d,", (intensityFromDutyCycle(bracelet, bracelet.motor[i]->duty)));
        ESP_LOGI(TAGb, "getIntensity: datum %s", datum);
        write(sockfd, datum, strlen(datum));
    }
    sprintf(datum, "%d\n", (intensityFromDutyCycle(bracelet, bracelet.motor[i]->duty)));
    ESP_LOGI(TAGb, "getIntensity: datum %s", datum);
    write(sockfd, datum, strlen(datum));
}

void addMotor(Bracelet *brclt, ledc_channel_t channel, int gpio_num)
{
    ESP_LOGI(TAGb, "addMotor");
    ledc_channel_config_t *channel_config = malloc(sizeof(ledc_channel_config_t));
    if (channel_config != NULL)
    {
        ESP_LOGI(TAGb, "addMotor:channel_config (1)");
        channel_config->speed_mode = LEDC_MODE;
        channel_config->sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE;
        channel_config->channel = channel;
        channel_config->timer_sel = LEDC_TIMER;
        channel_config->intr_type = LEDC_INTR_DISABLE;
        channel_config->gpio_num = gpio_num;
        channel_config->duty = 0;
        channel_config->hpoint = 0;
        ESP_LOGI(TAGb, "addMotor:channel_config (3)%d", brclt->motor_count);
        if (brclt->motor_count == 0)
            brclt->motor = malloc(sizeof(ledc_channel_config_t));
        else
            brclt->motor[brclt->motor_count] = realloc(brclt->motor, sizeof(ledc_channel_config_t) * brclt->motor_count);

        if (brclt->motor[brclt->motor_count] != NULL)
        {
            brclt->motor_count++;
            brclt->motor[brclt->motor_count - 1] = (ledc_channel_config_t *)channel_config;
            ESP_ERROR_CHECK(ledc_channel_config(channel_config));
        }

        else
            ESP_LOGI(TAGb, "Malloc error: array of motors");
    }
    else
        ESP_LOGI(TAGb, "Malloc error: new motor");
}

void initializeHaptic(Bracelet *brclt)
{
    // Configure LEDC timer

    ESP_LOGI(TAGb, "InitializeHaptic");
    brclt->motor_count = 0;
    ESP_LOGI(TAGb, "InitializeHaptic..timer");

    brclt->timer.speed_mode = LEDC_MODE;
    brclt->timer.timer_num = LEDC_TIMER;
    brclt->timer.duty_resolution = LEDC_DUTY_RES;
    brclt->timer.freq_hz = LEDC_FREQUENCY;
    brclt->timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&(brclt->timer)));

    ESP_LOGI(TAGb, "InitializeHaptic..motor0");
    
    addMotor(brclt, LEDC_CHANNEL_0, 0);

    ESP_LOGI(TAGb, "InitializeHaptic..motor1");
    
    addMotor(brclt, LEDC_CHANNEL_1, 2);

    // Set duty cycle (example: 50%)
    ESP_LOGI(TAGb, "InitializeHaptic.max_duty");
    brclt->max_duty = (1 << LEDC_DUTY_RES) - 1;

    // Example to ramp up the duty cycle over time

    ESP_LOGI(TAGb, "InitializeHaptic.motor count %d", brclt->motor_count);
    
    for (int current = 0; current < brclt->motor_count; current++)
    {
        for (int i = 0; i < brclt->max_duty; i += 10)
        { // Adjust increment as needed
            ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, brclt->motor[current]->channel, i));
            ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, brclt->motor[current]->channel));
            vTaskDelay(pdMS_TO_TICKS(10)); // Delay for smooth ramp-up
        }
        ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, brclt->motor[current]->channel, 0));
        ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, brclt->motor[current]->channel));
    }
}
