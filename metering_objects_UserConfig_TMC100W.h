/*
 * metering_objects_UserConfig.h
 *
 *  Created on: Sep 2019
 *      Author: raulMrello
 *
 *	Selección de opciones para el módulo metering_objects en el proyecto TMC100W
 */

#ifndef METERING_OBJECTS_USER_CONFIG_TMC100W
#define METERING_OBJECTS_USER_CONFIG_TMC100W

/** M�ximo n�mero de analizadores permitidos */
#define MeteringManagerCfgMaxNumAnalyzers		1


#define VERS_METERING_AM_IVYD1C9002							10
#define VERS_METERING_AM_IVYD1C9002_NAME					(const char*)"am-IVYD1C9002"
#define VERS_METERING_AM_IVYD1C9002_ANALYZERS				1
#define VERS_METERING_AM_IVYD1C9002_MEASCYCLE				3000
#define VERS_METERING_AM_IVYD1C9002_ADDR					1


#define METERING_TOPIC_TIME		"astcal"
#define METERING_DEFAULT_PLANT_VOLTAGE		(double)230.0
#define METERING_DEFAULT_PLANT_MINVOLTAGE	(double)(0.8 * 230.0)
#define METERING_DEFAULT_PLANT_FREQ			(double) 50.0
#define METERING_DEFAULT_PLANT_PFACTOR		(double)  0.9

#define METERING_POW_TO_CURRENT(pow)			((double)pow/METERING_DEFAULT_PLANT_VOLTAGE)
#define METERING_POW_TO_MILLIS(pow)				(int32_t)round((double)(pow * 1000)/METERING_DEFAULT_PLANT_VOLTAGE)
#define METERING_POW_TO_MILLIS_V(pow,volt)		(int32_t)round((double)(pow * 1000)/((volt > METERING_DEFAULT_PLANT_MINVOLTAGE)? volt : METERING_DEFAULT_PLANT_VOLTAGE))
#define METERING_MILLIS_TO_POW(millis)			(int32_t)round(((double)millis)*METERING_DEFAULT_PLANT_VOLTAGE/1000)
#define METERING_MILLIS_TO_POW_V(millis,volt)	(int32_t)round(((double)millis)*((volt > METERING_DEFAULT_PLANT_MINVOLTAGE)? volt : METERING_DEFAULT_PLANT_VOLTAGE)/1000)

#define METERING_PFACTOR_MIN_ALLOWED		(double)0.5


#endif
