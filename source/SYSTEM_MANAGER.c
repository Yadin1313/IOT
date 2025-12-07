/*
 * SYSTEM_MANAGER.c
 * * Código completo: Slider y Tilt hacen vaivén cada 2 segundos.
 */

#include "SYSTEM_MANAGER.h"

/*-----------------------------------------------------------------------------
 * HANDLES GLOBALES DE FREERTOS
 *----------------------------------------------------------------------------*/
QueueHandle_t      g_qSensorsToControl  = NULL;
QueueHandle_t      g_qRefsToControl     = NULL;
QueueHandle_t      g_qLogSamples        = NULL;
SemaphoreHandle_t  g_mutexI2C           = NULL;
EventGroupHandle_t g_evtSystemStatus    = NULL;

/*-----------------------------------------------------------------------------
 * PROTOTIPOS DE TAREAS
 *----------------------------------------------------------------------------*/
static void CONTROL_Task(void *pvParameters);
static void SENSORS_Task(void *pvParameters);
static void COMM_Task(void *pvParameters);
static void LOGGER_Task(void *pvParameters);
static void STATUS_Task(void *pvParameters);

/*-----------------------------------------------------------------------------
 * FUNCIONES PRIVADAS AUXILIARES
 *----------------------------------------------------------------------------*/
static void SYSTEM_MANAGER_CreateRTOSObjects(void)
{
    g_qSensorsToControl = xQueueCreate(1u, sizeof(SENSOR_SAMPLE_T));
    g_qRefsToControl    = xQueueCreate(1u, sizeof(CONTROL_REF_T));
    g_qLogSamples       = xQueueCreate(16u, sizeof(LOG_SAMPLE_T));
    g_mutexI2C          = xSemaphoreCreateMutex();
    g_evtSystemStatus   = xEventGroupCreate();

    configASSERT(g_qSensorsToControl != NULL);
    configASSERT(g_qRefsToControl    != NULL);
    configASSERT(g_qLogSamples       != NULL);
    configASSERT(g_evtSystemStatus   != NULL);
}

static void SYSTEM_MANAGER_CreateTasks(void)
{
    BaseType_t status;

    status = xTaskCreate(CONTROL_Task, "CONTROL",
                         CONTROL_TASK_STACK_SIZE, NULL,
                         CONTROL_TASK_PRIORITY, NULL);
    configASSERT(status == pdPASS);

    status = xTaskCreate(SENSORS_Task, "SENSORS",
                         SENSORS_TASK_STACK_SIZE, NULL,
                         SENSORS_TASK_PRIORITY, NULL);
    configASSERT(status == pdPASS);

    status = xTaskCreate(COMM_Task, "COMM",
                         COMM_TASK_STACK_SIZE, NULL,
                         COMM_TASK_PRIORITY, NULL);
    configASSERT(status == pdPASS);

    status = xTaskCreate(LOGGER_Task, "LOGGER",
                         LOGGER_TASK_STACK_SIZE, NULL,
                         LOGGER_TASK_PRIORITY, NULL);
    configASSERT(status == pdPASS);

    status = xTaskCreate(STATUS_Task, "STATUS",
                         STATUS_TASK_STACK_SIZE, NULL,
                         STATUS_TASK_PRIORITY, NULL);
    configASSERT(status == pdPASS);
}

/*-----------------------------------------------------------------------------
 * API PÚBLICA
 *----------------------------------------------------------------------------*/
