/*
 * metering_objects.h
 *
 *  Created on: Mar 2019
 *      Author: raulMrello
 *
 *	Objetos JSON relativos al m�dulo metering
 */

#ifndef METERING_OBJECTS_
#define METERING_OBJECTS_

#include <cstdint>
#include <type_traits>
#include "common_objects.h"
#include "cpp_utils.h"
#include "cJSON.h"



/** Selecci�n de la versi�n utilizada 	*/
/** DEFINIR SEG�N APLICACI�N 			*/
#include "metering_objects_UserConfig.h"

#ifndef VERS_METERING_SELECTED
#error En metering_objects_UserConfig.h se require definir VERS_METERING_SELECTED
#endif

/** Macro de generaci�n de UIDs*/
#define UID_METERING_MANAGER		(uint32_t)(0x00000001 | ((uint32_t)VERS_METERING_SELECTED << 20))
#define UID_METERING_ANALYZER		(uint32_t)(0x00000002 | ((uint32_t)VERS_METERING_SELECTED << 20))


/** Flags para la variable ppl:energy:cfg/updFlags */
enum metering_manager_cfg_updFlags {
	MeteringManagerCfgUpdNotif 		= (1 << 0),	/// Habilita notificaci�n de cambios en cualquier par�metro de la configuraci�n
};

/** Flags para la variable ppl:energy:analyzer:cfg/updFlags */
enum metering_analyzer_cfg_updFlags {
	MeteringAnalyzerCfgUpdNotif 		= (1 << 0),	/// Habilita notificaci�n de cambios en cualquier par�metro de la configuraci�n
	MeteringAnalyzerCfgMinMaxUpdNotif 	= (1 << 1),	/// Habilita notificaci�n de cambios en ppl:energy:analyzer:cfg:minmax
	MeteringAnalyzerCfgCalibUpdNotif 	= (1 << 2),	/// Habilita notificaci�n de cambios en ppl:energy:analyzer:cfg:calib
};


/** Flags para la variable ppl:energy:analyzer:cfg/evtFlags */
enum metering_analyzer_cfg_evtFlags{
	MeteringAnalyzerNoEvents				= 0,			//!< Flag para indicar que no hay eventos
	MeteringAnalyzerInstantMeasureEvt 		= (1 << 0),	//!< Evento al realizar una medida
	MeteringAnalyzerVoltage					= (1 << 1),	//!< Evento al recibir actualizaci�n en la medida de tensi�n
	MeteringAnalyzerCurrent					= (1 << 2),	//!< Evento al recibir actualizaci�n en la medida de corriente
	MeteringAnalyzerActivePower				= (1 << 3),	//!< Evento al recibir actualizaci�n en la medida de aPow
	MeteringAnalyzerReactivePower			= (1 << 4),	//!< Evento al recibir actualizaci�n en la medida de rPow
	MeteringAnalyzerPowerFactor				= (1 << 5),	//!< Evento al recibir actualizaci�n en la medida de pf
	MeteringAnalyzerFrequency				= (1 << 6),	//!< Evento al recibir actualizaci�n en la medida de freq
	MeteringAnalyzerPoweredUp				= (1 << 7),	//!< Evento al recibir actualizaci�n en la medida de freq
	MeteringAnalyzerEnergyActive			= (1 << 8),	//!< Evento al recibir actualizaci�n en la medida de energ�a activa
	MeteringAnalyzerEnergyReactive			= (1 << 9),	//!< Evento al recibir actualizaci�n en la medida de energ�a reactiva
	MeteringAnalyzerStatus                  = (1 << 10), //!< Evento al recibir actualización en el valor de estado
	MeteringAnalyzerTemperature				= (1 << 11), //!< Evento al recibir actualización en la medida de temperatura
	MeteringAnalyzerElectricParam 			= (MeteringAnalyzerVoltage|MeteringAnalyzerCurrent|MeteringAnalyzerActivePower|MeteringAnalyzerReactivePower|MeteringAnalyzerPowerFactor|MeteringAnalyzerFrequency|MeteringAnalyzerPoweredUp|MeteringAnalyzerEnergyActive|MeteringAnalyzerEnergyReactive|MeteringAnalyzerStatus|MeteringAnalyzerTemperature),

	/* elemento inv�lido */
	MeteringAnalyzerInvalidEvt				= (1 << 31)
};

/** Flags para la variable ppl:energy:analyzer:stat/flags */
typedef metering_analyzer_cfg_evtFlags metering_analyzer_stat_flags;


