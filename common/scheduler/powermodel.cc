#include <algorithm>
#include <iostream>
#include "powermodel.h"
#include "simulator.h"
#include "config.hpp"

using namespace std;

/**
 * Calculate the frequency that is expected to cause a power consumption as close as possible to the power budget, but still respecting it.
 */
int PowerModel::getExpectedGoodFrequency(int currentFrequency, float powerConsumption, float powerBudget, int minFrequency, int maxFrequency, int frequencyStepSize) {
	int expectedGoodFrequency = minFrequency;
	for (int f = minFrequency; f <= maxFrequency; f += frequencyStepSize) {
		float expectedPower = estimatePower(currentFrequency, powerConsumption, f);
		if (expectedPower <= powerBudget) {
			expectedGoodFrequency = f;
		}
	}

	if (expectedGoodFrequency > maxFrequency) {
		expectedGoodFrequency = maxFrequency;
	}
	if (expectedGoodFrequency < minFrequency) {
		expectedGoodFrequency = minFrequency;
	}
	return expectedGoodFrequency;
}

/** estimatePower
 * Get the estimated power consumption when switching to the new frequency.
 */
float PowerModel::estimatePower(int currentFrequency, float currentPowerConsumption, int newFrequency) {

	float staticFreqA = Sim()->getCfg()->getFloat("power/static_frequency_a") * 1000;
	float staticFreqB = Sim()->getCfg()->getFloat("power/static_frequency_b") * 1000;
	float staticPowerA = Sim()->getCfg()->getFloat("power/static_power_a");
	float staticPowerB = Sim()->getCfg()->getFloat("power/static_power_b");
	const float staticPowerM = (staticPowerB - staticPowerA) / (staticFreqB - staticFreqA);
	const float staticPowerOffset = staticPowerA - staticPowerM * staticFreqA;
	const float staticPower = staticPowerM * currentFrequency + staticPowerOffset;

	if (currentPowerConsumption <= staticPower) {
		currentPowerConsumption = staticPower;
	}
	float dynamicPower = currentPowerConsumption - staticPower;
	float a = dynamicPower / pow(currentFrequency, 3);
	float expectedPower = staticPowerM * newFrequency + staticPowerOffset + a * pow(newFrequency, 3);
	return expectedPower;
}