void SYSTEM_MANAGER_Init(void)
{
    float Ts_control_s = ((float)CONTROL_TASK_PERIOD_MS) / 1000.0f;

    PRINTF(">>> SYSTEM_MANAGER: Creando objetos RTOS...\r\n");
    SYSTEM_MANAGER_CreateRTOSObjects();
    xEventGroupSetBits(g_evtSystemStatus, EVT_SYS_OK_BIT);

    PRINTF(">>> SYSTEM_MANAGER: Init GPIO y PWM...\r\n");
    GPIO_RW612_Init();
    MOTOR_PWM_Init(); /* Aquí los motores se deben detener, es normal */

    PID_CONTROL_Init(Ts_control_s);

    PRINTF(">>> SYSTEM_MANAGER: Init I2C...\r\n");
    (void)I2C_RW612_Init();

    /* --- PRUEBA DE MOTORES: SENSORES DESACTIVADOS TEMPORALMENTE ---
     * Comentamos esto para evitar que el código se congele si fallan los cables.
     */

    // if (VL53L0X_Init() != kStatus_Success)
    //    xEventGroupSetBits(g_evtSystemStatus, EVT_SYS_ERROR_SENSOR_BIT);

    // if (MPU6050_Init() != kStatus_Success)
    //    xEventGroupSetBits(g_evtSystemStatus, EVT_SYS_ERROR_SENSOR_BIT);

    PRINTF(">>> SYSTEM_MANAGER: Creando Tareas...\r\n");
    SYSTEM_MANAGER_CreateTasks();

    PRINTF(">>> SYSTEM_MANAGER: Listo. Arrancando Scheduler.\r\n");
}
/*!
 * @brief Inserta referencia proveniente del sistema YOLO (Wi-Fi)
 */
void SYSTEM_MANAGER_SetYoloRef(int32_t slide, int32_t yaw, int32_t pitch)
{
    CONTROL_REF_T ref;
    /* Ajusta los factores de escala según tu mecánica */
    const float SLIDER_SCALE_M = 0.001f;  /* 1 unidad = 1 mm  */
    const float PAN_SCALE_DEG  = 1.0f;    /* 1 unidad = 1°   */
    const float TILT_SCALE_DEG = 1.0f;    /* 1 unidad = 1°   */

    ref.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);
    ref.slider_ref_m = (float)slide * SLIDER_SCALE_M;
    ref.pan_ref_deg  = (float)yaw   * PAN_SCALE_DEG;
    ref.tilt_ref_deg = (float)pitch * TILT_SCALE_DEG;
    ref.source       = CONTROL_REF_SOURCE_VISION;
    ref.valid        = TRUE;

    if (g_qRefsToControl != NULL)
        xQueueOverwrite(g_qRefsToControl, &ref);
}

/*-----------------------------------------------------------------------------
 * IMPLEMENTACIÓN DE TAREAS
 *----------------------------------------------------------------------------*/
