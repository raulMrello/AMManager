/*
 * ppl_energy_objects.h
 *
 *  Created on: Mar 2019
 *      Author: raulMrello
 *
 *	Objetos JSON relativos al módulo ppl:energy
 */
 
#ifndef PPL_ENERGY_
#define PPL_ENERGY_

#include <cstdint>
#include <list>
#include "common_objects.h"


/** Flags para la variable ppl:energy:cfg/updFlags */
enum ppl_energy_cfg_updFlags {
	PplEnergyCfgUpdNotif 		= (1 << 0),	/// Habilita notificación de cambios en cualquier parámetro de la configuración
	PplEnergyCfgMinMaxUpdNotif 	= (1 << 1),	/// Habilita notificación de cambios en ppl:energy:analyzer:cfg:minmax
	PplEnergyCfgCalibUpdNotif 	= (1 << 2),	/// Habilita notificación de cambios en ppl:energy:analyzer:cfg:calib
};

/** Flags para la variable ppl:energy:analyzer:cfg/updFlags */
typedef ppl_energy_cfg_updFlags ppl_energy_analyzer_cfg_updFlags;


/** Flags para la variable ppl:energy:analyzer:cfg/evtFlags */
enum ppl_energy_analyzer_cfg_evtFlags{
	PplEnergyNoEvents				= 0,			//!< Flag para indicar que no hay eventos
	PplEnergyVoltageOverLimitEvt	= (1 << 0),		//!< Evento al superar el límite superior (voltaje)
	PplEnergyVoltageBelowLimitEvt	= (1 << 1),		//!< Evento al superar el límite inferior (voltaje)
	PplEnergyVoltageInRangeEvt 		= (1 << 2),		//!< Evento al volver a entrar en rango (voltaje)
	PplEnergyCurrentOverLimitEvt	= (1 << 3),		//!< Evento al superar el límite superior (corriente)
	PplEnergyCurrentBelowLimitEvt 	= (1 << 4),		//!< Evento al superar el límite inferior (corriente)
	PplEnergyCurrentInRangeEvt 		= (1 << 5),		//!< Evento al volver a entrar en rango (corriente)
	PplEnergyPhaseOverLimitEvt		= (1 << 6),		//!< Evento al superar el límite superior (fase)
	PplEnergyPhaseBelowLimitEvt 	= (1 << 7),		//!< Evento al superar el límite inferior (fase)
	PplEnergyPhaseInRangeEvt 		= (1 << 8),		//!< Evento al volver a entrar en rango (fase)
	PplEnergyPFactorOverLimitEvt	= (1 << 9),		//!< Evento al superar el límite superior (factor de potencia)
	PplEnergyPFactorBelowLimitEvt 	= (1 << 10),	//!< Evento al superar el límite inferior (factor de potencia)
	PplEnergyPFactorInRangeEvt 		= (1 << 11),	//!< Evento al volver a entrar en rango (factor de potencia)
	PplEnergyCosPhyOverLimitEvt		= (1 << 12),	//!< Evento al superar el límite superior (coseno phy)
	PplEnergyCosPhyBelowLimitEvt 	= (1 << 13),	//!< Evento al superar el límite inferior (coseno phy)
	PplEnergyCosPhyInRangeEvt 		= (1 << 14),	//!< Evento al volver a entrar en rango (coseno phy)
	PplEnergyActPowOverLimitEvt		= (1 << 15),	//!< Evento al superar el límite superior (potencia activa)
	PplEnergyActPowBelowLimitEvt 	= (1 << 16),	//!< Evento al superar el límite inferior (potencia activa)
	PplEnergyActPowInRangeEvt 		= (1 << 17),	//!< Evento al volver a entrar en rango (potencia activa)
	PplEnergyReactPowOverLimitEvt	= (1 << 18),	//!< Evento al superar el límite superior (potencia reactiva)
	PplEnergyReactPowBelowLimitEvt	= (1 << 19),	//!< Evento al superar el límite inferior (potencia reactiva)
	PplEnergyReactPowInRangeEvt 	= (1 << 20),	//!< Evento al volver a entrar en rango (potencia reactiva)
	PplEnergyFrequencyOverLimitEvt	= (1 << 21),	//!< Evento al superar el límite superior (frecuencia)
	PplEnergyFrequencyBelowLimitEvt	= (1 << 22),	//!< Evento al superar el límite inferior (frecuencia)
	PplEnergyFrequencyInRangeEvt 	= (1 << 23),	//!< Evento al volver a entrar en rango (frecuencia)
	PplEnergyThdAOverLimitEvt		= (1 << 24),	//!< Evento al superar el límite superior (THD-A)
	PplEnergyThdABelowLimitEvt		= (1 << 25),	//!< Evento al superar el límite inferior (THD-A)
	PplEnergyThdAInRangeEvt 		= (1 << 26),	//!< Evento al volver a entrar en rango (THD-A)
	PplEnergyThdVOverLimitEvt		= (1 << 27),	//!< Evento al superar el límite superior (THD-V)
	PplEnergyThdVBelowLimitEvt		= (1 << 28),	//!< Evento al superar el límite inferior (THD-V)
	PplEnergyThdVInRangeEvt 		= (1 << 29),	//!< Evento al volver a entrar en rango (THD-V)
	PplEnergyInstantMeasureEvt 		= (1 << 30),	//!< Evento al realizar una medida
	/* elemento inválido */
	PplEnergyInvalidEvt				= (1 << 31)
};

