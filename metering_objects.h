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
#include "cJSON.h"


/** UIDs */
#define UID_METERING_MANAGER(vers)					(uint32_t)(0x00000001 | ((uint32_t)vers << 20))
#define UID_METERING_MANAGER_CFG(vers)				(uint32_t)(0x00000002 | ((uint32_t)vers << 20))
#define UID_METERING_MANAGER_STAT(vers)				(uint32_t)(0x00000003 | ((uint32_t)vers << 20))
#define UID_METERING_ANALYZER(vers)					(uint32_t)(0x00000004 | ((uint32_t)vers << 20))
#define UID_METERING_ANALYZER_CFG(vers)				(uint32_t)(0x00000005 | ((uint32_t)vers << 20))
#define UID_METERING_ANALYZER_CFG_MINMAX(vers)		(uint32_t)(0x00000006 | ((uint32_t)vers << 20))
#define UID_METERING_ANALYZER_CFG_CALIB(vers)		(uint32_t)(0x00000007 | ((uint32_t)vers << 20))
#define UID_METERING_ANALYZER_STAT(vers)			(uint32_t)(0x00000008 | ((uint32_t)vers << 20))
#define UID_METERING_ANALYZER_STAT_TOTALS(vers)		(uint32_t)(0x00000009 | ((uint32_t)vers << 20))
#define UID_METERING_ANALYZER_STAT_MEASURE(vers)	(uint32_t)(0x0000000A | ((uint32_t)vers << 20))

/** Versiones */
#define VERS_METERING_EMi10_YTL		0
#define VERS_METERING_NAME_0		"EMi10YTL"

#define VERS_METERING_M90E26		1
#define VERS_METERING_NAME_1		"M90E26"


static inline const char* VERS_METERING_NAME(int vers){
	switch(vers){
		case VERS_METERING_EMi10_YTL:	return VERS_METERING_NAME_0;
		case VERS_METERING_M90E26:		return VERS_METERING_NAME_1;
		default: 						return "";
	}
}


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
	MeteringAnalyzerVoltageOverLimitEvt		= (1 << 0),		//!< Evento al superar el l�mite superior (voltaje)
	MeteringAnalyzerVoltageBelowLimitEvt	= (1 << 1),		//!< Evento al superar el l�mite inferior (voltaje)
	MeteringAnalyzerVoltageInRangeEvt 		= (1 << 2),		//!< Evento al volver a entrar en rango (voltaje)
	MeteringAnalyzerCurrentOverLimitEvt		= (1 << 3),		//!< Evento al superar el l�mite superior (corriente)
	MeteringAnalyzerCurrentBelowLimitEvt 	= (1 << 4),		//!< Evento al superar el l�mite inferior (corriente)
	MeteringAnalyzerCurrentInRangeEvt 		= (1 << 5),		//!< Evento al volver a entrar en rango (corriente)
	MeteringAnalyzerPhaseOverLimitEvt		= (1 << 6),		//!< Evento al superar el l�mite superior (fase)
	MeteringAnalyzerPhaseBelowLimitEvt 		= (1 << 7),		//!< Evento al superar el l�mite inferior (fase)
	MeteringAnalyzerPhaseInRangeEvt 		= (1 << 8),		//!< Evento al volver a entrar en rango (fase)
	MeteringAnalyzerPFactorOverLimitEvt		= (1 << 9),		//!< Evento al superar el l�mite superior (factor de potencia)
	MeteringAnalyzerPFactorBelowLimitEvt 	= (1 << 10),	//!< Evento al superar el l�mite inferior (factor de potencia)
	MeteringAnalyzerPFactorInRangeEvt 		= (1 << 11),	//!< Evento al volver a entrar en rango (factor de potencia)
	MeteringAnalyzerCosPhyOverLimitEvt		= (1 << 12),	//!< Evento al superar el l�mite superior (coseno phy)
	MeteringAnalyzerCosPhyBelowLimitEvt 	= (1 << 13),	//!< Evento al superar el l�mite inferior (coseno phy)
	MeteringAnalyzerCosPhyInRangeEvt 		= (1 << 14),	//!< Evento al volver a entrar en rango (coseno phy)
	MeteringAnalyzerActPowOverLimitEvt		= (1 << 15),	//!< Evento al superar el l�mite superior (potencia activa)
	MeteringAnalyzerActPowBelowLimitEvt 	= (1 << 16),	//!< Evento al superar el l�mite inferior (potencia activa)
	MeteringAnalyzerActPowInRangeEvt 		= (1 << 17),	//!< Evento al volver a entrar en rango (potencia activa)
	MeteringAnalyzerReactPowOverLimitEvt	= (1 << 18),	//!< Evento al superar el l�mite superior (potencia reactiva)
	MeteringAnalyzerReactPowBelowLimitEvt	= (1 << 19),	//!< Evento al superar el l�mite inferior (potencia reactiva)
	MeteringAnalyzerReactPowInRangeEvt 		= (1 << 20),	//!< Evento al volver a entrar en rango (potencia reactiva)
	MeteringAnalyzerFrequencyOverLimitEvt	= (1 << 21),	//!< Evento al superar el l�mite superior (frecuencia)
	MeteringAnalyzerFrequencyBelowLimitEvt	= (1 << 22),	//!< Evento al superar el l�mite inferior (frecuencia)
	MeteringAnalyzerFrequencyInRangeEvt 	= (1 << 23),	//!< Evento al volver a entrar en rango (frecuencia)
	MeteringAnalyzerThdAOverLimitEvt		= (1 << 24),	//!< Evento al superar el l�mite superior (THD-A)
	MeteringAnalyzerThdABelowLimitEvt		= (1 << 25),	//!< Evento al superar el l�mite inferior (THD-A)
	MeteringAnalyzerThdAInRangeEvt 			= (1 << 26),	//!< Evento al volver a entrar en rango (THD-A)
	MeteringAnalyzerThdVOverLimitEvt		= (1 << 27),	//!< Evento al superar el l�mite superior (THD-V)
	MeteringAnalyzerThdVBelowLimitEvt		= (1 << 28),	//!< Evento al superar el l�mite inferior (THD-V)
	MeteringAnalyzerThdVInRangeEvt 			= (1 << 29),	//!< Evento al volver a entrar en rango (THD-V)
	MeteringAnalyzerInstantMeasureEvt 		= (1 << 30),	//!< Evento al realizar una medida
	/* elemento inv�lido */
	MeteringAnalyzerInvalidEvt				= (1 << 31)
};

