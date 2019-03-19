/*
 * AMManagerBlob.h
 *
 *  Created on: Ene 2018
 *      Author: raulMrello
 *
 *	AMManagerBlob es el componente del módulo AMManager en el que se definen los objetos y tipos relativos a
 *	los objetos BLOB de este módulo.
 *	Todos los tipos definidos en este componente están asociados al namespace "Blob", de forma que puedan ser
 *	accesibles mediante el uso de: "Blob::"  e importando este archivo de cabecera.
 */
 
#ifndef AMManagerBlob
#define AMManagerBlob

#include "Blob.h"
#include "mbed.h"


#define AM_OBJ_VERSION_DEFAULT	""
#define AM_OBJ_VERSION_M90E26	"m90e26"

namespace Blob {


/** Flags para la configuración de notificaciones de un objeto AMManager cuando su configuración se ha
 *  modificado.
 */
 enum AMUpdFlags{
	 EnableAMCfgUpdNotif 		= (1 << 0),	/// Flag activado para notificar cambios en la configuración en bloque del objeto
 };


/** Cadencia mínima y por defecto permitida de envío de medidas instantáneas en segundos (1min) */
static const uint32_t AMMinMeasPeriod = 10;
static const uint32_t AMDefaultMeasPeriod = 900;


/** Número de registros de calibración para la medida y para el analizador */
static const uint16_t AMCalibRegCount = 16;


/** Tamaño máximo de los parámetros textuales */
static const uint16_t AMTextParamLength = 32;


/** Tamaño máximo del nombre de las versiones de los objetos */
static const uint8_t AMVersionLength = 16;


 /** Flags de evento que utiliza AMManager para notificar un cambio de estado o alarma
  */
 enum AMEvtFlags{
	 AMNoEvents					= 0,			//!< Flag para indicar que no hay eventos
 	 AMVoltageOverLimitEvt		= (1 << 0),		//!< Evento al superar el límite superior (voltaje)
	 AMVoltageBelowLimitEvt		= (1 << 1),		//!< Evento al superar el límite inferior (voltaje)
	 AMVoltageInRangeEvt 		= (1 << 2),		//!< Evento al volver a entrar en rango (voltaje)
	 AMCurrentOverLimitEvt		= (1 << 3),		//!< Evento al superar el límite superior (corriente)
	 AMCurrentBelowLimitEvt 	= (1 << 4),		//!< Evento al superar el límite inferior (corriente)
	 AMCurrentInRangeEvt 		= (1 << 5),		//!< Evento al volver a entrar en rango (corriente)
	 AMPhaseOverLimitEvt		= (1 << 6),		//!< Evento al superar el límite superior (fase)
	 AMPhaseBelowLimitEvt 		= (1 << 7),		//!< Evento al superar el límite inferior (fase)
	 AMPhaseInRangeEvt 			= (1 << 8),		//!< Evento al volver a entrar en rango (fase)
	 AMPFactorOverLimitEvt		= (1 << 9),		//!< Evento al superar el límite superior (factor de potencia)
	 AMPFactorBelowLimitEvt 	= (1 << 10),	//!< Evento al superar el límite inferior (factor de potencia)
	 AMPFactorInRangeEvt 		= (1 << 11),	//!< Evento al volver a entrar en rango (factor de potencia)
	 AMCosPhyOverLimitEvt		= (1 << 12),	//!< Evento al superar el límite superior (coseno phy)
	 AMCosPhyBelowLimitEvt 		= (1 << 13),	//!< Evento al superar el límite inferior (coseno phy)
	 AMCosPhyInRangeEvt 		= (1 << 14),	//!< Evento al volver a entrar en rango (coseno phy)
	 AMActPowOverLimitEvt		= (1 << 15),	//!< Evento al superar el límite superior (potencia activa)
	 AMActPowBelowLimitEvt 		= (1 << 16),	//!< Evento al superar el límite inferior (potencia activa)
	 AMActPowInRangeEvt 		= (1 << 17),	//!< Evento al volver a entrar en rango (potencia activa)
	 AMReactPowOverLimitEvt		= (1 << 18),	//!< Evento al superar el límite superior (potencia reactiva)
	 AMReactPowBelowLimitEvt	= (1 << 19),	//!< Evento al superar el límite inferior (potencia reactiva)
	 AMReactPowInRangeEvt 		= (1 << 20),	//!< Evento al volver a entrar en rango (potencia reactiva)
	 AMFrequencyOverLimitEvt	= (1 << 21),	//!< Evento al superar el límite superior (frecuencia)
	 AMFrequencyBelowLimitEvt	= (1 << 22),	//!< Evento al superar el límite inferior (frecuencia)
	 AMFrequencyInRangeEvt 		= (1 << 23),	//!< Evento al volver a entrar en rango (frecuencia)
	 AMThdAOverLimitEvt			= (1 << 24),	//!< Evento al superar el límite superior (THD-A)
	 AMThdABelowLimitEvt		= (1 << 25),	//!< Evento al superar el límite inferior (THD-A)
	 AMThdAInRangeEvt 			= (1 << 26),	//!< Evento al volver a entrar en rango (THD-A)
	 AMThdVOverLimitEvt			= (1 << 27),	//!< Evento al superar el límite superior (THD-V)
	 AMThdVBelowLimitEvt		= (1 << 28),	//!< Evento al superar el límite inferior (THD-V)
	 AMThdVInRangeEvt 			= (1 << 29),	//!< Evento al volver a entrar en rango (THD-V)
	 AMInstantMeasureEvt 		= (1 << 30),	//!< Evento al realizar una medida
	 /* elemento inválido */
	 AMInvalidEvt				= (1 << 31)
  };


