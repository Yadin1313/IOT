/** @file main.c
 * @brief RW612 UDP Controller for YOLO
 */

#include "FreeRTOS.h"
#include "task.h"
#include "pin_mux.h"
#include "clock_config.h"
#include "board.h"
#include "fsl_debug_console.h"
#include "fsl_shell.h"
#include "wpl.h"
#include "shell_task.h"
#include "fsl_power.h"

/* LwIP Includes */
#include "lwip/sockets.h"
#include "lwip/sys.h"
#include "lwip/netdb.h"
#include "SYSTEM_MANAGER.h"

/*******************************************************************************
 * Definitions
 ******************************************************************************/
#define main_task_PRIORITY          1
#define main_task_STACK_DEPTH       800
#define SHELL_ADDITIONAL_STACK_SIZE 256
#define DEMO_WIFI_LABEL             "MyWifi"

/* --- TUS CREDENCIALES --- */
#define TARGET_SSID                 "YadinRdz"
#define TARGET_PASS                 "pymr1313"
#define UDP_PORT                    6000
/* ------------------------ */

static volatile bool wlan_connected = false;

/*******************************************************************************
 * Prototypes & Shell Commands
 ******************************************************************************/
// (Mismos comandos shell de siempre para no romper compatibilidad)
static shell_status_t cmd_connect(void *shellHandle, int32_t argc, char **argv);
static shell_status_t cmd_scan(void *shellHandle, int32_t argc, char **argv);
static shell_status_t cmd_disconnect(void *shellHandle, int32_t argc, char **argv);

SHELL_COMMAND_DEFINE(wlan_scan, "\r\n\"wlan_scan\": Scans networks.\r\n", cmd_scan, 0);
SHELL_COMMAND_DEFINE(wlan_connect_with_password, "\r\n\"wlan_connect...\"", cmd_connect, 2);
SHELL_COMMAND_DEFINE(wlan_connect, "\r\n\"wlan_connect...\"", cmd_connect, 1);
SHELL_COMMAND_DEFINE(wlan_disconnect, "\r\n\"wlan_disconnect...\"", cmd_disconnect, 0);

static void printSeparator(void) { PRINTF("========================================\r\n"); }

/* Link lost callback */
static void LinkStatusChangeCallback(bool linkState)
{
    if (linkState == false) {
        PRINTF("-------- LINK LOST --------\r\n");
        wlan_connected = false;
    } else {
        PRINTF("-------- LINK REESTABLISHED --------\r\n");
        wlan_connected = true;
    }
}

void MoverMotores(int slide, int yaw, int pitch)
{
    // Por ahora solo imprimimos para verificar que el dato llega bien
    PRINTF("[MOTOR ACTION] Slide:%d | Yaw:%d | Pitch:%d\r\n", slide, yaw, pitch);
}

/* =================================================================
   TAREA UDP: Escucha el puerto 6000 y parsea comandos
   ================================================================= */
static void udp_server_task(void *arg)
{
    int sock;
    struct sockaddr_in server_addr, client_addr;
    socklen_t addr_len = sizeof(client_addr);
    char rx_buffer[256]; // Buffer para recibir el string
    int recv_len;

    // Variables para los valores parseados
    int val_slide, val_yaw, val_pitch;

    PRINTF("\r\n[UDP] Iniciando servidor de control en PUERTO %d...\r\n", UDP_PORT);

    // 1. Crear Socket UDP (SOCK_DGRAM)
    sock = socket(AF_INET, SOCK_DGRAM, IPPROTO_UDP);
    if (sock < 0) {
        PRINTF("[UDP] Error creando socket.\r\n");
        vTaskDelete(NULL);
    }

    // 2. Configurar dirección
    memset(&server_addr, 0, sizeof(server_addr));
    server_addr.sin_family = AF_INET;
    server_addr.sin_port = htons(UDP_PORT);
    server_addr.sin_addr.s_addr = htonl(INADDR_ANY);

    // 3. Bind
    if (bind(sock, (struct sockaddr *)&server_addr, sizeof(server_addr)) < 0) {
        PRINTF("[UDP] Error en Bind. Puerto ocupado?\r\n");
        close(sock);
        vTaskDelete(NULL);
    }

    PRINTF("[UDP] LISTO! Esperando comandos de Python...\r\n");

    while (1) {
        // Recibir paquete (Bloqueante)
        recv_len = recvfrom(sock, rx_buffer, sizeof(rx_buffer) - 1, 0,
                            (struct sockaddr *)&client_addr, &addr_len);

        if (recv_len > 0) {
            rx_buffer[recv_len] = 0; // Asegurar null-termination

            // --- LOGICA DE PARSEO ---
            // El formato que manda python es: "SLIDE:x;YAW:y;PITCH:z;"
            // Si empieza con 'S', es comando. Si empieza con '{', es JSON.

            if (rx_buffer[0] == 'S')
            {
                // Usamos sscanf para extraer los numeros enteros
                if (sscanf(rx_buffer, "SLIDE:%d;YAW:%d;PITCH:%d;", &val_slide, &val_yaw, &val_pitch) == 3)
                {
                    // Datos validos recibidos, mover motores
                    MoverMotores(val_slide, val_yaw, val_pitch);
                }
            }
            else if (rx_buffer[0] == '{')
            {
                // Es el JSON con bounding boxes, solo imprimimos aviso (para no saturar consola)
                // PRINTF("[JSON] Bounding box info recibida\r\n");
            }
        }
    }
}

