/*
 * ppl_energy_objects.cpp
 *
 * Implementaci�n de los codecs JSON-OBJ
 *
 *  Created on: Feb 2019
 *      Author: raulMrello
 */

#include "JsonParserBlob.h"

//------------------------------------------------------------------------------------
//-- PRIVATE TYPEDEFS ----------------------------------------------------------------
//------------------------------------------------------------------------------------

static const char* _MODULE_ = "[metering:].....";
#define _EXPR_	(!IS_ISR())


namespace JSON{


//------------------------------------------------------------------------------------
cJSON* getJsonFromMeteringManager(const metering_manager& obj, ObjDataSelection type){
	cJSON* json = NULL;
	cJSON* item = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// uid
	cJSON_AddNumberToObject(json, JsonParser::p_uid, obj.uid);

	// cfg
	if(type != ObjSelectState && type != ObjSelectStateSub){
		if((item = getJsonFromMeteringManagerCfg(obj.cfg)) == NULL){
			cJSON_Delete(json);
			return NULL;
		}
		cJSON_AddItemToObject(json, JsonParser::p_cfg, item);
	}

	// stat
	if(type != ObjSelectCfg && type != ObjSelectStateSub){
		if((item = getJsonFromMeteringManagerStat(obj.stat)) == NULL){
			cJSON_Delete(json);
			return NULL;
		}
		cJSON_AddItemToObject(json, JsonParser::p_stat, item);
	}

	// analyzers
	cJSON* array = NULL;
	if((array=cJSON_CreateArray()) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	for(int i=0; i<obj._numAnalyzers; i++){
		cJSON* item = NULL;
		if((item = getJsonFromMeteringAnalyzer(obj.analyzers[i], type)) == NULL){
			cJSON_Delete(array);
			cJSON_Delete(json);
			return NULL;
		}
		cJSON_AddItemToArray(array, item);
	}
	cJSON_AddItemToObject(json, JsonParser::p_analyzers, array);
	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromMeteringManagerCfg(const metering_manager_cfg& obj){
	cJSON* json = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// updFlags
	cJSON_AddNumberToObject(json, JsonParser::p_updFlags, obj.updFlags);
	// measPeriod
	cJSON_AddNumberToObject(json, JsonParser::p_measPeriod, obj.measPeriod);
	// verbosity
	cJSON_AddNumberToObject(json, JsonParser::p_verbosity, obj.verbosity);

	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromMeteringManagerStat(const metering_manager_stat& obj){
	cJSON* json = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// loadPercentage
	cJSON* array = NULL;
	if((array=cJSON_CreateArray()) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	for(int i=0; i<obj._numAnalyzers; i++){
		cJSON* item = NULL;
		if((item = cJSON_CreateNumber(obj.loadPercent[i])) == NULL){
			cJSON_Delete(array);
			cJSON_Delete(json);
			return NULL;
		}
		cJSON_AddItemToArray(array, item);
	}
	cJSON_AddItemToObject(json, JsonParser::p_loadPercent, array);

	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromMeteringAnalyzer(const metering_analyzer& obj, ObjDataSelection type){
	cJSON* json = NULL;
	cJSON* item = NULL;

	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// uid
	cJSON_AddNumberToObject(json, JsonParser::p_uid, obj.uid);

	// serial
	cJSON_AddStringToObject(json, JsonParser::p_serial, obj.serial);

	// cfg
	if(type != ObjSelectState && type != ObjSelectStateSub){
		if((item = getJsonFromMeteringAnalyzerCfg(obj.cfg)) == NULL){
			cJSON_Delete(json);
			return NULL;
		}
		cJSON_AddItemToObject(json, JsonParser::p_cfg, item);
	}

	// stat
	if(type != ObjSelectCfg){
		if((item = getJsonFromMeteringAnalyzerStat(obj.stat)) == NULL){
			cJSON_Delete(json);
			return NULL;
		}
		cJSON_AddItemToObject(json, JsonParser::p_stat, item);
	}
	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromMeteringAnalyzerCfg(const metering_analyzer_cfg& obj){
	cJSON* json = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// updFlags
	cJSON_AddNumberToObject(json, JsonParser::p_updFlags, obj.updFlags);
	// evtFlags
	cJSON_AddNumberToObject(json, JsonParser::p_evtFlags, obj.evtFlags);
	// minmaxData
	/*cJSON* item = NULL;
	if((item = getJsonFromMeteringAnalyzerCfgMinMax(obj.minmaxData)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_minmaxData, item);
	// calibData
	if((item = getJsonFromMeteringAnalyzerCfgCalib(obj.calibData)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_calibData, item);*/
	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromMeteringAnalyzerCfgMinMax(const metering_analyzer_cfg_minmax& obj){
	cJSON* json = NULL;
	cJSON* item = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// voltage
	if((item = JsonParser::getJsonFromObj(obj.voltage)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_voltage, item);
	// current
	if((item = JsonParser::getJsonFromObj(obj.current)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_current, item);
	// phase
	if((item = JsonParser::getJsonFromObj(obj.phase)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_phase, item);
	// pfactor
	if((item = JsonParser::getJsonFromObj(obj.pfactor)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_pfactor, item);
	// aPow
	if((item = JsonParser::getJsonFromObj(obj.aPow)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_aPow, item);
	// rPow
	if((item = JsonParser::getJsonFromObj(obj.rPow)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_rPow, item);
	// msPow
	if((item = JsonParser::getJsonFromObj(obj.msPow)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_msPow, item);
	// freq
	if((item = JsonParser::getJsonFromObj(obj.freq)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_freq, item);

	// thdA, thdV s�lo para versi�n EMi10 YTL
	#if VERS_METERING_SELECTED == VERS_METERING_EMi10_YTL
		// thdA
		if((item = JsonParser::getJsonFromObj(obj.thdA)) == NULL){
			cJSON_Delete(json);
			return NULL;
		}
		cJSON_AddItemToObject(json, JsonParser::p_thdA, item);
		// thdV
		if((item = JsonParser::getJsonFromObj(obj.thdV)) == NULL){
			cJSON_Delete(json);
			return NULL;
		}
		cJSON_AddItemToObject(json, JsonParser::p_thdV, item);
	#endif
	// active
	if((item = JsonParser::getJsonFromObj(obj.active)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_active, item);
	// reactive
	if((item = JsonParser::getJsonFromObj(obj.reactive)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_reactive, item);

	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromMeteringAnalyzerCfgCalib(const metering_analyzer_cfg_calib& obj){
	cJSON* json = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// meterRegs, measRegs s�lo en versi�n M90E26
	#if VERS_METERING_SELECTED == VERS_METERING_M90E26
		cJSON* regs = NULL;
		// key: calibData.meterRegs
		if((regs=cJSON_CreateArray()) == NULL){
			cJSON_Delete(json);
			return NULL;
		}
		for(int i=0; i < MeteringAnalyzerCfgCalibRegCount; i++){
			cJSON* value = NULL;
			if((value = cJSON_CreateNumber(obj.meterRegs[i])) == NULL){
				cJSON_Delete(regs);
				cJSON_Delete(json);
				return NULL;
			}
			cJSON_AddItemToArray(regs, value);
		}
		cJSON_AddItemToObject(json, JsonParser::p_meterRegs, regs);

		// key: calibData.measRegs
		if((regs=cJSON_CreateArray()) == NULL){
			cJSON_Delete(json);
			return NULL;
		}
		for(int i=0; i < MeteringAnalyzerCfgCalibRegCount; i++){
			cJSON* value = NULL;
			if((value = cJSON_CreateNumber(obj.measRegs[i])) == NULL){
				cJSON_Delete(regs);
				cJSON_Delete(json);
				return NULL;
			}
			cJSON_AddItemToArray(regs, value);
		}
		cJSON_AddItemToObject(json, JsonParser::p_measRegs, regs);
	#endif

	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromMeteringAnalyzerStat(const metering_analyzer_stat& obj){
	cJSON* json = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}
	// flags
	cJSON_AddNumberToObject(json, JsonParser::p_flags, obj.flags);

	cJSON* item = NULL;
	// energyValues
	if((item = getJsonFromMeteringAnalyzerStatTotals(obj.energyValues)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_energyValues, item);

	// measureValues
	if((item = getJsonFromMeteringAnalyzerStatMeasure(obj.measureValues)) == NULL){
		cJSON_Delete(json);
		return NULL;
	}
	cJSON_AddItemToObject(json, JsonParser::p_measureValues, item);
	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromMeteringAnalyzerStatTotals(const metering_analyzer_stat_totals& obj){
	cJSON* json = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}
	// active
	cJSON_AddNumberToObject(json, JsonParser::p_active, obj.active);
	// reactive
	cJSON_AddNumberToObject(json, JsonParser::p_reactive, obj.reactive);

	return json;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromMeteringAnalyzerStatMeasure(const metering_analyzer_stat_measure& obj){
	cJSON* json = NULL;
	if((json=cJSON_CreateObject()) == NULL){
		return NULL;
	}
	// voltage
	cJSON_AddNumberToObject(json, JsonParser::p_voltage, obj.voltage);
	// current
	cJSON_AddNumberToObject(json, JsonParser::p_current, obj.current);
	// phase
	cJSON_AddNumberToObject(json, JsonParser::p_phase, obj.phase);
	// pfactor
	cJSON_AddNumberToObject(json, JsonParser::p_pfactor, obj.pfactor);
	// aPow
	cJSON_AddNumberToObject(json, JsonParser::p_aPow, obj.aPow);
	// rPow
	cJSON_AddNumberToObject(json, JsonParser::p_rPow, obj.rPow);
	// msPow
	cJSON_AddNumberToObject(json, JsonParser::p_msPow, obj.msPow);
	// freq
	cJSON_AddNumberToObject(json, JsonParser::p_freq, obj.freq);

	// thdA, thdV s�lo para versi�n EMi10 YTL
	#if VERS_METERING_SELECTED == VERS_METERING_EMi10_YTL
		// thdA
		cJSON_AddNumberToObject(json, JsonParser::p_thdA, obj.thdA);
		// thdV
		cJSON_AddNumberToObject(json, JsonParser::p_thdV, obj.thdV);
	#endif

	return json;
}


//------------------------------------------------------------------------------------
uint32_t getMeteringManagerFromJson(metering_manager &obj, cJSON* json){
	uint32_t keys = 0;
	uint32_t subkey = 0;
	cJSON* value = NULL;
	if(json == NULL){
		return 0;
	}

	// uid
	if((value = cJSON_GetObjectItem(json,JsonParser::p_uid)) != NULL){
		obj.uid = value->valueint;
		keys |= (1 << 0);
	}
	// cfg
	if((value = cJSON_GetObjectItem(json, JsonParser::p_cfg)) != NULL){
		subkey = getMeteringManagerCfgFromJson(obj.cfg, value)? (1 << 1) : 0;
		keys |= subkey;
	}
	// stat
	if((value = cJSON_GetObjectItem(json, JsonParser::p_stat)) != NULL){
		subkey = getMeteringManagerStatFromJson(obj.stat, value)? (1 << 2) : 0;
		keys |= subkey;
	}
	// analyzers
	cJSON* array = NULL;
	if((array = cJSON_GetObjectItem(json, JsonParser::p_analyzers)) != NULL){
		if(cJSON_GetArraySize(array) <= MeteringManagerCfgMaxNumAnalyzers){
			obj._numAnalyzers = cJSON_GetArraySize(array);
			subkey = 0;
			for(int i=0;i<obj._numAnalyzers;i++){
				value = cJSON_GetArrayItem(array, i);
				subkey = getMeteringAnalyzerFromJson(obj.analyzers[i], value)? (subkey+1) : subkey;
			}
			if(subkey == obj._numAnalyzers){
				keys |= (1 << 3);
			}
		}
	}
	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getMeteringManagerCfgFromJson(metering_manager_cfg &obj, cJSON* json){
	uint32_t keys = 0;
	uint32_t subkey = 0;
	cJSON* value = NULL;
	obj._keys = 0;
	if(json == NULL){
		return 0;
	}

	// updFlags
	if((value = cJSON_GetObjectItem(json,JsonParser::p_updFlags)) != NULL){
		obj.updFlags = value->valueint;
		keys |= (1 << 1);
	}
	// measPeriod
	if((value = cJSON_GetObjectItem(json,JsonParser::p_measPeriod)) != NULL){
		obj.measPeriod = value->valueint;
		keys |= (1 << 2);
	}
	// verbosity
	if((value = cJSON_GetObjectItem(json,JsonParser::p_verbosity)) != NULL){
		obj.verbosity = value->valueint;
		keys |= (1 << 3);
	}
	obj._keys = keys;
	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getMeteringManagerStatFromJson(metering_manager_stat &obj, cJSON* json){
	uint32_t keys = 0;
	uint32_t subkey = 0;
	cJSON* value = NULL;
	if(json == NULL){
		return 0;
	}

	// loadPercent
	cJSON* array = NULL;
	if((array = cJSON_GetObjectItem(json, JsonParser::p_loadPercent)) != NULL){
		if(cJSON_GetArraySize(array) <= MeteringManagerCfgMaxNumAnalyzers){
			obj._numAnalyzers = cJSON_GetArraySize(array);
			subkey = 0;
			for(int i=0;i<obj._numAnalyzers;i++){
				value = cJSON_GetArrayItem(array, i);
				if(value != NULL){
					obj.loadPercent[i] = value->valueint;
					subkey++;
				}
			}
			if(subkey == obj._numAnalyzers){
				keys |= (1 << 1);
			}
		}
	}	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getMeteringAnalyzerFromJson(metering_analyzer &obj, cJSON* json){
	uint32_t keys = 0;
	uint32_t subkey = 0;
	cJSON* value = NULL;
	if(json == NULL){
		return 0;
	}

	// uid
	if((value = cJSON_GetObjectItem(json,JsonParser::p_uid)) != NULL){
		obj.uid = value->valueint;
		keys |= (1 << 0);
	}
	// serial
	if((value = cJSON_GetObjectItem(json,JsonParser::p_serial)) != NULL){
		strncpy(obj.serial, value->valuestring, MeteringAnalyzerSerialLength-1);
		keys |= (1 << 1);
	}
	// cfg
	if((value = cJSON_GetObjectItem(json,JsonParser::p_cfg)) != NULL){
		subkey = getMeteringAnalyzerCfgFromJson(obj.cfg, value)? (1 << 2) : 0;
		keys |= subkey;
	}
	// stat
	if((value = cJSON_GetObjectItem(json,JsonParser::p_stat)) != NULL){
		subkey = getMeteringAnalyzerStatFromJson(obj.stat, value)? (1 << 3) : 0;
		keys |= subkey;
	}
	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getMeteringAnalyzerCfgFromJson(metering_analyzer_cfg &obj, cJSON* json){
	uint32_t keys = 0;
	uint32_t subkey = 0;
	cJSON* value = NULL;
	obj._keys = 0;
	if(json == NULL){
		return 0;
	}

	// updFlags
	if((value = cJSON_GetObjectItem(json,JsonParser::p_updFlags)) != NULL){
		obj.updFlags = value->valueint;
		keys |= (1 << 1);
	}
	// evtFlags
	if((value = cJSON_GetObjectItem(json,JsonParser::p_evtFlags)) != NULL){
		obj.evtFlags = value->valueint;
		keys |= (1 << 2);
	}
	// minmaxData
	/*if((value = cJSON_GetObjectItem(json,JsonParser::p_minmaxData)) != NULL){
		subkey = getMeteringAnalyzerCfgMinMaxFromJson(obj.minmaxData, value)? (1 << 3) : 0;
		keys |= subkey;
	}
	// calibData
	if((value = cJSON_GetObjectItem(json,JsonParser::p_calibData)) != NULL){
		subkey = getMeteringAnalyzerCfgCalibFromJson(obj.calibData, value)? (1 << 4) : 0;
		keys |= subkey;
	}*/
	obj._keys = keys;
	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getMeteringAnalyzerCfgMinMaxFromJson(metering_analyzer_cfg_minmax &obj, cJSON* json){
	uint32_t keys = 0;
	uint32_t subkey = 0;
	cJSON* value = NULL;
	obj._keys = 0;
	if(json == NULL){
		return 0;
	}

	// voltage
	if((value = cJSON_GetObjectItem(json,JsonParser::p_voltage)) != NULL){
		subkey = JsonParser::getObjFromJson(obj.voltage, value)? (1 << 1) : 0;
		keys |= subkey;
	}
	// current
	if((value = cJSON_GetObjectItem(json,JsonParser::p_current)) != NULL){
		subkey = JsonParser::getObjFromJson(obj.current, value)? (1 << 2) : 0;
		keys |= subkey;
	}
	// phase
	if((value = cJSON_GetObjectItem(json,JsonParser::p_phase)) != NULL){
		subkey = JsonParser::getObjFromJson(obj.phase, value)? (1 << 3) : 0;
		keys |= subkey;
	}
	// pfactor
	if((value = cJSON_GetObjectItem(json,JsonParser::p_pfactor)) != NULL){
		subkey = JsonParser::getObjFromJson(obj.pfactor, value)? (1 << 4) : 0;
		keys |= subkey;
	}
	// aPow
	if((value = cJSON_GetObjectItem(json,JsonParser::p_aPow)) != NULL){
		subkey = JsonParser::getObjFromJson(obj.aPow, value)? (1 << 5) : 0;
		keys |= subkey;
	}
	// rPow
	if((value = cJSON_GetObjectItem(json,JsonParser::p_rPow)) != NULL){
		subkey = JsonParser::getObjFromJson(obj.rPow, value)? (1 << 6) : 0;
		keys |= subkey;
	}
	// msPow
	if((value = cJSON_GetObjectItem(json,JsonParser::p_msPow)) != NULL){
		subkey = JsonParser::getObjFromJson(obj.msPow, value)? (1 << 7) : 0;
		keys |= subkey;
	}
	// freq
	if((value = cJSON_GetObjectItem(json,JsonParser::p_freq)) != NULL){
		subkey = JsonParser::getObjFromJson(obj.freq, value)? (1 << 8) : 0;
		keys |= subkey;
	}

	// thdA, thdV s�lo en versi�n EMi10 YTL
	#if VERS_METERING_SELECTED == VERS_METERING_EMi10_YTL
		// thdA
		if((value = cJSON_GetObjectItem(json,JsonParser::p_thdA)) != NULL){
			subkey = JsonParser::getObjFromJson(obj.thdA, value)? (1 << 9) : 0;
			keys |= subkey;
		}
		// thdV
		if((value = cJSON_GetObjectItem(json,JsonParser::p_thdV)) != NULL){
			subkey = JsonParser::getObjFromJson(obj.thdV, value)? (1 << 10) : 0;
			keys |= subkey;
		}
	#endif

	// active
	if((value = cJSON_GetObjectItem(json,JsonParser::p_active)) != NULL){
		subkey = JsonParser::getObjFromJson(obj.active, value)? (1 << 11) : 0;
		keys |= subkey;
	}
	// reactive
	if((value = cJSON_GetObjectItem(json,JsonParser::p_reactive)) != NULL){
		subkey = JsonParser::getObjFromJson(obj.reactive, value)? (1 << 12) : 0;
		keys |= subkey;
	}
	obj._keys = keys;
	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getMeteringAnalyzerCfgCalibFromJson(metering_analyzer_cfg_calib &obj, cJSON* json){
	uint32_t keys = 0;
	uint32_t subkey = 0;
	cJSON* value = NULL;
	obj._keys = 0;
	if(json == NULL){
		return 0;
	}

	// meterRegs, measRegs s�lo en versi�n M90E26
	#if VERS_METERING_SELECTED == VERS_METERING_M90E26
		// meterRegs
		cJSON* array = NULL;
		if((array = cJSON_GetObjectItem(json, JsonParser::p_meterRegs)) != NULL){
			if(cJSON_GetArraySize(array) <= MeteringAnalyzerCfgCalibRegCount){
				subkey = cJSON_GetArraySize(array);
				for(int i=0;i<cJSON_GetArraySize(array);i++){
					value = cJSON_GetArrayItem(array, i);
					obj.meterRegs[i] = value->valueint;
					subkey--;
				}
				if(subkey == 0){
					keys |= (1 << 1);
				}
			}
		}
		// measRegs
		if((array = cJSON_GetObjectItem(json, JsonParser::p_measRegs)) != NULL){
			if(cJSON_GetArraySize(array) <= MeteringAnalyzerCfgCalibRegCount){
				subkey = cJSON_GetArraySize(array);
				for(int i=0;i<cJSON_GetArraySize(array);i++){
					value = cJSON_GetArrayItem(array, i);
					obj.measRegs[i] = value->valueint;
					subkey--;
				}
				if(subkey == 0){
					keys |= (1 << 2);
				}
			}
		}
	#endif
	obj._keys = keys;
	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getMeteringAnalyzerStatFromJson(metering_analyzer_stat &obj, cJSON* json){
	uint32_t keys = 0;
	uint32_t subkey = 0;
	cJSON* value = NULL;
	if(json == NULL){
		return 0;
	}

	// flags
	if((value = cJSON_GetObjectItem(json,JsonParser::p_flags)) != NULL){
		obj.flags = value->valueint;
		keys |= (1 << 1);
	}
	// energyValues
	if((value = cJSON_GetObjectItem(json, JsonParser::p_energyValues)) != NULL){
		subkey = getMeteringAnalyzerStatTotalsFromJson(obj.energyValues, value)? (1 << 2) : 0;
		keys |= subkey;
	}
	// measureValues
	if((value = cJSON_GetObjectItem(json, JsonParser::p_measureValues)) != NULL){
		subkey = getMeteringAnalyzerStatMeasureFromJson(obj.measureValues, value)? (1 << 3) : 0;
		keys |= subkey;
	}
	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getMeteringAnalyzerStatTotalsFromJson(metering_analyzer_stat_totals &obj, cJSON* json){
	uint32_t keys = 0;
	cJSON* value = NULL;
	if(json == NULL){
		return 0;
	}

	// active
	if((value = cJSON_GetObjectItem(json,JsonParser::p_active)) != NULL){
		obj.active = value->valuedouble;
		keys |= (1 << 1);
	}
	// reactive
	if((value = cJSON_GetObjectItem(json,JsonParser::p_reactive)) != NULL){
		obj.reactive = value->valuedouble;
		keys |= (1 << 2);
	}
	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getMeteringAnalyzerStatMeasureFromJson(metering_analyzer_stat_measure &obj, cJSON* json){
	uint32_t keys = 0;
	cJSON* value = NULL;
	if(json == NULL){
		return 0;
	}

	// voltage
	if((value = cJSON_GetObjectItem(json,JsonParser::p_voltage)) != NULL){
		obj.voltage = value->valuedouble;
		keys |= (1 << 1);
	}
	// current
	if((value = cJSON_GetObjectItem(json,JsonParser::p_current)) != NULL){
		obj.current = value->valuedouble;
		keys |= (1 << 2);
	}
	// phase
	if((value = cJSON_GetObjectItem(json,JsonParser::p_phase)) != NULL){
		obj.phase = value->valuedouble;
		keys |= (1 << 3);
	}
	// pfactor
	if((value = cJSON_GetObjectItem(json,JsonParser::p_pfactor)) != NULL){
		obj.pfactor = value->valuedouble;
		keys |= (1 << 4);
	}
	// aPow
	if((value = cJSON_GetObjectItem(json,JsonParser::p_aPow)) != NULL){
		obj.aPow = value->valuedouble;
		keys |= (1 << 5);
	}
	// rPow
	if((value = cJSON_GetObjectItem(json,JsonParser::p_rPow)) != NULL){
		obj.rPow = value->valuedouble;
		keys |= (1 << 6);
	}
	// msPow
	if((value = cJSON_GetObjectItem(json,JsonParser::p_msPow)) != NULL){
		obj.msPow = value->valuedouble;
		keys |= (1 << 7);
	}
	// freq
	if((value = cJSON_GetObjectItem(json,JsonParser::p_freq)) != NULL){
		obj.freq = value->valuedouble;
		keys |= (1 << 8);
	}

	// thdA, thdV s�lo en versi�n EMi10 YTL
	#if VERS_METERING_SELECTED == VERS_METERING_EMi10_YTL
		// thdA
		if((value = cJSON_GetObjectItem(json,JsonParser::p_thdA)) != NULL){
			obj.thdA = value->valuedouble;
			keys |= (1 << 9);
		}
		// thdV
		if((value = cJSON_GetObjectItem(json,JsonParser::p_thdV)) != NULL){
			obj.thdV = value->valuedouble;
			keys |= (1 << 10);
		}
	#endif

	return keys;
}


}	// end namespace JSON