 /** Flags para identificar cada key-value de los objetos JSON que se han modificado en un SET remoto
  */
enum AMKeyNames{
	AMKeyNone 			= 0,
 	AMKeyCfgUpd			= (1 << 0),
 	AMKeyCfgEvt			= (1 << 1),
 	AMKeyCfgMeas		= (1 << 2),
 	AMKeyCfgMnxVolt		= (1 << 3),
 	AMKeyCfgMnxCurr		= (1 << 4),
 	AMKeyCfgMnxPhase	= (1 << 5),
 	AMKeyCfgMnxPfact	= (1 << 6),
 	AMKeyCfgMnxApow		= (1 << 7),
 	AMKeyCfgMnxRpow		= (1 << 8),
 	AMKeyCfgMnxMpow		= (1 << 9),
 	AMKeyCfgMnxFreq		= (1 << 10),
 	AMKeyCfgMnxThdA		= (1 << 11),
 	AMKeyCfgMnxThdV		= (1 << 12),
 	AMKeyCfgCalMetr		= (1 << 13),
 	AMKeyCfgCalMea		= (1 << 14),
	AMKeyCfgVerbosity	= (1 << 15),
	AMKeyCfgVersion		= (1 << 16),
	//
	AMKeyCfgAll			= 0x1FFFF,
};


 /** Estructuras de datos para la configuración de rangos de funcionamiento min-max
  * 	Se forma por diferentes pares min-max
  * 	@var voltage Voltaje (V)
  * 	@var current Corriente (A)
  * 	@var phase Fase (º)
  * 	@var pfactor Factor de potencia (pf)
  * 	@var aPow Potencia activa (KW)
  * 	@var rPow Potencia reactiva (KW)
  * 	@var msPow Potencia media aparente (KVA)
  * 	@var freq Frecuencia (Hz)
  */
struct __packed AMMinMax_t{
	 double min;
	 double max;
	 double thres;
 };


struct __packed AMMinMaxData_t{
	char version[AMVersionLength];
 	AMMinMax_t voltage;
 	AMMinMax_t current;
 	AMMinMax_t phase;
 	AMMinMax_t pfactor;
 	AMMinMax_t msPow;
 	AMMinMax_t aPow;
 	AMMinMax_t rPow;
 	AMMinMax_t freq;
 	AMMinMax_t thdA;
 	AMMinMax_t thdV;
};


/** Estructuras de datos para la calibración
 * 	@var meterRegs Valores de calibración de los registros de medidor
 * 	@var measRegs Valores de calibración de los registros de medida
 */
struct __packed AMCalibData_t{
	char version[AMVersionLength];
	uint16_t meterRegs[AMCalibRegCount];
	uint16_t measRegs[AMCalibRegCount];
};


