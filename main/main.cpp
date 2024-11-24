#include "FreeRTOS.h"
#include "task.h"
#include "timers.h"

#include "log.h"
#include "ui/menu.h"
#include "display.h"

#include "ota.h"

#include "hw_init.h"
#include "ui/usb_connected.h"
#include "input.h"

static const char *TAG = "MAIN";

void dumpDebugFunc(TimerHandle_t) {
  auto *args = get_board();
  args->PmLock();
  args->DebugInfo();

//  TaskStatus_t tasks[20];
//  unsigned int count = uxTaskGetSystemState(tasks, 20, nullptr);
//  for (unsigned int i = 0; i < count; i++) {
//    LOGI(TAG, "%s Highwater: %lu", tasks[i].pcTaskName, tasks[i].usStackHighWaterMark);
//  }
  args->PmRelease();

}

static void dumpDebugTimerInit() {
  xTimerStart(xTimerCreate("dumpDebugTimer", 10000/portTICK_PERIOD_MS, pdTRUE, nullptr, dumpDebugFunc), 0);
}

MAIN_STATES currentState = MAIN_NONE;

/*!
 * Task to check battery status, and update device
 * @param args
 */
static void lowBatteryTask(TimerHandle_t) {
  auto *board = get_board();
  TaskHandle_t lvglHandle;
  TaskHandle_t display_task_handle;

  if (currentState != MAIN_OTA) {
    switch (board->PowerState()) {
      case Boards::BOARD_POWER_NORMAL:
        if (currentState == MAIN_LOW_BATT) {
          currentState = MAIN_NORMAL;
        }
        break;
      case Boards::BOARD_POWER_LOW:
        lvglHandle = xTaskGetHandle("LVGL");
        xTaskNotifyIndexed(lvglHandle, 0, LVGL_STOP, eSetValueWithOverwrite);
        display_task_handle = xTaskGetHandle("display_task");
        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_BATT, eSetValueWithOverwrite);
        currentState = MAIN_LOW_BATT;
        break;
      case Boards::BOARD_POWER_CRITICAL:
        display_task_handle = xTaskGetHandle("display_task");
        xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_BATT, eSetValueWithOverwrite);
        vTaskDelay(15000 / portTICK_PERIOD_MS);
        board->PowerOff();
        break;
    }
  }
}

static void initLowBatteryTask() {
  xTimerStart(xTimerCreate("low_battery_handler", 10000/portTICK_PERIOD_MS, pdTRUE, nullptr, lowBatteryTask),0);
}

static void usbCall(tinyusb_msc_event_t *e){
  TaskHandle_t mainHandle = xTaskGetHandle("main");
  LOGI(TAG, "USB Callback %d", e->mount_changed_data.is_mounted);
  if(currentState == MAIN_NORMAL && !e->mount_changed_data.is_mounted){
    currentState = MAIN_USB;
    TaskHandle_t display_task_handle = xTaskGetHandle("display_task");
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NOTIFY_USB, eSetValueWithOverwrite);
    xTaskNotifyIndexed(mainHandle, 0, 0, eSetValueWithOverwrite);
  } else if(e->mount_changed_data.is_mounted && currentState == MAIN_USB){
    currentState = MAIN_NORMAL;
    xTaskNotifyIndexed(mainHandle, 0, 0, eSetValueWithOverwrite);
  }
}



extern "C" void app_main(void) {
  Boards::Board *board = get_board();
  switch(board->BootReason()){

    case Boards::WakeupSource::NONE:
      LOGI(TAG, "Wakeup Reason: None");
      break;
    case Boards::WakeupSource::VBUS:
      LOGI(TAG, "Wakeup Reason: VBUS");
      break;
    case Boards::WakeupSource::KEY:
      LOGI(TAG, "Wakeup Reason: KEY");
      break;
    case Boards::WakeupSource::REBOOT:
      LOGI(TAG, "Wakeup Reason: REBOOT");
      break;
  }
  if(!(board->BootReason() == Boards::WakeupSource::KEY || board->BootReason() == Boards::WakeupSource::REBOOT)){
    board->PowerOff();
  }

  board->LateInit();

  TaskHandle_t display_task_handle = nullptr;

  dumpDebugTimerInit();
  board->BootInfo();

  lvgl_init(board);

  initLowBatteryTask();

  vTaskDelay(500 / portTICK_PERIOD_MS); //Let USB Settle

#ifdef ESP_PLATFORM
  xTaskCreatePinnedToCore(display_task, "display_task", 5000, board, 2, &display_task_handle, 1);
#else
  xTaskCreate(display_task, "display_task", 5000, board, 2, &display_task_handle);
#endif

  initInputTimer(board);

  if (!board->StorageReady()) {
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NO_STORAGE, eSetValueWithOverwrite);
    while (true){
      return;
    }
  }

  MAIN_STATES oldState = MAIN_NONE;
  TaskHandle_t lvglHandle = xTaskGetHandle("LVGL");

  board->UsbCallBack(&usbCall);
  if(board->UsbConnected()){
    currentState = MAIN_USB;
  } else {
    currentState = MAIN_NORMAL;
    xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
  }

  while (true) {
    if (oldState != currentState) {
      //Handle state transitions
      LOGI(TAG, "State %d", currentState);
      switch (currentState) {
        case MAIN_NONE:
          break;
        case MAIN_NORMAL:
          if (oldState == MAIN_USB) {
            vTaskDelay(200 / portTICK_PERIOD_MS);
            xTaskNotifyIndexed(lvglHandle, 0, LVGL_STOP, eSetValueWithOverwrite);
            //Check for OTA File
            if (board->OtaCheck()) {
              xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_OTA, eSetValueWithOverwrite);
              vTaskDelay(100 / portTICK_PERIOD_MS);
              board->OtaInstall();
              currentState = MAIN_OTA;
              break;
            }
            else {
              xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_FILE, eSetValueWithOverwrite);
            }
          }
          break;
        case MAIN_USB:
          xTaskNotifyIndexed(display_task_handle, 0, DISPLAY_NOTIFY_USB, eSetValueWithOverwrite);
          vTaskDelay(100 / portTICK_PERIOD_MS);
          lvgl_wake_up();
          lvgl_usb_connected();
          break;
        default:
          break;
      }
      oldState = currentState;
    }

    xTaskNotifyWaitIndexed(0, 0, 0xffffffff, nullptr, portMAX_DELAY);
  }

}

