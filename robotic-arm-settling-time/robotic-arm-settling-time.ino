#include <Wire.h>
#include <QueueList.h>

// Detect settling within 100 microns in worst-case scenario for Toshiba robotic arm in StackIT configuration
// Using Arduino Mega 2560 board and LPY510AL Gyroscope

// Struct definition
struct data {
  double t;      // time
  double omega;  // angular velocity [deg/sec]
  double th;     // filtered theta [deg]
  double th_raw; // raw theta value [deg]
};

// Gyroscope Constants from datasheet: https://www.pololu.com/file/0J239/lpy510al.pdf
int gyroPin_X = 1;                 //Gyro is connected to analog pin 1
double gyroVoltage = 5;            //Gyro is running at 5V
double gyroZeroVoltage = 1.23;     //Gyro is zeroed at 1.23V
double gyroSensitivity = 2.5/1000; // V/deg/sec

// Initializing Variables (previous and current values)
double theta_prev = 0.000;
double theta_cur = 0.000;
double omega_prev = 0;
double omega_cur = 0;
double theta_filter_prev = 0;
double theta_filter_cur = 0;
double t_prev = 0;
double omega_max = 0;
QueueList<data> all_data;
QueueList<data> settled;
QueueList<double> times;
unsigned long time;

// Debugging Variables
bool first_run = true;
double time_settled = 0;
bool light = false;
double time_moved = 0;

// Sampling variables
unsigned int counter = 0;
int num_samples = 0;
int total_samples = 320;
float period = 0;
float frequency = 0;

// Threshold variables
int suc_thresh = 0;
int fail_thresh = 0;
int suc = 0;
int fail = 0;

// Data taking variables
bool taking_data = false;   // print output to console for debugging
bool long_run = true;       // print output of long run with many settling times

// Filtering variables
double time_threshold = 0.2;  // 0.2 sec of settling
double percent_thresh = 0.90; // 90% acceptable threshold
double alpha = 0.980;         // Low pass filter smoothing factor alpha
double static_cal = 1.18;     // Static calobration constant
double deg_thresh = 0.020;    // 0.010 deg = 50 microns, 0.020 deg = 100 microns

void setup()
{
  Serial.begin(9600);
  while(Serial.available()==0) { }  // Wait for keypress to start taking data
  pinMode(11, OUTPUT);
  digitalWrite(11, LOW);
}

void loop() {
  time = micros();
  
  // Update previous
  theta_prev = theta_cur;
  omega_prev = omega_cur;
  theta_filter_prev = theta_filter_cur;

  // Update omega
  get_omega();

  // Initialize current data node
  data cur;
  cur.t = time/1000000.0;
  if (first_run)
    t_prev = cur.t;
  
  // theta calculations
  double dt = cur.t - t_prev;
  theta_cur = dt*omega_cur + theta_prev;

  // Sampling rate calculations
  if (counter > 2 && counter < 8)   // taking average of 3rd to 7th dt values
    period += dt;
  
  if (suc < 0)
    suc = 0;
  if (suc > time_threshold*frequency)
    suc = time_threshold*frequency;
  if (fail < 0)
    fail = 0;
  if (fail > time_threshold*frequency)
    fail = time_threshold*frequency;

  // Percentage criteria
  if (counter == 8)
  {
    period /= 5;
    frequency = 1/period;
    suc_thresh = time_threshold*frequency*percent_thresh;
    fail_thresh = time_threshold*frequency*(1.0-percent_thresh);
    suc = 0;
    fail = time_threshold*frequency;
  }

  // Storing data
  cur.omega = omega_cur;
  cur.th_raw = theta_cur;

  // Filtering
  
  // Low pass filter
  theta_filter_cur = (alpha*theta_filter_prev) + (alpha*(theta_cur - theta_prev));
  cur.th = theta_filter_cur;
  
  if (first_run)  {
    cur.th = cur.th_raw;  // For first run, do not filter since no prior filtered data
    first_run = false;
  }
  t_prev = cur.t;

  if (abs(cur.th) < deg_thresh)  {  // within 100 microns  
    time_settled += dt;
    suc++;
    fail--;
  }
  else  {    // not within 100 microns
    time_settled = 0;
    suc--;
    fail++;
  }

  // Saving data  
  if(counter % 30 == 0)// ensures that 100*num_samples is taken, but only report data at every 100th iteration
  {
    if(taking_data) {
    num_samples++;  
    all_data.push(cur);
    }
  }
  counter++;

  // Turn LED on/off for debugging
  if (suc >= suc_thresh && fail < fail_thresh && suc_thresh != 0 && !light) // LED is off, and a full 0.2 s (>1 period) is in threshold
  {
    settled.push(cur);
    light = true;    
    digitalWrite(11, HIGH);
    if(long_run)
    {
      times.push(cur.t - time_moved);
      omega_max = 0;
    }
  }
  
  if ( abs(omega_cur) > 10 && (light == true))
  {
    digitalWrite(11, LOW);
    light = false;
    omega_max = 0;                        // reset omega_max
  }

  if (abs(omega_cur) > omega_max )          // find largest, reset to 0 whenever settling is reached    use omega_max
  {
    omega_max = abs(omega_cur);
    time_moved = cur.t;
  }   

 // Print to console if taking data
  if (num_samples == total_samples)  
  {
    /*Serial.println("Freq, Time_threshold, suc_thresh, suc, fail_thresh, fail ");
    Serial.println(frequency);
    Serial.println(time_threshold);
    Serial.println(suc_thresh);
    Serial.println(suc);
    Serial.println(fail_thresh);
    Serial.println(fail);*/

     // Print results
    Serial.println("Time\tOmega\tTheta\tFilteredTheta");
    while(!all_data.isEmpty())
    {
      Serial.print(all_data.peek().t, 5);
      Serial.print("\t");
      Serial.print(all_data.peek().omega, 7);
      Serial.print("\t");
      Serial.print(all_data.peek().th_raw, 7);
      Serial.print("\t");
      Serial.print(all_data.peek().th, 7);
      Serial.println();
      all_data.pop();
    }
    
    Serial.println("settled");
    while(!settled.isEmpty())
    {
      Serial.println(settled.peek().t, 5);
      settled.pop();
    }
      
    while (1) {};
  } 

  // Print settling times for long run of data
  if (long_run)
  {
    if(times.count() > 50)
    {
      Serial.println("Settling times");
      while(!settled.isEmpty())
      {
        Serial.print(times.peek(), 5);
        Serial.print("\t");
        times.pop();
        times.pop();
      }
     
      while (1) {};
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
  
  // add static calibration 
  omega_cur += static_cal;

  
}
