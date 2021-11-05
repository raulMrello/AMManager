/*
 * metering_objects_UserConfig.h
 *
 *  Created on: Sep 2019
 *      Author: raulMrello
 *
 *	Selección de opciones para el módulo metering_objects en el proyecto Viaris UNI
 */
 
#ifndef METERING_OBJECTS_USER_CONFIG_Chargers
#define METERING_OBJECTS_USER_CONFIG_Chargers

// ANALIZADOR VIRTUAL (SIMULADOR)
#define VERS_METERING_VIRTUALAM								0
#define VERS_METERING_VIRTUALAM_NAME						(const char*)"Simulator"
#define VERS_METERING_VIRTUALAM_ANALYZERS					1

// ANALIZADORES DE LOS CONECTORES MENNEKES+SCHUKO
#define VERS_METERING_AM_UNI_CONNECTORS						1
#define VERS_METERING_AM_UNI_CONNECTORS_NAME				(const char*)"am-conn"
#define VERS_METERING_AM_UNI_CONNECTORS_ANALYZERS			4
#define VERS_METERING_AM_UNI_CONNECTORS_MEASCYCLE			2000
#define VERS_METERING_AM_UNI_CONNECTORS_1PHASE_ID			"1PH"
#define VERS_METERING_AM_UNI_CONNECTORS_3PHASE_ID			"3PH"

// ANALIZADORES CONTAX
#define VERS_METERING_AM_CTX_COMMON_NAME					(const char*)"am-ctx"

#define VERS_METERING_AM_CTX1								2
#define VERS_METERING_AM_CTX1_NAME							(const char*)"am-ctx1"
#define VERS_METERING_AM_CTX1_ANALYZERS						1
#define VERS_METERING_AM_CTX1_MEASCYCLE						2000
#define VERS_METERING_AM_CTX1_ADDR							1
#define VERS_METERING_AM_CTX1_MODEL_DDS353					2 //Driver_Ctx0643RegData::DriverRegModel::DDS353H2

#define VERS_METERING_AM_CTX3								3
#define VERS_METERING_AM_CTX3_NAME							(const char*)"am-ctx3"
#define VERS_METERING_AM_CTX3_ANALYZERS						3
#define VERS_METERING_AM_CTX3_MEASCYCLE						2000
#define VERS_METERING_AM_CTX3_ADDR_0643						151
#define VERS_METERING_AM_CTX3_ADDR_DTS353					1
#define VERS_METERING_AM_CTX3_MODEL_0643					0 //Driver_Ctx0643RegData::DriverRegModel::CTX_0643
#define VERS_METERING_AM_CTX3_MODEL_DTS353					2 //Driver_Ctx0643RegData::DriverRegModel::DTS353F2

// ANALIZADORES MBUS TMC100
#define VERS_METERING_AM_MBUS_COMMON_NAME					(const char*)"am-mbus"

#define VERS_METERING_AM_MBUS012							4
#define VERS_METERING_AM_MBUS012_NAME						(const char*)"am-mbus012"
#define VERS_METERING_AM_MBUS012_ANALYZERS					3
#define VERS_METERING_AM_MBUS012_MEASCYCLE					2000
static const uint8_t VERS_METERING_AM_MBUS012_SERIAL[][7] = {{0,0,0,0,0,0,0}, {1,0,0,0,0,0,0}, {2,0,0,0,0,0,0}};

#define VERS_METERING_AM_MBUS03								5
#define VERS_METERING_AM_MBUS03_NAME						(const char*)"am-mbus03"
#define VERS_METERING_AM_MBUS03_ANALYZERS					2
#define VERS_METERING_AM_MBUS03_MEASCYCLE					2000
static const uint8_t VERS_METERING_AM_MBUS03_SERIAL[][7] = {{0,0,0,0,0,0,0}, {3,0,0,0,0,0,0}};

#define VERS_METERING_AM_MBUS0								6
#define VERS_METERING_AM_MBUS0_NAME							(const char*)"am-mbus0"
#define VERS_METERING_AM_MBUS0_ANALYZERS					1
#define VERS_METERING_AM_MBUS0_MEASCYCLE					2000
static const uint8_t VERS_METERING_AM_MBUS0_SERIAL[][7] = 	{{0,0,0,0,0,0,0}};

#ifndef COMBI_PLUS
/** Versi�n seleccionada */
#define VERS_METERING_SELECTED			VERS_METERING_AM_UNI_CONNECTORS

/** Macro de generaci�n de nombre de versi�n */
static inline const char* VERS_METERING_NAME(){
	switch(VERS_METERING_SELECTED){
		case VERS_METERING_VIRTUALAM:			return VERS_METERING_VIRTUALAM_NAME;
		case VERS_METERING_AM_UNI_CONNECTORS:	return VERS_METERING_AM_UNI_CONNECTORS_NAME;
		default: 								return "";
	}
}
#endif


/** Versiones soportadas en el proyecto Viaris COMBIPLUS*/
// ANALIZADOR VIRTUAL (SIMULADOR)
#define VERS_METERING_VIRTUALAM								0
#define VERS_METERING_VIRTUALAM_NAME						(const char*)"Simulator"
#define VERS_METERING_VIRTUALAM_ANALYZERS					1

// ANALIZADORES DE LOS CONECTORES MENNEKES+SCHUKO
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS				1
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS_NAME			(const char*)"am-o"
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS_ANALYZERS		6
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS_MEASCYCLE		2000
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS_1PHASE_ID		"1PH"
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS_3PHASE_ID		"3PH"

#ifdef COMBI_PLUS
/** Versi�n seleccionada */
#define VERS_METERING_SELECTED			VERS_METERING_AM_COMBIPLUS_CONNECTORS

/** Macro de generaci�n de nombre de versi�n */
static inline const char* VERS_METERING_NAME(){
	switch(VERS_METERING_SELECTED){
		case VERS_METERING_VIRTUALAM:				return VERS_METERING_VIRTUALAM_NAME;
		case VERS_METERING_AM_COMBIPLUS_CONNECTORS:	return VERS_METERING_AM_COMBIPLUS_CONNECTORS_NAME;
		default: 									return "";
	}
}
#endif



#define METERING_TOPIC_TIME		"astcal"
#define METERING_DEFAULT_PLANT_VOLTAGE		(double)230.0
#define METERING_DEFAULT_PLANT_MINVOLTAGE	(double)(0.8 * 230.0)
#define METERING_DEFAULT_PLANT_FREQ			(double) 50.0
#define METERING_DEFAULT_PLANT_PFACTOR		(double)  0.9

#define METERING_POW_TO_CURRENT(pow)			((double)pow/METERING_DEFAULT_PLANT_VOLTAGE)
#define METERING_POW_TO_MILLIS(pow)				(int32_t)((pow * 1000)/METERING_DEFAULT_PLANT_VOLTAGE)
#define METERING_POW_TO_MILLIS_V(pow,volt)		(int32_t)((pow * 1000)/((volt > METERING_DEFAULT_PLANT_MINVOLTAGE)? volt : METERING_DEFAULT_PLANT_VOLTAGE))
#define METERING_MILLIS_TO_POW(millis)			(int32_t)round(((double)millis)*METERING_DEFAULT_PLANT_VOLTAGE/1000)
#define METERING_MILLIS_TO_POW_V(millis,volt)	(int32_t)round(((double)millis)*((volt > METERING_DEFAULT_PLANT_MINVOLTAGE)? volt : METERING_DEFAULT_PLANT_VOLTAGE)/1000)

#define METERING_PFACTOR_MIN_ALLOWED		(double)0.5


#endif