#ifndef ESP_PLATFORM

void vLoggingPrintf( const char * pcFormat,
                     ... )
{
  va_list arg;

  va_start( arg, pcFormat );
  vprintf( pcFormat, arg );
  va_end( arg );
}

void vApplicationIdleHook( void )
{
  /* vApplicationIdleHook() will only be called if configUSE_IDLE_HOOK is set
   * to 1 in FreeRTOSConfig.h.  It will be called on each iteration of the idle
   * task.  It is essential that code added to this hook function never attempts
   * to block in any way (for example, call xQueueReceive() with a block time
   * specified, or call vTaskDelay()).  If application tasks make use of the
   * vTaskDelete() API function to delete themselves then it is also important
   * that vApplicationIdleHook() is permitted to return to its calling function,
   * because it is the responsibility of the idle task to clean up memory
   * allocated by the kernel to any task that has since deleted itself. */


  usleep( 15000 );
}

void vAssertCalled( const char * const pcFileName,
                    unsigned long ulLine )
{
  static BaseType_t xPrinted = pdFALSE;
  volatile uint32_t ulSetToNonZeroInDebuggerToContinue = 0;

  /* Called if an assertion passed to configASSERT() fails.  See
   * https://www.FreeRTOS.org/a00110.html#configASSERT for more information. */

  /* Parameters are not used. */
  ( void ) ulLine;
  ( void ) pcFileName;


  taskENTER_CRITICAL();
  {
    /* Stop the trace recording. */
    if( xPrinted == pdFALSE )
    {
      xPrinted = pdTRUE;

#if ( projENABLE_TRACING == 1 )
      {
                prvSaveTraceFile();
            }
#endif /* if ( projENABLE_TRACING == 0 ) */
    }

    /* You can step out of this function to debug the assertion by using
     * the debugger to set ulSetToNonZeroInDebuggerToContinue to a non-zero
     * value. */
    while( ulSetToNonZeroInDebuggerToContinue == 0 )
    {
      __asm volatile ( "NOP" );
      __asm volatile ( "NOP" );
    }
  }
  taskEXIT_CRITICAL();
}

void vApplicationMallocFailedHook( void )
{
  /* vApplicationMallocFailedHook() will only be called if
   * configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h.  It is a hook
   * function that will get called if a call to pvPortMalloc() fails.
   * pvPortMalloc() is called internally by the kernel whenever a task, queue,
   * timer or semaphore is created.  It is also called by various parts of the
   * demo application.  If heap_1.c, heap_2.c or heap_4.c is being used, then the
   * size of the    heap available to pvPortMalloc() is defined by
   * configTOTAL_HEAP_SIZE in FreeRTOSConfig.h, and the xPortGetFreeHeapSize()
   * API function can be used to query the size of free heap space that remains
   * (although it does not provide information on how the remaining heap might be
   * fragmented).  See http://www.freertos.org/a00111.html for more
   * information. */
  vAssertCalled( __FILE__, __LINE__ );
}

void vApplicationStackOverflowHook( TaskHandle_t pxTask,
                                    char * pcTaskName )
{
  ( void ) pcTaskName;
  ( void ) pxTask;

  /* Run time stack overflow checking is performed if
   * configCHECK_FOR_STACK_OVERFLOW is defined to 1 or 2.  This hook
   * function is called if a stack overflow is detected.  This function is
   * provided as an example only as stack overflow checking does not function
   * when running the FreeRTOS POSIX port. */
  vAssertCalled( __FILE__, __LINE__ );
}

#include <thread>
#include "drivers/display_sdl.h"
#include "drivers/key_sdl.h"
#include <SDL_events.h>
#include <unistd.h>
#include <sys/stat.h>

extern "C" int main(void) {
  //chroot the process, so it's closer to being on the device for paths, etc
  char path[255];
  getcwd(path, sizeof(path));
  printf("Chrooting to %s\n", path);
  unshare(CLONE_FS|CLONE_NEWUSER);
  chroot(path);
  chdir("/");
  mkdir("/data", 0700);

  displaySdl = new display_sdl();
  console_init();
  console_print("test\n");
//  signal( SIGINT, handle_sigint );
    xTaskCreate((void (*)(void*))app_main, "app_main", 10000, NULL, 1, NULL);
  std::thread schedular([](){
    vTaskStartScheduler();
  });

  while(1){
    displaySdl->update();
    if(get_board()->getKeys()) {
      dynamic_cast<keys_sdl *>(get_board()->getKeys())->poll();
    }
    usleep(1000);
  }

}

#endif