static void CONTROL_Task(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime;
    const TickType_t xPeriod = pdMS_TO_TICKS(CONTROL_TASK_PERIOD_MS);

    SENSOR_SAMPLE_T sensorSample;
    CONTROL_REF_T   controlRef;
    LOG_SAMPLE_T    logSample;

    boolean_t sensorDataAvailable = FALSE;
    boolean_t refDataAvailable    = FALSE;

    /* Inicializar referencias locales */
    controlRef.slider_ref_m = 0.0f;
    controlRef.pan_ref_deg  = 0.0f;
    controlRef.tilt_ref_deg = 0.0f;
    controlRef.valid        = FALSE;

    xLastWakeTime = xTaskGetTickCount();

    for (;;)
    {
        /* Esperar al siguiente ciclo (1ms) */
        vTaskDelayUntil(&xLastWakeTime, xPeriod);

        /* --- 1. RECEPCIÓN DE DATOS --- */
        if (xQueueReceive(g_qSensorsToControl, &sensorSample, 0u) == pdPASS)
            sensorDataAvailable = TRUE;

        if (xQueueReceive(g_qRefsToControl, &controlRef, 0u) == pdPASS)
            refDataAvailable = (controlRef.valid == TRUE);

        /* Variables para logging */
        float slider_meas = sensorDataAvailable ? sensorSample.slider_pos_m : 0.0f;
        float pan_meas    = sensorDataAvailable ? sensorSample.pan_deg      : 0.0f;
        float tilt_meas   = sensorDataAvailable ? sensorSample.tilt_deg     : 0.0f;
        float slider_ref  = refDataAvailable ? controlRef.slider_ref_m : 0.0f;
        float pan_ref     = refDataAvailable ? controlRef.pan_ref_deg  : 0.0f;
        float tilt_ref    = refDataAvailable ? controlRef.tilt_ref_deg : 0.0f;

        /* --- 2. CONTROL DE MOTORES (PRUEBA DE REVERSA) --- */

        float u_slider;
        float u_tilt;
        float u_pan  = 0.0f;

        uint32_t current_time_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

        /* FORZAR REVERSA CONTINUA
         * Si el motor gira hacia adelante con esto, el cable de dirección está mal.
         */
        u_slider = -0.90f;  /* Negativo (Reversa) al 90% */
        u_tilt   = -0.80f;  /* Negativo (Reversa) al 80% */

        MOTOR_PWM_SetOutput(MOTOR_ID_SLIDER, u_slider);
        MOTOR_PWM_SetOutput(MOTOR_ID_PAN,    u_pan);
        MOTOR_PWM_SetOutput(MOTOR_ID_TILT,   u_tilt);

        /* --- 3. LOGGING --- */

        /* Imprimir diagnóstico cada 1 segundo */
        if ((current_time_ms % 1000u) == 0u)
        {
             PRINTF(">> MODO REVERSA | Slider: %d | Tilt: %d\r\n", (int)(u_slider*100), (int)(u_tilt*100));
        }

        logSample.timestamp_ms  = current_time_ms;
        logSample.slider_ref_m  = slider_ref;
        logSample.slider_meas_m = slider_meas;
        logSample.slider_u      = u_slider;
        logSample.pan_ref_deg   = pan_ref;
        logSample.pan_meas_deg  = pan_meas;
        logSample.pan_u         = u_pan;
        logSample.tilt_ref_deg  = tilt_ref;
        logSample.tilt_meas_deg = tilt_meas;
        logSample.tilt_u        = u_tilt;
        logSample.sensors_ok    = sensorDataAvailable;
        logSample.refs_ok       = refDataAvailable;
        logSample.sat_slider    = FALSE;
        logSample.sat_pan       = FALSE;
        logSample.sat_tilt      = FALSE;

        (void)xQueueSend(g_qLogSamples, &logSample, 0u);
    }
}

static void SENSORS_Task(void *pvParameters)
{
    (void)pvParameters;
    TickType_t xLastWakeTime = xTaskGetTickCount();
    const TickType_t xPeriod = pdMS_TO_TICKS(SENSORS_TASK_PERIOD_MS);
    const float dt_s = ((float)SENSORS_TASK_PERIOD_MS) / 1000.0f;

    SENSOR_SAMPLE_T sample;
    status_t status;
    uint16_t d_mm;

    for (;;)
    {
        vTaskDelayUntil(&xLastWakeTime, xPeriod);
        sample.timestamp_ms = (uint32_t)(xTaskGetTickCount() * portTICK_PERIOD_MS);

        /* Leer Sensor VL53L0X (Slider) */
        status = VL53L0X_ReadDistanceMm(&d_mm);
        if (status == kStatus_Success)
        {
            sample.slider_raw_mm = d_mm;
            sample.slider_pos_m  = (float)d_mm / 1000.0f;
            sample.slider_valid  = TRUE;
        }
        else
        {
            sample.slider_raw_mm = 0u;
            sample.slider_pos_m  = 0.0f;
            sample.slider_valid  = FALSE;
            xEventGroupSetBits(g_evtSystemStatus, EVT_SYS_ERROR_SENSOR_BIT);
        }

        /* Leer Sensor MPU6050 (Pan/Tilt) */
        status = MPU6050_Update(dt_s);
        if (status == kStatus_Success)
        {
            float pitch, roll;
            MPU6050_GetAngles(&pitch, &roll);
            sample.pan_deg   = roll;
            sample.tilt_deg  = pitch;
            sample.imu_valid = TRUE;
        }
        else
        {
            sample.pan_deg   = 0.0f;
            sample.tilt_deg  = 0.0f;
            sample.imu_valid = FALSE;
            xEventGroupSetBits(g_evtSystemStatus, EVT_SYS_ERROR_SENSOR_BIT);
        }

        if (sample.slider_valid && sample.imu_valid)
            xEventGroupClearBits(g_evtSystemStatus, EVT_SYS_ERROR_SENSOR_BIT);

        (void)xQueueOverwrite(g_qSensorsToControl, &sample);
    }
}