  /** Estructura de datos para la configuración en bloque del objeto AMManager.
   * 	Se forma por las distintas estructuras de datos de configuración
   * 	@var updFlags Flags de configuración de notificación de cambios de configuración
   * 	@var evtFlags Flags de configuración de notificación de eventos
   * 	@var measPeriod Cadencia de envío de medidas instantáneas (en segundos)
   * 	@var minmaxData Datos de rangos mín-max.
   * 	@var calibData Datos de calibración
   * 	@var verbosity Nivel de visualización de trazas de depuración
   */
struct __packed AMCfgData_t{
	char version[AMVersionLength];
	AMUpdFlags updFlagMask;
	AMEvtFlags evtFlagMask;
	uint32_t measPeriod;
	AMMinMaxData_t minmaxData;
	AMCalibData_t calibData;
	esp_log_level_t verbosity;
};


/** Valores de medida de energía
 * 	@var active Energía activa en KW
 * 	@var reactive Energía reactiva en KW
 */
struct __packed AMEnergyValues_t{
	char version[AMVersionLength];
	int32_t active;
	int32_t reactive;
};


/** Valores de medida instantánea
 * 	@var current Corriente RMS (Amp)
 * 	@var aPow Potencia activa (KW)
 * 	@var rPow Potencia reactiva (KW)
 * 	@var voltage Tensión RMS (V)
 * 	@var freq Frecuencia (Hz)
 * 	@var pfactor Factor de potencia
 * 	@var phase Ángulo de fase V-I (º)
 * 	@var msPow Potencia aparente media (KVA)
 * 	@var thdA Distorsión harmónica total de corriente
 * 	@var thdV Distorsión harmónicoa total de tensión
 */
struct __packed AMMeasureValues_t{
	char version[AMVersionLength];
	double current;
	double aPow;
	double rPow;
	double voltage;
	double freq;
	double pfactor;
	double phase;
	double msPow;
	double thdA;
	double thdV;
};


/** Estructura de estado del objeto AMManager notificada tras una medida, una alarma, evento, etc...
 * 	Se forma por las distintas estructuras de datos de configuración
 * 	@var flags Flags de estado
 * 	@var energyValues Valores de medida de energía
 * 	@var measureValues Valores de medida de potencia
 */
struct __packed AMStatData_t{
	char version[AMVersionLength];
	uint32_t flags;
	AMEnergyValues_t energyValues;
	AMMeasureValues_t measureValues;
};


/** Estructura de datos asociado al boot
 */
struct __packed AMBootData_t{
	char version[AMVersionLength];
	AMCfgData_t cfg;
	AMStatData_t stat;
};


/** Estructura asociada al porcentaje de activación de la carga del medidor de energía (0..100) para chequeo de alarmas
 */
struct __packed AMLoadData_t{
	uint8_t outValue;
};

}	// end namespace Blob


namespace JSON {

/**
 * Codifica la configuración actual en un objeto JSON
 * @param cfg Configuración
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromAMCfg(const Blob::AMCfgData_t& cfg);

/**
 * Codifica el estado actual en un objeto JSON
 * @param stat Estado
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromAMStat(const Blob::AMStatData_t& stat);

/**
 * Codifica el estado de arranque en un objeto JSON
 * @param boot Estado de arranque
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromAMBoot(const Blob::AMBootData_t& boot);

/**
 * Codifica el % de activación en un objeto JSON
 * @param stat Estado
 * @return Objeto JSON o NULL en caso de error
 */
cJSON* getJsonFromAMLoad(const Blob::AMLoadData_t& data);

/**
 * Decodifica el mensaje JSON en un objeto de configuración
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getAMCfgFromJson(Blob::AMCfgData_t &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto de estado
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getAMStatFromJson(Blob::AMStatData_t &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto de arranque
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getAMBootFromJson(Blob::AMBootData_t &obj, cJSON* json);

/**
 * Decodifica el mensaje JSON en un objeto de % de activación
 * @param obj Recibe el objeto decodificado
 * @param json Objeto JSON a decodificar
 * @return keys Parámetros decodificados o 0 en caso de error
 */
uint32_t getAMLoadFromJson(Blob::AMLoadData_t &obj, cJSON* json);

}	// end namespace JSON





#endif
