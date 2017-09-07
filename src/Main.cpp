#include "main.h"

#define  numberOfPid  3
// reportLength max size is 64 for HID
Ticker pidTimer;
static PIDBowler*  pid[numberOfPid];
HIDSimplePacket coms;
float  calibrations[3] = {1847,1992,1155};
//float  calibrations[3] = {114,784,-10};
//these are fake
AnalogOut test(PA_4); //Instantiates pin PA_4 as an AnalogOut for the DAC
void runPid(){
  // update all positions fast and together
  for (int i=0;i<numberOfPid;i++)
     pid[i]->updatePosition();
 // next update all control outputs
  for (int i=0;i<numberOfPid;i++)
      pid[i]->updateControl();
}

int main() {
	printf("\r\n\r\n Top of Main \r\n\r\n");

#if defined(DUMMYLINKS)
   pid[0] =(PIDBowler*) new DummyPID();
   pid[1] =(PIDBowler*) new DummyPID();
   pid[2] =(PIDBowler*) new DummyPID();
#else
   SPI * spiDev = new SPI(MOSI, MISO, CLK);
   pid[0] = new PIDimp( new Servo(SERVO_1, 5),
                         new AS5050(spiDev, ENC_1));  // mosi, miso, sclk, cs
   pid[1] = new PIDimp( new Servo(SERVO_2, 5),
                         new AS5050(spiDev, ENC_2));  // mosi, miso, sclk, cs
   pid[2] = new PIDimp( new Servo(SERVO_3, 5),
                         new AS5050(spiDev, ENC_3));  // mosi, miso, sclk, cs
#endif

   // Invert the direction of the motor vs the input
   //pid[0]->state.config.Polarity = true;
   for (int i=0;i<numberOfPid;i++){
     pid[i]->state.config.Enabled=false;// disable PID to start with
   }
   wait_ms(500);// Cosines delay
   pidTimer.attach(&runPid, 0.005);
   // capture 100 ms of encoders before starting
   wait_ms(100);
   for (int i=0;i<numberOfPid;i++){
     //reset after encoders have been updated a few times
     pid[i]->InitilizePidController();
     #if defined(DUMMYLINKS)
        pid[i]->ZeroPID();// set the current encoder value to 0
                       // this should be replaced by calibration routine
     #else
       if(pid[i]->GetPIDPosition()>2048){
         pid[i]->pidReset(pid[i]->GetPIDPosition()-4095);
       }
       //apply calibrations
       pid[i]->pidReset(pid[i]->GetPIDPosition()-calibrations[i]);
     #endif

     pid[i]->SetPIDEnabled( true);// Enable PID to start control
     pid[i]->SetPIDTimed(0, 1000);
   }

/*
   // Run PID controller calibration
   // only one can be run at a time, and the control loop needs to wait
   // until the calibration is done before advancing to the next
   pid[0]->runPidHysterisisCalibration();
   // Run a homing procedure down where 123 is the value of the encoder at home
   pid[0]->startHomingLink( CALIBRARTION_home_down, 0);
   // Run a homing procedure up  where 123 is the value of the encoder at home
   pid[0]->startHomingLink( CALIBRARTION_home_up, 0);
   // Run a homing procedure to scale the velocity outputs  where 123 is the value of the encoder at home
   pid[0]->startHomingLink( CALIBRARTION_home_velocity, 0);
*/
   coms.attach(new PidServer2 (pid, numberOfPid ));
   printf("\n\n Starting Core \n\n");
   RunEveryObject* print = new RunEveryObject(0,500);
    while(1) {
        coms.server();
        if(print->RunEvery(pid[0]->getMs())>0){
          printf("\r\nEncoder Value = %f , %f , %f",
          pid[0]->GetPIDPosition(),
          pid[1]->GetPIDPosition(),
          pid[2]->GetPIDPosition());
          float analogValue = (pid[0]->GetPIDPosition()+987)/2290; //Converts the encoder values from ticks to an analog float from 0-1
          test.write(analogValue); //Writes the calculated analogValue to pin PA_4
        }


    }
}