/** Flags para la variable ppl:energy:analyzer:stat/flags */
typedef metering_analyzer_cfg_evtFlags metering_analyzer_stat_flags;


/** Rango m�nimo y valor por defecto de la variable ppl:energy:cfg/measPeriod */
static const uint32_t MeteringManagerCfgMeasPeriodMin 	= 15;
static const uint32_t MeteringManagerCfgMeasPeriodDefault = 900;

/** M�ximo n�mero de analizadores permitidos */
static const uint8_t MeteringManagerCfgMaxNumAnalyzers = 3;

/** Tama�o m�ximo del texto asociado a la variable ppl:energy:analyzer/serial */
static const uint8_t MeteringAnalyzerSerialLength = 16;

/** N�mero de registros de calibraci�n del driver M90E26 en la variable ppl:energy:analyzer:cfg:calib/meterRegs */
static const uint8_t MeteringAnalyzerCfgCalibRegCount = 16;


/**Objeto ppl:energy:analyzer:stat:measure */
struct metering_analyzer_stat_measure{
	uint32_t uid;
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
};


/**Objeto ppl:energy:analyzer:stat:totals */
struct metering_analyzer_stat_totals{
	uint32_t uid;
	double active;
	double reactive;
};


/**Objeto ppl:energy:analyzer:stat */
struct metering_analyzer_stat{
	uint32_t uid;
	uint32_t flags;
	metering_analyzer_stat_totals energyValues;
	metering_analyzer_stat_measure measureValues;
};


/**Objeto ppl:energy:analyzer:cfg:calib */
struct metering_analyzer_cfg_calib{
	uint32_t uid;
	uint16_t meterRegs[MeteringAnalyzerCfgCalibRegCount];
	uint16_t measRegs[MeteringAnalyzerCfgCalibRegCount];
	uint8_t _keys;
};


/**Objeto ppl:energy:analyzer:cfg:minmax */
struct metering_analyzer_cfg_minmax{
	uint32_t uid;
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
	uint8_t _keys;
};


/**Objeto ppl:energy:analyzer:cfg */
struct metering_analyzer_cfg{
	uint32_t uid;
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
	uint32_t uid;
	uint8_t loadPercent[MeteringManagerCfgMaxNumAnalyzers];
	uint8_t _numAnalyzers;
};


/**Objeto ppl:energy:cfg */
struct metering_manager_cfg{
	uint32_t uid;
	uint32_t updFlags;
	uint16_t measPeriod;
	uint8_t  verbosity;
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
	if (std::is_same<T, metering_manager_cfg>::value && type != ObjSelectState){
		return getJsonFromMeteringManagerCfg((const metering_manager_cfg&)obj);
	}
	if (std::is_same<T, metering_manager_stat>::value && type != ObjSelectCfg){
		return getJsonFromMeteringManagerStat((const metering_manager_stat&)obj);
	}
	if (std::is_same<T, metering_analyzer>::value){
		return getJsonFromMeteringAnalyzer((const metering_analyzer&)obj, type);
	}
	if (std::is_same<T, metering_analyzer_cfg>::value && type != ObjSelectState){
		return getJsonFromMeteringAnalyzerCfg((const metering_analyzer_cfg&)obj);
	}
	if (std::is_same<T, metering_analyzer_cfg_minmax>::value && type != ObjSelectState){
		return getJsonFromMeteringAnalyzerCfgMinMax((const metering_analyzer_cfg_minmax&)obj);
	}
	if (std::is_same<T, metering_analyzer_cfg_calib>::value && type != ObjSelectState){
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