/** Flags para la variable ppl:energy:analyzer:stat/flags */
typedef ppl_energy_analyzer_cfg_evtFlags ppl_energy_analyzer_stat_flags;


/** Rango mínimo y valor por defecto de la variable ppl:energy:cfg/measPeriod */
static const uint32_t PplEnergyCfgMeasPeriodMin 	= 15;
static const uint32_t PplEnergyCfgMeasPeriodDefault = 900;

/** Tamaño máximo del texto asociado a la variable ppl:energy:analyzer/serial */
static const uint8_t PplEnergyAnalyzerSerialLength = 16;

/** Número de registros de calibración del driver M90E26 en la variable ppl:energy:analyzer:cfg:calib/meterRegs */
static const uint8_t PplEnergyAnalyzerCfgCalibRegCount = 16;


/**Objeto ppl:energy:analyzer:stat:measure */
struct ppl_energy_analyzer_stat_measure{
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
struct ppl_energy_analyzer_stat_totals{
	uint32_t uid;
	double active;
	double reactive;
};


/**Objeto ppl:energy:analyzer:stat */
struct ppl_energy_analyzer_stat{
	uint32_t uid;
	uint32_t flags;
	ppl_energy_analyzer_stat_totals energyValues;
	ppl_energy_analyzer_stat_measure measureValues;
};


/**Objeto ppl:energy:analyzer:cfg:calib */
struct ppl_energy_analyzer_cfg_calib{
	uint32_t uid;
	uint16_t meterRegs[PplEnergyAnalyzerCfgCalibRegCount];
	uint16_t measRegs[PplEnergyAnalyzerCfgCalibRegCount];
};


/**Objeto ppl:energy:analyzer:cfg:minmax */
struct ppl_energy_analyzer_cfg_minmax{
	uint32_t uid;
	range_minmaxthres_double voltage;
	range_minmaxthres_double current;
	range_minmaxthres_double phase;
	range_minmaxthres_double pfactor;
	range_minmaxthres_double aPow;
	range_minmaxthres_double rPow;
	range_minmaxthres_double msPow;
	range_minmaxthres_double freq;
	range_minmaxthres_double thdA;
	range_minmaxthres_double thdV;
};


/**Objeto ppl:energy:analyzer:cfg */
struct ppl_energy_analyzer_cfg{
	uint32_t uid;
	uint32_t updFlags;
	uint32_t evtFlags;
	ppl_energy_analyzer_cfg_minmax minmaxData;
	ppl_energy_analyzer_cfg_calib calibData;
};


/**Objeto ppl:energy:analyzer */
struct ppl_energy_analyzer{
	uint32_t uid;
	char serial[PplEnergyAnalyzerSerialLength];
	ppl_energy_analyzer_cfg cfg;
	ppl_energy_analyzer_stat stat;
};


/**Objeto ppl:energy:stat */
struct ppl_energy_stat{
	uint32_t uid;
	std::list<ppl_energy_analyzer_stat*> analyzers;
};


/**Objeto ppl:energy:cfg */
struct ppl_energy_cfg{
	uint32_t uid;
	uint32_t updFlags;
	uint16_t measPeriod;
	uint8_t  verbosity;
	std::list<ppl_energy_analyzer_cfg*> analyzers;
};


/** Objeto ppl:energy */
struct ppl_energy{
	uint32_t uid;
	ppl_energy_stat stat;
	ppl_energy_cfg cfg;
};

namespace JSON {

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromPplEnergy(const ppl_energy& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromPplEnergyCfg(const ppl_energy_cfg& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromPplEnergyStat(const ppl_energy_stat& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromPplEnergyAnalyzer(const ppl_energy_analyzer& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromPplEnergyAnalyzerCfg(const ppl_energy_analyzer_cfg& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromPplEnergyAnalyzerCfgMinMax(const ppl_energy_analyzer_cfg_minmax& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromPplEnergyAnalyzerCfgCalib(const ppl_energy_analyzer_cfg_calib& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromPplEnergyAnalyzerStat(const ppl_energy_analyzer_stat& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromPplEnergyAnalyzerStatTotals(const ppl_energy_analyzer_stat_totals& obj);

/**
 * Codifica el objeto en un JSON
 * @param obj Objeto
 * @return JSON resultante o NULL en caso de error
 */
cJSON* getJsonFromPplEnergyAnalyzerStatMeasure(const ppl_energy_analyzer_stat_measure& obj);


/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getPplEnergyFromJson(ppl_energy &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getPplEnergyCfgFromJson(ppl_energy_cfg &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getPplEnergyStatFromJson(ppl_energy_stat &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getPplEnergyAnalyzerFromJson(ppl_energy_analyzer &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getPplEnergyAnalyzerCfgFromJson(ppl_energy_analyzer_cfg &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getPplEnergyAnalyzerCfgMinMaxFromJson(ppl_energy_analyzer_cfg_minmax &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getPplEnergyAnalyzerCfgCalibFromJson(ppl_energy_analyzer_cfg_calib &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getPplEnergyAnalyzerStatFromJson(ppl_energy_analyzer_stat &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getPplEnergyAnalyzerStatTotalsFromJson(ppl_energy_analyzer_stat_totals &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getPplEnergyAnalyzerStatMeasureFromJson(ppl_energy_analyzer_stat_measure &obj, cJSON* json);


}	// end namespace JSON

#endif
