#include <Wire.h>
#include <QueueList.h>

struct data {
  double t;
  double val;
  double th;
};

unsigned long time;

int gyroPin_X = 1;               //Gyro is connected to analog pin 1
double gyroVoltage = 5;         //Gyro is running at 5V
double gyroZeroVoltage = 1.23;   //Gyro is zeroed at 1.23V
double gyroSensitivity = 2.5/1000;  // V/deg/sec

double theta_prev = 0.000;
double theta_cur = 0.000;
double omega_prev = 0;
double omega_cur = 0;
double t_prev = 0;

QueueList<data> all_data;

unsigned int counter = 0;
int num_samples = 0;
int total_samples = 100;

// Calibration stuff (don't touch)
int num_static = 0;
double static_cal = 0;
double calibration = 0;

void setup()
{
  Serial.begin(9600);
}

void loop() {
  time = micros();

  // Dynamic calibration 
  if(num_static == 10) // Static cal is done
  {
    Serial.println(static_cal, 3);  
    num_static = 0;
  } 
    

  counter++;
  if(counter % 100 == 0) // ensures that 100*num_samples is taken, but only report data at every 100th iteration
  {
    num_samples++;
  
  // Update previous
    theta_prev = theta_cur;
    omega_prev = omega_cur;

  // Update omega
    get_omega();

  // Calibration stuff
    calibration += omega_cur;
 }

  if(num_samples == total_samples)  
  {
      calibration /= num_samples;
      Serial.print("calibration: ", "\t");
      Serial.print("\t");
      Serial.print(calibration, 3);
      Serial.print("\t");
      Serial.print("static cal:");
      Serial.print("\t");
      Serial.print(static_cal, 3);
      Serial.println();
      
      num_samples = 0;
      if (num_static == 0)
      {
        static_cal -= calibration;
        num_static++;
      }
      else
      {
        num_static++;
        static_cal *= num_static;
        static_cal -= calibration;
        static_cal /= num_static;
      }         
  }    

}

void get_omega()
{
   //This line converts the 0-1023 signal to 0-5V
  omega_cur = (analogRead(gyroPin_X) * gyroVoltage) / 1023;

  //Account for zero voltage offset
  omega_cur -= gyroZeroVoltage;
  
  //This line divides the voltage we found by the gyro's sensitivity
  omega_cur /= gyroSensitivity;
  
  // extra static calibration 
  omega_cur += static_cal;
}