static void COMM_Task(void *pvParameters)
{
    (void)pvParameters;
    TickType_t lastWake = xTaskGetTickCount();
    for (;;)
    {
        vTaskDelayUntil(&lastWake, pdMS_TO_TICKS(100u));
    }
}

static void LOGGER_Task(void *pvParameters)
{
    (void)pvParameters;
    LOG_SAMPLE_T logMsg;
    boolean_t headerPrinted = FALSE;

    for (;;)
    {
        if (xQueueReceive(g_qLogSamples, &logMsg, portMAX_DELAY) == pdPASS)
        {
            unsigned int t_ms_u = (unsigned int)logMsg.timestamp_ms;

            /* Convertir floats a enteros para imprimir fácil */
            int slider_ref_mm   = (int)(logMsg.slider_ref_m  * 1000.0f);
            int slider_meas_mm  = (int)(logMsg.slider_meas_m * 1000.0f);
            int slider_u_milli  = (int)(logMsg.slider_u      * 1000.0f);

            int pan_ref_mdeg    = (int)(logMsg.pan_ref_deg   * 1000.0f);
            int pan_meas_mdeg   = (int)(logMsg.pan_meas_deg  * 1000.0f);
            int pan_u_milli     = (int)(logMsg.pan_u         * 1000.0f);

            int tilt_ref_mdeg   = (int)(logMsg.tilt_ref_deg  * 1000.0f);
            int tilt_meas_mdeg  = (int)(logMsg.tilt_meas_deg * 1000.0f);
            int tilt_u_milli    = (int)(logMsg.tilt_u        * 1000.0f);

            if (!headerPrinted)
            {
                PRINTF("t_ms,slider_ref_mm,slider_meas_mm,slider_u_milli,"
                       "pan_ref_mdeg,pan_meas_mdeg,pan_u_milli,"
                       "tilt_ref_mdeg,tilt_meas_mdeg,tilt_u_milli,"
                       "sensors_ok,refs_ok,sat_slider,sat_pan,sat_tilt\r\n");
                headerPrinted = TRUE;
            }

            PRINTF("%u,%d,%d,%d,%d,%d,%d,%d,%d,%d,%u,%u,%u,%u,%u\r\n",
                   t_ms_u,
                   slider_ref_mm, slider_meas_mm, slider_u_milli,
                   pan_ref_mdeg,  pan_meas_mdeg,  pan_u_milli,
                   tilt_ref_mdeg, tilt_meas_mdeg, tilt_u_milli,
                   (unsigned int)logMsg.sensors_ok,
                   (unsigned int)logMsg.refs_ok,
                   (unsigned int)logMsg.sat_slider,
                   (unsigned int)logMsg.sat_pan,
                   (unsigned int)logMsg.sat_tilt);
        }
    }
}

static void STATUS_Task(void *pvParameters)
{
    (void)pvParameters;
    EventBits_t bits;

    for (;;)
    {
        bits = xEventGroupGetBits(g_evtSystemStatus);
        GPIO_RW612_LED_RedOff();
        GPIO_RW612_LED_GreenOff();
        GPIO_RW612_LED_BlueOff();

        if (bits & EVT_SYS_ERROR_SENSOR_BIT)
            GPIO_RW612_LED_RedOn();
        else if (bits & EVT_SYS_ERROR_COMM_BIT)
            GPIO_RW612_LED_BlueOn();
        else
            GPIO_RW612_LED_GreenOn();

        vTaskDelay(pdMS_TO_TICKS(STATUS_TASK_PERIOD_MS));
    }
}

/* Hook de FreeRTOS para Stack Overflow */
void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    PRINTF("\r\n*** Stack overflow en tarea: %s ***\r\n", pcTaskName);
    GPIO_RW612_LED_RedOn();
    GPIO_RW612_LED_GreenOff();
    GPIO_RW612_LED_BlueOff();
    taskDISABLE_INTERRUPTS();
    for (;;);
}