void task_main(void *param)
{
    wpl_ret_t err = WPLRET_FAIL;
    char ipAddr[16] = {0};
    int retry_count = 0;

    static shell_command_t *wifi_commands[] = {
        SHELL_COMMAND(wlan_scan), SHELL_COMMAND(wlan_connect), SHELL_COMMAND(wlan_connect_with_password),
        SHELL_COMMAND(wlan_disconnect), NULL
    };

    PRINTF("Initialize WLAN \r\n");
    printSeparator();

    WPL_Init();
    WPL_Start(LinkStatusChangeCallback);

    PRINTF("\r\n[AUTO] Conectando a Hotspot: %s ...\r\n", TARGET_SSID);
    WPL_AddNetwork(TARGET_SSID, TARGET_PASS, DEMO_WIFI_LABEL);

    // Auto-Connect Loop
    while (retry_count < 10 && !wlan_connected) { // 10 intentos
        if (WPL_Join(DEMO_WIFI_LABEL) == WPLRET_SUCCESS) {
            wlan_connected = true;
        } else {
            PRINTF(".");
            vTaskDelay(pdMS_TO_TICKS(1000));
            retry_count++;
        }
    }

    if (wlan_connected) {
        // Esperamos IP
        int ip_retries = 0;
        while (ip_retries < 10) {
             if (WPL_GetIP(ipAddr, 1) == WPLRET_SUCCESS) {
                 break;
             }
             vTaskDelay(pdMS_TO_TICKS(1000));
             ip_retries++;
        }

        PRINTF("\r\n************************************************\r\n");
        PRINTF("   CONEXION LISTA PARA YOLO\r\n");
        PRINTF("   IP DE LA TARJETA: %s\r\n", ipAddr); // <--- IMPORTANTE
        PRINTF("   PUERTO UDP: %d\r\n", UDP_PORT);
        PRINTF("************************************************\r\n");
        PRINTF(">> Actualiza la variable RW612_IP en tu script de Python con esta IP.\r\n");

        // ARRANCAR TAREA UDP
        sys_thread_new("udp_server", udp_server_task, NULL, 1024, 2);

    } else {
        PRINTF("\r\n[Error] No se pudo conectar al WiFi. Revisa el Hotspot.\r\n");
    }

    PRINTF("Initialize CLI\r\n");
    shell_task_init(wifi_commands, SHELL_ADDITIONAL_STACK_SIZE);
    vTaskDelete(NULL);
}

/* Shell commands stubs (sin cambios) */
static shell_status_t cmd_connect(void *shellHandle, int32_t argc, char **argv) { return kStatus_SHELL_Success; }
static shell_status_t cmd_disconnect(void *shellHandle, int32_t argc, char **argv) { return kStatus_SHELL_Success; }
static shell_status_t cmd_scan(void *shellHandle, int32_t argc, char **argv) { return kStatus_SHELL_Success; }

int main(void)
{
    BOARD_InitBootPins();
    if (BOARD_IS_XIP()) {
        BOARD_BootClockLPR();
        CLOCK_EnableClock(kCLOCK_Otp);
        CLOCK_EnableClock(kCLOCK_Els);
        CLOCK_EnableClock(kCLOCK_ElsApb);
        RESET_PeripheralReset(kOTP_RST_SHIFT_RSTn);
        RESET_PeripheralReset(kELS_APB_RST_SHIFT_RSTn);
    } else {
        BOARD_InitBootClocks();
    }
    BOARD_InitDebugConsole();
    RESET_PeripheralReset(kGDMA_RST_SHIFT_RSTn);
    POWER_ConfigCauInSleep(false);
    BOARD_InitSleepPinConfig();


    xTaskCreate(task_main, "main", main_task_STACK_DEPTH, NULL, main_task_PRIORITY, NULL);
    SYSTEM_MANAGER_Init();
    vTaskStartScheduler();
    for (;;) ;
}