/** Rango m�nimo y valor por defecto de la variable ppl:energy:cfg/measPeriod */
static const uint32_t MeteringManagerCfgMeasPeriodMin 	= 1;
static const uint32_t MeteringManagerCfgMeasPeriodDefault = 1;

/** M�ximo n�mero de analizadores permitidos */
static const uint8_t MeteringManagerCfgMaxNumAnalyzers = 29; //26;

/** Tama�o m�ximo del texto asociado a la variable ppl:energy:analyzer/serial */
static const uint8_t MeteringAnalyzerSerialLength = 16;

/** N�mero de registros de calibraci�n del driver M90E26 en la variable ppl:energy:analyzer:cfg:calib/meterRegs */
static const uint8_t MeteringAnalyzerCfgCalibRegCount = 16;


/**Objeto ppl:energy:analyzer:stat:measure */
struct metering_analyzer_stat_measure{
	double voltage;
	double current;
	double phase;
	double pfactor;
	double aPow;
	double rPow;
	double msPow;
	double freq;
	double thdA;
	double thdV;
	double temp;
	double status;
};


/**Objeto ppl:energy:analyzer:stat:totals */
struct metering_analyzer_stat_totals{
	double active;
	double reactive;
};


/**Objeto ppl:energy:analyzer:stat */
struct metering_analyzer_stat{
	uint32_t flags;
	metering_analyzer_stat_totals energyValues;
	metering_analyzer_stat_measure measureValues;
};

struct ThreePhaseAnalyzerStat {
	metering_analyzer_stat stat[3];
	ThreePhaseAnalyzerStat(){
		reset();
	}
	void reset(bool erase_energy = true){
		for(int i=0;i<3;i++){
			stat[i].flags = 0;
			stat[i].measureValues = {0};
			if(erase_energy){
				stat[i].energyValues = {0};
			}
		}
	}
	void copyMeasures(const ThreePhaseAnalyzerStat& tphas, bool discard_energy = false){
		for(int i=0;i<3;i++){
			// s�lo copia analizadores con medidas
			if(tphas.stat[i].flags != 0){
				if(!discard_energy){
					stat[i] = tphas.stat[i];
				}
				else{
					stat[i].flags = tphas.stat[i].flags;
					stat[i].measureValues = tphas.stat[i].measureValues;
				}
			}
		}
	}
	double getVoltage(uint8_t i=0xff){
		if(i==0xff)
			return cpp_utils::max3(stat[0].measureValues.voltage, stat[1].measureValues.voltage, stat[2].measureValues.voltage);
		else
			return (stat[i].measureValues.voltage);
	}
	void setVoltage(uint8_t i, double voltage){
		stat[i].measureValues.voltage = voltage;
	}
	double getCurrent(uint8_t i=0xff){
		if(i==0xff)
			return (stat[0].measureValues.current + stat[1].measureValues.current + stat[2].measureValues.current);
		else
			return (stat[i].measureValues.current);
	}
	double getCurrentPF(uint8_t i=0xff){
		if(i==0xff)
			return ((stat[0].measureValues.current * stat[0].measureValues.pfactor)+(stat[1].measureValues.current * stat[1].measureValues.pfactor)+(stat[2].measureValues.current * stat[2].measureValues.pfactor));
		else
			return (stat[i].measureValues.current * stat[i].measureValues.pfactor);
	}
	void setCurrent(uint8_t i, double curr){
		stat[i].measureValues.current = curr;
	}
	int32_t getMilliamps(uint8_t i=0xff){
		if(i==0xff)
			return (int32_t)round(1000*(stat[0].measureValues.current+stat[1].measureValues.current+stat[2].measureValues.current));
		else
			return (int32_t)round(1000 * stat[i].measureValues.current);

	}
	int32_t getMilliampsPF(uint8_t i=0xff){
		if(i==0xff)
			return (int32_t)round(1000*((stat[0].measureValues.current * stat[0].measureValues.pfactor)+(stat[1].measureValues.current * stat[1].measureValues.pfactor)+(stat[2].measureValues.current * stat[2].measureValues.pfactor)));
		else
			return (int32_t)round(1000 * stat[i].measureValues.current * stat[i].measureValues.pfactor);

	}
	void setMilliamps(uint8_t i, int32_t curr){
		stat[i].measureValues.current = (double)curr/1000.0;
	}
	double getActivePower(uint8_t i=0xff){
		if(i==0xff)
			return (stat[0].measureValues.aPow+stat[1].measureValues.aPow+stat[2].measureValues.aPow);
		else
			return (stat[i].measureValues.aPow);
	}
	void setActivePower(uint8_t i, double aPow){
		stat[i].measureValues.aPow = aPow;
	}
	void setActivePowerAndUpdateCurrent(uint8_t i, double aPow){
		stat[i].measureValues.aPow = aPow;
		if(stat[i].measureValues.pfactor != 0 && stat[i].measureValues.voltage != 0){
			stat[i].measureValues.current = aPow/(stat[i].measureValues.voltage * stat[i].measureValues.pfactor);
		}
		else{
			stat[i].measureValues.current = 0;
			stat[i].measureValues.aPow = 0;
		}
	}

