
#include "driver/ledc.h"

#define LEDC_TIMER LEDC_TIMER_0
#define LEDC_MODE LEDC_LOW_SPEED_MODE // or LEDC_HIGH_SPEED_MODE
#define LEDC_CHANNEL LEDC_CHANNEL_0
#define LEDC_DUTY_RES LEDC_TIMER_11_BIT // LEDC_TIMER_13_BIT // Adjust resolution as needed
#define LEDC_FREQUENCY (200)            // 200 Hz
#define LEDC_OUTPUT_IO (2)              //(GPIO_NUM_YOUR_MOTOR_PIN) // Replace with your motor pin

static const char *TAGb = "bracelet";

typedef struct VibrationMotor
{
    ledc_channel_config_t *channel_t;
    unsigned char enabled;
    //unsigned char map_index; // gpio_num * 1 (!)
} VibrationMotor;

typedef struct Bracelet
{
    ledc_timer_config_t timer;
    VibrationMotor motor[6];
    uint32_t max_duty;
    unsigned int motor_count;
} Bracelet;

Bracelet bracelet;

uint32_t dutyCycleFromIntensity(Bracelet brclt, int intensity)
{
    return 0 == intensity ? 0 : bracelet.max_duty * intensity / 100;
}

void motorIntensity(VibrationMotor motor, int intensity)
{

    motor.channel_t->duty = dutyCycleFromIntensity(bracelet, intensity);
    ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, motor.channel_t->channel, dutyCycleFromIntensity(bracelet, (intensity))));
    ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, motor.channel_t->channel));
}

int enabled_motor_count(Bracelet bracelet)
{
    int count=0;
    for (int idx=0; idx < bracelet.motor_count; idx++)
        count += bracelet.motor[idx].enabled; // enabled == 1, disabled == 0
    return count;
}

int intensityFromDutyCycle(Bracelet brclt, uint32_t dutyCycle)
{
    return (int)(0 == dutyCycle ? 0 : dutyCycle * 100 / bracelet.max_duty);
}

/*PoMA handlers*/
/*
getMotorCount, setMotorCount
getIntensity, setIntensity
getMotorConfig,setMotorConfig
getEnabledMotors, setEnabledMotors
*/
void getMotorCount(int sockfd, char *parameters)
{
    char response[10];
    sprintf(response, "%d\n", enabled_motor_count(bracelet));
    write(sockfd, response, strlen(response));
}

void setMotorCount(int sockfd, char *parameters)
{
    write(sockfd, "done\n", 6);
}

void setIntensity(int sockfd, char *parameters)
{
    // params is a list of comma separated integers
    char **params;
    int idx = 0;

    params = (char **)malloc(1 * sizeof(int *));
    char *token = strtok(parameters, ",");

    while (token != NULL)
    {

        params[idx] = (char *)malloc(sizeof(char) * strlen(token));
        strcpy(params[idx++], token);
        token = strtok(NULL, ",");
    }
    // set intensity for each motor
    if (idx != enabled_motor_count(bracelet))
        write(sockfd, "Error: parameters do not match motor count\n", 6);
    else
    {
        for (int i = 0; i < idx; i++)
        {
            // bracelet.motor[i].channel_t->duty = dutyCycleFromIntensity(bracelet, atoi(params[i]));
            // ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, bracelet.motor[i].channel_t->channel, bracelet.motor[i].max_duty));
            // ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, bracelet.motor[i].channel_t->channel));
            motorIntensity(bracelet.motor[i], atoi(params[i]));
        }
        write(sockfd, "done\n", 6);
    }
    free(params);
}

void getIntensity(int sockfd, char *parameters)
{
    char datum[10];
    int i;

    // params is a list of comma separated integers
    for (i = 0; i < enabled_motor_count(bracelet) - 1; i++)
    {
        sprintf(datum, "%d,", (intensityFromDutyCycle(bracelet, bracelet.motor[i].channel_t->duty)));

        write(sockfd, datum, strlen(datum));
    }
    sprintf(datum, "%d\n", (intensityFromDutyCycle(bracelet, bracelet.motor[i].channel_t->duty)));

    write(sockfd, datum, strlen(datum));
}

void addMotor(Bracelet *brclt, int motor_index, ledc_channel_t channel, int gpio_num)
{
    ledc_channel_config_t *channel_config = malloc(sizeof(ledc_channel_config_t));

    if (channel_config != NULL)
    {
        channel_config->speed_mode = LEDC_MODE;
        channel_config->sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE;
        channel_config->channel = channel;
        channel_config->timer_sel = LEDC_TIMER;
        channel_config->intr_type = LEDC_INTR_DISABLE;
        channel_config->gpio_num = gpio_num;
        channel_config->duty = 0;
        channel_config->hpoint = 0;

        ESP_ERROR_CHECK(ledc_channel_config(channel_config));

        brclt->motor[motor_index].enabled = 1; //enabled
        //brclt->motor[motor_index].map_index = gpio_num * 1;
        brclt->motor[motor_index].channel_t = channel_config;
    }
    else
        ESP_LOGI(TAGb, "Malloc error: new motor");
}

