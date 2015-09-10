/*
Cette page récupere les informations du signal radio recu par le raspberry PI et execute une page PHP
en lui fournissant tout les paramêtres.

Vous pouvez compiler cette source via la commande :
	g++ radioReception.cpp -o radioReception -lwiringPi
	
	N'oubliez pas d'installer auparavant la librairie wiring pi ainsi que l'essentiel des paquets pour compiler

Vous pouvez lancer le programme via la commande :
	sudo chmod 777 radioReception
	./radioReception /var/www/radioReception/radioReception.php  7

	Les deux parametres de fin étant le chemin vers le PHP a appeller, et le numéro wiringPi du PIN relié au récepteur RF 433 mhz
	
@author : Valentin CARRUESCO (idleman@idleman.fr)
@contributors : Yann PONSARD, Jimmy LALANDE
@webPage : http://blog.idleman.fr
@references & Libraries: https://projects.drogon.net/raspberry-pi/wiringpi/, http://playground.arduino.cc/Code/HomeEasy
@licence : CC by sa (http://creativecommons.org/licenses/by-sa/3.0/fr/)
RadioPi de Valentin CARRUESCO (Idleman) est mis à disposition selon les termes de la 
licence Creative Commons Attribution - Partage dans les Mêmes Conditions 3.0 France.
Les autorisations au-delà du champ de cette licence peuvent être obtenues à idleman@idleman.fr.
*/


//#include <wiringPi.h>
#include <iostream>
#include <stdio.h>
#include <sys/time.h>
#include <time.h>
#include <stdlib.h>
#include <sched.h>
#include <sstream>
#include <cstring>
#include <math.h>


#include <unistd.h> // usleep
#include <fstream> // readfile
#include <string> // readfile

using namespace std;

//initialisation du pin de reception
int pin;

//Fonction de log
void log(string a){
	//Décommenter pour avoir les logs
	cout << endl << a << endl;
}

//Fonction de conversion long vers string
string longToString(long mylong){
    string mystring;
    stringstream mystream;
    mystream << mylong;
    return mystream.str();
}

//Fonction de passage du programme en temps réel (car la reception se joue a la micro seconde près)
void scheduler_realtime() {
	struct sched_param p;
	p.__sched_priority = sched_get_priority_max(SCHED_RR);
	if( sched_setscheduler( 0, SCHED_RR, &p ) == -1 ) {
	perror("Failed to switch to realtime scheduler.");
	}
}

//Fonction de remise du programme en temps standard
void scheduler_standard() {
	struct sched_param p;
	p.__sched_priority = 0;
	if( sched_setscheduler( 0, SCHED_OTHER, &p ) == -1 ) {
	perror("Failed to switch to normal scheduler.");
	}
}

int digitalRead(int pin) // pin not used
{
  string line;
  ifstream myfile ("/sys/class/gpio_sw/PA1/data");
  if (myfile.is_open())
  {
    getline (myfile,line);
    //cout << line << '\n';
    myfile.close();
  }

  return atoi(line.c_str());
}

//Recuperation du temp (en micro secondes) d'une pulsation
int pulseIn(int pin, int level, int timeout)
{
   struct timeval tn, t0, t1;
   long micros;
   gettimeofday(&t0, NULL);
   micros = 0;
   while (digitalRead(pin) != level)
   {
      gettimeofday(&tn, NULL);
      if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0;
      micros += (tn.tv_usec - t0.tv_usec);
      if (micros > timeout) return 0;
   }
   gettimeofday(&t1, NULL);
   while (digitalRead(pin) == level)
   {
      gettimeofday(&tn, NULL);
      if (tn.tv_sec > t0.tv_sec) micros = 1000000L; else micros = 0;
      micros = micros + (tn.tv_usec - t0.tv_usec);
      if (micros > timeout) return 0;
   }
   if (tn.tv_sec > t1.tv_sec) micros = 1000000L; else micros = 0;
   micros = micros + (tn.tv_usec - t1.tv_usec);
//cout << micros << '\n';
   return micros;
}

/*int bin2dec(char * bin)
{
	int dec;
	int pos = strlen(bin);
	
	while(pos--) {
		dec <<= 1;
		if (*bin == '1') {
			dec++;
		}
		bin++;
	}
	
	return dec;
}*/

int bin2dec(char* binary)
{
    int len,dec=0,i,exp;

    len = strlen(binary);
    exp = len-1;

    for(i=0;i<len;i++,exp--)
        dec += binary[i]=='1'?pow(2,exp):0;
    return dec;
}

//Programme principal
int main (int argc, char** argv)
{
//810010110100000010000111110100011011008
//810010110100000100000111110110011001108
//810000110100000000000111110000010100008
	char code[39];
	char lastCode[39];
	char channelCode[18];
	char channel1[18] = "81001011010000000";
	char humidity[7];
	char temperature[9];
	char bit;
	unsigned long t = 0;
	int len;
	
	scheduler_realtime();
	
	pin = 1;

	for(;;)
	{
		t = pulseIn(pin, 0, 1000000);
		bit = '\0';
		
		if(t > 3000 && t < 4500)
		{
			bit = '1';
		}
		else if(t > 600 && t < 2200)
		{
			bit = '0';
		}
		else if(t > 8000 && t < 9000)
		{
			bit = '8';
		}
		
		if (bit != '\0') {
			len = strlen(code);
			//cout << bit << endl;
			if (len == 39) {
				memmove(code, code+1, len);
				len--;
			}
			code[len] = bit;
			code[len+1] = '\0';
			if (code[0] == '8' && code[38] == '8') {
				strncpy(channelCode, code, 17);
				channelCode[17] = '\0';
				if (strcmp(channelCode, channel1) == 0) {
					if (strcmp(code, lastCode) == 0) {
						//cout << code << endl;
						strncpy(humidity, code+30, 7);
						humidity[7] = '\0';
						strncpy(temperature, code+20, 9);
						temperature[9] = '\0';
						//cout << "humidity: " << humidity << " temperature: " << temperature << endl;
						cout << "humidity: " << bin2dec(humidity) << " temperature: " << bin2dec(temperature)*0.1 << endl;
					}
					strcpy(lastCode, code);
				}
			}
		}
    }
	
	scheduler_standard();
}

