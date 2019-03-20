/*
 * ppl_energy_objects.cpp
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

static const char* _MODULE_ = "[ppl:energy]....";
#define _EXPR_	(!IS_ISR())


namespace JSON{


//------------------------------------------------------------------------------------
cJSON* getJsonFromPplEnergy(const ppl_energy& obj){

}


//------------------------------------------------------------------------------------
cJSON* getJsonFromPplEnergyCfg(const ppl_energy_cfg& obj){

}


//------------------------------------------------------------------------------------
cJSON* getJsonFromPplEnergyStat(const ppl_energy_stat& obj){

}


//------------------------------------------------------------------------------------
cJSON* getJsonFromPplEnergyAnalyzer(const ppl_energy_analyzer& obj){

}


//------------------------------------------------------------------------------------
cJSON* getJsonFromPplEnergyAnalyzerCfg(const ppl_energy_analyzer_cfg& obj){

}


//------------------------------------------------------------------------------------
cJSON* getJsonFromPplEnergyAnalyzerCfgMinMax(const ppl_energy_analyzer_cfg_minmax& obj){

}


//------------------------------------------------------------------------------------
cJSON* getJsonFromPplEnergyAnalyzerCfgCalib(const ppl_energy_analyzer_cfg_calib& obj){

}


//------------------------------------------------------------------------------------
cJSON* getJsonFromPplEnergyAnalyzerStat(const ppl_energy_analyzer_stat& obj){

}


//------------------------------------------------------------------------------------
cJSON* getJsonFromPplEnergyAnalyzerStatTotals(const ppl_energy_analyzer_stat_totals& obj){

}


//------------------------------------------------------------------------------------
cJSON* getJsonFromPplEnergyAnalyzerStatMeasure(const ppl_energy_analyzer_stat_measure& obj){

}


//------------------------------------------------------------------------------------
uint32_t getPplEnergyFromJson(ppl_energy &obj, cJSON* json){

}


//------------------------------------------------------------------------------------
uint32_t getPplEnergyCfgFromJson(ppl_energy_cfg &obj, cJSON* json){

}


//------------------------------------------------------------------------------------
uint32_t getPplEnergyStatFromJson(ppl_energy_stat &obj, cJSON* json){

}


//------------------------------------------------------------------------------------
uint32_t getPplEnergyAnalyzerFromJson(ppl_energy_analyzer &obj, cJSON* json){

}


//------------------------------------------------------------------------------------
uint32_t getPplEnergyAnalyzerCfgFromJson(ppl_energy_analyzer_cfg &obj, cJSON* json){

}


//------------------------------------------------------------------------------------
uint32_t getPplEnergyAnalyzerCfgMinMaxFromJson(ppl_energy_analyzer_cfg_minmax &obj, cJSON* json){

}


//------------------------------------------------------------------------------------
uint32_t getPplEnergyAnalyzerCfgCalibFromJson(ppl_energy_analyzer_cfg_calib &obj, cJSON* json){

}


//------------------------------------------------------------------------------------
uint32_t getPplEnergyAnalyzerStatFromJson(ppl_energy_analyzer_stat &obj, cJSON* json){

}


//------------------------------------------------------------------------------------
uint32_t getPplEnergyAnalyzerStatTotalsFromJson(ppl_energy_analyzer_stat_totals &obj, cJSON* json){

}


//------------------------------------------------------------------------------------
uint32_t getPplEnergyAnalyzerStatMeasureFromJson(ppl_energy_analyzer_stat_measure &obj, cJSON* json){

}


//------------------------------------------------------------------------------------






//------------------------------------------------------------------------------------
cJSON* getJsonFromAMCfg(const Blob::AMCfgData_t& cfg){
	cJSON* energy = NULL;

	if((energy=cJSON_CreateObject()) == NULL){
		return NULL;
	}
	cJSON *minmaxData = NULL;
	cJSON *calibData = NULL;
	cJSON *regs = NULL;
	cJSON *value = NULL;

	// key: energy.version
	cJSON_AddStringToObject(energy, JsonParser::p_version, cfg.version);

	// key: energy.updFlags
	cJSON_AddNumberToObject(energy, JsonParser::p_updFlags, cfg.updFlagMask);

	// key: energy.evtFlags
	cJSON_AddNumberToObject(energy, JsonParser::p_evtFlags, cfg.evtFlagMask);

	// key: energy.measPeriod
	cJSON_AddNumberToObject(energy, JsonParser::p_measPeriod, cfg.measPeriod);

	// key: minmaxData
	if((minmaxData=cJSON_CreateObject()) == NULL){
		cJSON_Delete(energy);
		return NULL;
	}

	// key: minmaxData.version
	cJSON_AddStringToObject(minmaxData, JsonParser::p_version, cfg.minmaxData.version);

	// key: minmaxData.voltage
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.voltage.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.voltage.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.voltage.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_voltage, value);

	// key: minmaxData.current
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.current.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.current.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.current.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_current, value);

	// key: minmaxData.phase
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.phase.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.phase.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.phase.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_phase, value);

	// key: minmaxData.pfactor
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.pfactor.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.pfactor.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.pfactor.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_pfactor, value);

	// key: minmaxData.aPow
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.aPow.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.aPow.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.aPow.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_aPow, value);

	// key: minmaxData.rPow
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.rPow.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.rPow.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.rPow.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_rPow, value);

	// key: minmaxData.msPow
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.msPow.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.msPow.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.msPow.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_msPow, value);

	// key: minmaxData.freq
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.freq.min);
	cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.freq.max);
	cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.freq.thres);
	cJSON_AddItemToObject(minmaxData, JsonParser::p_freq, value);

	// key: minmaxData.thdA
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(minmaxData);
		cJSON_Delete(energy);
		return NULL;
	}
	if(strcmp(cfg.minmaxData.version, AM_OBJ_VERSION_DEFAULT)==0){
		cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.thdA.min);
		cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.thdA.max);
		cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.thdA.thres);
		cJSON_AddItemToObject(minmaxData, JsonParser::p_thdA, value);

		// key: minmaxData.thdV
		if((value=cJSON_CreateObject()) == NULL){
			cJSON_Delete(minmaxData);
			cJSON_Delete(energy);
			return NULL;
		}
		cJSON_AddNumberToObject(value, JsonParser::p_min, cfg.minmaxData.thdV.min);
		cJSON_AddNumberToObject(value, JsonParser::p_max, cfg.minmaxData.thdV.max);
		cJSON_AddNumberToObject(value, JsonParser::p_thres, cfg.minmaxData.thdV.thres);
		cJSON_AddItemToObject(minmaxData, JsonParser::p_thdV, value);
}

	cJSON_AddItemToObject(energy, JsonParser::p_minmaxData, minmaxData);

	// key: calibData
	if((calibData=cJSON_CreateObject()) == NULL){
		cJSON_Delete(energy);
		return NULL;
	}

	// key: calibData.version
	cJSON_AddStringToObject(calibData, JsonParser::p_version, cfg.calibData.version);

	if(strcmp(cfg.calibData.version, AM_OBJ_VERSION_M90E26)==0){
		// key: calibData.meterRegs
		if((regs=cJSON_CreateArray()) == NULL){
			cJSON_Delete(calibData);
			cJSON_Delete(energy);
			return NULL;
		}
		for(int i=0; i < Blob::AMCalibRegCount; i++){
			if((value = cJSON_CreateNumber(cfg.calibData.meterRegs[i])) == NULL){
				cJSON_Delete(regs);
				cJSON_Delete(calibData);
				cJSON_Delete(energy);
				return NULL;
			}
			cJSON_AddItemToArray(regs, value);
		}
		cJSON_AddItemToObject(calibData, JsonParser::p_meterRegs, regs);

		// key: calibData.measRegs
		if((regs=cJSON_CreateArray()) == NULL){
			cJSON_Delete(calibData);
			cJSON_Delete(energy);
			return NULL;
		}
		for(int i=0; i < Blob::AMCalibRegCount; i++){
			if((value = cJSON_CreateNumber(cfg.calibData.measRegs[i])) == NULL){
				cJSON_Delete(regs);
				cJSON_Delete(calibData);
				cJSON_Delete(energy);
				return NULL;
			}
			cJSON_AddItemToArray(regs, value);
		}
		cJSON_AddItemToObject(calibData, JsonParser::p_measRegs, regs);
	}

	cJSON_AddItemToObject(energy, JsonParser::p_calibData, calibData);

	// key: verbosity
	cJSON_AddNumberToObject(energy, JsonParser::p_verbosity, cfg.verbosity);

	return energy;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromAMStat(const Blob::AMStatData_t& stat) {
	cJSON* energy = NULL;
	cJSON* value = NULL;

	if((energy=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// key: version
	cJSON_AddStringToObject(energy, JsonParser::p_version, stat.version);

	// key: flags
	cJSON_AddNumberToObject(energy, JsonParser::p_flags, stat.flags);

	// key: energyValues
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddStringToObject(value, JsonParser::p_version, stat.energyValues.version);
	cJSON_AddNumberToObject(value, JsonParser::p_active, stat.energyValues.active);
	cJSON_AddNumberToObject(value, JsonParser::p_reactive, stat.energyValues.reactive);
	cJSON_AddItemToObject(energy, JsonParser::p_energyValues, value);

	// key: measureValues
	if((value=cJSON_CreateObject()) == NULL){
		cJSON_Delete(energy);
		return NULL;
	}
	cJSON_AddStringToObject(value, JsonParser::p_version, stat.measureValues.version);
	cJSON_AddNumberToObject(value, JsonParser::p_voltage, stat.measureValues.voltage);
	cJSON_AddNumberToObject(value, JsonParser::p_current, stat.measureValues.current);
	cJSON_AddNumberToObject(value, JsonParser::p_phase, stat.measureValues.phase);
	cJSON_AddNumberToObject(value, JsonParser::p_pfactor, stat.measureValues.pfactor);
	cJSON_AddNumberToObject(value, JsonParser::p_aPow, stat.measureValues.aPow);
	cJSON_AddNumberToObject(value, JsonParser::p_rPow, stat.measureValues.rPow);
	cJSON_AddNumberToObject(value, JsonParser::p_msPow, stat.measureValues.msPow);
	cJSON_AddNumberToObject(value, JsonParser::p_freq, stat.measureValues.freq);
	if(strcmp(stat.measureValues.version, AM_OBJ_VERSION_DEFAULT)==0){
		cJSON_AddNumberToObject(value, JsonParser::p_thdA, stat.measureValues.thdA);
		cJSON_AddNumberToObject(value, JsonParser::p_thdV, stat.measureValues.thdV);
	}
	cJSON_AddItemToObject(energy, JsonParser::p_measureValues, value);
	return energy;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromAMBoot(const Blob::AMBootData_t& boot){
	cJSON* energy = NULL;
	cJSON* item = NULL;
	if((energy=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	cJSON_AddStringToObject(energy, JsonParser::p_version, boot.version);

	if((item = getJsonFromAMCfg(boot.cfg)) == NULL){
		goto __encodeBoot_Err;
	}
	cJSON_AddItemToObject(energy, JsonParser::p_cfg, item);

	if((item = getJsonFromAMStat(boot.stat)) == NULL){
		goto __encodeBoot_Err;
	}
	cJSON_AddItemToObject(energy, JsonParser::p_stat, item);
	return energy;

__encodeBoot_Err:
	cJSON_Delete(energy);
	return NULL;
}


//------------------------------------------------------------------------------------
cJSON* getJsonFromAMLoad(const Blob::AMLoadData_t& data) {
	cJSON* obj = NULL;
	cJSON* value = NULL;

	if((obj=cJSON_CreateObject()) == NULL){
		return NULL;
	}

	// key: flags
	cJSON_AddNumberToObject(obj, JsonParser::p_outValue, data.outValue);
	return obj;
}


//------------------------------------------------------------------------------------
uint32_t getAMCfgFromJson(Blob::AMCfgData_t &cfg, cJSON* json){
	cJSON* obj = NULL;
	cJSON* minmaxData = NULL;
	cJSON* calibData = NULL;
	cJSON* array = NULL;
	cJSON* value = NULL;
	uint32_t keys = 0;

	if(json == NULL){
		return 0;
	}

	if((obj = cJSON_GetObjectItem(json,JsonParser::p_version)) != NULL){
		strncpy(cfg.version, obj->valuestring, Blob::AMVersionLength-1);
		keys |= Blob::AMKeyCfgVersion;
	}
	if((obj = cJSON_GetObjectItem(json,JsonParser::p_updFlags)) != NULL){
		cfg.updFlagMask = (Blob::AMUpdFlags)obj->valueint;
		keys |= Blob::AMKeyCfgUpd;
	}
	if((obj = cJSON_GetObjectItem(json,JsonParser::p_evtFlags)) != NULL){
		cfg.evtFlagMask = (Blob::AMEvtFlags)obj->valueint;
		keys |= Blob::AMKeyCfgEvt;
	}
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_measPeriod)) != NULL){
		cfg.measPeriod = (uint32_t)obj->valueint;
		keys |= Blob::AMKeyCfgMeas;
	}
	if((obj = cJSON_GetObjectItem(json,JsonParser::p_verbosity)) != NULL){
		cfg.verbosity = obj->valueint;
		keys |= Blob::AMKeyCfgVerbosity;
	}

	//key: minmaxData
	if((minmaxData = cJSON_GetObjectItem(json, JsonParser::p_minmaxData)) != NULL){
		if((obj = cJSON_GetObjectItem(minmaxData,JsonParser::p_version)) != NULL){
			strncpy(cfg.minmaxData.version, obj->valuestring, Blob::AMVersionLength-1);
		}
		//key: minmaxData.voltage
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_voltage)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.voltage.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.voltage.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value,JsonParser::p_thres)) != NULL){
				cfg.minmaxData.voltage.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxVolt;
		}

		//key: minmaxData.current
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_current)) != NULL){
			if((obj = cJSON_GetObjectItem(value,JsonParser::p_min)) != NULL){
				cfg.minmaxData.current.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.current.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.current.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxCurr;
		}

		//key: minmaxData.phase
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_phase)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.phase.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.phase.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.phase.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxPhase;
		}

		//key: minmaxData.pfactor
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_pfactor)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.pfactor.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.pfactor.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.pfactor.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxPfact;
		}

		//key: minmaxData.aPow
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_aPow)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.aPow.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.aPow.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.aPow.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxApow;
		}

		//key: minmaxData.rPow
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_rPow)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.rPow.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.rPow.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.rPow.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxRpow;
		}

		//key: minmaxData.msPow
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_msPow)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.msPow.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.msPow.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.msPow.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxMpow;
		}

		//key: minmaxData.freq
		if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_freq)) != NULL){
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
				cfg.minmaxData.freq.min = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
				cfg.minmaxData.freq.max = obj->valuedouble;
			}
			if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
				cfg.minmaxData.freq.thres = obj->valuedouble;
			}
			keys |= Blob::AMKeyCfgMnxFreq;
		}

		if(strcmp(cfg.minmaxData.version, AM_OBJ_VERSION_DEFAULT)==0){
			//key: minmaxData.thdA
			if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_thdA)) != NULL){
				if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
					cfg.minmaxData.thdA.min = obj->valuedouble;
				}
				if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
					cfg.minmaxData.thdA.max = obj->valuedouble;
				}
				if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
					cfg.minmaxData.thdA.thres = obj->valuedouble;
				}
				keys |= Blob::AMKeyCfgMnxThdA;
			}

			//key: minmaxData.thdV
			if((value = cJSON_GetObjectItem(minmaxData, JsonParser::p_thdV)) != NULL){
				if((obj = cJSON_GetObjectItem(value, JsonParser::p_min)) != NULL){
					cfg.minmaxData.thdV.min = obj->valuedouble;
				}
				if((obj = cJSON_GetObjectItem(value, JsonParser::p_max)) != NULL){
					cfg.minmaxData.thdV.max = obj->valuedouble;
				}
				if((obj = cJSON_GetObjectItem(value, JsonParser::p_thres)) != NULL){
					cfg.minmaxData.thdV.thres = obj->valuedouble;
				}
				keys |= Blob::AMKeyCfgMnxThdV;
			}
		}
	}

	if((calibData = cJSON_GetObjectItem(json, JsonParser::p_calibData)) != NULL){
		if((obj = cJSON_GetObjectItem(calibData,JsonParser::p_version)) != NULL){
			strncpy(cfg.calibData.version, obj->valuestring, Blob::AMVersionLength-1);
		}
		if(strcmp(cfg.calibData.version, AM_OBJ_VERSION_M90E26)==0){
			if((array = cJSON_GetObjectItem(calibData, JsonParser::p_meterRegs)) != NULL){
				if(cJSON_GetArraySize(array) <= Blob::AMCalibRegCount){
					for(int i=0;i<cJSON_GetArraySize(array);i++){
						value = cJSON_GetArrayItem(array, i);
						cfg.calibData.meterRegs[i] = value->valueint;
					}
					keys |= Blob::AMKeyCfgCalMetr;
				}
			}
			if((array = cJSON_GetObjectItem(calibData, JsonParser::p_measRegs)) != NULL){
				if(cJSON_GetArraySize(array) <= Blob::AMCalibRegCount){
					for(int i=0;i<cJSON_GetArraySize(array);i++){
						value = cJSON_GetArrayItem(array, i);
						cfg.calibData.measRegs[i] = value->valueint;
					}
					keys |= Blob::AMKeyCfgCalMea;
				}
			}
		}
	}

	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getAMStatFromJson(Blob::AMStatData_t &stat, cJSON* json) {
	cJSON* obj = NULL;
	cJSON* minmaxData = NULL;
	cJSON* calibData = NULL;
	cJSON* array = NULL;
	cJSON* value = NULL;
	uint32_t keys = 0;

	if(json == NULL){
		return 0;
	}
	if((obj = cJSON_GetObjectItem(json,JsonParser::p_version)) != NULL){
		strncpy(stat.version, obj->valuestring, Blob::AMVersionLength-1);
	}

	if((obj = cJSON_GetObjectItem(json,JsonParser::p_flags)) == NULL){
		return 0;
	}
	stat.flags = (Blob::AMUpdFlags)obj->valueint;

	//key: energyValues
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_energyValues)) == NULL){
		return 0;
	}
	// key: energyValues.version
	if((value = cJSON_GetObjectItem(obj,JsonParser::p_version)) != NULL){
		strncpy(stat.energyValues.version, value->valuestring, Blob::AMVersionLength-1);
	}
	//key: active
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_active)) == NULL){
		return 0;
	}
	stat.energyValues.active = value->valueint;
	//key: reactive
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_reactive)) == NULL){
		return 0;
	}
	stat.energyValues.reactive = value->valueint;

	//key: measureValues
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_measureValues)) == NULL){
		return 0;
	}
	//key: version
	if((value = cJSON_GetObjectItem(obj,JsonParser::p_version)) != NULL){
		strncpy(stat.measureValues.version, value->valuestring, Blob::AMVersionLength-1);
	}
	//key: current
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_current)) == NULL){
		return 0;
	}
	stat.measureValues.current = value->valuedouble;
	//key: voltage
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_voltage)) == NULL){
		return 0;
	}
	stat.measureValues.voltage = value->valuedouble;
	//key: freq
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_freq)) == NULL){
		return 0;
	}
	stat.measureValues.freq = value->valuedouble;
	//key: pfactor
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_pfactor)) == NULL){
		return 0;
	}
	stat.measureValues.pfactor = value->valuedouble;
	//key: phase
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_phase)) == NULL){
		return 0;
	}
	stat.measureValues.phase = value->valuedouble;
	//key: aPow
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_aPow)) == NULL){
		return 0;
	}
	stat.measureValues.aPow = value->valuedouble;
	//key: rPow
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_rPow)) == NULL){
		return 0;
	}
	stat.measureValues.rPow = value->valuedouble;
	//key: msPow
	if((value = cJSON_GetObjectItem(obj, JsonParser::p_msPow)) == NULL){
		return 0;
	}
	stat.measureValues.msPow = value->valuedouble;

	if(strcmp(stat.measureValues.version, AM_OBJ_VERSION_DEFAULT)==0){
		//key: thdA
		if((value = cJSON_GetObjectItem(obj, JsonParser::p_thdA)) == NULL){
			return 0;
		}
		stat.measureValues.thdA = value->valuedouble;
		//key: thdV
		if((value = cJSON_GetObjectItem(obj, JsonParser::p_thdV)) == NULL){
			return 0;
		}
		stat.measureValues.thdV = value->valuedouble;
	}

	return 1;
}


//------------------------------------------------------------------------------------
uint32_t getAMBootFromJson(Blob::AMBootData_t &obj, cJSON* json){
	cJSON* cfg = NULL;
	cJSON* stat = NULL;
	cJSON* value = NULL;
	uint32_t keys = 0;

	if(json == NULL){
		return 0;
	}
	//key: version
	if((value = cJSON_GetObjectItem(json,JsonParser::p_version)) != NULL){
		strncpy(obj.version, value->valuestring, Blob::AMVersionLength-1);
	}
	if((cfg = cJSON_GetObjectItem(json, JsonParser::p_cfg)) == NULL){
		return 0;
	}
	keys |= getAMCfgFromJson(obj.cfg, cfg);

	if((stat = cJSON_GetObjectItem(json, JsonParser::p_stat)) == NULL){
		return 0;
	}
	keys |= getAMStatFromJson(obj.stat, stat);
	return keys;
}


//------------------------------------------------------------------------------------
uint32_t getAMLoadFromJson(Blob::AMLoadData_t &data, cJSON* json){
	cJSON *obj = NULL;
	if(json == NULL){
		return 0;
	}

	// key: outValue
	if((obj = cJSON_GetObjectItem(json, JsonParser::p_outValue)) == NULL){
		return 0;
	}

	data.outValue = obj->valueint;
	return 1;
}

}	// end namespace JSON

