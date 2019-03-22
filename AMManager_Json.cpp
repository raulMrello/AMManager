/*
 * AMManager_JSONParser.cpp
 *
 * Implementación de los codecs JSON-OBJ
 *
 *  Created on: Feb 2019
 *      Author: raulMrello
 */

#include "JsonParserBlob.h"

//------------------------------------------------------------------------------------
//-- PRIVATE TYPEDEFS ----------------------------------------------------------------
//------------------------------------------------------------------------------------

static const char* _MODULE_ = "[AMM]...........";
#define _EXPR_	(!IS_ISR())


namespace JSON{


//------------------------------------------------------------------------------------
cJSON* getJsonFromAMLoad(const Blob::AMLoadData_t& data) {
	cJSON* json = NULL;

	// array de objetos [{id, loadPercent}]
	cJSON* array = NULL;
	if((array=cJSON_CreateArray()) == NULL){
		return NULL;
	}
	for(int i=0; i<MeteringManagerCfgMaxNumAnalyzers; i++){
		if(data.ids[i] == -1){
			break;
		}
		cJSON* item = NULL;
		if((item=cJSON_CreateObject()) == NULL){
			cJSON_Delete(array);
			return NULL;
		}
		cJSON_AddNumberToObject(item, JsonParser::p_id, data.ids[i]);
		cJSON_AddNumberToObject(item, JsonParser::p_loadPercent, data.loads[i]);
		cJSON_AddItemToArray(array, item);
	}
	return array;
}


//------------------------------------------------------------------------------------
uint32_t getAMLoadFromJson(Blob::AMLoadData_t &data, cJSON* json){
	// borro el resultado antes de iniciar
	for(int i=0;i<MeteringManagerCfgMaxNumAnalyzers;i++){
		data.ids[i] = -1;
	}

	// leo el array de objetos [{id, loadPercent}]
	uint8_t items = cJSON_GetArraySize(json);
	if(items <= MeteringManagerCfgMaxNumAnalyzers){
		for(int i=0;i<items;i++){
			cJSON* item = cJSON_GetArrayItem(json, i);
			if(item == NULL)
				return 0;
			cJSON* value = NULL;
			if((value = cJSON_GetObjectItem(item, JsonParser::p_id)) != NULL){
				uint8_t id = value->valueint;
				if((value = cJSON_GetObjectItem(item, JsonParser::p_loadPercent)) != NULL){
					data.ids[i] = id;
					data.loads[i] = value->valueint;
				}
			}
		}
	}
	return 1;
}

}	// end namespace JSON

