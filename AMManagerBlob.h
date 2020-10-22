/*
 * AMManagerBlob.h
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 *
 */
 
#ifndef __AMManagerBlob__H
#define __AMManagerBlob__H

#include "Blob.h"
#include "mbed.h"
#include "metering_objects.h"

  

namespace Blob {

// límites para descartar fallos en la lectura de los analizadores con interferencias
static const uint32_t AMMaxAllowedVoltage = 300;
static const uint32_t AMMaxAllowedCurrent = 40;

/** Estructura de datos para actualizar los porcentajes de activación de las cargas relativos
 *  a los diferentes analizadores
 * 	@var ids Lista de indicadores. Un id=-1 indica no utilizado
 * 	@var loads Nuevos valores de activación de los analizadores indexados con ids[]
 */
struct __packed AMLoadData_t{
	int8_t ids[MeteringManagerCfgMaxNumAnalyzers];
	uint8_t loads[MeteringManagerCfgMaxNumAnalyzers];
};

/** Tipo definido para referenciar la trama de arranque con la información del objeto metering:manager */
typedef metering_manager AMBootData_t;


}	// end namespace Blob


namespace JSON {

/**
 * Codifica el objeto  en un JSON de tipo array:
 * [
 * 	 {"id": 0, "loadPercent": 0},
 * 	 {"id": 1, "loadPercent": 0},
 * 	 ...
 * ]
 * @param obj Objeto a codificar
 * @return JSON o NULL en caso de error
 */
cJSON* getJsonFromAMLoad(const Blob::AMLoadData_t& obj);


/**
 * Decodifica el mensaje JSON en un objeto de arranque
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getAMLoadFromJson(Blob::AMLoadData_t &obj, cJSON* json);


}	// end namespace JSON



#endif