	int32_t getActivePowerAsInt(uint8_t i=0xff){
		if(i==0xff)
			return (int32_t)round(stat[0].measureValues.aPow+stat[1].measureValues.aPow+stat[2].measureValues.aPow);
		else
			return (int32_t)round(stat[i].measureValues.aPow);

	}
	double getReactivePower(uint8_t i=0xff){
		if(i==0xff)
			return (stat[0].measureValues.rPow+stat[1].measureValues.rPow+stat[2].measureValues.rPow);
		else
			return (stat[i].measureValues.rPow);
	}
	void setReactivePower(uint8_t i, double rPow){
		stat[i].measureValues.rPow = rPow;
	}
	int32_t getReactivePowerAsInt(uint8_t i=0xff){
		if(i==0xff)
			return (int32_t)round(stat[0].measureValues.rPow+stat[1].measureValues.rPow+stat[2].measureValues.rPow);
		else
			return (int32_t)round(stat[i].measureValues.rPow);

	}
	double getActiveEnergy(uint8_t i=0xff){
		if(i==0xff)
			return (stat[0].energyValues.active+stat[1].energyValues.active+stat[2].energyValues.active);
		else
			return (stat[i].energyValues.active);
	}
	void setActiveEnergy(uint8_t i, double active){
		stat[i].energyValues.active = active;
	}
	void addActiveEnergy(uint8_t i, double active){
		stat[i].energyValues.active += active;
	}
	double getReactiveEnergy(uint8_t i=0xff){
		if(i==0xff)
			return (stat[0].energyValues.reactive+stat[1].energyValues.reactive+stat[2].energyValues.reactive);
		else
			return (stat[i].energyValues.reactive);
	}
	void setReactiveEnergy(uint8_t i, double reactive){
		stat[i].energyValues.reactive = reactive;
	}
	int32_t getActiveEnergyAsInt(uint8_t i=0xff){
		if(i==0xff)
			return (int32_t)round(stat[0].energyValues.active+stat[1].energyValues.active+stat[2].energyValues.active);
		else
			return (int32_t)round(stat[i].energyValues.active);
	}
	int32_t getReactiveEnergyAsInt(uint8_t i=0xff){
		if(i==0xff)
			return (int32_t)round(stat[0].energyValues.reactive+stat[1].energyValues.reactive+stat[2].energyValues.reactive);
		else
			return (int32_t)round(stat[i].energyValues.reactive);
	}
	void updatePower(uint8_t i){
		stat[i].measureValues.aPow = stat[i].measureValues.current * stat[i].measureValues.voltage * stat[i].measureValues.pfactor;
		stat[i].measureValues.rPow = stat[i].measureValues.current * stat[i].measureValues.voltage * (1 - stat[i].measureValues.pfactor);
	}
	double getPFactor(uint8_t i){
		return stat[i].measureValues.pfactor;
	}
	void setPFactor(uint8_t i, double pfactor){
		stat[i].measureValues.pfactor = pfactor;
	}
	double getFreq(uint8_t i){
		return (stat[i].measureValues.freq);
	}
	void setFreq(uint8_t i, double freq){
		stat[i].measureValues.freq = freq;
	}
	void fixCurrent(uint8_t i=0xff){
		if(i==0xff){
			for(int j=0;j<3;j++){
				double divisor = (stat[j].measureValues.voltage * stat[j].measureValues.pfactor);
				stat[j].measureValues.current = (divisor < 0.01)? 0 : stat[j].measureValues.aPow/divisor;
			}
		}
		else{
			double divisor = (stat[i].measureValues.voltage * stat[i].measureValues.pfactor);
			stat[i].measureValues.current = (divisor < 0.01)? 0 : stat[i].measureValues.aPow/divisor;
		}
	}
	double getTemperature(uint8_t i=0xff){
		if(i==0xff)
			return cpp_utils::max3(stat[0].measureValues.temp, stat[1].measureValues.temp, stat[2].measureValues.temp);
		else
			return (stat[i].measureValues.temp);
	}
	double getStatus(uint8_t i=0xff){
		if(i==0xff)
			return cpp_utils::max3(stat[0].measureValues.status, stat[1].measureValues.status, stat[2].measureValues.status);
		else
			return (stat[i].measureValues.status);
	}

};



