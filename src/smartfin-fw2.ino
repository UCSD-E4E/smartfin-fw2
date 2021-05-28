/*
 * Project smartfin-fw2
 * Description:
 * Author:
 * Date:
 */

#include <cstdint>

#include "Particle.h"
#include "Serial5/Serial5.h"

#include "conio.hpp"
#include "cli.hpp"
#include "product.hpp"
#include "states.hpp"
#include "task.hpp"
#include "system.hpp"
#include "dataUpload.hpp"
#include "sleepTask.hpp"
#include "chargeTask.hpp"
#include "ride.hpp"
#include "vers.hpp"
// PRODUCT_VERSION(FW_MAJOR_VERSION << 8 | FW_MINOR_VERSION)

#define SF_DEBUG

typedef struct StateMachine_
{
  STATES_e state;
  Task* task;
}StateMachine_t;

static CLI cliTask;
static DataUpload uploadTask;
static SleepTask sleepTask;
static ChargeTask chargeTask;
static RideInitTask rideInitTask;
static RideTask rideTask;

static StateMachine_t stateMachine[] = 
{
  {STATE_CLI, &cliTask},
  {STATE_UPLOAD, &uploadTask},
  {STATE_DEEP_SLEEP, &sleepTask},
  {STATE_CHARGE, &chargeTask},
  {STATE_SESSION_INIT, &rideInitTask},
  {STATE_DEPLOYED, &rideTask},
  {STATE_NULL, NULL}
};

static STATES_e currentState;

static void initializeTaskObjects(void);
static StateMachine_t* findState(STATES_e state);
static void printState(STATES_e state);

static retained uint32_t RESET_GOOD;
static retained uint32_t nRESET_GOOD;

void mainThread(void* args);

SYSTEM_MODE(MANUAL);
SYSTEM_THREAD(ENABLED);

// setup() runs once, when the device is first turned on.
void setup()
{
  int i;
  // Put initialization like pinMode and begin functions here.

  // Doing Serial initialization here so that we have this available immediately
  Serial.begin(SERIAL_DEBUG_BAUD_RATE);
  // This protects against a boot loop due to idiot programming
  if(RESET_GOOD != ~nRESET_GOOD)
  {
    // if in boot loop, that's 15 seconds to save everything
    if(System.resetReason() == RESET_REASON_POWER_DOWN)
    {

    }
    else
    {
      SF_OSAL_printf("Waiting ");
      for(i = 0; i < 15; i++)
      {
        SF_OSAL_printf("%d  ", i);
        delay(1000);
      }
      SF_OSAL_printf("\n");
    }
  }
  RESET_GOOD = 0;
  nRESET_GOOD = 0;

  SYS_initSys();

  initializeTaskObjects();
  SF_OSAL_printf("Reset Reason: %d\n", System.resetReason());
  SF_OSAL_printf("Starting Device: %s\n", pSystemDesc->deviceID);

  SF_OSAL_printf("Battery = %lf %%\n", pSystemDesc->battery->getSoC());
  SF_OSAL_printf("Battery = %lf V\n", pSystemDesc->battery->getVCell());

  SF_OSAL_printf("Time.now() = %ld\n", Time.now());

  RESET_GOOD = 0xAA55;
  nRESET_GOOD = ~RESET_GOOD;

}

// loop() runs over and over again, as quickly as it can execute.
void loop()
{
  mainThread(NULL);
}

void mainThread(void* args)
{
  StateMachine_t* pState;
  SF_OSAL_printf("Starting main thread\n");
  while(1)
  {
    pState = findState(currentState);
    if(NULL == pState)
    {
      pState = findState(SF_DEFAULT_STATE);
    }
  #ifdef SF_DEBUG
    SF_OSAL_printf("Initializing State ");
    printState(pState->state);
    SF_OSAL_printf("\n");
  #endif
    pState->task->init();
    #ifdef SF_DEBUG
    SF_OSAL_printf("Executing state body\n");
    #endif
    currentState = pState->task->run();
    #ifdef SF_DEBUG
    SF_OSAL_printf("Next State: ");
    printState(currentState);
    SF_OSAL_printf("\n");
    #endif
    pState->task->exit();
    #ifdef SF_DEBUG
    SF_OSAL_printf("Exit complete\n");
    #endif
  }
}

static void initializeTaskObjects(void)
{
  currentState = SF_DEFAULT_STATE;
  // SleepTask::loadBootBehavior();

  // switch(SleepTask::getBootBehavior())
  // {
  // default:
  // case SleepTask::BOOT_BEHAVIOR_NORMAL:
  //   if(digitalRead(SF_USB_PWR_DETECT_PIN) == HIGH)
  //   {
  //     currentState = STATE_UPLOAD;
  //   }
  //   else
  //   {
  //     currentState = SF_DEFAULT_STATE;
  //   }
  //   break;
  // case SleepTask::BOOT_BEHAVIOR_TMP_CAL_START:
  // case SleepTask::BOOT_BEHAVIOR_TMP_CAL_CONTINUE:
  //   currentState = STATE_TMP_CAL;
  //   break;
  // case SleepTask::BOOT_BEHAVIOR_TMP_CAL_END:
  //   currentState = STATE_CLI;
  //   break;
  // case SleepTask::BOOT_BEHAVIOR_UPLOAD_REATTEMPT:
  //   // TODO: Fix to allow waking up into deployment
  //   currentState = STATE_UPLOAD;
  //   break;
  // }
}

static StateMachine_t* findState(STATES_e state)
{
  StateMachine_t* pStates;
  for(pStates = stateMachine; pStates->task; pStates++)
  {
    if(pStates->state == state)
    {
      return pStates;
    }
  }
  return NULL;
}

static void printState(STATES_e state)
{
  switch(state)
  {
    case STATE_DEEP_SLEEP:
    SF_OSAL_printf("STATE_DEEP_SLEEP");
    break;
    case STATE_SESSION_INIT:
    SF_OSAL_printf("STATE_SESSION_INIT");
    break;

    case STATE_DEPLOYED:
    SF_OSAL_printf("STATE_DEPLOYED");
    break;

    case STATE_MFG_TEST:
    SF_OSAL_printf("STATE_MFG_TEST");
    break;

    case STATE_CLI:
    SF_OSAL_printf("STATE_CLI");
    break;
    case STATE_CHARGE:
    SF_OSAL_printf("STATE_CHARGE");
    break;
    case STATE_TMP_CAL:
    SF_OSAL_printf("STATE_TMP_CAL");
    break;
    case STATE_UPLOAD:
    SF_OSAL_printf("STATE_UPLOAD");
    break;
    default:
    SF_OSAL_printf("UNKNOWN");
    break;
  }
}