void removeMotor(Bracelet *brclt, int gpio_num)
{ // it removes a given gpio_num
  // esp_err_t ledc_stop(ledc_mode_t speed_mode, ledc_channel_t channel, uint32_t idle_level)
}

void setMotorConfig(int sockfd, char *parameters)
{
    //= setMotorCofig gpio_num,status
    // gpio = {0,1,2,3,4,5} status={0,1}
    char **params;
    int idx = 0;

    params = (char **)malloc(1 * sizeof(int *));
    char *token = strtok(parameters, ",");

    while (token != NULL)
    {

        params[idx] = (char *)malloc(sizeof(char) * strlen(token));
        strcpy(params[idx++], token);
        token = strtok(NULL, ",");
    }
    // configure one gpio: two parameters
    if (idx != 2)
        write(sockfd, "Error: gpio_num {0,1,2,3,4,5} status {0,1}\n", 44);
    else
    {
        bracelet.motor[atoi(params[0])].enabled = atoi(params[1]);
        write(sockfd, "done\n", 6);
    }
    free(params);
}

void getMotorConfig(int sockfd, char *parameters)
{
    //? getMotorCofig gpio_num
    // returns status for that gpio_num
    write(sockfd, "Not implemented yet\n", 21);
}

void getEnabledMotors(int sockfd, char *parameters){
    char datum[10];
    int i;

    // params is a list of comma separated integers
    for (i = 0; i < 6; i++)
    {
        sprintf(datum, "%d,", bracelet.motor[i].enabled);

        write(sockfd, datum, strlen(datum));
    }
    sprintf(datum, "%d,", bracelet.motor[i].enabled);

    write(sockfd, datum, strlen(datum));
}
void setEnabledMotors(int sockfd, char *parameters)
{
    // params is a list of comma separated booleans (0|1) to disable|enable availablemotors
    char **params;
    int idx = 0;

    params = (char **)malloc(1 * sizeof(int *));
    char *token = strtok(parameters, ",");

    while (token != NULL)
    {

        params[idx] = (char *)malloc(sizeof(char) * strlen(token));
        strcpy(params[idx++], token);
        token = strtok(NULL, ",");
    }
    // set intensity for each motor
    if (idx != 5)
        write(sockfd, "Error: parameters do not match motor count\n", 6);
    else
    {
        for (int i = 0; i < idx; i++)
        {
            // bracelet.motor[i].channel_t->duty = dutyCycleFromIntensity(bracelet, atoi(params[i]));
            // ESP_ERROR_CHECK(ledc_set_duty(LEDC_MODE, bracelet.motor[i].channel_t->channel, bracelet.motor[i].max_duty));
            // ESP_ERROR_CHECK(ledc_update_duty(LEDC_MODE, bracelet.motor[i].channel_t->channel));
            bracelet.motor[i].enabled= atoi(params[i]);
        }
        write(sockfd, "done\n", 6);
    }
    free(params);
}

/*end PoMA Handleres*/

void initializeHaptic(Bracelet *brclt)
{
    // Configure LEDC timer

    ESP_LOGI(TAGb, "InitializeHaptic");
    brclt->motor_count = 6;

    brclt->timer.speed_mode = LEDC_MODE;
    brclt->timer.timer_num = LEDC_TIMER;
    brclt->timer.duty_resolution = LEDC_DUTY_RES;
    brclt->timer.freq_hz = LEDC_FREQUENCY;
    brclt->timer.clk_cfg = LEDC_AUTO_CLK;
    ESP_ERROR_CHECK(ledc_timer_config(&(brclt->timer)));

    /*addMotor(Bracelet *brclt, ledc_channel_t channel, int gpio_num)*/
    addMotor(brclt,0, LEDC_CHANNEL_0, 18);

    addMotor(brclt,1, LEDC_CHANNEL_1, 20);

    addMotor(brclt,2, LEDC_CHANNEL_2, 0);

    addMotor(brclt,3, LEDC_CHANNEL_3, 2);

    addMotor(brclt,4, LEDC_CHANNEL_4, 16);

    addMotor(brclt,5, LEDC_CHANNEL_5, 23);

    brclt->max_duty = (1 << LEDC_DUTY_RES) - 1;

    for (int current = 0; current < enabled_motor_count(*brclt); current++)
    {
        for (int intensity = 0; intensity <= brclt->max_duty; intensity += 10)
        { // Adjust increment as needed

            motorIntensity(brclt->motor[current], intensity);
            vTaskDelay(pdMS_TO_TICKS(10)); // Delay for smooth ramp-up
        }
    }
    for (int current = 0; current < enabled_motor_count(*brclt); current++)
    {
        motorIntensity(brclt->motor[current], 0);
    }
}