/**Objeto ppl:energy:analyzer:cfg:calib */
struct metering_analyzer_cfg_calib{
	uint16_t meterRegs[MeteringAnalyzerCfgCalibRegCount];
	uint16_t measRegs[MeteringAnalyzerCfgCalibRegCount];
	uint8_t _keys;
};


/**Objeto ppl:energy:analyzer:cfg:minmax */
struct metering_analyzer_cfg_minmax{
	common_range_minmaxthres_double voltage;
	common_range_minmaxthres_double current;
	common_range_minmaxthres_double phase;
	common_range_minmaxthres_double pfactor;
	common_range_minmaxthres_double aPow;
	common_range_minmaxthres_double rPow;
	common_range_minmaxthres_double msPow;
	common_range_minmaxthres_double freq;
	common_range_minmaxthres_double thdA;
	common_range_minmaxthres_double thdV;
	common_range_minmaxthres_double active;
	common_range_minmaxthres_double reactive;
	uint32_t _keys;
};


/**Objeto ppl:energy:analyzer:cfg */
struct metering_analyzer_cfg{
	uint32_t updFlags;
	uint32_t evtFlags;
	metering_analyzer_cfg_minmax minmaxData;
	metering_analyzer_cfg_calib calibData;
	uint8_t _keys;
};


/**Objeto ppl:energy:analyzer */
struct metering_analyzer{
	uint32_t uid;
	char serial[MeteringAnalyzerSerialLength];
	metering_analyzer_cfg cfg;
	metering_analyzer_stat stat;
};


/**Objeto ppl:energy:stat */
struct metering_manager_stat{
	uint8_t loadPercent[MeteringManagerCfgMaxNumAnalyzers];
	uint8_t _numAnalyzers;
};


/**Objeto ppl:energy:cfg */
struct metering_manager_cfg{
	uint32_t updFlags;
	uint16_t measPeriod;
	uint8_t  verbosity;
	uint32_t nvs_id;
	uint8_t _keys;
};


/** Objeto ppl:energy */
struct metering_manager{
	uint32_t uid;
	metering_manager_stat stat;
	metering_manager_cfg cfg;
	metering_analyzer analyzers[MeteringManagerCfgMaxNumAnalyzers];
	uint8_t _numAnalyzers;
};

