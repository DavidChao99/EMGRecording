// Copyright (C) 2013-2014 Thalmic Labs Inc.
// Distributed under the Myo SDK license agreement. See LICENSE.txt for details.

// This sample illustrates how to use EMG data. EMG streaming is only supported for one Myo at a time.

#include "stdafx.h"
#include <array>
#include <iostream>
#include <sstream>
#include <stdexcept>
#include <string>
#include <cmath>
#include <iomanip>
#include <fstream>
#include <time.h>

#include <myo/myo.hpp>

int done = 0;

class DataCollector : public myo::DeviceListener {
public:
	DataCollector()
		: emgSamples()
	{
		openFiles();
	}

	void openFiles() {
		time_t timestamp = std::time(0);

		// Open file for EMG log
		if (emgFile.is_open()) {
			emgFile.close();
		}
		std::ostringstream emgFileString;
		emgFileString << "emg-" << timestamp << ".csv";
		emgFile.open(emgFileString.str(), std::ios::out);
		emgFile << "timestamp" << std::endl;
	}

	// onUnpair() is called whenever the Myo is disconnected from Myo Connect by the user.
	void onUnpair(myo::Myo* myo, uint64_t timestamp)
	{
		// We've lost a Myo.
		// Let's clean up some leftover state.
		emgSamples.fill(0);
	}

	void onConnect(myo::Myo *myo, uint64_t timestamp, myo::FirmwareVersion firmwareVersion) {
		//Reneable streaming
		openFiles();
	}

	// onEmgData() is called whenever a paired Myo has provided new EMG data, and EMG streaming is enabled.
	void onEmgData(myo::Myo* myo, uint64_t timestamp, const int8_t* emg)
	{	
		if (counter < 250) {
			//Store the emg data in emgSamples2[pod][index]
			for (int i = 0; i < 8; i++) {
				emgSamples2[i][counter] = emg[i];
			}
			//Adds to moving average. Takes previous sixty sums of emg data, adds up and divides by 60
			movingAvg[counter] = 0;
			for (int i = counter - 60; i < counter; i++) {
				if (i > 0) {
					for (int j = 0; j < 8; j++) {
						movingAvg[counter] += emgSamples2[j][i]*emgSamples2[j][i];
					}
				}
			}
			movingAvg[counter] = movingAvg[counter] / 60;

			timestamps[counter] = timestamp;
		}
		counter++;
	}

	// There are other virtual functions in DeviceListener that we could override here, like onAccelerometerData().
	// For this example, the functions overridden above are sufficient.

	// We define this function to print the current values that were updated by the on...() functions above.
	void print()
	{
		int printed = 0;
		// Clear the current line
		for (int j = 0; j < counter; j++) {
			//if (movingAvg[j] > 800 && done == 0)
			//{
				std::cout << '\r';
				std::cout << '\n' << "Counter: " << counter;
				std::cout << "Moving Avg: " << movingAvg[j];
				emgFile << movingAvg[j];
				std::cout << '\n' << timestamps[j];
				// Print out the EMG data
				for (size_t i = 0; i < 8; i++) {
					//std::cout << '\n' << i;
					std::cout << '[' << emgSamples2[i][j] << ']';
					emgFile::cout << ',' << emgSamples2[i][j];
				}
				std::cout << '\n';
				emgFile << std::endl;

				std::cout << std::flush;
				printed = 1;
			//}
		}
		if (printed == 1)
		{
			done = 1;
		}
		counter = 0;
	}

	// The values of this array is set by onEmgData() above.
	std::array<int8_t, 8> emgSamples;
	// Better to use standard C arrays for 2Dimensional, less work than CPP arrays
	int emgSamples2[8][250];
	double movingAvg[250];
	uint64_t timestamps[250];
	int counter = 0;
	std::ofstream emgFile;
};

int main(int argc, char** argv)
{
	// We catch any exceptions that might occur below -- see the catch statement for more details.
	try {

		// First, we create a Hub with our application identifier. Be sure not to use the com.example namespace when
		// publishing your application. The Hub provides access to one or more Myos.
		myo::Hub hub("com.example.emg-data-sample");

		std::cout << "Attempting to find a Myo..." << std::endl;

		// Next, we attempt to find a Myo to use. If a Myo is already paired in Myo Connect, this will return that Myo
		// immediately.
		// waitForMyo() takes a timeout value in milliseconds. In this case we will try to find a Myo for 10 seconds, and
		// if that fails, the function will return a null pointer.
		myo::Myo* myo = hub.waitForMyo(10000);

		// If waitForMyo() returned a null pointer, we failed to find a Myo, so exit with an error message.
		if (!myo) {
			throw std::runtime_error("Unable to find a Myo!");
		}

		// We've found a Myo.
		std::cout << "Connected to a Myo armband!" << std::endl << std::endl;
		std::cout << "Hello there how are you today" << std::endl << std::endl;
		// Next we enable EMG streaming on the found Myo.
		myo->setStreamEmg(myo::Myo::streamEmgEnabled);

		// Next we construct an instance of our DeviceListener, so that we can register it with the Hub.
		DataCollector collector;

		// Hub::addListener() takes the address of any object whose class inherits from DeviceListener, and will cause
		// Hub::run() to send events to all registered device listeners.
		hub.addListener(&collector);
		std::cin.get();

		//std::cout << "HI" << std::endl << std::endl;

		// Finally we enter our main loop.
		//while (1) {
			// In each iteration of our main loop, we run the Myo event loop for a set number of milliseconds.
			// In this case, we wish to update our display 50 times a second, so we run for 1000/20 milliseconds.
			hub.run(250);
			//myo->vibrate(myo::Myo::vibrationLong);
			// After processing events, we call the print() member function we defined above to print out the values we've
			// obtained from any events that have occurred.
			//std::cout << "Printing " << std::endl << std::endl;
			collector.print();
		//}

		// If a standard exception occurred, we print out its message and exit.
	}
	catch (const std::exception& e) {
		std::cerr << "Error: " << e.what() << std::endl;
		std::cerr << "Press enter to continue.";
		std::cin.ignore();
		return 1;
	}
}
