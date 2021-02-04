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

#ifndef COMBI_PLUS
/** Versiones soportadas en el proyecto Viaris UNI*/
#define VERS_METERING_VIRTUALAM						0
#define VERS_METERING_VIRTUALAM_NAME				(const char*)"Simulator"
#define VERS_METERING_VIRTUALAM_ANALYZERS			1

#define VERS_METERING_AM_UNI_CONNECTORS				1
#define VERS_METERING_AM_UNI_CONNECTORS_NAME		(const char*)"am-conn"
#define VERS_METERING_AM_UNI_CONNECTORS_ANALYZERS	4
#define VERS_METERING_AM_UNI_CONNECTORS_MEASCYCLE	2000
#define VERS_METERING_AM_UNI_CONNECTORS_1PHASE_ID	"1PH"
#define VERS_METERING_AM_UNI_CONNECTORS_3PHASE_ID	"3PH"

#define VERS_METERING_AM_SPL						2
#define VERS_METERING_AM_SPL_NAME					(const char*)"am-spl"
#define VERS_METERING_AM_SPL_ANALYZERS				3
#define VERS_METERING_AM_SPL_MEASCYCLE				2000

#define VERS_METERING_AM_MBUS						3
#define VERS_METERING_AM_MBUS_NAME					(const char*)"am-mbus"
#define VERS_METERING_AM_MBUS_ANALYZERS				3
#define VERS_METERING_AM_MBUS_MEASCYCLE				2000
static const uint8_t VERS_METERING_AM_MBUS_SERIAL[][7] = {{0,0,0,0,0,0,0}, {1,0,0,0,0,0,0}, {2,0,0,0,0,0,0}};


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


#else

/** Versiones soportadas en el proyecto Viaris COMBIPLUS*/
#define VERS_METERING_VIRTUALAM						0
#define VERS_METERING_VIRTUALAM_NAME				(const char*)"Simulator"
#define VERS_METERING_VIRTUALAM_ANALYZERS			1

#define VERS_METERING_AM_COMBIPLUS_CONNECTORS				1
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS_NAME			(const char*)"am-conn"
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS_ANALYZERS		6
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS_MEASCYCLE		2000
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS_1PHASE_ID		"1PH"
#define VERS_METERING_AM_COMBIPLUS_CONNECTORS_3PHASE_ID		"3PH"

#define VERS_METERING_AM_SPL						2
#define VERS_METERING_AM_SPL_NAME					(const char*)"am-spl"
#define VERS_METERING_AM_SPL_ANALYZERS				3
#define VERS_METERING_AM_SPL_MEASCYCLE				2000

#define VERS_METERING_AM_MBUS						3
#define VERS_METERING_AM_MBUS_NAME					(const char*)"am-mbus"
#define VERS_METERING_AM_MBUS_ANALYZERS				3
#define VERS_METERING_AM_MBUS_MEASCYCLE				2000
static const uint8_t VERS_METERING_AM_MBUS_SERIAL[][7] = {{0,0,0,0,0,0,0}, {1,0,0,0,0,0,0}, {2,0,0,0,0,0,0}};


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




#endif