namespace JSON {

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromMeteringManager(const metering_manager& obj, ObjDataSelection type);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromMeteringManagerCfg(const metering_manager_cfg& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromMeteringManagerStat(const metering_manager_stat& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromMeteringAnalyzer(const metering_analyzer& obj, ObjDataSelection type);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromMeteringAnalyzerCfg(const metering_analyzer_cfg& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromMeteringAnalyzerCfgMinMax(const metering_analyzer_cfg_minmax& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromMeteringAnalyzerCfgCalib(const metering_analyzer_cfg_calib& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromMeteringAnalyzerStat(const metering_analyzer_stat& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromMeteringAnalyzerStatTotals(const metering_analyzer_stat_totals& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromMeteringAnalyzerStatMeasure(const metering_analyzer_stat_measure& obj);



/**
 * Codifica el objeto en un JSON dependiendo del tipo de objeto
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
template <typename T>
cJSON* getJsonFromMetering(const T& obj, ObjDataSelection type){
	if (std::is_same<T, metering_manager>::value){
		return getJsonFromMeteringManager((const metering_manager&)obj, type);
	}
	if (std::is_same<T, metering_manager_cfg>::value && type != ObjSelectState && type != ObjSelectStateSub){
		return getJsonFromMeteringManagerCfg((const metering_manager_cfg&)obj);
	}
	if (std::is_same<T, metering_manager_stat>::value && type != ObjSelectCfg){
		return getJsonFromMeteringManagerStat((const metering_manager_stat&)obj);
	}
	if (std::is_same<T, metering_analyzer>::value){
		return getJsonFromMeteringAnalyzer((const metering_analyzer&)obj, type);
	}
	if (std::is_same<T, metering_analyzer_cfg>::value && type != ObjSelectState && type != ObjSelectStateSub){
		return getJsonFromMeteringAnalyzerCfg((const metering_analyzer_cfg&)obj);
	}
	if (std::is_same<T, metering_analyzer_cfg_minmax>::value && type != ObjSelectState && type != ObjSelectStateSub){
		return getJsonFromMeteringAnalyzerCfgMinMax((const metering_analyzer_cfg_minmax&)obj);
	}
	if (std::is_same<T, metering_analyzer_cfg_calib>::value && type != ObjSelectState && type != ObjSelectStateSub){
		return getJsonFromMeteringAnalyzerCfgCalib((const metering_analyzer_cfg_calib&)obj);
	}
	if (std::is_same<T, metering_analyzer_stat>::value && type != ObjSelectCfg){
		return getJsonFromMeteringAnalyzerStat((const metering_analyzer_stat&)obj);
	}
	if (std::is_same<T, metering_analyzer_stat_totals>::value && type != ObjSelectCfg){
		return getJsonFromMeteringAnalyzerStatTotals((const metering_analyzer_stat_totals&)obj);
	}
	if (std::is_same<T, metering_analyzer_stat_measure>::value && type != ObjSelectCfg){
		return getJsonFromMeteringAnalyzerStatMeasure((const metering_analyzer_stat_measure&)obj);
	}
	return NULL;
}


/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getMeteringManagerFromJson(metering_manager &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getMeteringManagerCfgFromJson(metering_manager_cfg &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getMeteringManagerStatFromJson(metering_manager_stat &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getMeteringAnalyzerFromJson(metering_analyzer &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getMeteringAnalyzerCfgFromJson(metering_analyzer_cfg &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getMeteringAnalyzerCfgMinMaxFromJson(metering_analyzer_cfg_minmax &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getMeteringAnalyzerCfgCalibFromJson(metering_analyzer_cfg_calib &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getMeteringAnalyzerStatFromJson(metering_analyzer_stat &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getMeteringAnalyzerStatTotalsFromJson(metering_analyzer_stat_totals &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Par�metros decodificados o 0 en caso de error
 */
uint32_t getMeteringAnalyzerStatMeasureFromJson(metering_analyzer_stat_measure &obj, cJSON* json);


template <typename T>
uint32_t getMeteringObjFromJson(T& obj, cJSON* json_obj){
	if (std::is_same<T, metering_manager>::value){
		return JSON::getMeteringManagerFromJson((metering_manager&)obj, json_obj);
	}
	if (std::is_same<T, metering_manager_cfg>::value){
		return JSON::getMeteringManagerCfgFromJson((metering_manager_cfg&)obj, json_obj);
	}
	if (std::is_same<T, metering_manager_stat>::value){
		return JSON::getMeteringManagerStatFromJson((metering_manager_stat&)obj, json_obj);
	}
	if (std::is_same<T, metering_analyzer>::value){
		return JSON::getMeteringAnalyzerFromJson((metering_analyzer&)obj, json_obj);
	}
	if (std::is_same<T, metering_analyzer_cfg>::value){
		return JSON::getMeteringAnalyzerCfgFromJson((metering_analyzer_cfg&)obj, json_obj);
	}
	if (std::is_same<T, metering_analyzer_cfg_minmax>::value){
		return JSON::getMeteringAnalyzerCfgMinMaxFromJson((metering_analyzer_cfg_minmax&)obj, json_obj);
	}
	if (std::is_same<T, metering_analyzer_cfg_calib>::value){
		return JSON::getMeteringAnalyzerCfgCalibFromJson((metering_analyzer_cfg_calib&)obj, json_obj);
	}
	if (std::is_same<T, metering_analyzer_stat>::value){
		return JSON::getMeteringAnalyzerStatFromJson((metering_analyzer_stat&)obj, json_obj);
	}
	if (std::is_same<T, metering_analyzer_stat_totals>::value){
		return JSON::getMeteringAnalyzerStatTotalsFromJson((metering_analyzer_stat_totals&)obj, json_obj);
	}
	if (std::is_same<T, metering_analyzer_stat_measure>::value){
		return JSON::getMeteringAnalyzerStatMeasureFromJson((metering_analyzer_stat_measure&)obj, json_obj);
	}
	return 0;
}

}	// end namespace JSON

#endif
