#include <Wire.h>
#include <Servo.h>


Servo right_prop;
Servo left_prop;

int16_t Acc_rawX, Acc_rawY, Acc_rawZ,Gyr_rawX, Gyr_rawY, Gyr_rawZ;
 

float Acceleration_angle[2];
float Gyro_angle[2];
float Total_angle[2];


float elapsedTime, time, timePrev;
int i;
float rad_to_deg = 180/3.141592654;

float PID, pwmLeft, pwmRight, error, previous_error;
float pid_p=0;
float pid_i=0;
float pid_d=0;


/////////////////PID CONSTANTS/////////////////
double kp=2;
double ki=0.01;
double kd=0;
///////////////////////////////////////////////

double throttle=1300; //initial value of throttle to the motors
float desired_angle = 0; //This is the angle in which we whant the
                         //balance to stay steady


void setup() {
  Wire.begin(); //begin the wire comunication
  Wire.beginTransmission(0x68);
  Wire.write(0x6B);
  Wire.write(0);
  Wire.endTransmission(true);
  Serial.begin(250000);
  right_prop.attach(3);
  left_prop.attach(5);

  time = millis(); //Start counting time in milliseconds

  left_prop.writeMicroseconds(1000); 
  right_prop.writeMicroseconds(1000);
  delay(7000); 
}

void loop() {

    timePrev = time;  // the previous time is stored before the actual time read
    time = millis();  // actual time read
    elapsedTime = (time - timePrev) / 1000; 
  
 
   
     Wire.beginTransmission(0x68);
     Wire.write(0x3B); //Ask for the 0x3B register- correspond to AcX
     Wire.endTransmission(false);
     Wire.requestFrom(0x68,6,true); 
   
   /*We have asked for the 0x3B register. The IMU will send a brust of register.
    * The amount of register to read is specify in the requestFrom function.
    * In this case we request 6 registers. Each value of acceleration is made out of
    * two 8bits registers, low values and high values. For that we request the 6 of them  
    * and just make then sum of each pair. For that we shift to the left the high values 
    * register (<<) and make an or (|) operation to add the low values.*/
    
     Acc_rawX=Wire.read()<<8|Wire.read(); //each value needs two registres
     Acc_rawY=Wire.read()<<8|Wire.read();
     Acc_rawZ=Wire.read()<<8|Wire.read();
     
     Acceleration_angle[0] = atan((Acc_rawY/16384.0)/sqrt(pow((Acc_rawX/16384.0),2) + pow((Acc_rawZ/16384.0),2)))*rad_to_deg;
  
     Acceleration_angle[1] = atan(-1*(Acc_rawX/16384.0)/sqrt(pow((Acc_rawY/16384.0),2) + pow((Acc_rawZ/16384.0),2)))*rad_to_deg;
 
  
    
   Wire.beginTransmission(0x68);
   Wire.write(0x43); //Gyro data first adress
   Wire.endTransmission(false);
   Wire.requestFrom(0x68,4,true); //Just 4 registers
   
   Gyr_rawX=Wire.read()<<8|Wire.read(); //Once again we shif and sum
   Gyr_rawY=Wire.read()<<8|Wire.read();
 
   /*Now in order to obtain the gyro data in degrees/seconda we have to divide first
   the raw value by 131 because that's the value that the datasheet gives us*/

   /*X-axis*/
   Gyro_angle[0] = Gyr_rawX/131.0; 
   /*Y-axis*/
   Gyro_angle[1] = Gyr_rawY/131.0;

   /*Now in order to obtain degrees we have to multiply the degree/seconds
   *value by the elapsedTime.*/
   /*Finnaly we can apply the final filter where we add the acceleration
   *part that afects the angles and ofcourse multiply by 0.98 */

   /*X-axis*/
   Total_angle[0] = 0.98 *(Total_angle[0] + Gyro_angle[0]*elapsedTime) + 0.02*Acceleration_angle[0];
   /*Y-axis*/
   Total_angle[1] = 0.98 *(Total_angle[1] + Gyro_angle[1]*elapsedTime) + 0.02*Acceleration_angle[1];
   
   /*Now we have our angles in degree and values from -10??0 to 100?? aprox*/
    //Serial.println(Total_angle[1]);

   
  
/*///////////////////////////P I D///////////////////////////////////*/

/*First calculate the error between the desired angle and 
*the real measured angle*/
error = Total_angle[1] - desired_angle;
    
/*Next the proportional value of the PID is just a proportional constant
*multiplied by the error*/

pid_p = kp*error;

if(-3 <error <3)
{
  pid_i = pid_i+(ki*error);  
}

pid_d = kd*((error - previous_error)/elapsedTime);

/*The final PID values is the sum of each of this 3 parts*/
PID = pid_p + pid_i + pid_d;

if(PID < -1000)
{
  PID=-1000;
}
if(PID > 1000)
{
  PID=1000;
}

/*Finnaly we calculate the PWM width. We sum the desired throttle and the PID value*/
pwmLeft = throttle + PID;
pwmRight = throttle - PID;



//Right
if(pwmRight < 1000)
{
  pwmRight= 1000;
}
if(pwmRight > 2000)
{
  pwmRight=2000;
}
//Left
if(pwmLeft < 1000)
{
  pwmLeft= 1000;
}
if(pwmLeft > 2000)
{
  pwmLeft=2000;
}

left_prop.writeMicroseconds(pwmLeft);
right_prop.writeMicroseconds(pwmRight);
previous_error = error; //Remember to store the previous error.

}
